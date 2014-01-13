/*
	Copyright (c) 2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <string>
#include "lexer_scanner.h"
#include "lexer.h"

static const string g_token_strings[] =
{
	"==",
	"[]",
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
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
	"for",
	"goto",
	"if",
	"int",
	"mainloop",
	"onenter",
	"onexit",
	"state",
	"switch",
	"str"
	"void",
	"while",
	"enum",
	"func",
	"return",
};

static_assert (countof (g_token_strings) == (int) last_named_token,
	"Count of g_token_strings is not the same as the amount of named token identifiers.");

// =============================================================================
//
lexer_scanner::lexer_scanner (FILE* fp) :
	m_line (1)
{
	long fsize, bytes;

	fseek (fp, 0l, SEEK_END);
	fsize = ftell (fp);
	rewind (fp);
	m_data = new char[fsize];
	m_ptr = m_line_break_pos = &m_data[0];
	bytes = fread (m_data, 1, fsize, fp);
	assert (bytes >= fsize);
}

// =============================================================================
//
lexer_scanner::~lexer_scanner()
{
	delete m_data;
}

// =============================================================================
//
bool lexer_scanner::check_string (const char* c, int flags)
{
	bool r = strncmp (m_ptr, c, strlen (c)) == 0;

	// There is to be whitespace after words
	if (r && (flags & f_check_word) && !isspace (m_ptr[strlen (c)]))
		r = false;

	// Advance the cursor unless we want to just peek
	if (r && !(flags & f_check_peek))
		m_ptr += strlen (c);

	return r;
}

// =============================================================================
//
bool lexer_scanner::get_next_token()
{
	m_token_text = "";

	while (isspace (*m_ptr) == true)
	{
		if (*m_ptr == '\n')
		{
			m_line++;
			m_line_break_pos = m_ptr;
		}

		m_ptr++;
	}

	// Check for comments
	if (strncmp (m_ptr, "//", 2) == 0)
	{
		m_ptr += 2;

		while (*(++m_ptr) != '\n')
			;

		return get_next_token();
	}
	elif (strncmp (m_ptr, "/*", 2) == 0)
	{
		m_ptr += 2;

		while (strncmp (++m_ptr, "*/", 2) != 0)
		{
			if (*m_ptr == '\n')
			{
				m_line++;
				m_line_break_pos = m_ptr;
			}
		}

		m_ptr += 2; // skip the */
		return get_next_token();
	}

	if (*m_ptr == '\0')
		return false;

	// Check tokens
	for (int i = 0; i < (int) (sizeof g_token_strings / sizeof * g_token_strings); ++i)
	{
		if (check_string (g_token_strings[i], f_check_word))
		{
			m_token_text = g_token_strings[i];
			m_token_type = (e_token) i;
			return true;
		}
	}

	// Check and parse string
	if (*m_ptr == '\"')
	{
		m_ptr++;

		while (*m_ptr != '\"')
		{
			if (!*m_ptr)
				return false;

			if (check_string ("\\n"))
			{
				m_token_text += '\n';
				continue;
			}
			elif (check_string ("\\t"))
			{
				m_token_text += '\t';
				continue;
			}
			elif (check_string ("\\\""))
			{
				m_token_text += '"';
				continue;
			}

			m_token_text += *m_ptr++;
		}

		m_token_type = tk_string;
		m_ptr++; // skip the final quote
		return true;
	}

	m_token_type = tk_symbol;

	if (isdigit (*m_ptr))
	{
		while (isdigit (*m_ptr))
			m_token_text += *m_ptr++;

		m_token_type = tk_number;
		return true;
	}

	if (is_symbol_char (*m_ptr))
	{
		while (m_ptr != '\0')
		{
			if (!is_symbol_char (*m_ptr))
				break;

			bool stop_here = false;

			for (string i : g_token_strings)
			{
				if (check_string (i, f_check_peek | f_check_word))
				{
					stop_here = true;
					break;
				}
			}

			if (stop_here)
				break;

			m_token_text += *m_ptr++;
		}

		return true;
	}

	error ("unknown character \"%1\"", *m_ptr);
	return false;
}

// =============================================================================
//
string lexer_scanner::get_token_string (e_token a)
{
	assert ((int) a <= last_named_token);
	return g_token_strings[a];
}
