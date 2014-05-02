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
		using StringType	= std::string;
		using Iterator		= typename StringType::iterator;
		using ConstIterator	= StringType::const_iterator;

		String() {}

		explicit String (char a) :
			m_string (&a) {}

		String (const char* data) :
			m_string (data) {}

		String (const StringType& data) :
			m_string (data) {}

		void				dump() const;
		int					compare (const String& other) const;
		bool				endsWith (const String& other);
		int					count (char needle) const;
		int					firstIndexOf (const char* c, int a = 0) const;
		int					lastIndexOf (const char* c, int a = -1) const;
		String				toLowercase() const;
		bool				isNumeric() const;
		bool				maskAgainst (const String& pattern) const;
		int					wordPosition (int n) const;
		void				replace (const char* a, const char* b);
		StringList			split (const String& del) const;
		StringList			split (char del) const;
		void				sprintf (const char* fmtstr, ...);
		bool				startsWith (const String& other);
		String				strip (const List<char>& unwanted);
		String				mid (long a, long b = -1) const;
		double				toDouble (bool* ok = nullptr) const;
		float				toFloat (bool* ok = nullptr) const;
		long				toLong (bool* ok = nullptr, int base = 10) const;
		void				trim (int n);
		String				toUppercase() const;

		String				operator+ (const String& data) const;
		String				operator+ (const char* data) const;

		static String		fromNumber (int a);
		static String		fromNumber (long a);

		inline bool isEmpty() const
		{
			return m_string[0] == '\0';
		}

		inline void append (const char* data)
		{
			m_string.append (data);
		}

		inline void append (char data)
		{
			m_string.push_back (data);
		}

		inline void append (const String& data)
		{
			m_string.append (data.chars());
		}

		inline Iterator begin()
		{
			return m_string.begin();
		}

		inline ConstIterator begin() const
		{
			return m_string.cbegin();
		}

		inline const char* chars() const
		{
			return m_string.c_str();
		}

		inline const char* c_str() const
		{
			return m_string.c_str();
		}

		inline Iterator end()
		{
			return m_string.end();
		}

		inline ConstIterator end() const
		{
			return m_string.end();
		}

		inline void clear()
		{
			m_string.clear();
		}

		inline void removeAt (int pos)
		{
			m_string.erase (m_string.begin() + pos);
		}

		inline void insert (int pos, char c)
		{
			m_string.insert (m_string.begin() + pos, c);
		}

		inline int length() const
		{
			return m_string.length();
		}

		inline void remove (int pos, int len)
		{
			m_string.replace (pos, len, "");
		}

		inline void removeFromStart (int len)
		{
			remove (0, len);
		}

		inline void removeFromEnd (int len)
		{
			remove (length() - len, len);
		}

		inline void replace (int pos, int n, const String& a)
		{
			m_string.replace (pos, n, a.chars());
		}

		inline void shrinkToFit()
		{
			m_string.shrink_to_fit();
		}

		inline const StringType& stdString() const
		{
			return m_string;
		}

		inline String strip (char unwanted)
		{
			return strip ({unwanted});
		}

		// =============================================================================
		//
		inline String operator+ (int num) const
		{
			return *this + String::fromNumber (num);
		}

		// =============================================================================
		//
		inline String& operator+= (const String data)
		{
			append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String& operator+= (const char* data)
		{
			append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String& operator+= (int num)
		{
			return operator+= (String::fromNumber (num));
		}

		// =============================================================================
		//
		inline void prepend (String a)
		{
			m_string = (a + m_string).stdString();
		}

		// =============================================================================
		//
		inline String& operator+= (const char data)
		{
			append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String operator- (int n) const
		{
			String newString = m_string;
			newString -= n;
			return newString;
		}

		// =============================================================================
		//
		inline String& operator-= (int n)
		{
			trim (n);
			return *this;
		}

		// =============================================================================
		//
		inline String operator+() const
		{
			return toUppercase();
		}

		// =============================================================================
		//
		inline String operator-() const
		{
			return toLowercase();
		}

		// =============================================================================
		//
		inline bool operator== (const String& other) const
		{
			return stdString() == other.stdString();
		}

		// =============================================================================
		//
		inline bool operator== (const char* other) const
		{
			return operator== (String (other));
		}

		// =============================================================================
		//
		inline bool operator!= (const String& other) const
		{
			return stdString() != other.stdString();
		}

		// =============================================================================
		//
		inline bool operator!= (const char* other) const
		{
			return operator!= (String (other));
		}

		// =============================================================================
		//
		inline bool operator> (const String& other) const
		{
			return stdString() > other.stdString();
		}

		// =============================================================================
		//
		inline bool operator< (const String& other) const
		{
			return stdString() < other.stdString();
		}

		// =============================================================================
		//
		inline operator const char*() const
		{
			return chars();
		}

		// =============================================================================
		//
		inline operator const StringType&() const
		{
			return stdString();
		}

		void modifyIndex (int& a)
		{
			if (a < 0)
				a = length() - a;
		}

		int indexDifference (int a, int b)
		{
			modifyIndex (a);
			modifyIndex (b);
			return b - a;
		}

	private:
		StringType m_string;
};

// =============================================================================
//
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


// =============================================================================
//
inline bool operator== (const char* a, const String& b)
{
	return b == a;
}


// =============================================================================
//
inline String operator+ (const char* a, const String& b)
{
	return String (a) + b;
}

#endif // BOTC_STRING_H
