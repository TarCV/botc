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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "str.h"
#include "common.h"

#define ITERATE_STRING(u) for (unsigned int u = 0; u < strlen (text); u++)

// ============================================================================
// vdynformat: Try to write to a formatted string with size bytes first, if
// that fails, double the size and keep recursing until it works.
char* vdynformat (const char* c, va_list v, unsigned int size) {
	char* buffer = new char[size];
	int r = vsnprintf (buffer, size-1, c, v);
	if (r > (int)size-1 || r < 0) {
		delete buffer;
		buffer = vdynformat (c, v, size*2);
	}
	return buffer;
}

// ============================================================================
str::str () {
	text = new char[1];
	clear();
	alloclen = strlen (text);
}

str::str (const char* c) {
	text = new char[1];
	clear ();
	alloclen = strlen (text);
	append (c);
}

str::str (char c) {
	text = new char[1];
	clear ();
	alloclen = strlen (text);
	append (c);
}

// ============================================================================
void str::clear () {
	delete text;
	text = new char[1];
	text[0] = '\0';
	curs = 0;
}

unsigned int str::len () {return strlen(text);}

// ============================================================================
void str::resize (unsigned int len) {
	unsigned int oldlen = strlen (text);
	char* oldtext = new char[oldlen];
	strncpy (oldtext, text, oldlen);
	
	delete text;
	text = new char[len+1];
	for (unsigned int u = 0; u < len+1; u++)
		text[u] = 0;
	strncpy (text, oldtext, len);
	
	alloclen = len;
}

// ============================================================================
void str::dump () {
	for (unsigned int u = 0; u <= alloclen; u++)
		printf ("\t%u. %u (%c)\n", u, text[u], text[u]);
}

// ============================================================================
// Adds a new character at the end of the string.
void str::append (char c) {
	// Out of space, thus resize
	if (curs == alloclen)
		resize (alloclen+1);
	text[curs] = c;
	curs++;
}

void str::append (const char* c) {
	resize (alloclen + strlen (c));
	
	for (unsigned int u = 0; u < strlen (c); u++) {
		if (c[u] != 0)
			append (c[u]);
	}
}

void str::append (str c) {
	append (c.chars());
}

// ============================================================================
void str::appendformat (const char* c, ...) {
	va_list v;
	
	va_start (v, c);
	char* buf = vdynformat (c, v, 256);
	va_end (v);
	
	append (buf);
}

void str::appendformat (str c, ...) {
	va_list v;
	
	va_start (v, c);
	char* buf = vdynformat (c.chars(), v, 256);
	va_end (v);
	
	append (buf);
}

// ============================================================================
char* str::chars () {
	return text;
}

// ============================================================================
unsigned int str::first (const char* c, unsigned int a) {
	unsigned int r = 0;
	unsigned int index = 0;
	for (; a < alloclen; a++) {
		if (text[a] == c[r]) {
			if (r == 0)
				index = a;
			
			r++;
			if (r == strlen (c))
				return index;
		} else {
			if (r != 0) {
				// If the string sequence broke at this point, we need to
				// check this character again, for a new sequence just
				// might start right here.
				a--;
			}
			
			r = 0;
		}
	}
	
	return len ();
}

// ============================================================================
unsigned int str::last (const char* c, int a) {
	if (a == -1)
		a = len();
	
	int max = strlen (c)-1;
	
	int r = max;
	for (; a >= 0; a--) {
		if (text[a] == c[r]) {
			r--;
			if (r == -1)
				return a;
		} else {
			if (r != max)
				a++;
			
			r = max;
		}
	}
	
	return len ();
}

// ============================================================================
str str::substr (unsigned int a, unsigned int b) {
	if (a > len()) a = len();
	if (b > len()) b = len();
	
	if (b == a)
		return "";
	
	if (b < a) {
		printf ("str::substr: indices %u and %u given, should be the other way around, swapping..\n", a, b);
		
		// Swap the variables
		unsigned int c = a;
		a = b;
		b = c;
	}
	
	char* s = new char[b-a];
	strncpy (s, text+a, b-a);
	return str(s);
}

