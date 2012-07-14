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

#define __STRINGTABLE_CXX__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "bots.h"
#include "stringtable.h"

StringTable::StringTable() {
	// Zero out everything first.
	for (unsigned int a = 0; a < MAX_LIST_STRINGS; a++)
		for (unsigned int b = 0; b < MAX_STRING_LENGTH; b++)
			table[a][b] = 0;
}

unsigned int StringTable::Push (char* s) {
	// Determine the length
	size_t l1 = strlen (s);
	size_t l2 = MAX_LIST_STRINGS - 1;
	size_t len = (l1 < l2) ? l1 : l2;
	
	// First, copy the string to a temporary buffer, since otherwise
	// it gets lost and becomes empty.
	char tmp[len+1];
	strncpy (tmp, s, len);
	tmp[len] = 0;
	
	// Find a free slot in the table. 
	unsigned int a;
	for (a = 0; a < MAX_LIST_STRINGS; a++) {
		if (!strcmp (tmp, table[a]))
			return a;
		
		// String is empty, thus it's free.
		if (!strlen (table[a]))
			break;
	}
	
	// no free slots!
	if (a == MAX_LIST_STRINGS)
		error ("too many strings defined!");
	
	// Now, dump the string into the slot
	strncpy (table[a], tmp, len);
	table[a][len] = 0;
	
	return a;
}

unsigned int StringTable::Count () {
	unsigned int count = 0;
	for (unsigned int a = 0; a < MAX_LIST_STRINGS; a++) {
		if (!strlen (table[a]))
			break;
		else
			count++;
	}
	return count;
}

char* StringTable::operator [] (unsigned int a) {
	if (a > MAX_LIST_STRINGS)
		return const_cast<char*> ("");
	return table[a];
}