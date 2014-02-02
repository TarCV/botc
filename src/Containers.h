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

#ifndef BOTC_CONTAINERS_H
#define BOTC_CONTAINERS_H

#include <cassert>
#include <algorithm>
#include <deque>
#include <initializer_list>

template<class T>
class List
{
	public:
		using ListType					= typename std::deque<T>;
		using Iterator					= typename ListType::iterator;
		using ConstIterator				= typename ListType::const_iterator;
		using ReverseIterator			= typename ListType::reverse_iterator;
		using ConstReverseIterator		= typename ListType::const_reverse_iterator;
		using ValueType					= T;
		using SelfType					= List<T>;

		// =====================================================================
		//
		List() {}

		// =====================================================================
		//
		List (std::initializer_list<ValueType> vals)
		{
			m_data = vals;
		}

		// =====================================================================
		//
		List (const ListType& a) :
			m_data (a) {}

		// =====================================================================
		//
		Iterator begin()
		{
			return m_data.begin();
		}

		// =====================================================================
		//
		ConstIterator begin() const
		{
			return m_data.cbegin();
		}

		// =====================================================================
		//
		Iterator end()
		{
			return m_data.end();
		}

		// =====================================================================
		//
		ConstIterator end() const
		{
			return m_data.cend();
		}

		// =====================================================================
		//
		ReverseIterator rbegin()
		{
			return m_data.rbegin();
		}

		// =====================================================================
		//
		ConstReverseIterator crbegin() const
		{
			return m_data.crbegin();
		}

		// =====================================================================
		//
		ReverseIterator rend()
		{
			return m_data.rend();
		}

		// =====================================================================
		//
		ConstReverseIterator crend() const
		{
			return m_data.crend();
		}

		// =====================================================================
		//
		inline void RemoveAt (int pos)
		{
			assert (pos < Size());
			m_data.erase (m_data.begin() + pos);
		}

		// =====================================================================
		//
		ValueType& Prepend (const ValueType& value)
		{
			m_data.push_front (value);
			return m_data[0];
		}

		// =====================================================================
		//
		ValueType& Append (const ValueType& value)
		{
			m_data.push_back (value);
			return m_data[m_data.size() - 1];
		}

		// =====================================================================
		//
		void Merge (const SelfType& vals)
		{
			for (const T & val : vals)
				Append (val);
		}

		// =====================================================================
		//
		bool Pop (T& val)
		{
			if (IsEmpty())
				return false;

			val = m_data[Size() - 1];
			m_data.erase (m_data.end() - 1);
			return true;
		}

		// =====================================================================
		//
		T& operator<< (const T& value)
		{
			return Append (value);
		}

		// =====================================================================
		//
		void operator<< (const SelfType& vals)
		{
			Merge (vals);
		}

		// =====================================================================
		//
		bool operator>> (T& value)
		{
			return Pop (value);
		}

		// =====================================================================
		//
		SelfType Reverse() const
		{
			SelfType rev;

			for (const T & val : *this)
				val >> rev;

			return rev;
		}

		// =====================================================================
		//
		void Clear()
		{
			m_data.clear();
		}

		// =====================================================================
		//
		void Insert (int pos, const ValueType& value)
		{
			m_data.insert (m_data.begin() + pos, value);
		}

		// =====================================================================
		//
		void RemoveDuplicates()
		{
			// Remove duplicate entries. For this to be effective, the vector must be
			// sorted first.
			Sort();
			Iterator pos = std::unique (begin(), end());
			Resize (std::distance (begin(), pos));
		}

		// =====================================================================
		//
		int Size() const
		{
			return m_data.size();
		}

		// =====================================================================
		//
		ValueType& operator[] (int n)
		{
			assert (n < Size());
			return m_data[n];
		}

		// =====================================================================
		//
		const ValueType& operator[] (int n) const
		{
			assert (n < Size());
			return m_data[n];
		}

		// =====================================================================
		//
		void Resize (int size)
		{
			m_data.resize (size);
		}

		// =====================================================================
		//
		void Sort()
		{
			std::sort (begin(), end());
		}

		// =====================================================================
		//
		int Find (const ValueType& needle) const
		{
			int i = 0;

			for (const ValueType & hay : *this)
			{
				if (&hay == &needle)
					return i;

				i++;
			}

			return -1;
		}

		// =====================================================================
		//
		void Remove (const ValueType& it)
		{
			int idx;

			if ((idx = Find (it)) != -1)
				RemoveAt (idx);
		}

		// =====================================================================
		//
		inline bool IsEmpty() const
		{
			return Size() == 0;
		}

		// =====================================================================
		//
		SelfType Mid (int a, int b) const
		{
			assert (a >= 0 && b >= 0 && a < Size() && b < Size() && a <= b);
			SelfType result;

			for (int i = a; i <= b; ++i)
				result << operator[] (i);

			return result;
		}

		// =====================================================================
		//
		inline const ListType& GetDeque() const
		{
			return m_data;
		}

		// =====================================================================
		//
		inline const ValueType& First() const
		{
			return *m_data.begin();
		}

		// =====================================================================
		//
		inline const ValueType& Last() const
		{
			return *(m_data.end() - 1);
		}

		// =====================================================================
		//
		inline bool Contains (const ValueType& a) const
		{
			return Find (a) != -1;
		}

	private:
		ListType m_data;
};

// =============================================================================
//
template<class T>
List<T>& operator>> (const T& value, List<T>& haystack)
{
	haystack.push_front (value);
	return haystack;
}

#endif // BOTC_CONTAINERS_H
