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

template<typename T>
class List
{
	public:
		using WrappedList				= typename std::deque<T>;
		using Iterator					= typename WrappedList::iterator;
		using ConstIterator				= typename WrappedList::const_iterator;
		using ReverseIterator			= typename WrappedList::reverse_iterator;
		using ConstReverseIterator		= typename WrappedList::const_reverse_iterator;
		using ValueType					= T;
		using Self						= List<T>;

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
		List (const WrappedList& a) :
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
		inline void removeAt (int pos)
		{
			assert (pos < size());
			m_data.erase (m_data.begin() + pos);
		}

		// =====================================================================
		//
		ValueType& prepend (const ValueType& value)
		{
			m_data.push_front (value);
			return m_data[0];
		}

		// =====================================================================
		//
		ValueType& append (const ValueType& value)
		{
			m_data.push_back (value);
			return m_data[m_data.size() - 1];
		}

		// =====================================================================
		//
		void merge (const Self& other)
		{
			resize (size() + other.size());
			std::copy (other.begin(), other.end(), begin() + other.size());
		}

		// =====================================================================
		//
		bool pop (T& val)
		{
			if (isEmpty())
				return false;

			val = m_data[size() - 1];
			m_data.erase (m_data.end() - 1);
			return true;
		}

		// =====================================================================
		//
		Self& operator<< (const T& value)
		{
			append (value);
			return *this;
		}

		// =====================================================================
		//
		void operator<< (const Self& vals)
		{
			merge (vals);
		}

		// =====================================================================
		//
		bool operator>> (T& value)
		{
			return pop (value);
		}

		// =====================================================================
		//
		Self reverse() const
		{
			Self rev;

			for (const T & val : *this)
				val >> rev;

			return rev;
		}

		// =====================================================================
		//
		void clear()
		{
			m_data.clear();
		}

		// =====================================================================
		//
		void insert (int pos, const ValueType& value)
		{
			m_data.insert (m_data.begin() + pos, value);
		}

		// =====================================================================
		//
		void removeDuplicates()
		{
			// Remove duplicate entries. For this to be effective, the vector must be
			// sorted first.
			sort();
			Iterator pos = std::unique (begin(), end());
			resize (std::distance (begin(), pos));
		}

		// =====================================================================
		//
		int size() const
		{
			return m_data.size();
		}

		// =====================================================================
		//
		ValueType& operator[] (int n)
		{
			assert (n < size());
			return m_data[n];
		}

		// =====================================================================
		//
		const ValueType& operator[] (int n) const
		{
			assert (n < size());
			return m_data[n];
		}

		// =====================================================================
		//
		void resize (int size)
		{
			m_data.resize (size);
		}

		// =====================================================================
		//
		void sort()
		{
			std::sort (begin(), end());
		}

		// =====================================================================
		//
		int find (const ValueType& needle) const
		{
			int i = 0;

			for (const ValueType& hay : *this)
			{
				if (hay == needle)
					return i;

				i++;
			}

			return -1;
		}

		// =====================================================================
		//
		void removeOne (const ValueType& it)
		{
			int idx;

			if ((idx = find (it)) != -1)
				removeAt (idx);
		}

		// =====================================================================
		//
		inline bool isEmpty() const
		{
			return size() == 0;
		}

		// =====================================================================
		//
		Self splice (int a, int b) const
		{
			assert (a >= 0 && b >= 0 && a < size() && b < size() && a <= b);
			Self result;

			for (int i = a; i <= b; ++i)
				result << operator[] (i);

			return result;
		}

		// =====================================================================
		//
		inline const WrappedList& deque() const
		{
			return m_data;
		}

		// =====================================================================
		//
		inline const ValueType& first() const
		{
			return *m_data.begin();
		}

		// =====================================================================
		//
		inline const ValueType& last() const
		{
			return *(m_data.end() - 1);
		}

		// =====================================================================
		//
		inline bool contains (const ValueType& a) const
		{
			return find (a) != -1;
		}

		// =====================================================================
		//
		Self operator+ (const Self& other) const
		{
			Self out (*this);
			out.merge (other);
			return out;
		}

	private:
		WrappedList m_data;
};

// =============================================================================
//
template<typename T>
List<T>& operator>> (const T& value, List<T>& haystack)
{
	haystack.prepend (value);
	return haystack;
}

#endif // BOTC_CONTAINERS_H
