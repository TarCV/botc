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

#ifndef BOTC_LEXER_H
#define BOTC_LEXER_H

#include "main.h"
#include "lexer_scanner.h"

class lexer
{
types:
	struct token
	{
		e_token		type;
		string		text;
		string		file;
		int			line;
		int			column;
	};

	using token_list	= list<token>;
	using iterator		= token_list::iterator;

public:
	lexer();
	~lexer();

	void	process_file (string file_name);
	bool	get_next (e_token req = tk_any);
	void	must_get_next (e_token tok = tk_any);
	void	must_get_any_of (const list<e_token>& toks);
	int		get_one_symbol (const string_list& syms);
	void	must_be (e_token tok);
	bool	peek_next (token* tk = null);

	inline bool has_valid_token() const
	{
		return (m_token_position < m_tokens.end() && m_token_position >= m_tokens.begin());
	}

	inline token* get_token() const
	{
		assert (has_valid_token() == true);
		return &(*m_token_position);
	}

	inline bool is_at_end() const
	{
		return m_token_position == m_tokens.end();
	}

	inline e_token get_token_type() const
	{
		return get_token()->type;
	}

	// If @tok is given, describes the token. If not, describes @tok_type.
	static inline string describe_token_type (e_token tok_type)
	{
		return describe_token_private (tok_type, null);
	}

	static inline string describe_token (token* tok)
	{
		return describe_token_private (tok->type, tok);
	}

	static lexer* get_current_lexer();

	inline void skip (int a = 1)
	{
		m_token_position += a;
	}

	string peek_next_string (int a = 1);

private:
	token_list		m_tokens;
	iterator		m_token_position;

	// read a mandatory token from scanner
	void must_get_next_from_scanner (lexer_scanner& sc, e_token tt = tk_any);
	void check_file_header (lexer_scanner& sc);

	static string describe_token_private (e_token tok_type, token* tok);
};

#endif // BOTC_LEXER_H
