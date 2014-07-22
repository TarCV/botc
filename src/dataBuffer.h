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

#include "main.h"
#include "stringTable.h"

// _________________________________________________________________________________________________
//
//	The DataBuffer class stores a section of bytecode. Buffers are allocated on
//	the heap and written to using the @c write* functions. Buffers can be cut and
//	pasted together with @c mergeAndDestroy, note that this function destroys the
//	parameter buffer in the process
//
//	A mark is a "pointer" to a particular position in the bytecode. The actual
//	permanent position cannot be predicted in any way or form, thus these things
//	are used to "bookmark" a position like that for future use.
//
//	A reference acts as a pointer to a mark. The reference is four bytes in the
//	bytecode which will be replaced with its mark's position when the bytecode
//	is written to the output file.
//
//	This mark/reference system is used to know bytecode offset values when
//	compiling, even though actual final positions cannot be known.
//
class DataBuffer
{
	PROPERTY (private, char*,					buffer,			setBuffer,			STOCK_WRITE)
	PROPERTY (private, int,						allocatedSize,	setAllocatedSize,	STOCK_WRITE)
	PROPERTY (private, char*,					position,		setPosition,		STOCK_WRITE)
	PROPERTY (private, List<ByteMark*>,			marks,			setMarks,			STOCK_WRITE)
	PROPERTY (private, List<MarkReference*>,	references,		setReferences,		STOCK_WRITE)

public:
	DataBuffer (int size = 128);
	~DataBuffer();

	ByteMark*		addMark (const String& name);
	MarkReference*	addReference (ByteMark* mark);
	void			adjustMark (ByteMark* mark);
	void			checkSpace (int bytes);
	DataBuffer*		clone();
	void			dump();
	ByteMark*		findMarkByName (const String& name);
	void			mergeAndDestroy (DataBuffer* other);
	void			offsetMark (ByteMark* mark, int position);
	void			transferMarksTo (DataBuffer* other);
	void			writeStringIndex (const String& a);
	void			writeString (const String& a);
	void			writeByte (int8_t data);
	void			writeWord (int16_t data);
	void			writeDWord (int32_t data);
	void			writeHeader (DataHeader data);
	inline int		writtenSize() const;

private:
	void			copyBuffer (const DataBuffer* buf);
};

// _________________________________________________________________________________________________
//
//	Returns the amount of bytes written into the buffer.
//
inline int DataBuffer::writtenSize() const
{
	return position() - buffer();
}

#endif // BOTC_DATABUFFER_H
