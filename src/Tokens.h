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

#ifndef TOKENS_H
#define TOKENS_H

#include <climits>

// =======================================================
enum EToken
{
	// Non-word tokens
	tkEquals,				// ----- 0
	tkBrackets,				// - 1
	tkAddAssign,			// - 2
	tkSubAssign,			// - 3
	tkMultiplyAssign,		// - 4
	tkDivideAssign,			// ----- 5
	tkModulusAssign,		// - 6
	tkSingleQuote,			// - 7
	tkDollarSign,			// - 8
	tkParenStart,			// - 9
	tkParenEnd,				// ----- 10
	tkBracketStart,			// - 11
	tkBracketEnd,			// - 12
	tkBraceStart,			// - 13
	tkBraceEnd,				// - 14
	tkAssign,				// ----- 15
	tkPlus,					// - 16
	tkMinus,				// - 17
	tkMultiply,				// - 18
	tkDivide,				// - 19
	tkModulus,				// ----- 20
	tkComma,				// - 21
	tkLesser,				// - 22
	tkGreater,				// - 23
	tkDot,					// - 24
	tkColon,				// ----- 25
	tkSemicolon,			// - 26
	tkHash,					// - 27
	tkExclamationMark,		// - 28
	tkArrow,				// - 29

	// --------------
	// Named tokens
	tkBool,					// ----- 30
	tkBreak,				// - 31
	tkCase,					// - 32
	tkContinue,				// - 33
	tkConst,				// - 34
	tkDefault,				// ----- 35
	tkDo,					// - 36
	tkElse,					// - 37
	tkEvent,				// - 38
	tkEventdef,				// - 39
	tkFor,					// ----- 40
	tkFuncdef,				// - 41
	tkGoto,					// - 42
	tkIf,					// - 43
	tkInt,					// - 44
	tkMainloop,				// ----- 45
	tkOnenter,				// - 46
	tkOnexit,				// - 47
	tkState,				// - 48
	tkSwitch,				// - 49
	tkStr,					// ----- 50
	tkVoid,					// - 51
	tkWhile,				// - 52

	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	tkEnum,					// - 53
	tkFunc,					// - 54
	tkReturn,				// ----- 55

	// --------------
	// Generic tokens
	tkSymbol,				// - 56
	tkNumber,				// - 57
	tkString,				// - 58

	tkFirstNamedToken		= tkBool,
	tkLastNamedToken		= (int) tkSymbol - 1,
	tkAny					= INT_MAX
};

#endif