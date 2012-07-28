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

#define __OBJWRITER_CXX__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "str.h"
#include "objwriter.h"
#include "stringtable.h"

#include "bots.h"

extern bool g_GotMainLoop;

ObjWriter::ObjWriter (str path) {
	MainBuffer = new DataBuffer;
	MainLoopBuffer = new DataBuffer;
	OnEnterBuffer = new DataBuffer;
	numWrittenBytes = 0;
	filepath = path;
}

ObjWriter::~ObjWriter () {
	delete MainBuffer;
	delete OnEnterBuffer;
	
	// This crashes for some reason o_O
	// Should make no difference anyway, since ObjWriter
	// is only deleted at the end of the program anyway
	// delete MainLoopBuffer;
}

void ObjWriter::WriteString (char* s) {
	Write<long> (strlen (s));
	for (unsigned int u = 0; u < strlen (s); u++)
		Write<char> (s[u]);
}

void ObjWriter::WriteString (const char* s) {
	WriteString (const_cast<char*> (s));
}

void ObjWriter::WriteString (str s) {
	WriteString (s.chars());
}

void ObjWriter::WriteBuffer (DataBuffer* buf) {
	for (unsigned int x = 0; x < buf->writesize; x++) {
		unsigned char c = *(buf->buffer+x);
		Write<unsigned char> (c);
	}
}

void ObjWriter::WriteBuffers () {
	// If there was no mainloop defined, write a dummy one now.
	if (!g_GotMainLoop) {
		MainLoopBuffer->Write<long> (DH_MAINLOOP);
		MainLoopBuffer->Write<long> (DH_ENDMAINLOOP);
	}
	
	// Write the onenter and mainloop buffers, IN THAT ORDER
	for (int i = 0; i < 2; i++) {
		DataBuffer* buf = (!i) ? OnEnterBuffer : MainLoopBuffer;
		WriteBuffer (buf);
		
		// Clear the buffer afterwards for potential next state
		delete buf;
		buf = new DataBuffer;
	}
	
	// Next state definitely has no mainloop yet
	g_GotMainLoop = false;
}

// Write string table
void ObjWriter::WriteStringTable () {
	// If we added strings here, we need to write a list of them.
	unsigned int stringcount = CountStringTable ();
	if (stringcount) {
		Write<long> (DH_STRINGLIST);
		Write<long> (stringcount);
		
		for (unsigned int a = 0; a < stringcount; a++)
			WriteString (g_StringTable[a]);
	}
	
	printf ("%u string%s written\n", stringcount, PLURAL (stringcount));
}

// Write main buffer to file
void ObjWriter::WriteToFile () {
	fp = fopen (filepath, "w");
	CHECK_FILE (fp, filepath, "writing");
	
	if (sizeof (unsigned char) != 1)
		error ("size of unsigned char isn't 1, but %d!\n", sizeof (unsigned char));
	
	for (unsigned int x = 0; x < MainBuffer->writesize; x++) {
		unsigned char c = *(MainBuffer->buffer+x);
		fwrite (&c, 1, 1, fp);
		numWrittenBytes++;
	}
	
	printf ("-- %u byte%s written to %s\n", numWrittenBytes, PLURAL (numWrittenBytes), filepath.chars());
	fclose (fp);
}