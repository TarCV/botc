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
data_buffer::data_buffer (int size)
{
	set_writepos (get_buffer());
	set_buffer (new byte[size]);
	set_allocated_size (size);
}

// ============================================================================
//
data_buffer::~data_buffer()
{
	assert (count_marks() == 0 && count_refs() == 0);
	delete get_buffer();
}

// ============================================================================
//
void data_buffer::merge_and_destroy (data_buffer* other)
{
	if (!other)
		return;

	int oldsize = get_write_size();
	copy_buffer (other);

	// Assimilate in its marks and references
	for (byte_mark* mark : other->get_marks())
	{
		mark->pos += oldsize;
		push_to_marks (mark);
	}

	for (mark_reference* ref : other->get_refs())
	{
		ref->pos += oldsize;
		push_to_refs (ref);
	}

	clear_marks();
	clear_refs();
	delete other;
}

// ============================================================================
//
data_buffer* data_buffer::clone()
{
	data_buffer* other = new data_buffer;
	other->copy_buffer (this);
	return other;
}

// ============================================================================
//
void data_buffer::copy_buffer (const data_buffer* buf)
{
	check_space (buf->get_write_size());
	memcpy (m_writepos, buf->get_buffer(), buf->get_write_size());
	m_writepos += buf->get_write_size();
}

// ============================================================================
//
byte_mark* data_buffer::add_mark (string name)
{
	byte_mark* mark = new byte_mark;
	mark->name = name;
	mark->pos = get_write_size();
	push_to_marks (mark);
	return mark;
}

// ============================================================================
//
mark_reference* data_buffer::add_reference (byte_mark* mark, bool write_placeholder)
{
	mark_reference* ref = new mark_reference;
	ref->target = mark;
	ref->pos = get_write_size();
	push_to_refs (ref);

	// Write a dummy placeholder for the reference
	if (write_placeholder)
		write_dword (0xBEEFCAFE);

	return ref;
}

// ============================================================================
//
void data_buffer::adjust_mark (byte_mark* mark)
{
	mark->pos = get_write_size();
}

// ============================================================================
//
void data_buffer::offset_mark (byte_mark* mark, int offset)
{
	mark->pos += offset;
}

// ============================================================================
//
void data_buffer::write_float (float a)
{
	// TODO: Find a way to store the number without decimal loss.
	write_dword (dh_push_number);
	write_dword (abs (a));

	if (a < 0)
		write_dword (dh_unary_minus);
}

// ============================================================================
//
void data_buffer::write_string_index (const string& a)
{
	write_dword (dh_push_string_index);
	write_dword (get_string_table_index (a));
}

// ============================================================================
//
void data_buffer::dump()
{
	for (int i = 0; i < get_write_size(); ++i)
		printf ("%d. [%d]\n", i, get_buffer()[i]);
}

// ============================================================================
//
void data_buffer::check_space (int bytes)
{
	int writesize = get_write_size();

	if (writesize + bytes < get_allocated_size())
		return;

	// We don't have enough space in the buffer to write
	// the stuff - thus resize. First, store the old
	// buffer temporarily:
	char* copy = new char[get_allocated_size()];
	memcpy (copy, get_buffer(), get_allocated_size());

	// Remake the buffer with the new size. Have enough space
	// for the stuff we're going to write, as well as a bit
	// of leeway so we don't have to resize immediately again.
	size_t newsize = get_allocated_size() + bytes + 512;

	delete get_buffer();
	set_buffer (new byte[newsize]);
	set_allocated_size (newsize);

	// Now, copy the stuff back.
	memcpy (m_buffer, copy, get_allocated_size());
	set_writepos (get_buffer() + writesize);
	delete copy;
}

// =============================================================================
//
void data_buffer::write_byte (int8_t data)
{
	check_space (1);
	*m_writepos++ = data;
}

// =============================================================================
//
void data_buffer::write_word (int16_t data)
{
	check_space (2);

	for (int i = 0; i < 2; ++i)
		*m_writepos++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void data_buffer::write_dword (int32_t data)
{
	check_space (4);

	for (int i = 0; i < 4; ++i)
		*m_writepos++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void data_buffer::write_string (const string& a)
{
	check_space (a.length() + 1);

	for (char c : a)
		write_byte (c);

	write_byte ('\0');
}


// =============================================================================
//
byte_mark* data_buffer::find_mark_by_name (const string& target)
{
	for (byte_mark* mark : get_marks())
		if (mark->name == target)
			return mark;

	return null;
}