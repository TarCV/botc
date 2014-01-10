/*
 *	botc source code
 *	Copyright (C) 2012 Santeri `Dusk` Piippo
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	
 *	1. Redistributions of source code must retain the above copyright notice,
 *	   this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright notice,
 *	   this list of conditions and the following disclaimer in the documentation
 *	   and/or other materials provided with the distribution.
 *	3. Neither the name of the developer nor the names of its contributors may
 *	   be used to endorse or promote products derived from this software without
 *	   specific prior written permission.
 *	4. Redistributions in any form must be accompanied by information on how to
 *	   obtain complete source code for the software and any accompanying
 *	   software that uses the software. The source code must either be included
 *	   in the distribution or be available for no more than the cost of
 *	   distribution plus a nominal fee, and must be freely redistributable
 *	   under reasonable conditions. For an executable file, complete source
 *	   code means the source code for all modules it contains. It does not
 *	   include source code for modules or files that typically accompany the
 *	   major components of the operating system on which the executable file
 *	   runs.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */

#define __VARIABLES_CXX__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "array.h"
#include "bots.h"
#include "botcommands.h"
#include "objwriter.h"
#include "stringtable.h"
#include "variables.h"
#include "scriptreader.h"

array<ScriptVar> g_GlobalVariables;
array<ScriptVar> g_LocalVariables;

// ============================================================================
// Tries to declare a new global-scope variable. Returns pointer
// to new global variable, NULL if declaration failed.
ScriptVar* DeclareGlobalVariable (ScriptReader* r, type_e type, str name) {
	// Unfortunately the VM does not support string variables so yeah.
	if (type == TYPE_STRING)
		r->ParserError ("variables cannot be string\n");
	
	// Check that the variable is valid
	if (FindCommand (name))
		r->ParserError ("name of variable-to-be `%s` conflicts with that of a command", name.chars());
	
	if (IsKeyword (name))
		r->ParserError ("name of variable-to-be `%s` is a keyword", name.chars());
	
	if (g_GlobalVariables.size() >= MAX_SCRIPT_VARIABLES)
		r->ParserError ("too many global variables!");
	
	for (uint i = 0; i < g_GlobalVariables.size(); i++)
		if (g_GlobalVariables[i].name == name)
			r->ParserError ("attempted redeclaration of global variable `%s`", name.chars());
	
	ScriptVar g;
	g.index = g_GlobalVariables.size();
	g.name = name;
	g.statename = "";
	g.value = 0;
	g.type = type;
	
	g_GlobalVariables << g;
	return &g_GlobalVariables[g.index];
}

// ============================================================================
// Find a global variable by name
ScriptVar* FindGlobalVariable (str name) {
	for (uint i = 0; i < g_GlobalVariables.size(); i++) {
		ScriptVar* g = &g_GlobalVariables[i];
		if (g->name == name)
			return g;
	}
	
	return NULL;
}

// ============================================================================
// Count all declared global variables
uint CountGlobalVars () {
	return g_GlobalVariables.size();
}