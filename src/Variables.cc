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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StringTable.h"
#include "Variables.h"
#include "Parser.h"

List<ScriptVariable> g_GlobalVariables;
List<ScriptVariable> g_LocalVariables;

// ============================================================================
// Tries to declare a new global-scope variable. Returns pointer
// to new global variable, null if declaration failed.
ScriptVariable* DeclareGlobalVariable (EType type, String name)
{
	// Unfortunately the VM does not support string variables so yeah.
	if (type == EStringType)
		Error ("variables cannot be string\n");

	// Check that the variable is valid
	if (FindCommandByName (name))
		Error ("name of variable-to-be `%s` conflicts with that of a command", name.CString());

	if (g_GlobalVariables.Size() >= gMaxGlobalVars)
		Error ("too many global variables!");

	for (int i = 0; i < g_GlobalVariables.Size(); i++)
		if (g_GlobalVariables[i].name == name)
			Error ("attempted redeclaration of global variable `%s`", name.CString());

	ScriptVariable g;
	g.index = g_GlobalVariables.Size();
	g.name = name;
	g.statename = "";
	g.value = 0;
	g.type = type;

	g_GlobalVariables << g;
	return &g_GlobalVariables[g.index];
}

// ============================================================================
// Find a global variable by name
ScriptVariable* FindGlobalVariable (String name)
{
	for (ScriptVariable& var : g_GlobalVariables)
		if (var.name == name)
			return &var;

	return null;
}