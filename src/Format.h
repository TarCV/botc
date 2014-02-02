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

#ifndef BOTC_FORMAT_H
#define BOTC_FORMAT_H

#include "String.h"
#include "Containers.h"

class FormatArgument
{
	public:
		FormatArgument (const String& a) :
			mText (a) {}

		FormatArgument (char a) :
			mText (a) {}

		FormatArgument (int a) :
			mText (String::FromNumber (a)) {}

		FormatArgument (long a) :
			mText (String::FromNumber (a)) {}

		FormatArgument (const char* a) :
			mText (a) {}

		FormatArgument (void* a)
		{
			mText.SPrintf ("%p", a);
		}

		FormatArgument (const void* a)
		{
			mText.SPrintf ("%p", a);
		}

		template<class T> FormatArgument (const List<T>& list)
		{
			if (list.IsEmpty())
			{
				mText = "{}";
				return;
			}

			mText = "{ ";

			for (const T & a : list)
			{
				if (&a != &list[0])
					mText += ", ";

				mText += FormatArgument (a).AsString();
			}

			mText += " }";
		}

		inline const String& AsString() const
		{
			return mText;
		}

	private:
		String mText;
};

template<class T> String custom_format (T a, const char* fmtstr)
{
	String out;
	out.SPrintf (fmtstr, a);
	return out;
}

String FormatArgs (const List<FormatArgument>& args);
void PrintArgs (FILE* fp, const List<FormatArgument>& args);
void DoError (String msg);

#ifndef IN_IDE_PARSER
# define Format(...) FormatArgs ({__VA_ARGS__})
# define PrintTo(A, ...) PrintArgs (A, {__VA_ARGS__})
# define Print(...) PrintArgs (stdout, {__VA_ARGS__})
# define Error(...) DoError (Format (__VA_ARGS__))
#else
String Format (void, ...);
void PrintTo (FILE* fp, ...);
void Print (void, ...);
void Error (void, ...);
#endif

#ifndef IN_IDE_PARSER
# ifdef DEBUG
#  define devf(...) fprint (stderr, __VA_ARGS__)
#  define dvalof( A ) fprint (stderr, "value of '%1' = %2\n", #A, A)
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

#endif // BOTC_FORMAT_H
