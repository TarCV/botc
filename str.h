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

#ifndef __STR_H__
#define __STR_H__

char* vdynformat (const char* c, va_list v, unsigned int size);

#define SCCF_NUMBER	1<<0
#define SCCF_WORD	1<<1

// Dynamic string object, allocates memory when needed and
// features a good bunch of manipulation methods
class str {
private:
	// The actual message
	char* text;
	
	// Where will append() place new characters?
	unsigned int curs;
	
	// Allocated length
	unsigned int alloclen;
	
	// Resize the text buffer to len characters
	void resize (unsigned int len);
public:
	// ======================================================================
	// CONSTRUCTORS
	str ();
	str (const char* c);
	str (char c);
	
	// ======================================================================
	// METHODS
	
	// Empty the string
	void clear ();
	
	// Length of the string
	unsigned int len ();
	
	// The char* form of the string
	char* chars ();
	
	// Dumps the character table of the string
	void dump ();
	
	// Appends text to the string
	void append (char c);
	void append (const char* c);
	void append (str c);
	
	// Appends formatted text to the string.
	void appendformat (const char* c, ...);
	void appendformat (str c, ...);
	
	// Returns the first occurrence of c in the string, optionally starting
	// from a certain position rather than the start.
	unsigned int first (const char* c, unsigned int a = 0);
	
	// Returns the last occurrence of c in the string, optionally starting
	// from a certain position rather than the end.
	unsigned int last (const char* c, int a = -1);
	
	// Returns a substring of the string, from a to b.
	str substr (unsigned int a, unsigned int b);
	
	// Replace a substring with another substring.
	void replace (const char* o, const char* n, unsigned int a = 0);
	
	// Removes a given index from the string, optionally more characters than just 1.
	void remove (unsigned int idx, unsigned int dellen=1);
	
	void trim (int dellen);
	
	// Inserts a substring into a certain position.
	void insert (char* c, unsigned int pos);
	
	// Reverses the string.
	void reverse ();
	
	// Repeats the string a given amount of times.
	void repeat (unsigned int n);
	
	// Is the string a number?
	bool isnumber ();
	
	// Is the string a word, i.e consists only of alphabetic letters?
	bool isword ();
	
	// Convert string to lower case
	str tolower ();
	
	// Convert string to upper case
	str toupper ();
	
	bool contentcheck (int flags);
	
	int compare (const char* c);
	int compare (str c);
	
	// Counts the amount of substrings in the string
	unsigned int count (char* s);
	
#if 0
	str** split (char* del);
#endif
	
	// ======================================================================
	// OPERATORS
	str operator + (str& c);
	str& operator += (char c);
	str& operator += (const char* c);
	str& operator += (const str c);
	char operator [] (unsigned int pos);
	operator char* () const;
	operator int () const;
	operator unsigned int () const;
};

#endif // __STR_H__