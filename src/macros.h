/*
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

#ifndef BOTC_MACROS_H
#define BOTC_MACROS_H

#include <ciso646>

#if !defined (__cplusplus) or __cplusplus < 201103L
# error botc requires a C++11-compliant compiler to be built
#endif

// Application name and version
#define APPNAME "botc"
#define VERSION_MAJOR	1
#define VERSION_MINOR	1
#define VERSION_PATCH 	0

#define MACRO_TO_STRING_HELPER(A) #A
#define MACRO_TO_STRING(A) MACRO_TO_STRING_HELPER(A)

#define MAKE_VERSION_NUMBER(MAJ, MIN, PAT) ((MAJ * 10000) + (MIN * 100) + PAT)
#define VERSION_NUMBER MAKE_VERSION_NUMBER (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

#if VERSION_PATCH > 0
# define VERSION_STRING MACRO_TO_STRING(VERSION_MAJOR) \
					"." MACRO_TO_STRING(VERSION_MINOR) \
					"." MACRO_TO_STRING(VERSION_PATCH)
#else
# define VERSION_STRING MACRO_TO_STRING(VERSION_MAJOR) \
					"." MACRO_TO_STRING(VERSION_MINOR)
#endif

// On Windows, files are case-insensitive
#if (defined(WIN32) or defined(_WIN32) or defined(__WIN32)) and !defined(__CYGWIN__)
# define FILE_CASEINSENSITIVE
#endif

#define named_enum enum
#define elif else if
#define types public
#define countof(A) ((int) (sizeof A / sizeof *A))

#ifndef __GNUC__
# define __attribute__(X)
#endif

template<typename... argtypes>
void error (const char* fmtstr, const argtypes&... args);

#ifndef RELEASE
# define BOTC_GENERIC_ASSERT(A,B,OP,COMPLAINT) ((A OP B) ? (void) 0 : \
	error ("assertion failed at " __FILE__ ":" MACRO_TO_STRING(__LINE__) ": " #A " (%1) " COMPLAINT " " #B " (%2)", A, B));
#else
# define BOTC_GENERIC_ASSERT(A,B,OP,COMPLAINT) {}
#endif

#define ASSERT_EQ(A,B) BOTC_GENERIC_ASSERT (A, B, ==, "does not equal")
#define ASSERT_NE(A,B) BOTC_GENERIC_ASSERT (A, B, !=, "is no different from")
#define ASSERT_LT(A,B) BOTC_GENERIC_ASSERT (A, B, <, "is not less than")
#define ASSERT_GT(A,B) BOTC_GENERIC_ASSERT (A, B, >, "is not greater than")
#define ASSERT_LT_EQ(A,B) BOTC_GENERIC_ASSERT (A, B, <=, "is not at most")
#define ASSERT_GT_EQ(A,B) BOTC_GENERIC_ASSERT (A, B, >=, "is not at least")
#define ASSERT_RANGE(A,MIN,MAX) { ASSERT_LT_EQ(A, MAX); ASSERT_GT_EQ(A, MIN); }
#define ASSERT(A) ASSERT_EQ (A, true)

#define DELETE_COPY(T) \
public: \
	T (T& other) = delete; \
	T& operator= (T& other) = delete;

#endif // BOTC_MACROS_H
