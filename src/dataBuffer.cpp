/*
	Copyright 2012-2014 Teemu Piippo
	Copyright 2019-2020 TarCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from this
	   software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstring>
#include "dataBuffer.h"

// _________________________________________________________________________________________________
//
DataBuffer::DataBuffer (int size) :
	m_buffer (new char[size]),
	m_allocatedSize (size),
	m_position (&buffer()[0]) {}

// _________________________________________________________________________________________________
//
DataBuffer::~DataBuffer()
{
	ASSERT (marks().isEmpty());
	ASSERT (references().isEmpty());
	delete buffer();
}

// _________________________________________________________________________________________________
//
//	Copies the contents of the given buffer into this buffer. The other buffer's marks and 
//	references will be moved along and the buffer is then destroyed.
//
void DataBuffer::mergeAndDestroy (DataBuffer* other)
{
	if (other == null)
		return;

	// Note: We transfer the marks before the buffer is copied, so that the
	// offset uses the proper value (which is the written size of @other, which
	// we don't want our written size to be added to yet).
	other->transferMarksTo (this);
	copyBuffer (other);
	delete other;
}

// _________________________________________________________________________________________________
//
//	Clones this databuffer to a new one and returns it. Note that the original transfers its marks
//	and references and loses them in the process.
//
DataBuffer* DataBuffer::clone()
{
	DataBuffer* other = new DataBuffer;
	transferMarksTo (other);
	other->copyBuffer (this);
	return other;
}

// ============================================================================
//
void DataBuffer::copyBuffer (const DataBuffer* buf)
{
	checkSpace (buf->writtenSize());
	memcpy (m_position, buf->buffer(), buf->writtenSize());
	m_position += buf->writtenSize();
}

// ============================================================================
//
void DataBuffer::transferMarksTo (DataBuffer* dest)
{
	int offset = dest->writtenSize();

	for (ByteMark* mark : marks())
	{
		mark->pos += offset;
		dest->m_marks << mark;
	}

	for (MarkReference* ref : references())
	{
		ref->pos += offset;
		dest->m_references << ref;
	}

	m_marks.clear();
	m_references.clear();
}

// _________________________________________________________________________________________________
//
//	Adds a new mark to the current position with the given name
//
ByteMark* DataBuffer::addMark (const String& name)
{
	ByteMark* mark = new ByteMark;
	mark->name = name;
	mark->pos = writtenSize();
	m_marks << mark;
	return mark;
}

// _________________________________________________________________________________________________
//
//	Adds a new reference to the given mark at the current position. This function will write 4 
//	bytes to the buffer whose value will be determined at final output writing.
//
MarkReference* DataBuffer::addReference (ByteMark* mark)
{
	MarkReference* ref = new MarkReference;
	ref->target = mark;
	ref->pos = writtenSize();
	m_references << ref;

	// Write a dummy placeholder for the reference
	writeDWord (0xBEEFCAFE);

	return ref;
}

// _________________________________________________________________________________________________
//
//	Moves the given mark to the current bytecode position.
//
void DataBuffer::adjustMark (ByteMark* mark)
{
	mark->pos = writtenSize();
}

// _________________________________________________________________________________________________
//
//	Shifts the given mark by the amount of bytes
//
void DataBuffer::offsetMark (ByteMark* mark, int bytes)
{
	mark->pos += bytes;
}

// _________________________________________________________________________________________________
//
//	Writes a push of the index of the given string. 8 bytes will be written and the string index
//	will be pushed to stack.
//
void DataBuffer::writeStringIndex (const String& a)
{
	writeHeader (DataHeader::PushStringIndex);
	writeDWord (getStringTableIndex (a));
}

// _________________________________________________________________________________________________
//
//	Writes a data header. 4 bytes.
//
void DataBuffer::writeHeader (DataHeader data)
{
	writeDWord (static_cast<int32_t> (data));
}

// _________________________________________________________________________________________________
//
//	Prints the buffer to stdout.
//
void DataBuffer::dump()
{
	for (int i = 0; i < writtenSize(); ++i)
		printf ("%d. [0x%X]\n", i, buffer()[i]);
}

// _________________________________________________________________________________________________
//
// Ensures there's at least the given amount of bytes left in the buffer. Will resize if necessary,
// no-op if not. On resize, 512 extra bytes are allocated to reduce the amount of resizes.
//
void DataBuffer::checkSpace (int bytes)
{
	int writesize = writtenSize();

	if (writesize + bytes < allocatedSize())
		return;

	// We don't have enough space in the buffer to write
	// the stuff - thus resize.

	// Remake the buffer with the new size. Have enough space
	// for the stuff we're going to write, as well as a bit
	// of leeway so we don't have to resize immediately again.

	int oldsize = allocatedSize();
	int newsize = oldsize + bytes + 512;

	char* buf = buffer();
	setBuffer (new char[newsize]);
	setAllocatedSize (newsize);

	// Now, copy the stuff back.
	std::memcpy (m_buffer, buf, oldsize);
	setPosition (buffer() + writesize);

	delete buf;
}

// _________________________________________________________________________________________________
//
//	Writes the given byte into the buffer.
//
void DataBuffer::writeByte (int8_t data)
{
	checkSpace (1);
	*m_position++ = data;
}

// _________________________________________________________________________________________________
//
//	Writes the given word into the buffer. 2 bytes will be written.
//
void DataBuffer::writeWord (int16_t data)
{
	checkSpace (2);

	for (int i = 0; i < 2; ++i)
		*m_position++ = (data >> (i * 8)) & 0xFF;
}

// _________________________________________________________________________________________________
//
//	Writes the given dword into the buffer. 4bytes will be written.
//
void DataBuffer::writeDWord (int32_t data)
{
	checkSpace (4);

	for (int i = 0; i < 4; ++i)
		*m_position++ = (data >> (i * 8)) & 0xFF;
}

// _________________________________________________________________________________________________
//
//	Writes the given string to the databuffer. The string will be written as-is without using string
//	indices. This will write 4 + length bytes. No header will be written.
//
void DataBuffer::writeString (const String& a)
{
	checkSpace (a.length() + 4);
	writeDWord (a.length());

	for (char c : a)
		writeByte (c);
}

// _________________________________________________________________________________________________
//
//	Tries to locate the mark by the given name. Returns null if not found.
//
ByteMark* DataBuffer::findMarkByName (const String& name)
{
	for (ByteMark* mark : marks())
	{
		if (mark->name == name)
			return mark;
	}

	return null;
}
