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
#include "databuffer.h"
#include "stringtable.h"

#include "bots.h"

extern bool g_GotMainLoop;

ObjWriter::ObjWriter (str path) {
	MainBuffer = new DataBuffer;
	MainLoopBuffer = new DataBuffer;
	OnEnterBuffer = new DataBuffer;
	SwitchBuffer = NULL; // created on demand
	numWrittenBytes = 0;
	numWrittenReferences = 0;
	filepath = path;
}

void ObjWriter::WriteString (char* s) {
	Write (strlen (s));
	for (unsigned int u = 0; u < strlen (s); u++)
		Write ((s)[u]);
}

void ObjWriter::WriteString (const char* s) {
	WriteString (const_cast<char*> (s));
}

void ObjWriter::WriteString (str s) {
	WriteString (s.chars());
}

void ObjWriter::WriteBuffer (DataBuffer* buf) {
	GetCurrentBuffer()->Merge (buf);
}

void ObjWriter::WriteBuffers () {
	// If there was no mainloop defined, write a dummy one now.
	if (!g_GotMainLoop) {
		MainLoopBuffer->Write (DH_MAINLOOP);
		MainLoopBuffer->Write (DH_ENDMAINLOOP);
	}
	
	// Write the onenter and mainloop buffers, IN THAT ORDER
	for (int i = 0; i < 2; i++) {
		DataBuffer** buf = (!i) ? &OnEnterBuffer : &MainLoopBuffer;
		WriteBuffer (*buf);
		
		// Clear the buffer afterwards for potential next state
		*buf = new DataBuffer;
	}
	
	// Next state definitely has no mainloop yet
	g_GotMainLoop = false;
}

// Write string table
void ObjWriter::WriteStringTable () {
	unsigned int stringcount = CountStringTable ();
	if (!stringcount)
		return;
	
	// Write header
	Write (DH_STRINGLIST);
	Write (stringcount);
	
	// Write all strings
	for (unsigned int a = 0; a < stringcount; a++)
		WriteString (g_StringTable[a]);
}

// Write main buffer to file
void ObjWriter::WriteToFile () {
	fp = fopen (filepath, "w");
	CHECK_FILE (fp, filepath, "writing");
	
	// First, resolve references
	numWrittenReferences = 0;
	for (unsigned int u = 0; u < MAX_MARKS; u++) {
		ScriptMarkReference* ref = MainBuffer->refs[u];
		if (!ref)
			continue;
		
		// Substitute the placeholder with the mark position
		union_t<word> uni;
		uni.val = static_cast<word> (MainBuffer->marks[ref->num]->pos);
		for (unsigned int v = 0; v < sizeof (word); v++)
			memset (MainBuffer->buffer + ref->pos + v, uni.b[v], 1);
		
		/*
		printf ("reference %u at %d resolved to %u at %d\n",
			u, ref->pos, ref->num, MainBuffer->marks[ref->num]->pos);
		*/
		numWrittenReferences++;
	}
	
	// Then, dump the main buffer to the file
	for (unsigned int x = 0; x < MainBuffer->writesize; x++)
		WriteDataToFile<byte> (*(MainBuffer->buffer+x));
	
	printf ("-- %u byte%s written to %s\n", numWrittenBytes, PLURAL (numWrittenBytes), filepath.chars());
	fclose (fp);
}

DataBuffer* ObjWriter::GetCurrentBuffer() {
	return	SwitchBuffer ? SwitchBuffer :
		(g_CurMode == MODE_MAINLOOP) ? MainLoopBuffer :
		(g_CurMode == MODE_ONENTER) ? OnEnterBuffer :
		MainBuffer;
}

ScriptMark* g_ScriptMark = NULL;

// Adds a mark
unsigned int ObjWriter::AddMark (str name) {
	return GetCurrentBuffer()->AddMark (name);
}

// Adds a reference
unsigned int ObjWriter::AddReference (unsigned int mark) {
	DataBuffer* b = GetCurrentBuffer();
	return b->AddMarkReference (mark);
}

// Finds a mark
unsigned int ObjWriter::FindMark (str name) {
	DataBuffer* b = GetCurrentBuffer();
	for (unsigned int u = 0; u < MAX_MARKS; u++) {
		if (b->marks[u] && !b->marks[u]->name.icompare (name))
			return u;
	}
	return MAX_MARKS;
}

// Moves a mark to the current position
void ObjWriter::MoveMark (unsigned int mark) {
	GetCurrentBuffer()->MoveMark (mark);
}

// Deletes a mark
void ObjWriter::DeleteMark (unsigned int mark) {
	GetCurrentBuffer()->DeleteMark (mark);
}

void ObjWriter::OffsetMark (unsigned int mark, int offset) {
	GetCurrentBuffer()->OffsetMark (mark, offset);
}