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

// Plural expression
#define PLURAL(n) (n != 1) ? "s" : ""

// Shortcut for zeroing something
#define ZERO(obj) memset (&obj, 0, sizeof (obj));

void error (const char* text, ...);
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

// Byte datatype
typedef int32_t word;
typedef unsigned char byte;

// Keywords
extern const char** g_Keywords;

bool IsKeyword (string s);
unsigned int NumKeywords ();

// Script mark and reference
struct ScriptMark {
	string name;
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

#endif // BOTC_COMMON_H