// ============================================================================
void str::remove (unsigned int idx, unsigned int dellen) {
	str s1 = substr (0, idx);
	str s2 = substr (idx + dellen, len());
	
	clear();
	
	append (s1);
	append (s2);
}

// ============================================================================
void str::trim (int dellen) {
	if (!dellen)
		return;
	
	str s;
	// If dellen is positive, trim from the end,
	// if negative, trim from beginning.
	if (dellen > 0)
		s = substr (0, len()-dellen);
	else
		s = substr (-dellen, len());
	
	clear();
	append (s);
}

// ============================================================================
void str::replace (const char* o, const char* n, unsigned int a) {
	unsigned int idx;
	
	while ((idx = first (o, a)) != len()) {
		str s1 = substr (0, idx);
		str s2 = substr (idx + strlen (o), len());
		
		clear();
		
		append (s1);
		append (n);
		append (s2);
	}
}

void str::insert (char* c, unsigned int pos) {
	str s1 = substr (0, pos);
	str s2 = substr (pos, len());
	
	clear();
	append (s1);
	append (c);
	append (s2);
}

void str::reverse () {
	char* tmp = new char[alloclen];
	strcpy (tmp, text);
	
	clear();
	curs = 0;
	resize (alloclen);
	for (int i = alloclen-1; i >= 0; i--)
		append (tmp[i]);
}

void str::repeat (unsigned int n) {
	char* tmp = new char[alloclen];
	strcpy (tmp, text);
	
	for (; n > 0; n--)
		append (tmp);
}

// ============================================================================
bool str::isnumber () {
	ITERATE_STRING (u)
		if (text[u] < '0' || text[u] > '9')
			return false;
	return true;
}

// ============================================================================
bool str::isword () {
	ITERATE_STRING (u) {
		// lowercase letters
		if (text[u] >= 'a' || text[u] <= 'z')
			continue;
		
		// uppercase letters
		if (text[u] >= 'A' || text[u] <= 'Z')
			continue;
		
		return false;
	}
	return true;
}

// ============================================================================
int str::compare (const char* c) {
	return strcmp (text, c);
}

int str::compare (str c) {
	return compare (c.chars());
}

int str::icompare (const char* c) {
	return icompare (str ((char*)c));
}

int str::icompare (str b) {
	return strcmp (tolower().chars(), b.tolower().chars());
}

// ============================================================================
str str::tolower () {
	str n = text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] > 'A' && n[u] < 'Z')
			n.text[u] += ('a' - 'A');
	}
	
	return n;
}

// ============================================================================
str str::toupper () {
	str n = text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] > 'a' && n[u] < 'z')
			n.text[u] -= ('A' - 'a');
	}
	
	return n;
}

// ============================================================================
unsigned int str::count (char* c) {
	unsigned int r = 0;
	unsigned int tmp = 0;
	ITERATE_STRING (u) {
		if (text[u] == c[r]) {
			r++;
			if (r == strlen (c)) {
				r = 0;
				tmp++;
			}
		} else {
			if (r != 0)
				u--;
			r = 0;
		}
	}
	
	return tmp;
}

// ============================================================================
#if 0
str** str::split (char* del) {
	unsigned int arrcount = count (del) + 1;
	str** arr = new str* [arrcount];
	
	unsigned int a = 0;
	unsigned int index = 0;
	while (1) {
		unsigned int b = first (del, a+1);
		printf ("next: %u (<-> %u)\n", b, len());
		
		if (b == len())
			break;
		
		str* x = new str;
		x->append (substr (a, b));
		arr[index] = x;
		index++;
		a = b;
	}
	
	return arr;
}
#endif

// ============================================================================
// OPERATORS
str str::operator + (str& c) {
	append (c);
	return *this;
}

str& str::operator += (char c) {
	append (c);
	return *this;
}

str& str::operator += (const char* c) {
	append (c);
	return *this;
}

str& str::operator += (const str c) {
	append (c);
	return *this;
}

char str::operator [] (unsigned int pos) {
	return text[pos];
}

str::operator char* () const {
	return text;
}

str::operator int () const {return atoi(text);}
str::operator unsigned int () const {return atoi(text);}