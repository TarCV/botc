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

#ifndef __OBJWRITER_H__
#define __OBJWRITER_H__

#include <stdio.h>
#include <typeinfo>
#include <string.h>
#include "common.h"
#include "str.h"

extern int g_CurMode;

// ============================================================================
// DataBuffer: A dynamic data buffer.
class DataBuffer {
public:
	// The actual buffer
	unsigned char* buffer;
	
	// How big is the buffer?
	unsigned int bufsize;
	
	// Position in the buffer
	unsigned int writepos;
	unsigned int readpos;
	
	// METHODS
	DataBuffer (unsigned int size=128) {
		writepos = 0;
		readpos = 0;
		
		buffer = new unsigned char[size];
		bufsize = size;
	}
	
	~DataBuffer () {
		delete buffer;
	}
	
	template<class T> void Write(T stuff) {
		if (sizeof (char) != 1) {
			error ("DataBuffer: sizeof(char) must be 1!\n");
		}
		
		// Out of space, must resize
		if (writepos + sizeof(T) >= bufsize) {
			// First, store the old buffer temporarily
			char* copy = new char[bufsize];
			printf ("Resizing buffer: copy buffer to safety. %u bytes to copy\n", bufsize);
			memcpy (copy, buffer, bufsize);
			
			// Remake the buffer with the new size.
			// Have a bit of leeway so we don't have to
			// resize immediately again.
			size_t newsize = bufsize + sizeof (T) + 128;
			delete buffer;
			buffer = new unsigned char[newsize];
			bufsize = newsize;
			
			// Now, copy the new stuff over.
			memcpy (buffer, copy, bufsize);
			
			// Nuke the copy now as it's no longer needed
			delete copy;
		}
		
		// Write the new stuff one byte at a time
		for (unsigned int x = 0; x < sizeof (T); x++) {
			buffer[writepos] = CharByte<T> (stuff, x);
			writepos++;
		}
	}
	
	template<class T> T Read() {
		T result = buffer[readpos];
		readpos += sizeof (T);
		return result;
	}

private:
	template <class T> unsigned char CharByte (T a, unsigned int b) {
		if (b >= sizeof (T))
			error ("CharByte: tried to get byte %u out of a %u-byte %s\n",
				b, sizeof (T), typeid(T).name());
		
		unsigned long p1 = pow<unsigned long> (256, b);
		unsigned long p2 = pow<unsigned long> (256, b+1);
		unsigned long r = (a % p2) / p1;
		
		if (r > 256)
			error ("result %lu too big!", r);
		
		unsigned char ur = static_cast<unsigned char> (r);
		return ur;
	}
};

class ObjWriter {
public:
	// ====================================================================
	// MEMBERS
	FILE* fp;
	DataBuffer* OnEnterBuffer;
	DataBuffer* MainLoopBuffer;
	unsigned int numWrittenBytes;
	
	// ====================================================================
	// METHODS
	ObjWriter (str path);
	~ObjWriter ();
	void WriteString (char* s);
	void WriteString (const char* s);
	void WriteString (str s);
	void WriteBuffers ();
	
	template <class T> void Write (T stuff) {
		// Mainloop and onenter are written into a separate buffer.
		if (g_CurMode == MODE_MAINLOOP || g_CurMode == MODE_ONENTER) {
			DataBuffer* buffer = (g_CurMode == MODE_MAINLOOP) ? MainLoopBuffer : OnEnterBuffer;
			buffer->Write<T> (stuff);
			return;
		}
		
		fwrite (&stuff, sizeof (T), 1, fp);
		numWrittenBytes += sizeof (T);
	}
	// Cannot use default arguments in function templates..
	void Write (long stuff) {Write<long> (stuff);}
};

#endif // __OBJWRITER_H__