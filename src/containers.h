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

#ifndef BOTC_CONTAINERS_H
#define BOTC_CONTAINERS_H

#include <cassert>
#include <algorithm>
#include <deque>
#include <initializer_list>

template<class T> class list
{
	public:
		using list_type					= typename ::std::deque<T>;
		using iterator					= typename list_type::iterator;
		using const_iterator			= typename list_type::const_iterator;
		using reverse_iterator			= typename list_type::reverse_iterator;
		using const_reverse_iterator	= typename list_type::const_reverse_iterator;
		using element_type				= T;
		using self_type					= list<T>;

		// =====================================================================
		//
		list() {}

		// =====================================================================
		//
		list (std::initializer_list<element_type> vals)
		{
			m_data = vals;
		}

		// =====================================================================
		//
		list (const list_type& a) :
			m_data (a) {}

		// =====================================================================
		//
		iterator begin()
		{
			return m_data.begin();
		}

		// =====================================================================
		//
		const_iterator begin() const
		{
			return m_data.cbegin();
		}

		// =====================================================================
		//
		iterator end()
		{
			return m_data.end();
		}

		// =====================================================================
		//
		const_iterator end() const
		{
			return m_data.cend();
		}

		// =====================================================================
		//
		reverse_iterator rbegin()
		{
			return m_data.rbegin();
		}

		// =====================================================================
		//
		const_reverse_iterator crbegin() const
		{
			return m_data.crbegin();
		}

		// =====================================================================
		//
		reverse_iterator rend()
		{
			return m_data.rend();
		}

		// =====================================================================
		//
		const_reverse_iterator crend() const
		{
			return m_data.crend();
		}

		// =====================================================================
		//
		inline void erase (int pos)
		{
			assert (pos < size());
			m_data.erase (m_data.begin() + pos);
		}

		// =====================================================================
		//
		element_type& push_front (const element_type& value)
		{
			m_data.push_front (value);
			return m_data[0];
		}

		// =====================================================================
		//
		element_type& push_back (const element_type& value)
		{
			m_data.push_back (value);
			return m_data[m_data.size() - 1];
		}

		// =====================================================================
		//
		void push_back (const self_type& vals)
		{
			for (const T & val : vals)
				push_back (val);
		}

		// =====================================================================
		//
		bool pop (T& val)
		{
			if (is_empty())
				return false;

			val = m_data[size() - 1];
			m_data.erase (m_data.end() - 1);
			return true;
		}

		// =====================================================================
		//
		T& operator<< (const T& value)
		{
			return push_back (value);
		}

		// =====================================================================
		//
		void operator<< (const self_type& vals)
		{
			push_back (vals);
		}

		// =====================================================================
		//
		bool operator>> (T& value)
		{
			return pop (value);
		}

		// =====================================================================
		//
		self_type reverse() const
		{
			self_type rev;

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
		void insert (int pos, const element_type& value)
		{
			m_data.insert (m_data.begin() + pos, value);
		}

		// =====================================================================
		//
		void makeUnique()
		{
			// Remove duplicate entries. For this to be effective, the vector must be
			// sorted first.
			sort();
			iterator pos = std::unique (begin(), end());
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
		element_type& operator[] (int n)
		{
			assert (n < size());
			return m_data[n];
		}

		// =====================================================================
		//
		const element_type& operator[] (int n) const
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
		int find (const element_type& needle) const
		{
			int i = 0;

			for (const element_type& hay : *this)
			{
				if (hay == needle)
					return i;

				i++;
			}

			return -1;
		}

		// =====================================================================
		//
		void remove (const element_type& it)
		{
			int idx;

			if ((idx = find (it)) != -1)
				erase (idx);
		}

		// =====================================================================
		//
		inline bool is_empty() const
		{
			return size() == 0;
		}

		// =====================================================================
		//
		self_type mid (int a, int b) const
		{
			assert (a >= 0 && b >= 0 && a < size() && b < size() && a <= b);
			self_type result;

			for (int i = a; i <= b; ++i)
				result << operator[] (i);

			return result;
		}

		// =====================================================================
		//
		inline const list_type& std_deque() const
		{
			return m_data;
		}

		// =====================================================================
		//
		const element_type& first() const
		{
			return *(m_data.begin());
		}

		// =====================================================================
		//
		const element_type& last() const
		{
			return *(m_data.end());
		}

		// =====================================================================
		//
		bool contains (const element_type& a) const
		{
			return find (a) != -1;
		}

	private:
		list_type m_data;
};

// =============================================================================
//
template<class T> list<T>& operator>> (const T& value, list<T>& haystack)
{
	haystack.push_front (value);
	return haystack;
}

#endif // BOTC_CONTAINERS_H
