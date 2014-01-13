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

#include <cstring>
#include "main.h"
#include "str.h"

// =============================================================================
//
int string::compare (const string& other) const
{
    return m_string.compare (other.std_string());
}

// =============================================================================
//
void string::trim (string::length_type n)
{
    if (n > 0)
        m_string = substring (0, length() - n).std_string();
    else
        m_string = substring (n, -1).std_string();
}

// =============================================================================
//
string string::strip (list<char> unwanted)
{
    string copy (m_string);

    for (char c : unwanted)
        for (int i = 0; i < copy.length(); ++i)
            if (copy[i] == c)
                copy.erase (i);

    /*
    while(( pos = copy.first( c )) != -1 )
    	copy.erase( pos );
    */

    return copy;
}

// =============================================================================
//
string string::to_uppercase() const
{
    string newstr = m_string;

    for (char& c : newstr)
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';

    return newstr;
}

// =============================================================================
//
string string::to_lowercase() const
{
    string newstr = m_string;

    for (char & c : newstr)
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';

    return newstr;
}

// =============================================================================
//
string_list string::split (char del) const
{
    string delimstr;
    delimstr += del;
    return split (delimstr);
}

// =============================================================================
//
string_list string::split (string del) const
{
    string_list res;
    long a = 0;

    // Find all separators and store the text left to them.
    for (;;)
    {
        long b = first (del, a);

        if (b == -1)
            break;

        string sub = substring (a, b);

        if (sub.length() > 0)
            res.push_back (substring (a, b));

        a = b + strlen (del);
    }

    // Add the string at the right of the last separator
    if (a < (int) length())
        res.push_back (substring (a, length()));

    return res;
}

// =============================================================================
//
void string::replace (const char* a, const char* b)
{
    long pos;

    while ( (pos = first (a)) != -1)
        m_string = m_string.replace (pos, strlen (a), b);
}

// =============================================================================
//
int string::count (const char needle) const
{
    int needles = 0;

    for (const char & c : m_string)
        if (c == needle)
            needles++;

    return needles;
}

// =============================================================================
//
string string::substring (long a, long b) const
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
    strncpy (newstr, m_string.c_str() + a, b - a);
    newstr[b - a] = '\0';

    string other (newstr);
    delete[] newstr;
    return other;
}

// =============================================================================
//
string::length_type string::posof (int n) const
{
    int count = 0;

    for (int i = 0; i < length(); ++i)
    {
        if (m_string[i] != ' ')
            continue;

        if (++count < n)
            continue;

        return i;
    }

    return -1;
}

// =============================================================================
//
int string::first (const char* c, string::length_type a) const
{
    for (; a < length(); a++)
        if (m_string[a] == c[0] && strncmp (m_string.c_str() + a, c, strlen (c)) == 0)
            return a;

    return -1;
}

// =============================================================================
//
int string::last (const char* c, string::length_type a) const
{
    if (a == -1 || a >= length())
        a = length() - 1;

    for (; a > 0; a--)
        if (m_string[a] == c[0] && strncmp (m_string.c_str() + a, c, strlen (c)) == 0)
            return a;

    return -1;
}

// =============================================================================
//
void string::dump() const
{
    print ("`%1`:\n", chars());
    int i = 0;

    for (char u : m_string)
        print ("\t%1. [%d2] `%3`\n", i++, u, string (u));
}

// =============================================================================
//
long string::to_long (bool* ok, int base) const
{
    errno = 0;
    char* endptr;
    long i = strtol (m_string.c_str(), &endptr, base);
    *ok = (errno == 0 && *endptr == '\0');
    return i;
}

// =============================================================================
//
float string::to_float (bool* ok) const
{
    errno = 0;
    char* endptr;
    float i = strtof (m_string.c_str(), &endptr);
    *ok = (errno == 0 && *endptr == '\0');
    return i;
}

// =============================================================================
//
double string::to_double (bool* ok) const
{
    errno = 0;
    char* endptr;
    double i = strtod (m_string.c_str(), &endptr);
    *ok = (errno == 0 && *endptr == '\0');
    return i;
}

// =============================================================================
//
bool operator== (const char* a, const string& b)
{
    return b == a;
}

// =============================================================================
//
string operator+ (const char* a, const string& b)
{
    return string (a) + b;
}

// =============================================================================
//
string string::operator+ (const string data) const
{
    string newString = *this;
    newString += data;
    return newString;
}

// =============================================================================
//
string string::operator+ (const char* data) const
{
    string newstr = *this;
    newstr += data;
    return newstr;
}

// =============================================================================
//
string& string::operator+= (const string data)
{
    append (data);
    return *this;
}

// =============================================================================
//
string& string::operator+= (const char* data)
{
    append (data);
    return *this;
}

// =============================================================================
//
bool string::is_numeric() const
{
    bool gotDot = false;

    for (const char & c : m_string)
    {
        // Allow leading hyphen for negatives
        if (&c == &m_string[0] && c == '-')
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
bool string::ends_with (const string& other)
{
    if (length() < other.length())
        return false;

    const int ofs = length() - other.length();
    return strncmp (chars() + ofs, other.chars(), other.length()) == 0;
}

// =============================================================================
//
bool string::starts_with (const string& other)
{
    if (length() < other.length())
        return false;

    return strncmp (chars(), other.chars(), other.length()) == 0;
}

// =============================================================================
//
void string::sprintf (const char* fmtstr, ...)
{
    char* buf;
    int bufsize = 256;
    va_list va;
    va_start (va, fmtstr);

    do
        buf = new char[bufsize];

    while (vsnprintf (buf, bufsize, fmtstr, va) >= bufsize);

    va_end (va);
    m_string = buf;
    delete[] buf;
}

// =============================================================================
//
void string::prepend (string a)
{
    m_string = (a + m_string).std_string();
}

// =============================================================================
//
string string_list::join (const string& delim)
{
    string result;

    for (const string & it : std_deque())
    {
        if (!result.is_empty())
            result += delim;

        result += it;
    }

    return result;
}

// =============================================================================
//
bool string::mask (const string& pattern) const
{
    // Elevate to uppercase for case-insensitive matching
    string pattern_upper = pattern.to_uppercase();
    string this_upper = to_uppercase();
    const char* maskstring = pattern_upper.chars();
    const char* mptr = &maskstring[0];

    for (const char* sptr = this_upper.chars(); *sptr != '\0'; sptr++)
    {
        if (*mptr == '?')
        {
            if (* (sptr + 1) == '\0')
            {
                // ? demands that there's a character here and there wasn't.
                // Therefore, mask matching fails
                return false;
            }
        }

        elif (*mptr == '*')
        {
            char end = * (++mptr);

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
string string::from_number (int a)
{
	char buf[32];
	::sprintf (buf, "%d", a);
	return string (buf);
}

// =============================================================================
//
string string::from_number (long a)
{
	char buf[32];
	::sprintf (buf, "%ld", a);
	return string (buf);
}