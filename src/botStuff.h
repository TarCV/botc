/*
	Copyright 2000-2010 Brad Carney
	Copyright 2012-2014 Teemu Piippo
	Copyright 2019-2020 TarCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from this
	   software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

// Numeric values and stuff from zandronum bots.h

#ifndef BOTC_BOTSTUFF_H
#define BOTC_BOTSTUFF_H

#include "main.h"

namespace Limits
{
	static constexpr int MaxStates			= 256;
	static constexpr int MaxEvents			= 32;
	static constexpr int MaxGlobalEvents	= 32;
	static constexpr int MaxGlobalVars		= 128;
	static constexpr int MaxGlobalArrays	= 16;
	static constexpr int MaxArraySize		= 65536;
	static constexpr int MaxStateVars		= 16;
	static constexpr int MaxStringlistSize	= 128;
	static constexpr int MaxStringLength	= 256;
	static constexpr int MaxReactionTime	= 52;
	static constexpr int MaxStoredEvents	= 64;
};

named_enum class DataHeader
{
	Command,
	StateIndex,
	StateName,
	OnEnter,
	MainLoop,
	OnExit,
	Event,
	EndOnEnter,
	EndMainLoop,
	EndOnExit,
	EndEvent,
	IfGoto,
	IfNotGoto,
	Goto,
	OrLogical,
	AndLogical,
	OrBitwise,
	EorBitwise,
	AndBitwise,
	Equals,
	NotEquals,
	LessThan,
	AtMost,
	GreaterThan,
	AtLeast,
	NegateLogical,
	LeftShift,
	RightShift,
	Add,
	Subtract,
	UnaryMinus,
	Multiply,
	Divide,
	Modulus,
	PushNumber,
	PushStringIndex,
	PushGlobalVar,
	PushLocalVar,
	DropStackPosition,
	ScriptVarList,
	StringList,
	IncreaseGlobalVar,
	DecreaseGlobalVar,
	AssignGlobalVar,
	AddGlobalVar,
	SubtractGlobalVar,
	MultiplyGlobalVar,
	DivideGlobalVar,
	ModGlobalVar,
	IncreaseLocalVar,
	DecreaseLocalVar,
	AssignLocalVar,
	AddLocalVar,
	SubtractLocalVar,
	MultiplyLocalVar,
	DivideLocalVar,
	ModLocalVar,
	CaseGoto,
	Drop,
	IncreaseGlobalArray,
	DecreaseGlobalArray,
	AssignGlobalArray,
	AddGlobalArray,
	SubtractGlobalArray,
	MultiplyGlobalArray,
	DivideGlobalArray,
	ModGlobalArray,
	PushGlobalArray,
	Swap,
	Dup,
	ArraySet,

	NumValues
};

#endif	// BOTC_BOTSTUFF_H
