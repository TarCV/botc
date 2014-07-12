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

#include "dataBuffer.h"

// ============================================================================
//
DataBuffer::DataBuffer (int size)
{
	setBuffer (new char[size]);
	setPosition (&buffer()[0]);
	setAllocatedSize (size);
}

// ============================================================================
//
DataBuffer::~DataBuffer()
{
	ASSERT (marks().isEmpty());
	ASSERT (references().isEmpty());
	delete buffer();
}

// ============================================================================
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

// ============================================================================
//
// Clones this databuffer to a new one and returns it. Note that the original
// transfers its marks and references and loses them in the process.
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

// ============================================================================
//
ByteMark* DataBuffer::addMark (const String& name)
{
	ByteMark* mark = new ByteMark;
	mark->name = name;
	mark->pos = writtenSize();
	m_marks << mark;
	return mark;
}

// ============================================================================
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

// ============================================================================
//
void DataBuffer::adjustMark (ByteMark* mark)
{
	mark->pos = writtenSize();
}

// ============================================================================
//
void DataBuffer::offsetMark (ByteMark* mark, int position)
{
	mark->pos += position;
}

// ============================================================================
//
void DataBuffer::writeStringIndex (const String& a)
{
	writeDWord (DH_PushStringIndex);
	writeDWord (getStringTableIndex (a));
}

// ============================================================================
//
void DataBuffer::dump()
{
	for (int i = 0; i < writtenSize(); ++i)
		printf ("%d. [0x%X]\n", i, buffer()[i]);
}

// ============================================================================
//
void DataBuffer::checkSpace (int bytes)
{
	int writesize = writtenSize();

	if (writesize + bytes < allocatedSize())
		return;

	// We don't have enough space in the buffer to write
	// the stuff - thus resize. First, store the old
	// buffer temporarily:
	char* copy = new char[allocatedSize()];
	memcpy (copy, buffer(), allocatedSize());

	// Remake the buffer with the new size. Have enough space
	// for the stuff we're going to write, as well as a bit
	// of leeway so we don't have to resize immediately again.
	int newsize = allocatedSize() + bytes + 512;

	delete buffer();
	setBuffer (new char[newsize]);
	setAllocatedSize (newsize);

	// Now, copy the stuff back.
	memcpy (m_buffer, copy, allocatedSize());
	setPosition (buffer() + writesize);
	delete copy;
}

// =============================================================================
//
void DataBuffer::writeByte (int8_t data)
{
	checkSpace (1);
	*m_position++ = data;
}

// =============================================================================
//
void DataBuffer::writeWord (int16_t data)
{
	checkSpace (2);

	for (int i = 0; i < 2; ++i)
		*m_position++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void DataBuffer::writeDWord (int32_t data)
{
	checkSpace (4);

	for (int i = 0; i < 4; ++i)
		*m_position++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void DataBuffer::writeString (const String& a)
{
	checkSpace (a.length() + 4);
	writeDWord (a.length());

	for (char c : a)
		writeByte (c);
}


// =============================================================================
//
ByteMark* DataBuffer::findMarkByName (const String& name)
{
	for (ByteMark* mark : marks())
		if (mark->name == name)
			return mark;

	return null;
}
