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

#ifndef BOTC_VARIABLES_H
#define BOTC_VARIABLES_H

#include "Main.h"

class ExpressionValue;
class BotscriptParser;

struct ScriptVariable
{
	enum EWritability
	{
		WRITE_Mutable,		// normal read-many-write-many variable
		WRITE_Const,		// write-once const variable
		WRITE_Constexpr,	// const variable whose value is known to compiler
	};

	String			name;
	String			statename;
	EType			type;
	int				index;
	EWritability	writelevel;
	int				value;

	inline bool IsGlobal() const
	{
		return statename.IsEmpty();
	}
};

extern List<ScriptVariable> g_GlobalVariables;
extern List<ScriptVariable> g_LocalVariables;

ScriptVariable* DeclareGlobalVariable (EType type, String name);
ScriptVariable* FindGlobalVariable (String name);

#endif // BOTC_VARIABLES_H
