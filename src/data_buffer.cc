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

#include "data_buffer.h"

// ============================================================================
//
void data_buffer::merge (data_buffer* other)
{
	if (!other)
		return;

	int oldsize = writesize;

	for (int x = 0; x < other->writesize; x++)
		write (* (other->buffer + x));

	// Merge its marks and references
	int u = 0;

	for (u = 0; u < MAX_MARKS; u++)
	{
		if (other->marks[u])
		{
			// Merge the mark and offset its position.
			if (marks[u])
				error ("DataBuffer: duplicate mark %d!\n");

			marks[u] = other->marks[u];
			marks[u]->pos += oldsize;

			// The original mark becomes null so that the deconstructor
			// will not delete it prematurely. (should it delete any
			// marks in the first place since there is no such thing
			// as at temporary mark?)
			other->marks[u] = null;
		}

		if (other->refs[u])
		{
			// Same for references
			// TODO: add a g_NextRef system like here, akin to marks!
			int r = add_reference (other->refs[u]->num, false);
			refs[r]->pos = other->refs[u]->pos + oldsize;
		}
	}

	delete other;
}

// ============================================================================
//
data_buffer* data_buffer::clone()
{
	data_buffer* other = new data_buffer;

	for (int x = 0; x < writesize; x++)
		other->write (* (buffer + x));

	return other;
}

// ============================================================================
//
int data_buffer::add_mark (string name)
{
	// Find a free slot for the mark
	int u = g_NextMark++;

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

// ============================================================================
//
int data_buffer::add_reference (int marknum, bool placeholder)
{
	int u;

	for (u = 0; u < MAX_MARKS; u++)
		if (!refs[u])
			break;

	// TODO: get rid of this
	if (u == MAX_MARKS)
		error ("mark reference quota exceeded, all goto-statements, if-structs and loops add refs\n");

	ScriptMarkReference* r = new ScriptMarkReference;
	r->num = marknum;
	r->pos = writesize;
	refs[u] = r;

	// Write a dummy placeholder for the reference
	if (placeholder)
		write (0xBEEFCAFE);

	return u;
}

// ============================================================================
//
void data_buffer::delete_mark (int marknum)
{
	if (!marks[marknum])
		return;

	// Delete the mark
	delete marks[marknum];
	marks[marknum] = null;

	// Delete its references
	for (int u = 0; u < MAX_MARKS; u++)
	{
		if (refs[u]->num == marknum)
		{
			delete refs[u];
			refs[u] = null;
		}
	}
}

// ============================================================================
//
void data_buffer::move_mark (int i)
{
	if (!marks[i])
		return;

	marks[i]->pos = writesize;
}

// ============================================================================
//
void data_buffer::offset_mark (int mark, int offset)
{
	if (!marks[mark])
		return;

	marks[mark]->pos += offset;
}

// ============================================================================
//
int data_buffer::count_marks()
{
	int count = 0;

	for (int u = 0; u < MAX_MARKS; u++)
		count += !!marks[u];

	return count;
}

// ============================================================================
//
int data_buffer::count_references()
{
	int count = 0;

	for (int u = 0; u < MAX_MARKS; u++)
		count += !!refs[u];

	return count;
}

// ============================================================================
//
void data_buffer::write_float (string floatstring)
{
	// TODO: Casting float to word causes the decimal to be lost.
	// Find a way to store the number without such loss.
	float val = atof (floatstring);
	write (dh_push_number);
	write (static_cast<word> (abs (val)));

	if (val < 0)
		write (dh_unary_minus);
}

// ============================================================================
//
void data_buffer::write_string (string a)
{
	write (dh_push_string_index);
	write (get_string_table_index (a));
}

// ============================================================================
//
void data_buffer::dump()
{
	for (int x = 0; x < writesize; x++)
		printf ("%d. [%d]\n", x, * (buffer + x));
}

// ============================================================================
//
data_buffer::~data_buffer()
{
	delete buffer;

	// Delete any marks and references
	for (int i = 0; i < MAX_MARKS; i++)
	{
		delete marks[i];
		delete refs[i];
	}
}

// ============================================================================
//
data_buffer::data_buffer (int size)
{
	writesize = 0;

	buffer = new unsigned char[size];
	allocsize = size;

	// Clear the marks table out
	for (int u = 0; u < MAX_MARKS; u++)
	{
		marks[u] = null;
		refs[u] = null;
	}
}
