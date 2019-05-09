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

#ifndef TOKENS_H
#define TOKENS_H

#include <climits>
#include "macros.h"

named_enum class Token
{
	// Non-word tokens
	LeftShiftAssign,
	RightShiftAssign,
	Equals,
	NotEquals,
	AddAssign,
	SubAssign,
	MultiplyAssign,
	DivideAssign,
	ModulusAssign,
	LeftShift,
	RightShift,
	AtLeast,
	AtMost,
	DoubleAmperstand,
	DoubleBar,
	DoublePlus,
	DoubleMinus,
	SingleQuote,
	ParenStart,
	ParenEnd,
	BracketStart,
	BracketEnd,
	BraceStart,
	BraceEnd,
	Assign,
	Plus,
	Minus,
	Multiply,
	Divide,
	Modulus,
	Comma,
	Lesser,
	Greater,
	Dot,
	Colon,
	Semicolon,
	Hash,
	ExclamationMark,
	Amperstand,
	Bar,
	Caret,
	QuestionMark,
	Arrow,

	// --------------
	// Named tokens
	Bool,
	Break,
	Case,
	Continue,
	Const,
	Constexpr,
	Default,
	Do,
	Else,
	Event,
	Eventdef,
	For,
	Funcdef,
	If,
	Int,
	Mainloop,
	Onenter,
	Onexit,
	State,
	Switch,
	Str,
	Void,
	While,
	True,
	False,

	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	Enum,
	Func,
	Return,

	BuiltinDef,
	Array,

	// --------------
	// Generic tokens
	Symbol,
	Number,
	String,
	Any,

	NumValues
};

static Token const FirstNamedToken = Token::Bool;
static Token const LastNamedToken = Token::Array;

#endif
