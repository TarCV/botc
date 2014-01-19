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

#ifndef BOTC_OBJWRITER_H
#define BOTC_OBJWRITER_H

#include <stdio.h>
#include <typeinfo>
#include <string.h>
#include "main.h"
#include "str.h"
#include "data_buffer.h"

class object_writer
{
public:
	// ====================================================================
	// MEMBERS

	// Pointer to the file we're writing to
	FILE* fp;

	// The main buffer - the contents of this is what we
	// write to file after parsing is complete
	data_buffer* MainBuffer;

	// onenter buffer - the contents of the onenter{} block
	// is buffered here and is merged further at the end of state
	data_buffer* OnEnterBuffer;

	// Mainloop buffer - the contents of the mainloop{} block
	// is buffered here and is merged further at the end of state
	data_buffer* MainLoopBuffer;

	// Switch buffer - switch case data is recorded to this
	// buffer initially, instead of into main buffer.
	data_buffer* SwitchBuffer;

	// How many bytes have we written to file?
	int numWrittenBytes;

	// How many references did we resolve in the main buffer?
	int numWrittenReferences;

	// ====================================================================
	// METHODS
	object_writer();
	void write_string (string s);
	void write_buffer (data_buffer* buf);
	void write_member_buffers();
	void write_string_table();
	void write_to_file (string filepath);
	data_buffer* get_current_buffer();

	int add_mark (string name);
	int find_byte_mark (string name);
	int add_reference (int mark);
	void move_mark (int mark);
	void offset_mark (int mark, int offset);
	void delete_mark (int mark);

	template <class T> void write (T stuff)
	{
		get_current_buffer()->write (stuff);
	}

private:
	// Write given data to file.
	template <class T> void write_data_to_file (T stuff)
	{
		// One byte at a time
		generic_union<T> uni;
		uni.as_t = stuff;

		for (int x = 0; x < sizeof (T); x++)
		{
			fwrite (&uni.as_bytes[x], 1, 1, fp);
			numWrittenBytes++;
		}
	}
};

#endif // BOTC_OBJWRITER_H
