/*
	Copyright 2012-2014 Teemu Piippo
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

#ifndef BOTC_LIST_H
#define BOTC_LIST_H

#include "macros.h"
#include <algorithm>
#include <deque>
#include <initializer_list>
#include <functional>

template<typename T>
class List
{
public:
	using Iterator					= typename std::deque<T>::iterator;
	using ConstIterator				= typename std::deque<T>::const_iterator;
	using ReverseIterator			= typename std::deque<T>::reverse_iterator;
	using ConstReverseIterator		= typename std::deque<T>::const_reverse_iterator;

	List();
	List (std::size_t numvalues);
	List (const std::deque<T>& a);
	List (std::initializer_list<T>&& a);

	inline T&						append (const T& value);
	inline Iterator					begin();
	inline ConstIterator			begin() const;
	inline void						clear();
	inline bool						contains (const T& a) const;
	inline ConstReverseIterator		crbegin() const;
	inline ConstReverseIterator		crend() const;
	inline const std::deque<T>&		deque() const;
	inline Iterator					end();
	inline ConstIterator			end() const;
	int								find (const T& needle) const;
	T*								find (std::function<bool (T const&)> func);
	T const*						find (std::function<bool (T const&)> func) const;
	inline const T&					first() const;
	inline void						insert (int pos, const T& value);
	inline bool						isEmpty() const;
	inline const T&					last() const;
	void							merge (const List<T>& other);
	bool							pop (T& val);
	inline T&						prepend (const T& value);
	inline ReverseIterator			rbegin();
	inline void						removeAt (int pos);
	void							removeDuplicates();
	void							removeOne (const T& it);
	inline ReverseIterator			rend();
	inline void						resize (int size);
	List<T>							reverse() const;
	inline int						size() const;
	inline void						sort();
	List<T>							splice (int a, int b) const;

	inline List<T>&					operator<< (const T& value);
	inline List<T>&					operator<< (const List<T>& vals);
	inline T&						operator[] (int n);
	inline const T&					operator[] (int n) const;
	inline List<T>					operator+ (const List<T>& other) const;

private:
	std::deque<T> _deque;
};

template<typename T>
List<T>& operator>> (const T& value, List<T>& haystack);

//
// --- IMPLEMENTATIONS
//

template<typename T>
List<T>::List() {}

template<typename T>
List<T>::List (const std::deque<T>& other) :
	_deque (other) {}

template<typename T>
List<T>::List (std::initializer_list< T > && a) :
	_deque (a) {}

template<typename T>
List<T>::List (std::size_t numvalues) :
	_deque (numvalues) {}

template<typename T>
inline typename List<T>::Iterator List<T>::begin()
{
	return _deque.begin();
}

template<typename T>
inline typename List<T>::ConstIterator List<T>::begin() const
{
	return _deque.cbegin();
}

template<typename T>
inline typename List<T>::Iterator List<T>::end()
{
	return _deque.end();
}

template<typename T>
inline typename List<T>::ConstIterator List<T>::end() const
{
	return _deque.cend();
}

template<typename T>
inline typename List<T>::ReverseIterator List<T>::rbegin()
{
	return _deque.rbegin();
}

template<typename T>
inline typename List<T>::ConstReverseIterator List<T>::crbegin() const
{
	return _deque.crbegin();
}

template<typename T>
inline typename List<T>::ReverseIterator List<T>::rend()
{
	return _deque.rend();
}

template<typename T>
inline typename List<T>::ConstReverseIterator List<T>::crend() const
{
	return _deque.crend();
}

template<typename T>
void List<T>::removeAt (int pos)
{
	ASSERT_LT (pos, size());
	_deque.erase (_deque.begin() + pos);
}

template<typename T>
inline T& List<T>::prepend (const T& value)
{
	_deque.push_front (value);
	return _deque[0];
}

template<typename T>
inline T& List<T>::append (const T& value)
{
	_deque.push_back (value);
	return _deque[_deque.size() - 1];
}

template<typename T>
void List<T>::merge (const List<T>& other)
{
	int oldsize = size();
	resize (size() + other.size());
	std::copy (other.begin(), other.end(), begin() + oldsize);
}

template<typename T>
bool List<T>::pop (T& val)
{
	if (isEmpty())
		return false;

	val = _deque[size() - 1];
	_deque.erase (_deque.end() - 1);
	return true;
}

template<typename T>
inline List<T>& List<T>::operator<< (const T& value)
{
	append (value);
	return *this;
}

template<typename T>
inline List<T>& List<T>::operator<< (const List<T>& vals)
{
	merge (vals);
	return *this;
}

template<typename T>
List<T> List<T>::reverse() const
{
	List<T> rev;
	std::copy (rbegin(), rend(), rev.begin());
	return rev;
}

template<typename T>
inline void List<T>::clear()
{
	_deque.clear();
}

template<typename T>
inline void List<T>::insert (int pos, const T& value)
{
	_deque.insert (_deque.begin() + pos, value);
}

template<typename T>
void List<T>::removeDuplicates()
{
	sort();
	resize (std::distance (begin(), std::unique (begin(), end())));
}

template<typename T>
int List<T>::size() const
{
	return _deque.size();
}

template<typename T>
inline T& List<T>::operator[] (int n)
{
	ASSERT_LT (n, size());
	return _deque[n];
}

template<typename T>
inline const T& List<T>::operator[] (int n) const
{
	ASSERT_LT (n, size());
	return _deque[n];
}

template<typename T>
inline void List<T>::resize (int size)
{
	_deque.resize (size);
}

template<typename T>
inline void List<T>::sort()
{
	std::sort (begin(), end());
}

template<typename T>
int List<T>::find (const T& needle) const
{
	auto it = std::find (_deque.cbegin(), _deque.cend(), needle);

	if (it == _deque.end())
		return -1;

	return it - _deque.cbegin();
}

template<typename T>
T* List<T>::find (std::function<bool (T const&)> func)
{
	for (T& element : _deque)
	{
		if (func (element))
			return &element;
	}

	return nullptr;
}

template<typename T>
T const* List<T>::find (std::function<bool (T const&)> func) const
{
	for (T const& element : _deque)
	{
		if (func (element))
			return &element;
	}

	return nullptr;
}

template<typename T>
void List<T>::removeOne (const T& a)
{
	auto it = std::find (_deque.begin(), _deque.end(), a);

	if (it != _deque.end())
		_deque.erase (it);
}

template<typename T>
inline bool List<T>::isEmpty() const
{
	return size() == 0;
}

template<typename T>
List<T> List<T>::splice (int a, int b) const
{
	ASSERT_RANGE (a, 0, size() - 1)
	ASSERT_RANGE (b, 0, size() - 1)
	ASSERT_LT_EQ (a, b)
	List<T> result;

	for (int i = a; i <= b; ++i)
		result << operator[] (i);

	return result;
}

template<typename T>
inline const std::deque<T>& List<T>::deque() const
{
	return _deque;
}

template<typename T>
inline const T& List<T>::first() const
{
	return *_deque.cbegin();
}

template<typename T>
inline const T& List<T>::last() const
{
	return *(_deque.cend() - 1);
}

template<typename T>
inline bool List<T>::contains (const T& a) const
{
	return std::find (_deque.cbegin(), _deque.cend(), a) != _deque.end();
}

template<typename T>
inline List<T> List<T>::operator+ (const List<T>& other) const
{
	List<T> out (*this);
	out.merge (other);
	return out;
}

template<typename T>
List<T>& operator>> (const T& value, List<T>& haystack)
{
	haystack.prepend (value);
	return haystack;
}

#endif // BOTC_LIST_H
