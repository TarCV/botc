/*
	Copyright 2012-2014 Teemu Piippo
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
#include "lexer.h"

static StringList	FileNameStack;
static Lexer*		MainLexer = null;

// =============================================================================
//
Lexer::Lexer()
{
	ASSERT_EQ (MainLexer, null);
	MainLexer = this;
}

// =============================================================================
//
Lexer::~Lexer()
{
	MainLexer = null;
}

// =============================================================================
//
void Lexer::processFile (String fileName)
{
	FileNameStack << fileName;
	FILE* fp = fopen (fileName, "r");

	if (fp == null)
		error ("couldn't open %1 for reading: %2", fileName, strerror (errno));

	LexerScanner sc (fp);
	checkFileHeader (sc);

	while (sc.getNextToken())
	{
		// Preprocessor commands:
		if (sc.getTokenType() == Token::Hash)
		{
			mustGetFromScanner (sc,Token::Symbol);

			if (sc.getTokenText() == "include")
			{
				mustGetFromScanner (sc,Token::String);
				String fileName = sc.getTokenText();

				if (FileNameStack.contains (fileName))
					error ("attempted to #include %1 recursively", sc.getTokenText());

				processFile (fileName);
			}
			else
				error ("unknown preprocessor directive \"#%1\"", sc.getTokenText());
		}
		else
		{
			TokenInfo tok;
			tok.file = fileName;
			tok.line = sc.getLine();
			tok.column = sc.getColumn();
			tok.type = sc.getTokenType();
			tok.text = sc.getTokenText();

			// devf ("Token #%1: %2:%3:%4: %5 (%6)\n", mTokens.size(),
			// 	tok.file, tok.line, tok.column, DescribeToken (&tok),
			// 	GetTokenTypeString (tok.type));

			m_tokens << tok;
		}
	}

	m_tokenPosition = m_tokens.begin() - 1;
	FileNameStack.removeOne (fileName);
}

// ============================================================================
//
static bool isValidHeader (String header)
{
	if (header.endsWith ("\n"))
		header.removeFromEnd (1);

	StringList tokens = header.split (" ");

	if (tokens.size() != 2 or tokens[0] != "#!botc" or tokens[1].isEmpty())
		return false;

	StringList nums = tokens[1].split (".");

	if (nums.size() == 2)
		nums << "0";
	elif (nums.size() != 3)
		return false;

	bool okA, okB, okC;
	long major = nums[0].toLong (&okA);
	long minor = nums[1].toLong (&okB);
	long patch = nums[2].toLong (&okC);

	if (!okA or !okB or !okC)
		return false;

	if (VERSION_NUMBER < MAKE_VERSION_NUMBER (major, minor, patch))
		error ("The script file requires " APPNAME " v%1, this is v%2",
			makeVersionString (major, minor, patch), versionString (false));

	return true;
}

// ============================================================================
//
void Lexer::checkFileHeader (LexerScanner& sc)
{
	if (!isValidHeader (sc.readLine()))
		error ("Not a valid botscript file! File must start with '#!botc <version>'");
}

// =============================================================================
//
bool Lexer::next (Token req)
{
	Iterator pos = m_tokenPosition;

	if (m_tokens.isEmpty())
		return false;

	m_tokenPosition++;

	if (isAtEnd() or (req !=Token::Any and tokenType() != req))
	{
		m_tokenPosition = pos;
		return false;
	}

	return true;
}

// =============================================================================
//
void Lexer::mustGetNext (Token tok)
{
	if (!next())
		error ("unexpected EOF");

	if (tok !=Token::Any)
		tokenMustBe (tok);
}

// =============================================================================
// eugh..
//
void Lexer::mustGetFromScanner (LexerScanner& sc, Token tt)
{
	if (sc.getNextToken() == false)
		error ("unexpected EOF");

	if (tt != Token::Any and sc.getTokenType() != tt)
	{
		// TODO
		TokenInfo tok;
		tok.type = sc.getTokenType();
		tok.text = sc.getTokenText();

		error ("at %1:%2: expected %3, got %4",
			FileNameStack.last(),
			sc.getLine(),
			DescribeTokenType (tt),
			DescribeToken (&tok));
	}
}

// =============================================================================
//
void Lexer::mustGetAnyOf (const List<Token>& toks)
{
	if (!next())
		error ("unexpected EOF");

	for (Token tok : toks)
		if (tokenType() == tok)
			return;

	String toknames;

	for (const Token& tokType : toks)
	{
		if (&tokType == &toks.last())
			toknames += " or ";
		elif (toknames.isEmpty() == false)
			toknames += ", ";

		toknames += DescribeTokenType (tokType);
	}

	error ("expected %1, got %2", toknames, DescribeToken (token()));
}

// =============================================================================
//
int Lexer::getOneSymbol (const StringList& syms)
{
	if (!next())
		error ("unexpected EOF");

	if (tokenType() ==Token::Symbol)
	{
		for (int i = 0; i < syms.size(); ++i)
		{
			if (syms[i] == token()->text)
				return i;
		}
	}

	error ("expected one of %1, got %2", syms, DescribeToken (token()));
	return -1;
}

// =============================================================================
//
void Lexer::tokenMustBe (Token tok)
{
	if (tokenType() != tok)
		error ("expected %1, got %2", DescribeTokenType (tok),
			DescribeToken (token()));
}

// =============================================================================
//
String Lexer::DescribeTokenPrivate (Token tokType, Lexer::TokenInfo* tok)
{
	if (tokType < LastNamedToken)
		return "\"" + LexerScanner::GetTokenString (tokType) + "\"";

	switch (tokType)
	{
		case Token::Symbol:	return tok ? tok->text : "a symbol";
		case Token::Number:	return tok ? tok->text : "a number";
		case Token::String:	return tok ? ("\"" + tok->text + "\"") : "a string";
		case Token::Any:	return tok ? tok->text : "any token";
		default: break;
	}

	return "";
}

// =============================================================================
//
bool Lexer::peekNext (Lexer::TokenInfo* tk)
{
	Iterator pos = m_tokenPosition;
	bool r = next();

	if (r and tk != null)
		*tk = *m_tokenPosition;

	m_tokenPosition = pos;
	return r;
}

// =============================================================================
//
bool Lexer::peekNextType (Token req)
{
	Iterator pos = m_tokenPosition;
	bool result = false;

	if (next() and tokenType() == req)
		result = true;

	m_tokenPosition = pos;
	return result;
}

// =============================================================================
//
Lexer* Lexer::GetCurrentLexer()
{
	return MainLexer;
}

// =============================================================================
//
String Lexer::peekNextString (int a)
{
	if (m_tokenPosition + a >= m_tokens.end())
		return "";

	Iterator oldpos = m_tokenPosition;
	m_tokenPosition += a;
	String result = token()->text;
	m_tokenPosition = oldpos;
	return result;
}

// =============================================================================
//
String Lexer::describeCurrentPosition()
{
	return token()->file + ":" + token()->line;
}

// =============================================================================
//
String Lexer::describeTokenPosition()
{
	return format ("%1 / %2", m_tokenPosition - m_tokens.begin(), m_tokens.size());
}

// =============================================================================
//
void Lexer::mustGetSymbol (const String& a)
{
	mustGetNext (Token::Any);

	if (token()->text != a)
		error ("expected \"%1\", got \"%2\"", a, token()->text);
}
