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
#include "lexerScanner.h"
#include "lexer.h"

static const String gTokenStrings[] =
{
	"<<=",
	">>=",
	"==",
	"!=",
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
	"++",
	"--",
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
	"constexpr",
	"default",
	"do",
	"else",
	"event",
	"eventdef",
	"for",
	"funcdef",
	"if",
	"int",
	"mainloop",
	"onenter",
	"onexit",
	"state",
	"switch",
	"str",
	"using",
	"var",
	"void",
	"while",
	"true",
	"false",
	"enum",
	"func",
	"return",
};

static_assert (countof (gTokenStrings) == (int)gLastNamedToken + 1,
	"Count of gTokenStrings is not the same as the amount of named token identifiers.");

// =============================================================================
//
LexerScanner::LexerScanner (FILE* fp) :
	m_line (1)
{
	long fsize, bytes;

	fseek (fp, 0l, SEEK_END);
	fsize = ftell (fp);
	rewind (fp);
	m_data = new char[fsize];
	m_position = m_lineBreakPosition = &m_data[0];
	bytes = fread (m_data, 1, fsize, fp);
	assert (bytes >= fsize);
}

// =============================================================================
//
LexerScanner::~LexerScanner()
{
	delete m_data;
}

// =============================================================================
//
bool LexerScanner::checkString (const char* c, int flags)
{
	bool r = strncmp (m_position, c, strlen (c)) == 0;

	// There is to be a non-symbol character after words
	if (r && (flags & FCheckWord) && isSymbolChar (m_position[strlen (c)], true))
		r = false;

	// Advance the cursor unless we want to just peek
	if (r && !(flags & FCheckPeek))
		m_position += strlen (c);

	return r;
}

// =============================================================================
//
bool LexerScanner::getNextToken()
{
	m_tokenText = "";

	while (isspace (*m_position))
		skip();

	// Check for comments
	if (strncmp (m_position, "//", 2) == 0)
	{
		m_position += 2;

		while (*m_position != '\n')
			skip();

		return getNextToken();
	}
	elif (strncmp (m_position, "/*", 2) == 0)
	{
		skip (2); // skip the start symbols

		while (strncmp (m_position, "*/", 2) != 0)
			skip();

		skip (2); // skip the end symbols
		return getNextToken();
	}

	if (*m_position == '\0')
		return false;

	// Check tokens
	for (int i = 0; i < countof (gTokenStrings); ++i)
	{
		int flags = 0;

		if (i >= gFirstNamedToken)
			flags |= FCheckWord;

		if (checkString (gTokenStrings[i], flags))
		{
			m_tokenText = gTokenStrings[i];
			m_tokenType = (ETokenType) i;
			return true;
		}
	}

	// Check and parse string
	if (*m_position == '\"')
	{
		m_position++;

		while (*m_position != '\"')
		{
			if (!*m_position)
				error ("unterminated string");

			if (checkString ("\\n"))
			{
				m_tokenText += '\n';
				continue;
			}
			elif (checkString ("\\t"))
			{
				m_tokenText += '\t';
				continue;
			}
			elif (checkString ("\\\""))
			{
				m_tokenText += '"';
				continue;
			}

			m_tokenText += *m_position++;
		}

		m_tokenType =TK_String;
		skip(); // skip the final quote
		return true;
	}

	if (isdigit (*m_position))
	{
		while (isdigit (*m_position))
			m_tokenText += *m_position++;

		m_tokenType =TK_Number;
		return true;
	}

	if (isSymbolChar (*m_position, false))
	{
		m_tokenType =TK_Symbol;

		do
		{
			if (!isSymbolChar (*m_position, true))
				break;

			m_tokenText += *m_position++;
		} while (*m_position != '\0');

		return true;
	}

	error ("unknown character \"%1\"", *m_position);
	return false;
}

// =============================================================================
//
void LexerScanner::skip()
{
	if (*m_position == '\n')
	{
		m_line++;
		m_lineBreakPosition = m_position;
	}

	m_position++;
}

// =============================================================================
//
void LexerScanner::skip (int chars)
{
	for (int i = 0; i < chars; ++i)
		skip();
}

// =============================================================================
//
String LexerScanner::getTokenString (ETokenType a)
{
	assert ((int) a <= gLastNamedToken);
	return gTokenStrings[a];
}

// =============================================================================
//
String LexerScanner::readLine()
{
	String line;

	while (*m_position != '\n')
		line += *(m_position++);

	return line;
}
