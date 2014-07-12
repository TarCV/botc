/*
	Copyright 2012-2014 Teemu Piippo
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
#include "stringTable.h"

/**
 *    @class DataBuffer
 *    @brief Stores a buffer of bytecode
 *
 *    The DataBuffer class stores a section of bytecode. Buffers are allocated on
 *    the heap and written to using the @c write* functions. Buffers can be cut and
 *    pasted together with @c mergeAndDestroy, note that this function destroys the
 *    parameter buffer in the process
 *
 *    A mark is a "pointer" to a particular position in the bytecode. The actual
 *    permanent position cannot be predicted in any way or form, thus these things
 *    are used to "bookmark" a position like that for future use.
 *
 *    A reference acts as a pointer to a mark. The reference is four bytes in the
 *    bytecode which will be replaced with its mark's position when the bytecode
 *    is written to the output file.
 *
 *    This mark/reference system is used to know bytecode offset values when
 *    compiling, even though actual final positions cannot be known.
 */
class DataBuffer
{
	//! @
	PROPERTY (private, char*,					buffer,			setBuffer,			STOCK_WRITE)
	PROPERTY (private, int,						allocatedSize,	setAllocatedSize,	STOCK_WRITE)
	PROPERTY (private, char*,					position,		setPosition,		STOCK_WRITE)
	PROPERTY (private, List<ByteMark*>,			marks,			setMarks,			STOCK_WRITE)
	PROPERTY (private, List<MarkReference*>,	references,		setReferences,		STOCK_WRITE)

	public:
		//! Constructs a new databuffer with @c size bytes.
		DataBuffer (int size = 128);

		//! Destructs the databuffer.
		~DataBuffer();

		//! Adds a new mark to the current position with the name @c name.
		//! @param name the name of the new mark
		//! @return a pointer to the new mark
		ByteMark*		addMark (const String& name);

		//! Adds a new reference to @c mark at the current position. This
		//! function will write 4 bytes to the buffer whose value will
		//! be determined at final output writing.
		//! @param mark the mark which the new reference will attach to
		//! @return a pointer to the new reference
		MarkReference*	addReference (ByteMark* mark);

		//! Moves @c mark to the current bytecode position.
		//! @param mark the mark to adjust
		void			adjustMark (ByteMark* mark);

		//! Ensures there's at least @c bytes left in the buffer. Will resize
		//! if necessary, no-op if not. On resize, 512 extra bytes are allocated
		//! to reduce the amount of resizes.
		//! @param bytes the amount of space in bytes to ensure allocated
		void			checkSpace (int bytes);

		//! Creates a clone of this data buffer.
		//! @note All marks will be moved into the new databuffer as marks are
		//! @note never duplicated.
		//! @return The newly cloned databuffer.
		DataBuffer*		clone();

		//! Prints the buffer to stdout. Useful for debugging.
		void			dump();

		//! Finds the given mark by name.
		//! @param name the name of the mark to find
		ByteMark*		findMarkByName (const String& name);

		//! Merge another data buffer into this one.
		//! Note: @c other is destroyed in the process.
		//! @param other the buffer to merge in
		void			mergeAndDestroy (DataBuffer* other);

		//! Moves @c mark to the given bytecode position.
		//! @param mark the mark to adjust
		//! @param position where to adjust the mark
		void			offsetMark (ByteMark* mark, int position);

		//! Transfers all marks of this buffer to @c other.
		//! @param other the data buffer to transfer marks to
		void			transferMarksTo (DataBuffer* other);

		//! Writes the index of the given string to the databuffer.
		//! 4 bytes will be written to the bytecode.
		//! @param a the string whose index to write
		void			writeStringIndex (const String& a);

		//! Writes the given string as-is into the databuffer.
		//! @c a.length + 4 bytes will be written to the buffer.
		//! @param a the string to write
		void			writeString (const String& a);

		//! Writes the given byte to the buffer. 1 byte will be written.
		//! @c data the byte to write
		void			writeByte (int8_t data);

		//! Writes the given word to the buffer. 2 byte will be written.
		//! @c data the word to write
		void			writeWord (int16_t data);

		//! Writes the given double word to the buffer. 4 bytes will be written.
		//! @c data the double word to write
		void			writeDWord (int32_t data);

		//! @return the amount of bytes written to this buffer.
		inline int		writtenSize() const
		{
			return position() - buffer();
		}

	protected:
		//! Writes the buffer's contents from @c buf.
		//! @c buf.writtenSize() bytes will be written.
		//! @param buf the buffer to copy
		void			copyBuffer (const DataBuffer* buf);
};

#endif // BOTC_DATABUFFER_H
