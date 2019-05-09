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
#include <cerrno>
#include <cassert>
#include "lexer.h"

static StringList	FileNameStack;
static Lexer*		MainLexer = null;

// _________________________________________________________________________________________________
//
Lexer::Lexer()
{
	ASSERT_EQ (MainLexer, null);

	// Dummy token in the beginning to set m_tokenPosition to a pos before the first actual token
	{
		assert(m_tokens.isEmpty());
		TokenInfo nulltok;
		nulltok.type = Token::Any;
		nulltok.file = "";
		nulltok.line = -1;
		nulltok.column = -1;
		nulltok.text = "";
		m_tokens << nulltok;
	}
	m_tokenPosition = m_tokens.begin();

	MainLexer = this;
}

// _________________________________________________________________________________________________
//
Lexer::~Lexer()
{
	MainLexer = null;
}

// _________________________________________________________________________________________________
//
void Lexer::processFile(String fileName)
{
	processFileInternal(fileName);
	preprocessTokens();
}

// _________________________________________________________________________________________________
//
void Lexer::preprocessTokens()
{
	struct Insertion
	{
		int position;
		TokenInfo newToken;
	};

	int position(0), braceLevel(0);
	TokenInfo prevToken;
	List<int> insertClosingBraceAtReturnToLevel;
	bool checkInsertAtNextToken(false);
	List<Insertion> insertions;
	for (TokenInfo &token : m_tokens) {
		if (checkInsertAtNextToken) {
			if (token.type != Token::Else) {
				TokenInfo braceToken(token);
				braceToken.type = Token::BraceEnd;
				braceToken.text = "}";

				insertions.append({ position, braceToken });
				insertClosingBraceAtReturnToLevel.pop();
			}

			checkInsertAtNextToken = false;
		}

		switch (token.type) {
		case Token::BraceStart:
		{
			++braceLevel;
		}
			break;
		case Token::BraceEnd:
		{
			--braceLevel;

			if (!insertClosingBraceAtReturnToLevel.isEmpty() && insertClosingBraceAtReturnToLevel.last() == braceLevel) {
				checkInsertAtNextToken = true; // we should not insert if the next token is 'else', so wait for it
			}
		}
			break;
		case Token::If:
			if (prevToken.type == Token::Else) {
				TokenInfo braceToken(token);
				braceToken.type = Token::BraceStart;
				braceToken.text = "{";
				insertions.append({ position, braceToken });

				insertClosingBraceAtReturnToLevel.append(braceLevel);
			}
			break;
		}

		prevToken = token;
		++position;
	}

	// Apply insertion in the reverse order, otherwise positions should be corrected after each insertion
	for (auto it = insertions.crbegin(); it != insertions.crend(); ++it) {
		m_tokens.insert(it->position, it->newToken);
	}

	// Tokens list was changed, thus reinitialise the iterator
	m_tokenPosition = m_tokens.begin();
}

// _________________________________________________________________________________________________
//
void Lexer::processFileInternal(String fileName)
{
	FileNameStack << fileName;
	FILE* fp = fopen (fileName, "rb");

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

                processFileInternal(fileName);
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

	m_tokenPosition = m_tokens.begin();
	FileNameStack.removeOne (fileName);
}

// ============================================================================
//
static bool isValidHeader (String header)
{
	if (header.endsWith ("\n"))
		header.removeFromEnd (1);
    if (header.endsWith ("\r"))
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
			makeVersionString (major, minor, patch), versionString());

	if (major != VERSION_MAJOR)
		error("The script file requires " APPNAME " v%1, this is v%2 (major version change means there are breaking changes)",
			major, VERSION_MAJOR);

	return true;
}

// ============================================================================
//
void Lexer::checkFileHeader (LexerScanner& sc)
{
	if (!isValidHeader (sc.readLine()))
		error ("Not a valid botscript file! File must start with '#!botc <version>'");
}

// _________________________________________________________________________________________________
//
bool Lexer::next (Token req)
{
	Iterator pos = m_tokenPosition;

	if (m_tokens.isEmpty() || m_tokens.size() == 1) // is 1 due to extra dummy token in the beginning
		return false;

	m_tokenPosition++;

	if (isAtEnd() or (req !=Token::Any and tokenType() != req))
	{
		m_tokenPosition = pos;
		return false;
	}

	return true;
}

// _________________________________________________________________________________________________
//
void Lexer::mustGetNext (Token tok)
{
	if (!next())
		error ("unexpected EOF");

	if (tok !=Token::Any)
		tokenMustBe (tok);
}

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
//
void Lexer::tokenMustBe (Token tok)
{
	if (tokenType() != tok)
		error ("expected %1, got %2", DescribeTokenType (tok),
			DescribeToken (token()));
}

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
//
Lexer* Lexer::GetCurrentLexer()
{
	return MainLexer;
}

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
//
String Lexer::describeCurrentPosition()
{
	return token()->file + ":" + token()->line;
}

// _________________________________________________________________________________________________
//
String Lexer::describeTokenPosition()
{
	return format ("%1 / %2", m_tokenPosition - m_tokens.begin(), m_tokens.size());
}

// _________________________________________________________________________________________________
//
void Lexer::mustGetSymbol (const String& a)
{
	mustGetNext (Token::Any);

	if (token()->text != a)
		error ("expected \"%1\", got \"%2\"", a, token()->text);
}
