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

#ifndef BOTC_STRING_H
#define BOTC_STRING_H

#include <deque>
#include <string>
#include <stdarg.h>
#include "types.h"
#include "list.h"

class String;
class StringList;

// =============================================================================
//
class String
{
public:
	String() {}

	explicit String (char a) :
		_string ({ a, '\0' }) {}

	String (const char* data) :
		_string (data) {}

	String (const std::string& data) :
		_string (data) {}

	inline void							append (const char* data);
	inline void							append (char data);
	inline void							append (const String& data);
	inline std::string::iterator		begin();
	inline std::string::const_iterator	begin() const;
	void								dump() const;
	inline void							clear();
	int									compare (const String& other) const;
	int									count (char needle) const;
	inline const char*					c_str() const;
	inline std::string::iterator		end();
	inline std::string::const_iterator	end() const;
	bool								endsWith (const String& other);
	int									firstIndexOf (const char* c, int a = 0) const;
	String								toLowercase() const;
	inline int							indexDifference (int a, int b);
	inline void							insert (int pos, char c);
	inline bool							isEmpty() const;
	bool								isNumeric() const;
	int									lastIndexOf (const char* c, int a = -1) const;
	inline int							length() const;
	bool								maskAgainst (const String& pattern) const;
	String								mid (long a, long b = -1) const;
	inline void							modifyIndex (int& a);
	inline void							prepend (String a);
	inline void							removeAt (int pos);
	inline void							remove (int pos, int len);
	inline void							removeFromEnd (int len);
	inline void							removeFromStart (int len);
	void								replace (const char* a, const char* b);
	inline void							replace (int pos, int n, const String& a);
	inline void							shrinkToFit();
	StringList							split (const String& del) const;
	StringList							split (char del) const;
	void								sprintf (const char* fmtstr, ...);
	bool								startsWith (const String& other);
	inline const std::string&			stdString() const;
	inline String						strip (char unwanted);
	String								strip (const List<char>& unwanted);
	double								toDouble (bool* ok = nullptr) const;
	float								toFloat (bool* ok = nullptr) const;
	long								toLong (bool* ok = nullptr, int base = 10) const;
	void								trim (int n);
	String								toUppercase() const;
	int									wordPosition (int n) const;

	static String						fromNumber (int a);
	static String						fromNumber (long a);
	static String						fromNumber (long long a);
	static String						fromNumber (double a);

	String								operator+ (const String& data) const;
	String								operator+ (const char* data) const;
	inline String						operator+ (int num) const;
	inline String&						operator+= (const String data);
	inline String&						operator+= (const char* data);
	inline String&						operator+= (int num);
	inline String&						operator+= (const char data);
	inline String						operator- (int n) const;
	inline String&						operator-= (int n);
	inline bool							operator== (const String& other) const;
	inline bool							operator== (const char* other) const;
	inline bool							operator!= (const String& other) const;
	inline bool							operator!= (const char* other) const;
	inline bool							operator> (const String& other) const;
	inline bool							operator< (const String& other) const;
	inline								operator const char*() const;
	inline								operator const std::string&() const;

private:
	std::string _string;
};

class StringList : public List<String>
{
	public:
		StringList() {}
		StringList (std::initializer_list<String> vals) :
			List<String> (vals) {}
		StringList (const List<String>& a) : List<String> (a.deque()) {}
		StringList (const std::deque<String>& a) : List<String> (a) {}

		String join (const String& delim);
};

inline bool operator== (const char* a, const String& b);
inline String operator+ (const char* a, const String& b);

String basename (String const& path);
String dirname (String const& path);

// =============================================================================
//
// IMPLEMENTATIONS
//

inline bool String::isEmpty() const
{
	return _string[0] == '\0';
}

inline void String::append (const char* data)
{
	_string.append (data);
}

inline void String::append (char data)
{
	_string.push_back (data);
}

inline void String::append (const String& data)
{
	_string.append (data.c_str());
}

inline std::string::iterator String::begin()
{
	return _string.begin();
}

inline std::string::const_iterator String::begin() const
{
	return _string.cbegin();
}

inline const char* String::c_str() const
{
	return _string.c_str();
}

inline std::string::iterator String::end()
{
	return _string.end();
}

inline std::string::const_iterator String::end() const
{
	return _string.end();
}

inline void String::clear()
{
	_string.clear();
}

inline void String::removeAt (int pos)
{
	_string.erase (_string.begin() + pos);
}

inline void String::insert (int pos, char c)
{
	_string.insert (_string.begin() + pos, c);
}

inline int String::length() const
{
	return _string.length();
}

inline void String::remove (int pos, int len)
{
	_string.replace (pos, len, "");
}

inline void String::removeFromStart (int len)
{
	remove (0, len);
}

inline void String::removeFromEnd (int len)
{
	remove (length() - len, len);
}

inline void String::replace (int pos, int n, const String& a)
{
	_string.replace (pos, n, a.c_str());
}

inline void String::shrinkToFit()
{
	_string.shrink_to_fit();
}

inline const std::string& String::stdString() const
{
	return _string;
}

inline String String::strip (char unwanted)
{
	return strip ({unwanted});
}

inline String String::operator+ (int num) const
{
	return *this + String::fromNumber (num);
}

inline String& String::operator+= (const String data)
{
	append (data);
	return *this;
}

inline String& String::operator+= (const char* data)
{
	append (data);
	return *this;
}

inline String& String::operator+= (int num)
{
	return operator+= (String::fromNumber (num));
}

inline void String::prepend (String a)
{
	_string = (a + _string).stdString();
}

inline String& String::operator+= (const char data)
{
	append (data);
	return *this;
}

inline String String::operator- (int n) const
{
	String newString = _string;
	newString -= n;
	return newString;
}

inline String& String::operator-= (int n)
{
	trim (n);
	return *this;
}

inline bool String::operator== (const String& other) const
{
	return stdString() == other.stdString();
}

inline bool String::operator== (const char* other) const
{
	return operator== (String (other));
}

inline bool String::operator!= (const String& other) const
{
	return stdString() != other.stdString();
}

inline bool String::operator!= (const char* other) const
{
	return operator!= (String (other));
}

inline bool String::operator> (const String& other) const
{
	return stdString() > other.stdString();
}

inline bool String::operator< (const String& other) const
{
	return stdString() < other.stdString();
}

inline String::operator const char*() const
{
	return c_str();
}

inline String::operator const std::string&() const
{
	return stdString();
}

inline void String::modifyIndex (int& a)
{
	if (a < 0)
		a = length() - a;
}

inline int String::indexDifference (int a, int b)
{
	modifyIndex (a);
	modifyIndex (b);
	return b - a;
}

inline bool operator== (const char* a, const String& b)
{
	return b == a;
}

inline String operator+ (const char* a, const String& b)
{
	return String (a) + b;
}

#endif // BOTC_STRING_H
