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

#ifndef BOTC_DATABUFFER_H
#define BOTC_DATABUFFER_H

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stringtable.h"

#define MAX_MARKS 512

// ============================================================================
// data_buffer: A dynamic data buffer.
//
// Notes:
//
// - A mark is a "pointer" to a particular position in the bytecode. The actual
//   permanent position cannot be predicted in any way or form, thus these things
//   are used to "mark" a position like that for future use.
//
// - A reference is another "mark" that references a mark. When the bytecode
//   is written to file, they are changed into their marks' current
//   positions. Marks themselves are never written to files, only refs are
//
class data_buffer
{
	PROPERTY (private, byte*,					buffer,			NO_OPS,		STOCK_WRITE)
	PROPERTY (private, int,						allocated_size,	NUM_OPS,	STOCK_WRITE)
	PROPERTY (private, byte*,					writepos,		NO_OPS,		STOCK_WRITE)
	PROPERTY (private, list<byte_mark*>,		marks,			LIST_OPS,	STOCK_WRITE)
	PROPERTY (private, list<mark_reference*>,	refs,			LIST_OPS,	STOCK_WRITE)

	public:
		data_buffer (int size = 128);
		~data_buffer();

		// ====================================================================
		// Merge another data buffer into this one.
		// Note: @other is destroyed in the process!
		void merge_and_destroy (data_buffer* other);

		// Clones this databuffer to a new one and returns it.
		data_buffer* clone ();

		byte_mark* add_mark (string name);

		mark_reference*	add_reference (byte_mark* mark, bool write_placeholder = true);
		void			check_space (int bytes);
		void			delete_mark (int marknum);
		void			adjust_mark(byte_mark* mark);
		void			offset_mark (byte_mark* mark, int offset);
		byte_mark*		find_mark_by_name (const string& target);
		void			dump();
		void			write_float (float a);
		void			write_string_index (const string& a);
		void			write_string (const string& a);
		void			write_byte (int8_t data);
		void			write_word (int16_t data);
		void			write_dword (int32_t data);
		void			copy_buffer (const data_buffer* buf);

		inline int get_write_size() const
		{
			return m_writepos - get_buffer();
		}
};

#endif // BOTC_DATABUFFER_H
