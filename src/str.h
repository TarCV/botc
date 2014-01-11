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

#ifndef STRING_H
#define STRING_H

#include <deque>
#include <string>
#include <stdarg.h>
#include "types.h"
#include "containers.h"

class string;
class string_list;

// =============================================================================
//
class string
{
	public:
		typedef typename ::std::string::iterator       iterator;
		typedef typename ::std::string::const_iterator const_iterator;
		using length_type = int;

		string() {}

		string (char a)
		{
			m_string = &a;
		}

		string (const char* data)
		{
			m_string = data;
		}

		string (std::string data)
		{
			m_string = data;
		}

		void               dump() const;
		int                compare (const string& other) const;
		bool               ends_with (const string& other);
		int                count (const char needle) const;
		int                first (const char* c, length_type a = 0) const;
		int                last (const char* c, length_type a = -1) const;
		string             to_lowercase() const;
		bool               is_numeric() const;
		bool				mask (const string& pattern) const;
		length_type        posof (int n) const;
		void               prepend (string a);
		void               replace (const char* a, const char* b);
		string_list        split (string del) const;
		string_list        split (char del) const;
		void               sprintf (const char* fmtstr, ...);
		bool               starts_with (const string& other);
		string             strip (list< char > unwanted);
		string             substring (long a, long b = -1) const;
		double             to_double (bool* ok = null) const;
		float              to_float (bool* ok = null) const;
		long               to_long (bool* ok = null, int base = 10) const;
		void               trim (length_type n);
		string             to_uppercase() const;

		string             operator+ (const string data) const;
		string             operator+ (const char* data) const;
		string&            operator+= (const string data);
		string&            operator+= (const char* data);

		inline bool is_empty() const
		{
			return m_string[0] == '\0';
		}

		inline void append (const char* data)
		{
			m_string.append (data);
		}

		inline void append (const char data)
		{
			m_string.push_back (data);
		}

		inline void append (const string& data)
		{
			m_string.append (data.chars());
		}

		inline iterator begin()
		{
			return m_string.begin();
		}

		inline const_iterator begin() const
		{
			return m_string.cbegin();
		}

		inline int capacity() const
		{
			return m_string.capacity();
		}

		inline const char* chars() const
		{
			return m_string.c_str();
		}

		inline const char* c_str() const
		{
			return m_string.c_str();
		}

		inline iterator end()
		{
			return m_string.end();
		}

		inline const_iterator end() const
		{
			return m_string.end();
		}

		inline void clear()
		{
			m_string.clear();
		}

		inline bool empty() const
		{
			return m_string.empty();
		}

		inline void erase (length_type pos)
		{
			m_string.erase (m_string.begin() + pos);
		}

		inline void insert (length_type pos, char c)
		{
			m_string.insert (m_string.begin() + pos, c);
		}

		inline length_type len() const
		{
			return length();
		}

		inline length_type length() const
		{
			return m_string.length();
		}

		inline int max_size() const
		{
			return m_string.max_size();
		}

		inline string mid (int a, int len) const
		{
			return m_string.substr (a, len);
		}

		inline void remove (int pos, int len)
		{
			m_string.replace (pos, len, "");
		}

		inline void remove_from_start (int len)
		{
			remove (0, len);
		}

		inline void remove_from_end (int len)
		{
			remove (length() - len, len);
		}

		inline void resize (int n)
		{
			m_string.resize (n);
		}

		inline void replace (int pos, int n, const string& a)
		{
			m_string.replace (pos, n, a.chars());
		}

		inline void shrink_to_fit()
		{
			m_string.shrink_to_fit();
		}

		inline const std::string& std_string() const
		{
			return m_string;
		}

		inline string strip (char unwanted)
		{
			return strip ( {unwanted});
		}

		string& operator+= (const char data)
		{
			append (data);
			return *this;
		}

		string operator- (int n) const
		{
			string new_string = m_string;
			new_string -= n;
			return new_string;
		}

		inline string& operator-= (int n)
		{
			trim (n);
			return *this;
		}

		inline string operator+() const
		{
			return to_uppercase();
		}

		inline string operator-() const
		{
			return to_lowercase();
		}

		inline bool operator== (const string& other) const
		{
			return std_string() == other.std_string();
		}

		inline bool operator== (const char* other) const
		{
			return operator== (string (other));
		}

		inline bool operator!= (const string& other) const
		{
			return std_string() != other.std_string();
		}

		inline bool operator!= (const char* other) const
		{
			return operator!= (string (other));
		}

		inline bool operator> (const string& other) const
		{
			return std_string() > other.std_string();
		}

		inline bool operator< (const string& other) const
		{
			return std_string() < other.std_string();
		}

		inline operator const char*() const
		{
			return chars();
		}

		inline operator const std::string&() const
		{
			return std_string();
		}

		// Difference between indices @a and @b. @b can be -1, in which
		// case it will be length() - 1.
		inline int index_difference (int a, int b)
		{
			assert (b == -1 || b >= a);
			return (b != -1 ? b - a : length() - 1 - a);
		}

	private:
		std::string m_string;
};

// =============================================================================
//
class string_list : public list<string>
{
	public:
		string_list() {}
		string_list (std::initializer_list<string> vals) :
			list<string> (vals) {}
		string_list (const list<string>& a) : list<string> (a.std_deque()) {}
		string_list (const list_type& a) : list<string> (a) {}

		string join (const string& delim);
};

bool operator== (const char* a, const string& b);
string operator+ (const char* a, const string& b);

#endif // STRING_H
