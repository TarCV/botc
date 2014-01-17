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

#include <cstring>
#include "lexer.h"

static string_list	g_file_name_stack;
static lexer*		g_main_lexer = null;

lexer::lexer()
{
	assert (g_main_lexer == null);
	g_main_lexer = this;
}

lexer::~lexer()
{
	g_main_lexer = null;
}

void lexer::process_file (string file_name)
{
	FILE* fp = fopen (file_name, "r");

	if (fp == null)
		error ("couldn't open %1 for reading: %2", file_name, strerror (errno));

	lexer_scanner sc (fp);

	while (sc.get_next_token())
	{
		// Preprocessor commands:
		if (sc.get_token_type() == tk_hash)
		{
			must_get_next_from_scanner (sc, tk_symbol);

			if (sc.get_token_text() == "include")
			{
				must_get_next_from_scanner (sc, tk_string);
				string file_name = sc.get_token_text();

				if (g_file_name_stack.contains (file_name))
					error ("attempted to #include %1 recursively", sc.get_token_text());

				process_file (file_name);
			}
			else
				error ("unknown preprocessor directive \"#%1\"", sc.get_token_text());
		}
		else
		{
			token tok;
			tok.file = file_name;
			tok.line = sc.get_line();
			tok.column = sc.get_column();
			tok.type = sc.get_token_type();
			tok.text = sc.get_token_text();
			m_tokens << tok;
		}
	}

	m_token_position = m_tokens.begin() - 1;
}

// =============================================================================
//
bool lexer::get_next (e_token req)
{
	iterator pos = m_token_position;

	if (m_tokens.is_empty())
		return false;

	m_token_position++;

	if (is_at_end())
		return false;

	if (req != tk_any && get_token_type() != req)
	{
		m_token_position = pos;
		return false;
	}

	return true;
}

// =============================================================================
//
void lexer::must_get_next (e_token tok)
{
	if (!get_next())
		error ("unexpected EOF");

	must_be (tok);
}

// =============================================================================
// eugh..
//
void lexer::must_get_next_from_scanner (lexer_scanner& sc, e_token tok)
{
	if (!sc.get_next_token())
		error ("unexpected EOF");

	if (tok != tk_any && sc.get_token_type() != tok)
		error ("expected %1, got %2", describe_token_type (tok),
			   describe_token (get_token()));
}

// =============================================================================
//
void lexer::must_get_any_of (const list<e_token>& toks)
{
	if (!get_next())
		error ("unexpected EOF");

	for (e_token tok : toks)
		if (get_token_type() == tok)
			return;

	string toknames;

	for (const e_token& tok_type : toks)
	{
		if (&tok_type == &toks.last())
			toknames += " or ";
		elif (toknames.is_empty() == false)
			toknames += ", ";

		toknames += describe_token_type (tok_type);
	}

	error ("expected %1, got %2", toknames, describe_token (get_token()));
}

// =============================================================================
//
int lexer::get_one_symbol (const string_list& syms)
{
	if (!get_next())
		error ("unexpected EOF");

	if (get_token_type() == tk_symbol)
	{
		for (int i = 0; i < syms.size(); ++i)
		{
			if (syms[i] == get_token()->text)
				return i;
		}
	}

	error ("expected one of %1, got %2", syms, describe_token (get_token()));
	return -1;
}

// =============================================================================
//
void lexer::must_be (e_token tok)
{
	if (get_token_type() != tok)
		error ("expected %1, got %2", describe_token_type (tok),
			describe_token (get_token()));
}

// =============================================================================
//
string lexer::describe_token_private (e_token tok_type, lexer::token* tok)
{
	if (tok_type < tk_last_named_token)
		return "\"" + lexer_scanner::get_token_string (tok_type) + "\"";

	switch (tok_type)
	{
		case tk_symbol:
			return tok ? tok->text : "a symbol";

		case tk_number:
			return tok ? tok->text : "a number";

		case tk_string:
			return tok ? ("\"" + tok->text + "\"") : "a string";

		case tk_any:
			return tok ? tok->text : "any token";

		default:
			break;
	}

	return "";
}

// =============================================================================
//
bool lexer::peek_next (lexer::token* tk)
{
	iterator pos = m_token_position;
	bool r = get_next();

	if (r && tk != null)
		*tk = *m_token_position;

	m_token_position = pos;
	return r;
}

// =============================================================================
//
lexer* lexer::get_main_lexer()
{
	return g_main_lexer;
}

// =============================================================================
//
string lexer::peek_next_string (int a)
{
	if (m_token_position + a >= m_tokens.end())
		return "";

	iterator oldpos = m_token_position;
	m_token_position += a;
	string result = get_token()->text;
	m_token_position = oldpos;
	return result;
}
