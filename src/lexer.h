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

#ifndef BOTC_LEXER_H
#define BOTC_LEXER_H

#include "main.h"
#include "lexerScanner.h"

class Lexer
{
public:
	struct TokenInfo
	{
		Token	type;
		String		text;
		String		file;
		int			line;
		int			column;
	};

	using TokenList	= List<TokenInfo>;
	using Iterator	= TokenList::Iterator;

public:
	Lexer();
	~Lexer();

    // Disable copying and moving to keep m_tokenPosition iteration valid
    Lexer(const Lexer&) = delete;
    Lexer& operator= (const Lexer&) = delete;
    Lexer(const Lexer&&) = delete;
    Lexer& operator=(const Lexer&& other) = delete;

	void	processFile (String fileName);
	bool	next (Token req = Token::Any);
	void	mustGetNext (Token tok);
	void	mustGetAnyOf (const List<Token>& toks);
	void	mustGetSymbol (const String& a);
	int		getOneSymbol (const StringList& syms);
	void	tokenMustBe (Token tok);
	bool	peekNext (TokenInfo* tk = null);
	bool	peekNextType (Token req);
	String	peekNextString (int a = 1);
	String	describeCurrentPosition();
	String	describeTokenPosition();

	static Lexer* GetCurrentLexer();

	inline bool hasValidToken() const
	{
		return (m_tokenPosition < m_tokens.end() and m_tokenPosition >= m_tokens.begin());
	}

	inline TokenInfo* token() const
	{
		ASSERT (hasValidToken());
		return &(*m_tokenPosition);
	}

	inline bool isAtEnd() const
	{
		return m_tokenPosition == m_tokens.end();
	}

	inline Token tokenType() const
	{
		return token()->type;
	}

	inline void skip (int a = 1)
	{
		m_tokenPosition += a;
	}

	inline int position()
	{
		return m_tokenPosition - m_tokens.begin();
	}

	inline void setPosition (int pos)
	{
		m_tokenPosition = m_tokens.begin() + pos;
	}

	// If @tok is given, describes the token. If not, describes @tok_type.
	static inline String DescribeTokenType (Token toktype)
	{
		return DescribeTokenPrivate (toktype, null);
	}

	static inline String DescribeToken (TokenInfo* tok)
	{
		return DescribeTokenPrivate (tok->type, tok);
	}

private:
	TokenList		m_tokens;
	Iterator		m_tokenPosition;

	void	processFileInternal(String fileName);

	// read a mandatory token from scanner
	void mustGetFromScanner (LexerScanner& sc, Token tt =Token::Any);
	void checkFileHeader (LexerScanner& sc);

	static String DescribeTokenPrivate (Token tok_type, TokenInfo* tok);
};

#endif // BOTC_LEXER_H
