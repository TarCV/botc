/*
	Copyright (c) 2013-2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BOTC_COMMON_H
#define BOTC_COMMON_H

#if !defined (__cplusplus) || __cplusplus < 201103L
# error botc requires a C++11-compliant compiler to be built
#endif

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include "types.h"
#include "bots.h"
#include "str.h"
#include "containers.h"
#include "format.h"
#include "tokens.h"

// Application name and version
#define APPNAME "botc"
#define VERSION_MAJOR 0
#define VERSION_MINOR 999

// On Windows, files are case-insensitive
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
	#define FILE_CASEINSENSITIVE
#endif

// Parser mode: where is the parser at?
enum parsermode_e {
	MODE_TOPLEVEL,	// at top level
	MODE_EVENT,		// inside event definition
	MODE_MAINLOOP,	// inside mainloop
	MODE_ONENTER,	// inside onenter
	MODE_ONEXIT,	// inside onexit
};

enum type_e {
	TYPE_UNKNOWN = 0,
	TYPE_VOID,
	TYPE_INT,
	TYPE_STRING,
	TYPE_BOOL,
};

#define elif else if

#define CHECK_FILE(pointer,path,action) \
	if (!pointer) { \
		error ("couldn't open %s for %s!\n", path.chars(), action); \
		exit (1); \
	}

#define types public
#define countof(A) ((int) (sizeof A / sizeof *A))
#define autocast(A) (decltype(A))
#define assign_autocast(A,B) a = autocast(A) b

// Shortcut for zeroing something
#define ZERO(obj) memset (&obj, 0, sizeof (obj));

string ObjectFileName (string s);
bool fexists (string path);
type_e GetTypeByName (string token);
string GetTypeName (type_e type);

// Make the parser's variables globally available
extern int g_NumStates;
extern int g_NumEvents;
extern parsermode_e g_CurMode;
extern string g_CurState;

#define neurosphere if (g_Neurosphere)
#define twice for (int repeat_token = 0; repeat_token < 2; repeat_token++)

#ifndef __GNUC__
#define __attribute__(X)
#endif
#define deprecated __attribute__ ((deprecated))

// Power function
template<class T> T pow (T a, int b) {
	if (!b)
		return 1;
	
	T r = a;
	while (b > 1) {
		b--;
		r = r * a;
	}
	
	return r;
}

// Byte datatype
typedef int32_t word;
typedef unsigned char byte;

// Keywords
extern const char** g_Keywords;

bool IsKeyword (string s);
int NumKeywords ();

// Script mark and reference
struct ScriptMark {
	string name;
	size_t pos;
};

struct ScriptMarkReference {
	int num;
	size_t pos;
};

// ====================================================================
// Generic union
template <class T> union generic_union {
	T as_t;
	byte as_bytes[sizeof (T)];
	char as_chars[sizeof (T)];
	double as_double;
	float as_float;
	int as_int;
	word as_word;
};

// ====================================================================
// Finds a byte in the given value.
template <class T> inline unsigned char GetByteIndex (T a, int b) {
	union_t<T> uni;
	uni.val = a;
	return uni.b[b];
}

#endif // BOTC_COMMON_H
