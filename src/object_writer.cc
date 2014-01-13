/*
	Copyright (c) 2012-2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "main.h"
#include "object_writer.h"
#include "data_buffer.h"
#include "stringtable.h"

extern bool g_GotMainLoop;

object_writer::object_writer()
{
	MainBuffer = new data_buffer;
	MainLoopBuffer = new data_buffer;
	OnEnterBuffer = new data_buffer;
	SwitchBuffer = null; // created on demand
	numWrittenBytes = 0;
	numWrittenReferences = 0;
}

void object_writer::write_string (string a)
{
	write (a.length());

	for (int i = 0; i < a.length(); i++)
		write (a[i]);
}

void object_writer::write_buffer (data_buffer* buf)
{
	get_current_buffer()->merge (buf);
}

void object_writer::write_member_buffers()
{
	// If there was no mainloop defined, write a dummy one now.
	if (!g_GotMainLoop)
	{
		MainLoopBuffer->write (dh_main_loop);
		MainLoopBuffer->write (dh_end_main_loop);
	}

	// Write the onenter and mainloop buffers, IN THAT ORDER
	for (int i = 0; i < 2; i++)
	{
		data_buffer** buf = (!i) ? &OnEnterBuffer : &MainLoopBuffer;
		write_buffer (*buf);

		// Clear the buffer afterwards for potential next state
		*buf = new data_buffer;
	}

	// Next state definitely has no mainloop yet
	g_GotMainLoop = false;
}

// Write string table
void object_writer::write_string_table()
{
	int stringcount = num_strings_in_table();

	if (!stringcount)
		return;

	// Write header
	write (dh_string_list);
	write (stringcount);

	// Write all strings
	for (int a = 0; a < stringcount; a++)
		write_string (get_string_table()[a]);
}

// Write main buffer to file
void object_writer::write_to_file (string filepath)
{
	fp = fopen (filepath, "w");
	CHECK_FILE (fp, filepath, "writing");

	// First, resolve references
	numWrittenReferences = 0;

	for (int u = 0; u < MAX_MARKS; u++)
	{
		ScriptMarkReference* ref = MainBuffer->refs[u];

		if (!ref)
			continue;

		// Substitute the placeholder with the mark position
		generic_union<word> uni;
		uni.as_word = static_cast<word> (MainBuffer->marks[ref->num]->pos);

		for (int v = 0; v < (int) sizeof (word); v++)
			memset (MainBuffer->buffer + ref->pos + v, uni.as_bytes[v], 1);

		/*
		printf ("reference %u at %d resolved to %u at %d\n",
			u, ref->pos, ref->num, MainBuffer->marks[ref->num]->pos);
		*/
		numWrittenReferences++;
	}

	// Then, dump the main buffer to the file
	for (int x = 0; x < MainBuffer->writesize; x++)
		write_data_to_file<byte> (*(MainBuffer->buffer + x));

	print ("-- %1 byte%s1 written to %2\n", numWrittenBytes, filepath);
	fclose (fp);
}

data_buffer* object_writer::get_current_buffer()
{
	return	SwitchBuffer ? SwitchBuffer :
			(g_CurMode == MODE_MAINLOOP) ? MainLoopBuffer :
			(g_CurMode == MODE_ONENTER) ? OnEnterBuffer :
			MainBuffer;
}

ScriptMark* g_ScriptMark = null;

// Adds a mark
int object_writer::add_mark (string name)
{
	return get_current_buffer()->add_mark (name);
}

// Adds a reference
int object_writer::add_reference (int mark)
{
	data_buffer* b = get_current_buffer();
	return b->add_reference (mark);
}

// Finds a mark
int object_writer::find_byte_mark (string name)
{
	data_buffer* b = get_current_buffer();

	for (int u = 0; u < MAX_MARKS; u++)
	{
		if (b->marks[u] && b->marks[u]->name.to_uppercase() == name.to_uppercase())
			return u;
	}

	return MAX_MARKS;
}

// Moves a mark to the current position
void object_writer::move_mark (int mark)
{
	get_current_buffer()->move_mark (mark);
}

// Deletes a mark
void object_writer::delete_mark (int mark)
{
	get_current_buffer()->delete_mark (mark);
}

void object_writer::offset_mark (int mark, int offset)
{
	get_current_buffer()->offset_mark (mark, offset);
}
