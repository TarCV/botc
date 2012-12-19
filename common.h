/*
 *	botc source code
 *	Copyright (C) 2012 Santeri `Dusk` Piippo
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	
 *	1. Redistributions of source code must retain the above copyright notice,
 *	   this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright notice,
 *	   this list of conditions and the following disclaimer in the documentation
 *	   and/or other materials provided with the distribution.
 *	3. Neither the name of the developer nor the names of its contributors may
 *	   be used to endorse or promote products derived from this software without
 *	   specific prior written permission.
 *	4. Redistributions in any form must be accompanied by information on how to
 *	   obtain complete source code for the software and any accompanying
 *	   software that uses the software. The source code must either be included
 *	   in the distribution or be available for no more than the cost of
 *	   distribution plus a nominal fee, and must be freely redistributable
 *	   under reasonable conditions. For an executable file, complete source
 *	   code means the source code for all modules it contains. It does not
 *	   include source code for modules or files that typically accompany the
 *	   major components of the operating system on which the executable file
 *	   runs.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "bots.h"
#include "str.h"

// Application name and version
#define APPNAME "botc"
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_REVISION 999

// On Windows, files are case-insensitive
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
	#define FILE_CASEINSENSITIVE
#endif

// Parser mode: where is the parser at?
enum parsermode {
	MODE_TOPLEVEL,	// at top level
	MODE_EVENT,		// inside event definition
	MODE_MAINLOOP,	// inside mainloop
	MODE_ONENTER,	// inside onenter
	MODE_ONEXIT,	// inside onexit
};

#define CHECK_FILE(pointer,path,action) \
	if (!pointer) { \
		error ("couldn't open %s for %s!\n", (char*)path, action); \
		exit (1); \
	}

// Shortcut for formatting
#define PERFORM_FORMAT(in, out) \
	va_list v; \
	va_start (v, in); \
	char* out = vdynformat (in, v, 256); \
	va_end (v);

// Plural expression
#define PLURAL(n) (n != 1) ? "s" : ""

// Shortcut for zeroing something
#define ZERO(obj) memset (&obj, 0, sizeof (obj));

void error (const char* text, ...);
char* ObjectFileName (str s);
bool fexists (char* path);

// Make the parser's variables globally available
extern int g_NumStates;
extern int g_NumEvents;
extern int g_CurMode;
extern str g_CurState;

#define neurosphere if (g_Neurosphere)
#define twice for (int repeat_token = 0; repeat_token < 2; repeat_token++)

#ifndef __GNUC__
#define __attribute__(X)
#endif

// Power function
template<class T> T pow (T a, unsigned int b) {
	if (!b)
		return 1;
	
	T r = a;
	while (b > 1) {
		b--;
		r = r * a;
	}
	
	return r;
}

// Whitespace check
inline bool IsCharWhitespace (char c) {
	return (c <= 32 || c == 127 || c == 255);
}

// Byte datatype
typedef int32_t word;
typedef unsigned char byte;

// Keywords
#ifndef __MAIN_CXX__
extern const char** g_Keywords;
#endif

bool IsKeyword (str s);
unsigned int NumKeywords ();

// Script mark and reference
struct ScriptMark {
	str name;
	size_t pos;
};

struct ScriptMarkReference {
	unsigned int num;
	size_t pos;
};

// ====================================================================
// Generic union
template <class T> union union_t {
	T val;
	byte b[sizeof (T)];
	char c[sizeof (T)];
	double d;
	float f;
	int i;
	word w;
};

// ====================================================================
// Finds a byte in the given value.
template <class T> inline unsigned char GetByteIndex (T a, unsigned int b) {
	union_t<T> uni;
	uni.val = a;
	return uni.b[b];
}

#endif // __COMMON_H__