#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "str.h"
#include "objwriter.h"
#include "databuffer.h"
#include "stringtable.h"

#include "bots.h"

extern bool g_GotMainLoop;

ObjWriter::ObjWriter (string path) {
	MainBuffer = new DataBuffer;
	MainLoopBuffer = new DataBuffer;
	OnEnterBuffer = new DataBuffer;
	SwitchBuffer = null; // created on demand
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

void ObjWriter::WriteString (string s) {
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
	unsigned int stringcount = num_strings_in_table ();
	if (!stringcount)
		return;
	
	// Write header
	Write (DH_STRINGLIST);
	Write (stringcount);
	
	// Write all strings
	for (unsigned int a = 0; a < stringcount; a++)
		WriteString (get_string_table()[a]);
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

ScriptMark* g_ScriptMark = null;

// Adds a mark
unsigned int ObjWriter::AddMark (string name) {
	return GetCurrentBuffer()->AddMark (name);
}

// Adds a reference
unsigned int ObjWriter::AddReference (unsigned int mark) {
	DataBuffer* b = GetCurrentBuffer();
	return b->AddMarkReference (mark);
}

// Finds a mark
unsigned int ObjWriter::FindMark (string name) {
	DataBuffer* b = GetCurrentBuffer();
	for (unsigned int u = 0; u < MAX_MARKS; u++) {
		if (b->marks[u] && b->marks[u]->name.to_uppercase() == name.to_uppercase())
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