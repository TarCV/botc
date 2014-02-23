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
#include "Main.h"
#include "StringTable.h"

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
class DataBuffer
{
	PROPERTY (private, char*,					Buffer,			SetBuffer,			STOCK_WRITE)
	PROPERTY (private, int,						AllocatedSize,	SetAllocatedSize,	STOCK_WRITE)
	PROPERTY (private, char*,					Position,		SetPosition,		STOCK_WRITE)
	PROPERTY (private, List<ByteMark*>,			Marks,			SetMarks,			STOCK_WRITE)
	PROPERTY (private, List<MarkReference*>,	References,		SetReferences,		STOCK_WRITE)

	public:
		DataBuffer (int size = 128);
		~DataBuffer();

		// ====================================================================
		// Merge another data buffer into this one.
		// Note: @other is destroyed in the process!
		void MergeAndDestroy (DataBuffer* other);

		ByteMark*		AddMark (String name);
		MarkReference*	AddReference (ByteMark* mark);
		void			CheckSpace (int bytes);
		DataBuffer*		Clone();
		void			DeleteMark (int marknum);
		void			AdjustMark (ByteMark* mark);
		void			OffsetMark (ByteMark* mark, int offset);
		ByteMark*		FindMarkByName (const String& target);
		void			Dump();
		void			TransferMarks (DataBuffer* other);
		void			WriteStringIndex (const String& a);
		void			WriteString (const String& a);
		void			WriteByte (int8_t data);
		void			WriteWord (int16_t data);
		void			WriteDWord (int32_t data);
		void			CopyBuffer (const DataBuffer* buf);

		inline int WrittenSize() const
		{
			return Position() - Buffer();
		}
};

#endif // BOTC_DATABUFFER_H
