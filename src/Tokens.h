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
	tkLeftShiftAssign,
	tkRightShiftAssign,
	tkEquals,
	tkNotEquals,
	tkBrackets,
	tkAddAssign,
	tkSubAssign,
	tkMultiplyAssign,
	tkDivideAssign,
	tkModulusAssign,
	tkLeftShift,
	tkRightShift,
	tkAtLeast,
	tkAtMost,
	tkDoubleAmperstand,
	tkDoubleBar,
	tkDoublePlus,
	tkDoubleMinus,
	tkSingleQuote,
	tkDollarSign,
	tkParenStart,
	tkParenEnd,
	tkBracketStart,
	tkBracketEnd,
	tkBraceStart,
	tkBraceEnd,
	tkAssign,
	tkPlus,
	tkMinus,
	tkMultiply,
	tkDivide,
	tkModulus,
	tkComma,
	tkLesser,
	tkGreater,
	tkDot,
	tkColon,
	tkSemicolon,
	tkHash,
	tkExclamationMark,
	tkAmperstand,
	tkBar,
	tkCaret,
	tkQuestionMark,
	tkArrow,

	// --------------
	// Named tokens
	tkBool,
	tkBreak,
	tkCase,
	tkContinue,
	tkConst,
	tkDefault,
	tkDo,
	tkElse,
	tkEvent,
	tkEventdef,
	tkFor,
	tkFuncdef,
	tkGoto,
	tkIf,
	tkInt,
	tkMainloop,
	tkOnenter,
	tkOnexit,
	tkState,
	tkSwitch,
	tkStr,
	tkVoid,
	tkWhile,
	tkTrue,
	tkFalse,

	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	tkEnum,
	tkFunc,
	tkReturn,

	// --------------
	// Generic tokens
	tkSymbol,
	tkNumber,
	tkString,

	tkFirstNamedToken		= tkBool,
	tkLastNamedToken		= (int) tkSymbol - 1,
	tkAny					= INT_MAX
};

#endif