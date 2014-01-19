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

#ifndef BOTC_LEXER_SCANNER_H
#define BOTC_LEXER_SCANNER_H

#include <climits>
#include "main.h"

class lexer_scanner
{
	types:
		struct position_info
		{
			char*	pos;
			char*	line_break_pos;
			int		line;
		};

		// Flags for check_string()
		enum
		{
			f_check_word = (1 << 0),   // must be followed by whitespace
			f_check_peek = (1 << 1),   // don't advance cursor
		};

	public:
		static inline bool is_symbol_char (char c, bool allow_numbers)
		{
			if (allow_numbers && (c >= '0' && c <= '9'))
				return true;

			return (c >= 'a' && c <= 'z') ||
				   (c >= 'A' && c <= 'Z') ||
				   (c == '_');
		}

		lexer_scanner (FILE* fp);
		~lexer_scanner();
		bool get_next_token();
		string read_line();

		inline const string& get_token_text() const
		{
			return m_token_text;
		}

		inline int get_line() const
		{
			return m_line;
		}

		inline int get_column() const
		{
			return m_ptr - m_line_break_pos;
		}

		inline e_token get_token_type() const
		{
			return m_token_type;
		}

		static string get_token_string (e_token a);

	private:
		char*			m_data;
		char*			m_ptr;
		char*			m_line_break_pos;
		string			m_token_text,
						m_last_token;
		e_token			m_token_type;
		int				m_line;

		bool			check_string (const char* c, int flags = 0);

		// Yields a copy of the current position information.
		position_info	get_position() const;

		// Sets the current position based on given data.
		void			set_position (const position_info& a);

		// Skips one character
		void			skip();

		// Skips many characters
		void			skip (int chars);
};

#endif // BOTC_LEXER_SCANNER_H
