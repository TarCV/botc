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

#ifndef BOTC_TYPES_H
#define BOTC_TYPES_H

#include <cstdlib>
#include <stdexcept>
#include "String.h"

static const std::nullptr_t null = nullptr;

// =============================================================================
//
// Byte datatype
//
typedef int32_t word;
typedef unsigned char byte;

// =============================================================================
//
// Parser mode: where is the parser at?
//
enum EParserMode
{
	ETopLevelMode,		// at top level
	EEventMode,			// inside event definition
	EMainLoopMode,		// inside mainloop
	EOnenterMode,		// inside onenter
	EOnexitMode,		// inside onexit
};

// =============================================================================
//
enum EType
{
	EUnknownType = 0,
	EVoidType,
	EIntType,
	EStringType,
	EBoolType,
};

// =============================================================================
//
struct ByteMark
{
	String		name;
	int			pos;
};

// =============================================================================
//
struct MarkReference
{
	ByteMark*	target;
	int			pos;
};

// =============================================================================
//
class ScriptError : public std::exception
{
	public:
		ScriptError (const String& msg) :
			mMsg (msg) {}

		inline const char* what() const throw()
		{
			return mMsg;
		}

	private:
		String mMsg;
};

// =============================================================================
//
// Get absolute value of @a
//
template<class T> inline T abs (T a)
{
	return (a >= 0) ? a : -a;
}

#ifdef IN_IDE_PARSER
using FILE = void;
#endif

#endif // BOTC_TYPES_H