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

#include "DataBuffer.h"

// ============================================================================
//
DataBuffer::DataBuffer (int size)
{
	SetBuffer (new char[size]);
	SetPosition (&GetBuffer()[0]);
	SetAllocatedSize (size);
}

// ============================================================================
//
DataBuffer::~DataBuffer()
{
	assert (CountMarks() == 0 && CountReferences() == 0);
	delete GetBuffer();
}

// ============================================================================
//
void DataBuffer::MergeAndDestroy (DataBuffer* other)
{
	if (!other)
		return;

	// Note: We transfer the marks before the buffer is copied, so that the
	// offset uses the proper value (which is the written size of @other, which
	// we don't want our written size to be added to yet).
	other->TransferMarks (this);
	CopyBuffer (other);
	delete other;
}

// ============================================================================
//
// Clones this databuffer to a new one and returns it. Note that the original
// transfers its marks and references and loses them in the process.
//
DataBuffer* DataBuffer::Clone()
{
	DataBuffer* other = new DataBuffer;
	TransferMarks (other);
	other->CopyBuffer (this);
	return other;
}

// ============================================================================
//
void DataBuffer::CopyBuffer (const DataBuffer* buf)
{
	CheckSpace (buf->GetWrittenSize());
	memcpy (mPosition, buf->GetBuffer(), buf->GetWrittenSize());
	mPosition += buf->GetWrittenSize();
}

// ============================================================================
//
void DataBuffer::TransferMarks (DataBuffer* other)
{
	int offset = other->GetWrittenSize();

	for (ByteMark* mark : GetMarks())
	{
		mark->pos += offset;
		other->PushToMarks (mark);
	}

	for (MarkReference* ref : GetReferences())
	{
		ref->pos += offset;
		other->PushToReferences (ref);
	}

	ClearMarks();
	ClearReferences();
}

// ============================================================================
//
ByteMark* DataBuffer::AddMark (String name)
{
	ByteMark* mark = new ByteMark;
	mark->name = name;
	mark->pos = GetWrittenSize();
	PushToMarks (mark);
	return mark;
}

// ============================================================================
//
MarkReference* DataBuffer::AddReference (ByteMark* mark)
{
	MarkReference* ref = new MarkReference;
	ref->target = mark;
	ref->pos = GetWrittenSize();
	PushToReferences (ref);

	// Write a dummy placeholder for the reference
	WriteDWord (0xBEEFCAFE);

	return ref;
}

// ============================================================================
//
void DataBuffer::AdjustMark (ByteMark* mark)
{
	mark->pos = GetWrittenSize();
}

// ============================================================================
//
void DataBuffer::OffsetMark (ByteMark* mark, int offset)
{
	mark->pos += offset;
}

// ============================================================================
//
void DataBuffer::WriteStringIndex (const String& a)
{
	WriteDWord (DH_PushStringIndex);
	WriteDWord (GetStringTableIndex (a));
}

// ============================================================================
//
void DataBuffer::Dump()
{
	for (int i = 0; i < GetWrittenSize(); ++i)
		printf ("%d. [0x%X]\n", i, GetBuffer()[i]);
}

// ============================================================================
//
void DataBuffer::CheckSpace (int bytes)
{
	int writesize = GetWrittenSize();

	if (writesize + bytes < GetAllocatedSize())
		return;

	// We don't have enough space in the buffer to write
	// the stuff - thus resize. First, store the old
	// buffer temporarily:
	char* copy = new char[GetAllocatedSize()];
	memcpy (copy, GetBuffer(), GetAllocatedSize());

	// Remake the buffer with the new size. Have enough space
	// for the stuff we're going to write, as well as a bit
	// of leeway so we don't have to resize immediately again.
	int newsize = GetAllocatedSize() + bytes + 512;

	delete GetBuffer();
	SetBuffer (new char[newsize]);
	SetAllocatedSize (newsize);

	// Now, copy the stuff back.
	memcpy (mBuffer, copy, GetAllocatedSize());
	SetPosition (GetBuffer() + writesize);
	delete copy;
}

// =============================================================================
//
void DataBuffer::WriteByte (int8_t data)
{
	CheckSpace (1);
	*mPosition++ = data;
}

// =============================================================================
//
void DataBuffer::WriteWord (int16_t data)
{
	CheckSpace (2);

	for (int i = 0; i < 2; ++i)
		*mPosition++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void DataBuffer::WriteDWord (int32_t data)
{
	CheckSpace (4);

	for (int i = 0; i < 4; ++i)
		*mPosition++ = (data >> (i * 8)) & 0xFF;
}

// =============================================================================
//
void DataBuffer::WriteString (const String& a)
{
	CheckSpace (a.Length() + 4);
	WriteDWord (a.Length());

	for (char c : a)
		WriteByte (c);
}


// =============================================================================
//
ByteMark* DataBuffer::FindMarkByName (const String& target)
{
	for (ByteMark* mark : GetMarks())
		if (mark->name == target)
			return mark;

	return null;
}
