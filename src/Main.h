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

#ifndef BOTC_MAIN_H
#define BOTC_MAIN_H

#if !defined (__cplusplus) || __cplusplus < 201103L
# error botc requires a C++11-compliant compiler to be built
#endif

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include "Property.h"
#include "Types.h"
#include "Containers.h"
#include "String.h"
#include "Format.h"
#include "BotStuff.h"
#include "Tokens.h"

// Application name and version
#define APPNAME "botc"
#define VERSION_MAJOR	1
#define VERSION_MINOR	0
#define VERSION_PATCH 	0

#define MAKE_VERSION_NUMBER(MAJ, MIN, PAT) ((MAJ * 10000) + (MIN * 100) + PAT)
#define VERSION_NUMBER MAKE_VERSION_NUMBER (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

// On Windows, files are case-insensitive
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
#define FILE_CASEINSENSITIVE
#endif

#define elif else if

#define types public
#define countof(A) ((int) (sizeof A / sizeof *A))

// Shortcut for zeroing something
#define ZERO(obj) memset (&obj, 0, sizeof (obj));

enum EFormLength
{
	ELongForm,
	EShortForm
};

String MakeObjectFileName (String s);
EType GetTypeByName (String token);
String GetTypeName (EType type);
String GetVersionString (EFormLength len);
String MakeVersionString (int major, int minor, int patch);

#ifndef __GNUC__
#define __attribute__(X)
#endif
#define deprecated __attribute__ ((deprecated))

#endif // BOTC_MAIN_H
