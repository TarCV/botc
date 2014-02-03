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

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <string>
#include "LexerScanner.h"
#include "Lexer.h"

static const String gTokenStrings[] =
{
	"<<=",
	">>=",
	"==",
	"!=",
	"[]",
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
	"<<",
	">>",
	">=",
	"<=",
	"&&",
	"||",
	"'",
	"$",
	"(",
	")",
	"[",
	"]",
	"{",
	"}",
	"=",
	"+",
	"-",
	"*",
	"/",
	"%",
	",",
	"<",
	">",
	".",
	":",
	";",
	"#",
	"!",
	"&",
	"|",
	"^",
	"?",
	"->",
	"bool",
	"break",
	"case",
	"continue",
	"const",
	"default",
	"do",
	"else",
	"event",
	"eventdef",
	"for",
	"funcdef",
	"goto",
	"if",
	"int",
	"mainloop",
	"onenter",
	"onexit",
	"state",
	"switch",
	"str",
	"void",
	"while",
	"true",
	"false",
	"enum",
	"func",
	"return",
};

static_assert (countof (gTokenStrings) == (int) tkLastNamedToken + 1,
	"Count of gTokenStrings is not the same as the amount of named token identifiers.");

// =============================================================================
//
LexerScanner::LexerScanner (FILE* fp) :
	mLine (1)
{
	long fsize, bytes;

	fseek (fp, 0l, SEEK_END);
	fsize = ftell (fp);
	rewind (fp);
	mData = new char[fsize];
	mPosition = mLineBreakPosition = &mData[0];
	bytes = fread (mData, 1, fsize, fp);
	assert (bytes >= fsize);
}

// =============================================================================
//
LexerScanner::~LexerScanner()
{
	delete mData;
}

// =============================================================================
//
bool LexerScanner::CheckString (const char* c, int flags)
{
	bool r = strncmp (mPosition, c, strlen (c)) == 0;

	// There is to be a non-symbol character after words
	if (r && (flags & FCheckWord) && IsSymbolChar (mPosition[strlen (c)], true))
		r = false;

	// Advance the cursor unless we want to just peek
	if (r && !(flags & FCheckPeek))
		mPosition += strlen (c);

	return r;
}

// =============================================================================
//
bool LexerScanner::GetNextToken()
{
	mTokenText = "";

	while (isspace (*mPosition))
		Skip();

	// Check for comments
	if (strncmp (mPosition, "//", 2) == 0)
	{
		mPosition += 2;

		while (*mPosition != '\n')
			Skip();

		return GetNextToken();
	}
	elif (strncmp (mPosition, "/*", 2) == 0)
	{
		Skip (2); // skip the start symbols

		while (strncmp (mPosition, "*/", 2) != 0)
			Skip();

		Skip (2); // skip the end symbols
		return GetNextToken();
	}

	if (*mPosition == '\0')
		return false;

	// Check tokens
	for (int i = 0; i < countof (gTokenStrings); ++i)
	{
		int flags = 0;

		if (i >= tkFirstNamedToken)
			flags |= FCheckWord;

		if (CheckString (gTokenStrings[i], flags))
		{
			mTokenText = gTokenStrings[i];
			mTokenType = (EToken) i;
			return true;
		}
	}

	// Check and parse string
	if (*mPosition == '\"')
	{
		mPosition++;

		while (*mPosition != '\"')
		{
			if (!*mPosition)
				Error ("unterminated string");

			if (CheckString ("\\n"))
			{
				mTokenText += '\n';
				continue;
			}
			elif (CheckString ("\\t"))
			{
				mTokenText += '\t';
				continue;
			}
			elif (CheckString ("\\\""))
			{
				mTokenText += '"';
				continue;
			}

			mTokenText += *mPosition++;
		}

		mTokenType = tkString;
		Skip(); // skip the final quote
		return true;
	}

	if (isdigit (*mPosition))
	{
		while (isdigit (*mPosition))
			mTokenText += *mPosition++;

		mTokenType = tkNumber;
		return true;
	}

	if (IsSymbolChar (*mPosition, false))
	{
		mTokenType = tkSymbol;

		do
		{
			if (!IsSymbolChar (*mPosition, true))
				break;

			mTokenText += *mPosition++;
		} while (*mPosition != '\0');

		return true;
	}

	Error ("unknown character \"%1\"", *mPosition);
	return false;
}

// =============================================================================
//
void LexerScanner::Skip()
{
	if (*mPosition == '\n')
	{
		mLine++;
		mLineBreakPosition = mPosition;
	}

	mPosition++;
}

// =============================================================================
//
void LexerScanner::Skip (int chars)
{
	for (int i = 0; i < chars; ++i)
		Skip();
}

// =============================================================================
//
String LexerScanner::GetTokenString (EToken a)
{
	assert ((int) a <= tkLastNamedToken);
	return gTokenStrings[a];
}

// =============================================================================
//
String LexerScanner::ReadLine()
{
	String line;

	while (*mPosition != '\n')
		line += *(mPosition++);

	return line;
}