/*
 *	botc source code
 *	Copyright (C) 2012 Santeri `Dusk` Piippo
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	
 *	1. Redistributions of source code must retain the above copyright notice,
 *	   this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright notice,
 *	   this list of conditions and the following disclaimer in the documentation
 *	   and/or other materials provided with the distribution.
 *	3. Neither the name of the developer nor the names of its contributors may
 *	   be used to endorse or promote products derived from this software without
 *	   specific prior written permission.
 *	4. Redistributions in any form must be accompanied by information on how to
 *	   obtain complete source code for the software and any accompanying
 *	   software that uses the software. The source code must either be included
 *	   in the distribution or be available for no more than the cost of
 *	   distribution plus a nominal fee, and must be freely redistributable
 *	   under reasonable conditions. For an executable file, complete source
 *	   code means the source code for all modules it contains. It does not
 *	   include source code for modules or files that typically accompany the
 *	   major components of the operating system on which the executable file
 *	   runs.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

#define ITERATE_SUBSCRIPTS(link) \
	for (link = data; link; link = link->next)

// Single element of an array
template <class T> class arrayElement {
public:
	T value;
	arrayElement<T>* next;
	
	arrayElement () {
		next = NULL;
	}
};

// Dynamic array
template <class T> class array {
public:
	array () {
		data = NULL;
	}
	
	array (T* stuff, unsigned int c) {
		printf ("%d elements\n", c);
		data = NULL;
		
		for (unsigned int i = 0; i < c; i++)
			push (stuff[c]);
	}
	
	~array () {
		if (data)
			deleteElement (data);
	}
	
	void push (T stuff) {
		arrayElement<T>* e = new arrayElement<T>;
		e->value = stuff;
		e->next = NULL;
		
		if (!data) {
			data = e;
			return;
		}
		
		arrayElement<T>* link;
		for (link = data; link && link->next; link = link->next);
		link->next = e;
	}
	
	T pop () {
		int pos = size() - 1;
		if (pos == -1)
			error ("array::pop: tried to pop an array with no elements\n");
		T res = subscript (pos);
		remove (pos);
		return res;
	}
	
	void remove (unsigned int pos) {
		if (!data)
			error ("tried to use remove on an array with no elements");
		
		if (pos == 0) {
			// special case for first element
			arrayElement<T>* first = data;
			data = data->next;
			delete first;
			return;
		}
		
		arrayElement<T>* link = data;
		unsigned int x = 0;
		while (link->next) {
			if (x == pos - 1)
				break;
			link = link->next;
			x++;
		}
		if (!link)
			error ("no such element in array\n");
		
		arrayElement<T>* nextlink = link->next->next;
		delete link->next;
		link->next = nextlink;
	}
	
	unsigned int size () {
		unsigned int x = 0;
		arrayElement<T>* link;
		ITERATE_SUBSCRIPTS(link)
			x++;
		return x;
	}
	
	T& subscript (unsigned int i) {
		arrayElement<T>* link;
		unsigned int x = 0;
		ITERATE_SUBSCRIPTS(link) {
			if (x == i)
				return link->value;
			x++;
		}
		
		error ("array: tried to access subscript %u in an array of %u elements\n", i, x);
		return data->value;
	}
	
	T* out () {
		int s = size();
		T* out = new T[s];
		unsigned int x = 0;
		
		arrayElement<T>* link;
		ITERATE_SUBSCRIPTS(link) {
			out[x] = link->value;
			x++;
		}
		
		return out;
	}
	
	T& operator [] (const unsigned int i) {
		return subscript (i);
	}
	
	void operator << (const T stuff) {
		push (stuff);
	}
	
private:
	void deleteElement (arrayElement<T>* e) {
		if (e->next)
			deleteElement (e->next);
		delete e;
	}
	
	arrayElement<T>* data;
};