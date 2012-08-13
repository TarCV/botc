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

#ifndef  __DATABUFFER_H__
#define __DATABUFFER_H__
#include <stdio.h>
#include <string.h>
#include "common.h"

#define MAX_MARKS 512

extern int g_NextMark;

// ============================================================================
// DataBuffer: A dynamic data buffer.
class DataBuffer {
public:
	// The actual buffer
	unsigned char* buffer;
	
	// Allocated size of the buffer
	unsigned int allocsize;
	
	// Written size of the buffer
	unsigned int writesize;
	
	// Marks and references
	ScriptMark* marks[MAX_MARKS];
	ScriptMarkReference* refs[MAX_MARKS];
	
	// ====================================================================
	// METHODS
	
	// ====================================================================
	// Constructor
	DataBuffer (unsigned int size=128) {
		writesize = 0;
		
		buffer = new unsigned char[size];
		allocsize = size;
		
		// Clear the marks table out
		for (unsigned int u = 0; u < MAX_MARKS; u++) {
			marks[u] = NULL;
			refs[u] = NULL;
		}
	}
	
	// ====================================================================
	~DataBuffer () {
		delete buffer;
		
		// Delete any marks and references
		for (unsigned int u = 0; u < MAX_MARKS; u++) {
			if (marks[u])
				delete marks[u];
			
			if (refs[u])
				delete refs[u];
		}
	}
	
	// ====================================================================
	// Write stuff to the buffer
	template<class T> void Write (T stuff) {
		if (writesize + sizeof (T) >= allocsize) {
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
		union_t<T> uni;
		uni.val = stuff;
		for (unsigned int x = 0; x < sizeof (T); x++) {
			if (writesize >= allocsize) // should NEVER happen because resizing is done above
				error ("DataBuffer: written size exceeds allocated size!\n");
			
			buffer[writesize] = uni.b[x];
			writesize++;
		}
	}
	
	// ====================================================================
	// Merge another data buffer into this one.
	void Merge (DataBuffer* other) {
		if (!other)
			return;
		int oldsize = writesize;
		
		for (unsigned int x = 0; x < other->writesize; x++)
			Write<byte> (*(other->buffer+x));
		
		// Merge its marks and references
		unsigned int u = 0;
		for (u = 0; u < MAX_MARKS; u++) {
			if (other->marks[u]) {
				// Merge the mark and offset its position.
				if (marks[u])
					error ("DataBuffer: duplicate mark %d!\n");
				
				marks[u] = other->marks[u];
				marks[u]->pos += oldsize;
				
				// The original mark becomes NULL so that the deconstructor
				// will not delete it prematurely. (should it delete any
				// marks in the first place since there is no such thing
				// as at temporary mark?)
				other->marks[u] = NULL;
			}
			
			if (other->refs[u]) {
				// Same for references
				// TODO: add a g_NextRef system like here, akin to marks!
				unsigned int r = AddMarkReference (other->refs[u]->num, false);
				refs[r]->pos = other->refs[u]->pos + oldsize;
			}
		}
		
		delete other;
	}
	
	// Clones this databuffer to a new one and returns it.
	DataBuffer* Clone () {
		DataBuffer* other = new DataBuffer;
		for (unsigned int x = 0; x < writesize; x++)
			other->Write<unsigned char> (*(buffer+x));
		return other;
	}
	
	// ====================================================================
	// Adds a mark to the buffer. A mark is a "pointer" to a particular
	// position in the bytecode. The actual permanent position cannot
	// be predicted in any way or form, thus these things will be used
	// to "mark" a position like that for future use.
	unsigned int AddMark (str name) {
		// Find a free slot for the mark
		unsigned int u = g_NextMark++;
		
		if (marks[u])
			error ("DataBuffer: attempted to re-create mark %u!\n", u);
		
		if (u >= MAX_MARKS)
			error ("mark quota exceeded, all labels, if-structs and loops add marks\n");
		
		ScriptMark* m = new ScriptMark;
		m->name = name;
		m->pos = writesize;
		marks[u] = m;
		return u;
	}
	
	// ====================================================================
	// A ref is another "mark" that references a mark. When the bytecode
	// is written to file, they are changed into their marks' current
	// positions. Marks themselves are never written to files, only refs are
	unsigned int AddMarkReference (unsigned int marknum, bool placeholder = true) {
		unsigned int u;
		for (u = 0; u < MAX_MARKS; u++)
			if (!refs[u])
				break;
		
		if (u == MAX_MARKS)
			error ("mark reference quota exceeded, all goto-statements, if-structs and loops add refs\n");
		
		// NOTE: Do not check if the mark actually exists here since a
		// reference may come in the code earlier than the actual mark
		// and the new mark number can be predicted.
		//	11/8/12: eh? The mark is always created first.
		ScriptMarkReference* r = new ScriptMarkReference;
		r->num = marknum;
		r->pos = writesize;
		refs[u] = r;
		
		// Write a dummy placeholder for the reference
		if (placeholder)
			Write<word> (1234);
		
		return u;
	}
	
	// Delete a mark and all references to it.
	void DeleteMark (unsigned int marknum) {
		if (!marks[marknum])
			return;
		
		// Delete the mark
		delete marks[marknum];
		marks[marknum] = NULL;
		
		// Delete its references
		for (unsigned int u = 0; u < MAX_MARKS; u++) {
			if (refs[u]->num == marknum) {
				delete refs[u];
				refs[u] = NULL;
			}
		}
	}
	
	// Adjusts a mark to the current position
	void MoveMark (unsigned int mark, int offset = -1) {
		if (!marks[mark])
			return;
		marks[mark]->pos = writesize;
	}
	
	void OffsetMark (unsigned int mark, size_t offset) {
		if (!marks[mark])
			return;
		marks[mark]->pos += offset;
	}
	
	// Dump the buffer (for debugging purposes)
	void Dump() {
		for (unsigned int x = 0; x < writesize; x++)
			printf ("%d. [%d]\n", x, *(buffer+x));
	}
	
	// Count the amount of marks
	unsigned int CountMarks () {
		unsigned int count = 0;
		for (unsigned int u = 0; u < MAX_MARKS; u++)
			count += !!marks[u];
		return count;
	}
	
	// Count the amount of refs
	unsigned int CountReferences () {
		unsigned int count = 0;
		for (unsigned int u = 0; u < MAX_MARKS; u++)
			count += !!refs[u];
		return count;
	}
};

#endif // __DATABUFFER_H__