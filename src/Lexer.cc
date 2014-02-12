/*
	Copyright 2012-2014 Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstring>
#include "Lexer.h"

static StringList	gFileNameStack;
static Lexer*		gMainLexer = null;

// =============================================================================
//
Lexer::Lexer()
{
	assert (gMainLexer == null);
	gMainLexer = this;
}

// =============================================================================
//
Lexer::~Lexer()
{
	gMainLexer = null;
}

// =============================================================================
//
void Lexer::ProcessFile (String fileName)
{
	gFileNameStack << fileName;
	FILE* fp = fopen (fileName, "r");

	if (fp == null)
		Error ("couldn't open %1 for reading: %2", fileName, strerror (errno));

	LexerScanner sc (fp);
	CheckFileHeader (sc);

	while (sc.GetNextToken())
	{
		// Preprocessor commands:
		if (sc.GetTokenType() ==TK_Hash)
		{
			MustGetFromScanner (sc,TK_Symbol);

			if (sc.GetTokenText() == "include")
			{
				MustGetFromScanner (sc,TK_String);
				String fileName = sc.GetTokenText();

				if (gFileNameStack.Contains (fileName))
					Error ("attempted to #include %1 recursively", sc.GetTokenText());

				ProcessFile (fileName);
			}
			else
				Error ("unknown preprocessor directive \"#%1\"", sc.GetTokenText());
		}
		else
		{
			Token tok;
			tok.file = fileName;
			tok.line = sc.GetLine();
			tok.column = sc.GetColumn();
			tok.type = sc.GetTokenType();
			tok.text = sc.GetTokenText();

			// devf ("Token #%1: %2:%3:%4: %5 (%6)\n", mTokens.Size(),
			// 	tok.file, tok.line, tok.column, DescribeToken (&tok),
			// 	GetTokenTypeString (tok.type));

			mTokens << tok;
		}
	}

	mTokenPosition = mTokens.begin() - 1;
	gFileNameStack.Remove (fileName);
}

// ============================================================================
//
static bool IsValidHeader (String header)
{
	if (header.EndsWith ("\n"))
		header.RemoveFromEnd (1);

	StringList tokens = header.Split (" ");

	if (tokens.Size() != 2 || tokens[0] != "#!botc" || tokens[1].IsEmpty())
		return false;

	StringList nums = tokens[1].Split (".");

	if (nums.Size() == 2)
		nums << "0";
	elif (nums.Size() != 3)
		return false;

	bool okA, okB, okC;
	long major = nums[0].ToLong (&okA);
	long minor = nums[1].ToLong (&okB);
	long patch = nums[2].ToLong (&okC);

	if (!okA || !okB || !okC)
		return false;

	if (VERSION_NUMBER < MAKE_VERSION_NUMBER (major, minor, patch))
		Error ("The script file requires " APPNAME " v%1, this is v%2",
			MakeVersionString (major, minor, patch), GetVersionString (false));

	return true;
}

// ============================================================================
//
void Lexer::CheckFileHeader (LexerScanner& sc)
{
	if (!IsValidHeader (sc.ReadLine()))
		Error ("Not a valid botscript file! File must start with '#!botc <version>'");
}

// =============================================================================
//
bool Lexer::GetNext (TokenType req)
{
	Iterator pos = mTokenPosition;

	if (mTokens.IsEmpty())
		return false;

	mTokenPosition++;

	if (IsAtEnd() || (req !=TK_Any && GetTokenType() != req))
	{
		mTokenPosition = pos;
		return false;
	}

	return true;
}

// =============================================================================
//
void Lexer::MustGetNext (TokenType tok)
{
	if (!GetNext())
		Error ("unexpected EOF");

	if (tok !=TK_Any)
		TokenMustBe (tok);
}

// =============================================================================
// eugh..
//
void Lexer::MustGetFromScanner (LexerScanner& sc, TokenType tt)
{
	if (!sc.GetNextToken())
		Error ("unexpected EOF");

	if (tt !=TK_Any && sc.GetTokenType() != tt)
	{
		// TODO
		Token tok;
		tok.type = sc.GetTokenType();
		tok.text = sc.GetTokenText();

		Error ("at %1:%2: expected %3, got %4",
			   gFileNameStack.Last(),
			sc.GetLine(),
			DescribeTokenType (tt),
			DescribeToken (&tok));
	}
}

// =============================================================================
//
void Lexer::MustGetAnyOf (const List<TokenType>& toks)
{
	if (!GetNext())
		Error ("unexpected EOF");

	for (TokenType tok : toks)
		if (GetTokenType() == tok)
			return;

	String toknames;

	for (const TokenType& tokType : toks)
	{
		if (&tokType == &toks.Last())
			toknames += " or ";
		elif (toknames.IsEmpty() == false)
			toknames += ", ";

		toknames += DescribeTokenType (tokType);
	}

	Error ("expected %1, got %2", toknames, DescribeToken (GetToken()));
}

// =============================================================================
//
int Lexer::GetOneSymbol (const StringList& syms)
{
	if (!GetNext())
		Error ("unexpected EOF");

	if (GetTokenType() ==TK_Symbol)
	{
		for (int i = 0; i < syms.Size(); ++i)
		{
			if (syms[i] == GetToken()->text)
				return i;
		}
	}

	Error ("expected one of %1, got %2", syms, DescribeToken (GetToken()));
	return -1;
}

// =============================================================================
//
void Lexer::TokenMustBe (TokenType tok)
{
	if (GetTokenType() != tok)
		Error ("expected %1, got %2", DescribeTokenType (tok),
			DescribeToken (GetToken()));
}

// =============================================================================
//
String Lexer::DescribeTokenPrivate (TokenType tokType, Lexer::Token* tok)
{
	if (tokType <gLastNamedToken)
		return "\"" + LexerScanner::GetTokenString (tokType) + "\"";

	switch (tokType)
	{
		case TK_Symbol:	return tok ? tok->text : "a symbol";
		case TK_Number:	return tok ? tok->text : "a number";
		case TK_String:	return tok ? ("\"" + tok->text + "\"") : "a string";
		case TK_Any:	return tok ? tok->text : "any token";
		default: break;
	}

	return "";
}

// =============================================================================
//
bool Lexer::PeekNext (Lexer::Token* tk)
{
	Iterator pos = mTokenPosition;
	bool r = GetNext();

	if (r && tk != null)
		*tk = *mTokenPosition;

	mTokenPosition = pos;
	return r;
}

// =============================================================================
//
bool Lexer::PeekNextType (TokenType req)
{
	Iterator pos = mTokenPosition;
	bool result = false;

	if (GetNext() && GetTokenType() == req)
		result = true;

	mTokenPosition = pos;
	return result;
}

// =============================================================================
//
Lexer* Lexer::GetCurrentLexer()
{
	return gMainLexer;
}

// =============================================================================
//
String Lexer::PeekNextString (int a)
{
	if (mTokenPosition + a >= mTokens.end())
		return "";

	Iterator oldpos = mTokenPosition;
	mTokenPosition += a;
	String result = GetToken()->text;
	mTokenPosition = oldpos;
	return result;
}

// =============================================================================
//
String Lexer::DescribeCurrentPosition()
{
	return GetToken()->file + ":" + GetToken()->line;
}

// =============================================================================
//
String Lexer::DescribeTokenPosition()
{
	return Format ("%1 / %2", mTokenPosition - mTokens.begin(), mTokens.Size());
}
