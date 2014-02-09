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
types:
	struct Token
	{
		EToken		type;
		String		text;
		String		file;
		int			line;
		int			column;
	};

	using TokenList	= List<Token>;
	using Iterator	= TokenList::Iterator;

public:
	Lexer();
	~Lexer();

	void	ProcessFile (String file_name);
	bool	GetNext (EToken req = tkAny);
	void	MustGetNext (EToken tok);
	void	MustGetAnyOf (const List<EToken>& toks);
	int		GetOneSymbol (const StringList& syms);
	void	TokenMustBe (EToken tok);
	bool	PeekNext (Token* tk = null);
	bool	PeekNextType (EToken req);
	String	PeekNextString (int a = 1);
	String	DescribePosition();

	static Lexer* GetCurrentLexer();

	inline bool HasValidToken() const
	{
		return (mTokenPosition < mTokens.end() && mTokenPosition >= mTokens.begin());
	}

	inline Token* GetToken() const
	{
		assert (HasValidToken() == true);
		return &(*mTokenPosition);
	}

	inline bool IsAtEnd() const
	{
		return mTokenPosition == mTokens.end();
	}

	inline EToken GetTokenType() const
	{
		return GetToken()->type;
	}

	inline void Skip (int a = 1)
	{
		mTokenPosition += a;
	}

	inline int GetPosition()
	{
		return mTokenPosition - mTokens.begin();
	}

	inline void SetPosition (int pos)
	{
		mTokenPosition = mTokens.begin() + pos;
	}

	// If @tok is given, describes the token. If not, describes @tok_type.
	static inline String DescribeTokenType (EToken toktype)
	{
		return DescribeTokenPrivate (toktype, null);
	}

	static inline String DescribeToken (Token* tok)
	{
		return DescribeTokenPrivate (tok->type, tok);
	}

private:
	TokenList		mTokens;
	Iterator		mTokenPosition;

	// read a mandatory token from scanner
	void MustGetFromScanner (LexerScanner& sc, EToken tt = tkAny);
	void CheckFileHeader (LexerScanner& sc);

	static String DescribeTokenPrivate (EToken tok_type, Token* tok);
};

#endif // BOTC_LEXER_H
