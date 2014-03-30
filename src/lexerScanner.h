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

class LexerScanner
{
	public:
		struct PositionInfo
		{
			char*	pos;
			int		line;
		};

		// Flags for check_string()
		enum
		{
			FCheckWord = (1 << 0),   // must be followed by whitespace
			FCheckPeek = (1 << 1),   // don't advance cursor
		};

		static inline bool isSymbolChar (char c, bool allownumbers)
		{
			if (allownumbers && (c >= '0' && c <= '9'))
				return true;

			return (c >= 'a' && c <= 'z') ||
				   (c >= 'A' && c <= 'Z') ||
				   (c == '_');
		}

		LexerScanner (FILE* fp);
		~LexerScanner();
		bool getNextToken();
		String readLine();

		inline const String& getTokenText() const
		{
			return m_tokenText;
		}

		inline int getLine() const
		{
			return m_line;
		}

		inline int getColumn() const
		{
			return m_position - m_lineBreakPosition;
		}

		inline ETokenType getTokenType() const
		{
			return m_tokenType;
		}

		static String getTokenString (ETokenType a);

	private:
		char*			m_data;
		char*			m_position;
		char*			m_lineBreakPosition;
		String			m_tokenText,
						m_lastToken;
		ETokenType		m_tokenType;
		int				m_line;

		bool			checkString (const char* c, int flags = 0);

		// Yields a copy of the current position information.
		PositionInfo	getPosition() const;

		// Sets the current position based on given data.
		void			setPosition (const PositionInfo& a);

		// Skips one character
		void			skip();

		// Skips many characters
		void			skip (int chars);
};

#endif // BOTC_LEXER_SCANNER_H
