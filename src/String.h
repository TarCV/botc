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
#include "Types.h"
#include "Containers.h"

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
			mString (&a) {}

		String (const char* data) :
			mString (data) {}

		String (const StringType& data) :
			mString (data) {}

		void				Dump() const;
		int					Compare (const String& other) const;
		bool				EndsWith (const String& other);
		int					Count (char needle) const;
		int					FirstIndexOf (const char* c, int a = 0) const;
		int					LastIndexOf (const char* c, int a = -1) const;
		String				ToLowercase() const;
		bool				IsNumeric() const;
		bool				MaskAgainst (const String& pattern) const;
		int					WordPosition (int n) const;
		void				Replace (const char* a, const char* b);
		StringList			Split (String del) const;
		StringList			Split (char del) const;
		void				SPrintf (const char* fmtstr, ...);
		bool				StartsWith (const String& other);
		String				Strip (const List<char>& unwanted);
		String				Mid (long a, long b = -1) const;
		double				ToDouble (bool* ok = nullptr) const;
		float				ToFloat (bool* ok = nullptr) const;
		long				ToLong (bool* ok = nullptr, int base = 10) const;
		void				Trim (int n);
		String				ToUppercase() const;

		String				operator+ (const String& data) const;
		String				operator+ (const char* data) const;

		static String		FromNumber (int a);
		static String		FromNumber (long a);

		inline bool IsEmpty() const
		{
			return mString[0] == '\0';
		}

		inline void Append (const char* data)
		{
			mString.append (data);
		}

		inline void Append (const char data)
		{
			mString.push_back (data);
		}

		inline void Append (const String& data)
		{
			mString.append (data.CString());
		}

		inline Iterator begin()
		{
			return mString.begin();
		}

		inline ConstIterator begin() const
		{
			return mString.cbegin();
		}

		inline const char* CString() const
		{
			return mString.c_str();
		}

		inline const char* c_str() const
		{
			return mString.c_str();
		}

		inline Iterator end()
		{
			return mString.end();
		}

		inline ConstIterator end() const
		{
			return mString.end();
		}

		inline void Clear()
		{
			mString.clear();
		}

		inline void RemoveAt (int pos)
		{
			mString.erase (mString.begin() + pos);
		}

		inline void Insert (int pos, char c)
		{
			mString.insert (mString.begin() + pos, c);
		}

		inline int Length() const
		{
			return mString.length();
		}

		inline void Remove (int pos, int len)
		{
			mString.replace (pos, len, "");
		}

		inline void RemoveFromStart (int len)
		{
			Remove (0, len);
		}

		inline void RemoveFromEnd (int len)
		{
			Remove (Length() - len, len);
		}

		inline void Replace (int pos, int n, const String& a)
		{
			mString.replace (pos, n, a.CString());
		}

		inline void ShrinkToFit()
		{
			mString.shrink_to_fit();
		}

		inline const StringType& STDString() const
		{
			return mString;
		}

		inline String Strip (char unwanted)
		{
			return Strip ({unwanted});
		}

		// =============================================================================
		//
		inline String operator+ (int num) const
		{
			return *this + String::FromNumber (num);
		}

		// =============================================================================
		//
		inline String& operator+= (const String data)
		{
			Append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String& operator+= (const char* data)
		{
			Append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String& operator+= (int num)
		{
			return operator+= (String::FromNumber (num));
		}

		// =============================================================================
		//
		inline void Prepend (String a)
		{
			mString = (a + mString).STDString();
		}

		// =============================================================================
		//
		inline String& operator+= (const char data)
		{
			Append (data);
			return *this;
		}

		// =============================================================================
		//
		inline String operator- (int n) const
		{
			String newString = mString;
			newString -= n;
			return newString;
		}

		// =============================================================================
		//
		inline String& operator-= (int n)
		{
			Trim (n);
			return *this;
		}

		// =============================================================================
		//
		inline String operator+() const
		{
			return ToUppercase();
		}

		// =============================================================================
		//
		inline String operator-() const
		{
			return ToLowercase();
		}

		// =============================================================================
		//
		inline bool operator== (const String& other) const
		{
			return STDString() == other.STDString();
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
			return STDString() != other.STDString();
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
			return STDString() > other.STDString();
		}

		// =============================================================================
		//
		inline bool operator< (const String& other) const
		{
			return STDString() < other.STDString();
		}

		// =============================================================================
		//
		inline operator const char*() const
		{
			return CString();
		}

		// =============================================================================
		//
		inline operator const StringType&() const
		{
			return STDString();
		}

		// =============================================================================
		//
		// Difference between indices @a and @b. @b can be -1, in which
		// case it will be length() - 1.
		//
		inline int IndexDifference (int a, int b)
		{
			assert (b == -1 || b >= a);
			return (b != -1 ? b - a : Length() - 1 - a);
		}

	private:
		StringType mString;
};

// =============================================================================
//
class StringList : public List<String>
{
	public:
		StringList() {}
		StringList (std::initializer_list<String> vals) :
			List<String> (vals) {}
		StringList (const List<String>& a) : List<String> (a.GetDeque()) {}
		StringList (const ListType& a) : List<String> (a) {}

		String Join (const String& delim);
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
