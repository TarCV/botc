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
#include "str.h"

static const std::nullptr_t null = nullptr;

// Byte datatype
typedef int32_t word;
typedef unsigned char byte;

// Parser mode: where is the parser at?
enum parsermode_e
{
	MODE_TOPLEVEL,	// at top level
	MODE_EVENT,		// inside event definition
	MODE_MAINLOOP,	// inside mainloop
	MODE_ONENTER,	// inside onenter
	MODE_ONEXIT,	// inside onexit
};

enum type_e
{
	TYPE_UNKNOWN = 0,
	TYPE_VOID,
	TYPE_INT,
	TYPE_STRING,
	TYPE_BOOL,
};

// Script mark and reference
struct byte_mark
{
	string		name;
	int			pos;
};

struct mark_reference
{
	byte_mark*	target;
	int			pos;
};

class script_error : public std::exception
{
	public:
		script_error (const string& msg) : m_msg (msg) {}

		inline const char* what() const throw()
		{
			return m_msg.c_str();
		}

	private:
		string m_msg;
};

// ====================================================================
// Generic union
template <class T> union generic_union
{
	T as_t;
	byte as_bytes[sizeof (T)];
	char as_chars[sizeof (T)];
	double as_double;
	float as_float;
	int as_int;
	word as_word;
};

template<class T> inline T abs (T a)
{
	return (a >= 0) ? a : -a;
}

#ifdef IN_IDE_PARSER
using FILE = void;
#endif

#endif // BOTC_TYPES_H