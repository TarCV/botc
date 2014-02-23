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

#include "Main.h"
#include "LexerScanner.h"

class Lexer
{
public:
	struct TokenInfo
	{
		ETokenType	type;
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

	void	ProcessFile (String file_name);
	bool	Next (ETokenType req = TK_Any);
	void	MustGetNext (ETokenType tok);
	void	MustGetAnyOf (const List<ETokenType>& toks);
	int		GetOneSymbol (const StringList& syms);
	void	TokenMustBe (ETokenType tok);
	bool	PeekNext (TokenInfo* tk = null);
	bool	PeekNextType (ETokenType req);
	String	PeekNextString (int a = 1);
	String	DescribeCurrentPosition();
	String	DescribeTokenPosition();

	static Lexer* GetCurrentLexer();

	inline bool HasValidToken() const
	{
		return (mTokenPosition < mTokens.end() && mTokenPosition >= mTokens.begin());
	}

	inline TokenInfo* Token() const
	{
		assert (HasValidToken() == true);
		return &(*mTokenPosition);
	}

	inline bool IsAtEnd() const
	{
		return mTokenPosition == mTokens.end();
	}

	inline ETokenType TokenType() const
	{
		return Token()->type;
	}

	inline void Skip (int a = 1)
	{
		mTokenPosition += a;
	}

	inline int Position()
	{
		return mTokenPosition - mTokens.begin();
	}

	inline void SetPosition (int pos)
	{
		mTokenPosition = mTokens.begin() + pos;
	}

	// If @tok is given, describes the token. If not, describes @tok_type.
	static inline String DescribeTokenType (ETokenType toktype)
	{
		return DescribeTokenPrivate (toktype, null);
	}

	static inline String DescribeToken (TokenInfo* tok)
	{
		return DescribeTokenPrivate (tok->type, tok);
	}

private:
	TokenList		mTokens;
	Iterator		mTokenPosition;

	// read a mandatory token from scanner
	void MustGetFromScanner (LexerScanner& sc, ETokenType tt =TK_Any);
	void CheckFileHeader (LexerScanner& sc);

	static String DescribeTokenPrivate (ETokenType tok_type, TokenInfo* tok);
};

#endif // BOTC_LEXER_H
