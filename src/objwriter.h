#ifndef BOTC_OBJWRITER_H
#define BOTC_OBJWRITER_H

#include <stdio.h>
#include <typeinfo>
#include <string.h>
#include "main.h"
#include "str.h"
#include "databuffer.h"

class ObjWriter {
public:
	// ====================================================================
	// MEMBERS
	
	// Pointer to the file we're writing to
	FILE* fp;
	
	// Path to the file we're writing to
	string filepath;
	
	// The main buffer - the contents of this is what we
	// write to file after parsing is complete
	DataBuffer* MainBuffer;
	
	// onenter buffer - the contents of the onenter{} block
	// is buffered here and is merged further at the end of state
	DataBuffer* OnEnterBuffer;
	
	// Mainloop buffer - the contents of the mainloop{} block
	// is buffered here and is merged further at the end of state
	DataBuffer* MainLoopBuffer;
	
	// Switch buffer - switch case data is recorded to this
	// buffer initially, instead of into main buffer.
	DataBuffer* SwitchBuffer;
	
	// How many bytes have we written to file?
	unsigned int numWrittenBytes;
	
	// How many references did we resolve in the main buffer?
	unsigned int numWrittenReferences;
	
	// ====================================================================
	// METHODS
	ObjWriter (string path);
	void WriteString (char* s);
	void WriteString (const char* s);
	void WriteString (string s);
	void WriteBuffer (DataBuffer* buf);
	void WriteBuffers ();
	void WriteStringTable ();
	void WriteToFile ();
	DataBuffer* GetCurrentBuffer ();
	
	unsigned int AddMark (string name);
	unsigned int FindMark (string name);
	unsigned int AddReference (unsigned int mark);
	void MoveMark (unsigned int mark);
	void OffsetMark (unsigned int mark, int offset);
	void DeleteMark (unsigned int mark);
	
	template <class T> void Write (T stuff) {
		GetCurrentBuffer ()->Write (stuff);
	}
	
	// Default to word
	void Write (word stuff) {
		Write<word> (stuff);
	}
	
	void DoWrite (byte stuff) {
		Write<byte> (stuff);
	}
	
private:
	// Write given data to file.
	template <class T> void WriteDataToFile (T stuff) {
		// One byte at a time
		union_t<T> uni;
		uni.val = stuff;
		for (unsigned int x = 0; x < sizeof (T); x++) {
			fwrite (&uni.b[x], 1, 1, fp);
			numWrittenBytes++;
		}
	}
};

#endif // BOTC_OBJWRITER_H