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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StringTable.h"

static StringList gStringTable;

// ============================================================================
//
const StringList& GetStringTable()
{
	return gStringTable;
}

// ============================================================================
//
// Potentially adds a string to the table and returns the index of it.
//
int StringTableIndex (const String& a)
{
	// Find a free slot in the table.
	int idx;

	for (idx = 0; idx < gStringTable.Size(); idx++)
	{
		// String is already in the table, thus return it.
		if (gStringTable[idx] == a)
			return idx;
	}

	// Must not be too long.
	if (a.Length() >= gMaxStringLength)
		Error ("string `%1` too long (%2 characters, max is %3)\n",
			   a, a.Length(), gMaxStringLength);

	// Check if the table is already full
	if (gStringTable.Size() == gMaxStringlistSize - 1)
		Error ("too many strings!\n");

	// Now, dump the string into the slot
	gStringTable.Append (a);
	return (gStringTable.Size() - 1);
}

// ============================================================================
//
// Counts the amount of strings in the table.
//
int CountStringsInTable()
{
	return gStringTable.Size();
}
