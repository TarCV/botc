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
#include "databuffer.h"
#include "str.h"

extern int g_CurMode;

class ObjWriter {
public:
	// ====================================================================
	// MEMBERS
	FILE* fp;
	str filepath;
	DataBuffer* MainBuffer;
	DataBuffer* OnEnterBuffer;
	DataBuffer* MainLoopBuffer;
	unsigned int numWrittenBytes;
	unsigned int numWrittenReferences;
	
	// ====================================================================
	// METHODS
	ObjWriter (str path);
	void WriteString (char* s);
	void WriteString (const char* s);
	void WriteString (str s);
	void WriteBuffer (DataBuffer* buf);
	void WriteBuffers ();
	void WriteStringTable ();
	void WriteToFile ();
	DataBuffer* GetCurrentBuffer ();
	
	unsigned int AddMark (str name);
	unsigned int FindMark (str name);
	unsigned int AddReference (unsigned int mark);
	void MoveMark (unsigned int mark);
	void OffsetMark (unsigned int mark, int offset);
	void DeleteMark (unsigned int mark);
	
	template <class T> void Write (T stuff) {
		DataBuffer* buffer = GetCurrentBuffer ();
		buffer->Write<T> (stuff);
		return;
	}
	
	// Default to word
	void Write (word stuff) {
		Write<word> (stuff);
	}
	
	template <class T> void WriteDataToFile (T stuff) {
		// One byte at a time
		for (unsigned int x = 0; x < sizeof (T); x++) {
			union_t<T> uni;
			uni.val = stuff;
			fwrite (&uni.b[x], 1, 1, fp);
			numWrittenBytes++;
		}
	}
};

#endif // __OBJWRITER_H__