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

#include <cstring>
#include "Main.h"
#include "String.h"

// =============================================================================
//
int String::Compare (const String& other) const
{
	return mString.compare (other.STDString());
}

// =============================================================================
//
void String::Trim (int n)
{
	if (n > 0)
		mString = Mid (0, Length() - n).STDString();
	else
		mString = Mid (n, -1).STDString();
}

// =============================================================================
//
String String::Strip (const List< char >& unwanted)
{
	String copy (mString);

	for (char c : unwanted)
		for (int i = 0; i < copy.Length(); ++i)
			if (copy[i] == c)
				copy.RemoveAt (i);

	/*
	while(( pos = copy.first( c )) != -1 )
		copy.erase( pos );
	*/

	return copy;
}

// =============================================================================
//
String String::ToUppercase() const
{
	String newstr = mString;

	for (char & c : newstr)
		if (c >= 'a' && c <= 'z')
			c -= 'a' - 'A';

	return newstr;
}

// =============================================================================
//
String String::ToLowercase() const
{
	String newstr = mString;

	for (char & c : newstr)
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';

	return newstr;
}

// =============================================================================
//
StringList String::Split (char del) const
{
	String delimstr;
	delimstr += del;
	return Split (delimstr);
}

// =============================================================================
//
StringList String::Split (String del) const
{
	StringList res;
	long a = 0;

	// Find all separators and store the text left to them.
	for (;;)
	{
		long b = FirstIndexOf (del, a);

		if (b == -1)
			break;

		String sub = Mid (a, b);

		if (sub.Length() > 0)
			res.Append (Mid (a, b));

		a = b + del.Length();
	}

	// Add the string at the right of the last separator
	if (a < (int) Length())
		res.Append (Mid (a, Length()));

	return res;
}

// =============================================================================
//
void String::Replace (const char* a, const char* b)
{
	long pos;

	while ( (pos = FirstIndexOf (a)) != -1)
		mString = mString.replace (pos, strlen (a), b);
}

// =============================================================================
//
int String::Count (char needle) const
{
	int needles = 0;

	for (const char & c : mString)
		if (c == needle)
			needles++;

	return needles;
}

// =============================================================================
//
String String::Mid (long a, long b) const
{
	if (b == -1)
		b = Length();

	if (b == a)
		return "";

	if (b < a)
	{
		// Swap the variables
		int c = a;
		a = b;
		b = c;
	}

	char* newstr = new char[b - a + 1];
	strncpy (newstr, mString.c_str() + a, b - a);
	newstr[b - a] = '\0';

	String other (newstr);
	delete[] newstr;
	return other;
}

// =============================================================================
//
int String::WordPosition (int n) const
{
	int count = 0;

	for (int i = 0; i < Length(); ++i)
	{
		if (mString[i] != ' ')
			continue;

		if (++count < n)
			continue;

		return i;
	}

	return -1;
}

// =============================================================================
//
int String::FirstIndexOf (const char* c, int a) const
{
	for (; a < Length(); a++)
		if (mString[a] == c[0] && strncmp (mString.c_str() + a, c, strlen (c)) == 0)
			return a;

	return -1;
}

// =============================================================================
//
int String::LastIndexOf (const char* c, int a) const
{
	if (a == -1 || a >= Length())
		a = Length() - 1;

	for (; a > 0; a--)
		if (mString[a] == c[0] && strncmp (mString.c_str() + a, c, strlen (c)) == 0)
			return a;

	return -1;
}

// =============================================================================
//
void String::Dump() const
{
	Print ("`%1`:\n", CString());
	int i = 0;

	for (char u : mString)
		Print ("\t%1. [%d2] `%3`\n", i++, u, String (u));
}

// =============================================================================
//
long String::ToLong (bool* ok, int base) const
{
	errno = 0;
	char* endptr;
	long i = strtol (mString.c_str(), &endptr, base);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
float String::ToFloat (bool* ok) const
{
	errno = 0;
	char* endptr;
	float i = strtof (mString.c_str(), &endptr);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
double String::ToDouble (bool* ok) const
{
	errno = 0;
	char* endptr;
	double i = strtod (mString.c_str(), &endptr);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
String String::operator+ (const String& data) const
{
	String newString = *this;
	newString.Append (data);
	return newString;
}

// =============================================================================
//
String String::operator+ (const char* data) const
{
	String newstr = *this;
	newstr.Append (data);
	return newstr;
}

// =============================================================================
//
bool String::IsNumeric() const
{
	bool gotDot = false;

	for (const char & c : mString)
	{
		// Allow leading hyphen for negatives
		if (&c == &mString[0] && c == '-')
			continue;

		// Check for decimal point
		if (!gotDot && c == '.')
		{
			gotDot = true;
			continue;
		}

		if (c >= '0' && c <= '9')
			continue; // Digit

		// If the above cases didn't catch this character, it was
		// illegal and this is therefore not a number.
		return false;
	}

	return true;
}

// =============================================================================
//
bool String::EndsWith (const String& other)
{
	if (Length() < other.Length())
		return false;

	const int ofs = Length() - other.Length();
	return strncmp (CString() + ofs, other.CString(), other.Length()) == 0;
}

// =============================================================================
//
bool String::StartsWith (const String& other)
{
	if (Length() < other.Length())
		return false;

	return strncmp (CString(), other.CString(), other.Length()) == 0;
}

// =============================================================================
//
void String::SPrintf (const char* fmtstr, ...)
{
	char* buf;
	int bufsize = 256;
	va_list va;
	va_start (va, fmtstr);

	do
		buf = new char[bufsize];
	while (vsnprintf (buf, bufsize, fmtstr, va) >= bufsize);

	va_end (va);
	mString = buf;
	delete[] buf;
}

// =============================================================================
//
String StringList::Join (const String& delim)
{
	String result;

	for (const String& it : GetDeque())
	{
		if (result.IsEmpty() == false)
			result += delim;

		result += it;
	}

	return result;
}

// =============================================================================
//
bool String::MaskAgainst (const String& pattern) const
{
	// Elevate to uppercase for case-insensitive matching
	String pattern_upper = pattern.ToUppercase();
	String this_upper = ToUppercase();
	const char* maskstring = pattern_upper.CString();
	const char* mptr = &maskstring[0];

	for (const char* sptr = this_upper.CString(); *sptr != '\0'; sptr++)
	{
		if (*mptr == '?')
		{
			if (*(sptr + 1) == '\0')
			{
				// ? demands that there's a character here and there wasn't.
				// Therefore, mask matching fails
				return false;
			}
		}
		elif (*mptr == '*')
		{
			char end = *(++mptr);

			// If '*' is the final character of the message, all of the remaining
			// string matches against the '*'. We don't need to bother checking
			// the string any further.
			if (end == '\0')
				return true;

			// Skip to the end character
			while (*sptr != end && *sptr != '\0')
				sptr++;

			// String ended while the mask still had stuff
			if (*sptr == '\0')
				return false;
		}
		elif (*sptr != *mptr)
			return false;

		mptr++;
	}

	return true;
}

// =============================================================================
//
String String::FromNumber (int a)
{
	char buf[32];
	sprintf (buf, "%d", a);
	return String (buf);
}

// =============================================================================
//
String String::FromNumber (long a)
{
	char buf[32];
	sprintf (buf, "%ld", a);
	return String (buf);
}
