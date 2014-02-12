/*
	Copyright 2000-2010 Brad Carney
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

// Numeric values and stuff from zandronum bots.h

#ifndef BOTC_BOTSTUFF_H
#define BOTC_BOTSTUFF_H

#include "Main.h"

static const int gMaxStates			= 256;
static const int gMaxEvents			= 32;
static const int gMaxGlobalEvents	= 32;
static const int gMaxGlobalVars		= 128;
static const int gMaxGlobalArrays	= 16;
static const int gMaxArraySize		= 65536;
static const int gMaxStateVars		= 16;
static const int gMaxStringlistSize	= 128;
static const int gMaxStringLength	= 256;
static const int gMaxReactionTime	= 52;
static const int gMaxStoredEvents	= 64;

named_enum DataHeader
{
	DH_Command,
	DH_StateIndex,
	DH_StateName,
	DH_OnEnter,
	DH_MainLoop,
	DH_OnExit,
	DH_Event,
	DH_EndOnEnter,
	DH_EndMainLoop,
	DH_EndOnExit,
	DH_EndEvent,
	DH_IfGoto,
	DH_IfNotGoto,
	DH_Goto,
	DH_OrLogical,
	DH_AndLogical,
	DH_OrBitwise,
	DH_EorBitwise,
	DH_AndBitwise,
	DH_Equals,
	DH_NotEquals,
	DH_LessThan,
	DH_AtMost,
	DH_GreaterThan,
	DH_AtLeast,
	DH_NegateLogical,
	DH_LeftShift,
	DH_RightShift,
	DH_Add,
	DH_Subtract,
	DH_UnaryMinus,
	DH_Multiply,
	DH_Divide,
	DH_Modulus,
	DH_PushNumber,
	DH_PushStringIndex,
	DH_PushGlobalVar,
	DH_PushLocalVar,
	DH_DropStackPosition,
	DH_ScriptVarList,
	DH_StringList,
	DH_IncreaseGlobalVar,
	DH_DecreaseGlobalVar,
	DH_AssignGlobalVar,
	DH_AddGlobalVar,
	DH_SubtractGlobalVar,
	DH_MultiplyGlobalVar,
	DH_DivideGlobalVar,
	DH_ModGlobalVar,
	DH_IncreaseLocalVar,
	DH_DecreaseLocalVar,
	DH_AssignLocalVar,
	DH_AddLocalVar,
	DH_SubtractLocalVar,
	DH_MultiplyLocalVar,
	DH_DivideLocalVar,
	DH_ModLocalVar,
	DH_CaseGoto,
	DH_Drop,
	DH_IncreaseGlobalArray,
	DH_DecreaseGlobalArray,
	DH_AssignGlobalArray,
	DH_AddGlobalArray,
	DH_SubtractGlobalArray,
	DH_MultiplyGlobalArray,
	DH_DivideGlobalArray,
	DH_ModGlobalArray,
	DH_PushGlobalArray,
	DH_Swap,
	DH_Dup,
	DH_ArraySet,
	numDataHeaders
};

#endif	// BOTC_BOTSTUFF_H
