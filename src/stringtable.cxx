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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "bots.h"
#include "stringtable.h"

static string_list g_string_table;

// ============================================================================
//
const string_list& get_string_table()
{
	return g_string_table;
}

// ============================================================================
// Potentially adds a string to the table and returns the index of it.
//
int get_string_table_index (const string& a)
{
	// Must not be too long.
	if (strlen (a) >= MAX_STRING_LENGTH)
		error ("string `%s` too long (%d characters, max is %d)\n",
			   a.c_str(), a.length(), MAX_STRING_LENGTH);

	// Find a free slot in the table.
	int idx;

	for (idx = 0; idx < g_string_table.size(); idx++)
	{
		// String is already in the table, thus return it.
		if (g_string_table[idx] == a)
			return idx;
	}

	// Check if the table is already full
	if (g_string_table.size() == MAX_LIST_STRINGS - 1)
		error ("too many strings!\n");

	// Now, dump the string into the slot
	g_string_table[idx] = a;

	return idx;
}

// ============================================================================
// Counts the amount of strings in the table.
//
int num_strings_in_table()
{
	return g_string_table.size();
}
