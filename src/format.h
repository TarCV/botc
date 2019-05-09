/*
	Copyright 2012-2014 Teemu Piippo
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

#ifndef BOTC_FORMAT_H
#define BOTC_FORMAT_H

#include <vector>
#include "stringClass.h"
#include "list.h"
#include "enumstrings.h"

inline String MakeFormatArgument (const String& a)
{
	return a;
}

inline String MakeFormatArgument (char a)
{
	return String (a);
}

inline String MakeFormatArgument (int a)
{
	return String::fromNumber (a);
}

inline String MakeFormatArgument (long a)
{
	return String::fromNumber (a);
}

inline String MakeFormatArgument (long long a)
{
	return String::fromNumber (a);
}

inline String MakeFormatArgument (double a)
{
	return String::fromNumber (a);
}

inline String MakeFormatArgument (size_t a)
{
	return String::fromNumber (long (a));
}

inline String MakeFormatArgument (const char* a)
{
	return a;
}

inline String MakeFormatArgument (const void* a)
{
	String text;
	text.sprintf ("%p", a);
	return text;
}

inline String MakeFormatArgument (std::nullptr_t)
{
	return "(nullptr)";
}

template<class T>
inline String MakeFormatArgument (List<T> const& list)
{
	String result;

	if (list.isEmpty())
		return "{}";

	result = "{ ";

	for (auto it = list.begin(); it != list.end(); ++it)
	{
		if (it != list.begin())
			result += ", ";

		result += MakeFormatArgument (*it);
	}

	result += " }";
	return result;
}

class FormatArgument
{
public:
	template<typename T>
	FormatArgument (const T& a) :
		m_text (MakeFormatArgument (a)) {}

	inline const String& text() const
	{
		return m_text;
	}

private:
	String m_text;
};

#ifndef IN_IDE_PARSER
# ifdef DEBUG
#  define devf(...) PrintTo (stderr, __VA_ARGS__)
#  define dvalof( A ) PrintTo (stderr, "value of '%1' = %2\n", #A, A)
# else
#  define devf(...)
#  define dvalof( A )
# endif // DEBUG
#else
// print something in debug builds
void devf (void, ...);

// print the value of @a
void dvalof (void a);
#endif // IN_IDE_PARSER

// _________________________________________________________________________________________________
//
// Formats the given string with the given args.
//
String formatArgs (const String& fmtstr, const std::vector<String>& args);

// _________________________________________________________________________________________________
//
// Expands the given arguments into a vector of strings.
//
template<typename T, typename... RestTypes>
void expandFormatArguments (std::vector<String>& data, const T& arg, const RestTypes& ... rest)
{
	data.push_back (FormatArgument (arg).text());
	expandFormatArguments (data, rest...);
}

static void expandFormatArguments (std::vector<String>& data) __attribute__ ( (unused));
static void expandFormatArguments (std::vector<String>& data)
{
	(void) data;
}

// _________________________________________________________________________________________________
//
// Formats the given formatter string and args and returns the string.
// This is essentially a modernized sprintf.
//
// Args in the format string take the form %n where n is a digit. The argument
// will be expanded to the nth argument passed. This is essentially Qt's
// QString::arg() syntax. Note: %0 is invalid.
//
// Arguments can be passed a modifier which takes the form of a character
// just before the digit. Currently supported modifiers are s, d and x.
//
// - s: The argument will expand into "s" if it would've not expanded into "1"
//      otherwise. If it would have expanded into "1" it will expand into an
//      empty string.
//
// - d: The argument expands into the numeric form of the first character of
//      its previous expansion. Use this to get numeric forms of @c char
//      arguments.
//
// - x: The numeric argument will be represented in hexadecimal notation. This
//      will work if the argument is a string representing a number. If the
//      argument did not expand into a number in the first place, 0 is used
//      and 0x0 is printed.
//
template<typename... argtypes>
String format (const String& fmtstr, const argtypes&... raw_args)
{
	std::vector<String> args;
	expandFormatArguments (args, raw_args...);
	ASSERT_EQ (args.size(), sizeof... (raw_args))
	return formatArgs (fmtstr, args);
}

// _________________________________________________________________________________________________
//
// This is an overload of format() where no arguments are supplied.
// It returns the formatter string as-is.
//
static String format (const String& fmtstr) __attribute__ ( (unused));
static String format (const String& fmtstr)
{
	return fmtstr;
}

// _________________________________________________________________________________________________
//
// Processes the given formatter string using format() and prints it to the
// specified file pointer.
//
template<typename... argtypes>
void printTo (FILE* fp, const String& fmtstr, const argtypes&... args)
{
	fprintf (fp, "%s", format (fmtstr, args...).c_str());
}

// _________________________________________________________________________________________________
//
// Processes the given formatter string using format() and appends it to the
// specified file by name.
//
template<typename... argtypes>
void printTo (const String& filename, const String& fmtstr, const argtypes&... args)
{
	FILE* fp = fopen (filename, "a");

	if (fp != null)
	{
		fprintf (fp, "%s", format (fmtstr, args...).c_str());
		fflush (fp);
		fclose (fp);
	}
}

// _________________________________________________________________________________________________
//
// Processes the given formatter string using format() and prints the result to
// stdout.
//
template<typename... argtypes>
void print (const String& fmtstr, const argtypes&... args)
{
	printTo (stdout, fmtstr, args...);
}

// _________________________________________________________________________________________________
//
// Throws an std::runtime_error with the processed formatted string. The program
// execution terminates after a call to this function as the exception is first
// caught in main() which prints the error to stderr and then exits.
//
template<typename... argtypes>
void error (const char* fmtstr, const argtypes&... args)
{
	error (format (String (fmtstr), args...));
}

template<typename... argtypes>
void warning(const String& fmtstr, const argtypes&... args)
{
	print("WARNING: " + fmtstr, args...);
}

//
// An overload of error() with no string formatting in between.
//
void error (const String& msg);

#endif // BOTC_FORMAT_H
