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
#include "main.h"
#include "string.h"

// =============================================================================
//
int String::compare (const String& other) const
{
	return _string.compare (other.stdString());
}

// =============================================================================
//
void String::trim (int n)
{
	if (n > 0)
		_string = mid (0, length() - n).stdString();
	else
		_string = mid (n, -1).stdString();
}

// =============================================================================
//
String String::strip (const List<char>& unwanted)
{
	String copy (_string);

	for (char c : unwanted)
	{
		int pos = 0;
		while ((pos = copy.firstIndexOf (String (c))) != -1)
			copy.removeAt (pos--);
	}

	return copy;
}

// =============================================================================
//
String String::toUppercase() const
{
	String newstr (_string);

	for (char& c : newstr)
	{
		if (c >= 'a' && c <= 'z')
			c -= 'a' - 'A';
	}

	return newstr;
}

// =============================================================================
//
String String::toLowercase() const
{
	String newstr (_string);

	for (char& c : newstr)
	{
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
	}

	return newstr;
}

// =============================================================================
//
StringList String::split (char del) const
{
	String delimstr;
	delimstr += del;
	return split (delimstr);
}

// =============================================================================
//
StringList String::split (const String& del) const
{
	StringList res;
	int a = 0;
	int b;

	// Find all separators and store the text left to them.
	while ((b = firstIndexOf (del, a)) != -1)
	{
		String sub = mid (a, b);

		if (sub.length() > 0)
			res << sub;

		a = b + del.length();
	}

	// Add the string at the right of the last separator
	if (a < (int) length())
		res.append (mid (a, length()));

	return res;
}

// =============================================================================
//
void String::replace (const char* a, const char* b)
{
	long pos;

	while ((pos = firstIndexOf (a)) != -1)
		_string = _string.replace (pos, strlen (a), b);
}

// =============================================================================
//
int String::count (char needle) const
{
	int needles = 0;

	for (const char & c : _string)
		if (c == needle)
			needles++;

	return needles;
}

// =============================================================================
//
String String::mid (long a, long b) const
{
	if (b == -1)
		b = length();

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
	strncpy (newstr, _string.c_str() + a, b - a);
	newstr[b - a] = '\0';

	String other (newstr);
	delete[] newstr;
	return other;
}

// =============================================================================
//
int String::wordPosition (int n) const
{
	int count = 0;

	for (int i = 0; i < length(); ++i)
	{
		if (_string[i] != ' ')
			continue;

		if (++count < n)
			continue;

		return i;
	}

	return -1;
}

// =============================================================================
//
int String::firstIndexOf (const char* c, int a) const
{
	int pos = _string.find (c, a);

	if (pos == (int) std::string::npos)
		return -1;

	return pos;
}

// =============================================================================
//
int String::lastIndexOf (const char* c, int a) const
{
	if (a == -1 || a >= length())
		a = length() - 1;

	for (; a > 0; a--)
		if (_string[a] == c[0] && strncmp (_string.c_str() + a, c, strlen (c)) == 0)
			return a;

	return -1;
}

// =============================================================================
//
void String::dump() const
{
	print ("`%1`:\n", c_str());
	int i = 0;

	for (char u : _string)
		print ("\t%1. [%d2] `%3`\n", i++, u, String (u));
}

// =============================================================================
//
long String::toLong (bool* ok, int base) const
{
	errno = 0;
	char* endptr;
	long i = strtol (_string.c_str(), &endptr, base);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
float String::toFloat (bool* ok) const
{
	errno = 0;
	char* endptr;
	float i = strtof (_string.c_str(), &endptr);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
double String::toDouble (bool* ok) const
{
	errno = 0;
	char* endptr;
	double i = strtod (_string.c_str(), &endptr);

	if (ok)
		*ok = (errno == 0 && *endptr == '\0');

	return i;
}

// =============================================================================
//
String String::operator+ (const String& data) const
{
	String newString = *this;
	newString.append (data);
	return newString;
}

// =============================================================================
//
String String::operator+ (const char* data) const
{
	String newstr = *this;
	newstr.append (data);
	return newstr;
}

// =============================================================================
//
bool String::isNumeric() const
{
	bool gotDot = false;

	for (const char & c : _string)
	{
		// Allow leading hyphen for negatives
		if (&c == &_string[0] && c == '-')
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
bool String::endsWith (const String& other)
{
	if (length() < other.length())
		return false;

	const int ofs = length() - other.length();
	return strncmp (c_str() + ofs, other.c_str(), other.length()) == 0;
}

// =============================================================================
//
bool String::startsWith (const String& other)
{
	if (length() < other.length())
		return false;

	return strncmp (c_str(), other.c_str(), other.length()) == 0;
}

// =============================================================================
//
void String::sprintf (const char* fmtstr, ...)
{
	char* buf;
	int bufsize = 256;
	va_list va;
	va_start (va, fmtstr);

	do
		buf = new char[bufsize];
	while (vsnprintf (buf, bufsize, fmtstr, va) >= bufsize);

	va_end (va);
	_string = buf;
	delete[] buf;
}

// =============================================================================
//
String StringList::join (const String& delim)
{
	String result;

	for (const String& it : deque())
	{
		if (result.isEmpty() == false)
			result += delim;

		result += it;
	}

	return result;
}

// =============================================================================
//
bool String::maskAgainst (const String& pattern) const
{
	// Elevate to uppercase for case-insensitive matching
	String pattern_upper = pattern.toUppercase();
	String this_upper = toUppercase();
	const char* maskstring = pattern_upper.c_str();
	const char* mptr = &maskstring[0];

	for (const char* sptr = this_upper.c_str(); *sptr != '\0'; sptr++)
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
String String::fromNumber (int a)
{
	char buf[32];
	::sprintf (buf, "%d", a);
	return String (buf);
}

// =============================================================================
//
String String::fromNumber (long a)
{
	char buf[32];
	::sprintf (buf, "%ld", a);
	return String (buf);
}
