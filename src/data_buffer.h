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

#ifndef BOTC_DATABUFFER_H
#define BOTC_DATABUFFER_H

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stringtable.h"

#define MAX_MARKS 512

extern int g_NextMark;

// ============================================================================
// DataBuffer: A dynamic data buffer.
class data_buffer
{
public:
	// The actual buffer
	byte* buffer;

	// Allocated size of the buffer
	int allocsize;

	// Written size of the buffer
	int writesize;

	// Marks and references
	ScriptMark* marks[MAX_MARKS];
	ScriptMarkReference* refs[MAX_MARKS];

	data_buffer (int size = 128);
	~data_buffer ();

	// ====================================================================
	// Write stuff to the buffer
	// TODO: un-template and remove the union, move to source file
	template<class T> void write (T stuff)
	{
		if (writesize + sizeof (T) >= allocsize)
		{
			// We don't have enough space in the buffer to write
			// the stuff - thus resize. First, store the old
			// buffer temporarily:
			char* copy = new char[allocsize];
			memcpy (copy, buffer, allocsize);

			// Remake the buffer with the new size. Have enough space
			// for the stuff we're going to write, as well as a bit
			// of leeway so we don't have to resize immediately again.
			size_t newsize = allocsize + sizeof (T) + 128;

			delete buffer;
			buffer = new unsigned char[newsize];
			allocsize = newsize;

			// Now, copy the stuff back.
			memcpy (buffer, copy, allocsize);
			delete copy;
		}

		// Buffer is now guaranteed to have enough space.
		// Write the stuff one byte at a time.
		generic_union<T> uni;
		uni.as_t = stuff;

		for (int x = 0; x < sizeof (T); x++)
		{
			if (writesize >= allocsize) // should NEVER happen because resizing is done above
				error ("DataBuffer: written size exceeds allocated size!\n");

			buffer[writesize] = uni.as_bytes[x];
			writesize++;
		}
	}

	// ====================================================================
	// Merge another data buffer into this one.
	void merge (data_buffer* other);

	// Clones this databuffer to a new one and returns it.
	data_buffer* clone ();

	// ====================================================================
	// Adds a mark to the buffer. A mark is a "pointer" to a particular
	// position in the bytecode. The actual permanent position cannot
	// be predicted in any way or form, thus these things will be used
	// to "mark" a position like that for future use.
	int add_mark (string name);

	// ====================================================================
	// A ref is another "mark" that references a mark. When the bytecode
	// is written to file, they are changed into their marks' current
	// positions. Marks themselves are never written to files, only refs are
	int add_reference (int marknum, bool placeholder = true);

	// Delete a mark and all references to it.
	void delete_mark (int marknum);

	// Adjusts a mark to the current position
	void move_mark(int i);

	void offset_mark (int mark, int offset);

	// Dump the buffer (for debugging purposes)
	void dump();

	// Count the amount of marks
	int count_marks ();

	// Count the amount of refs
	int count_references ();

	// Write a float into the buffer
	void write_float (string floatstring);

	void write_string (string a);
};

#endif // BOTC_DATABUFFER_H
