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
#include "bots.h"
#include "botcommands.h"
#include "objwriter.h"
#include "stringtable.h"
#include "variables.h"
#include "scriptreader.h"

ScriptVar* g_GlobalVariables[MAX_SCRIPT_VARIABLES];
ScriptVar* g_LocalVariable;

void InitVariables () {
	unsigned int u;
	ITERATE_GLOBAL_VARS (u)
		g_GlobalVariables[u] = NULL;
}

// Tries to declare a new global-scope variable. Returns pointer
// to new global variable, NULL if declaration failed.
ScriptVar* DeclareGlobalVariable (ScriptReader* r, str name, int type) {
	// Apparently the Zandronum engine does not support string variables..
	if (type == RETURNVAL_VOID || type == RETURNVAL_STRING)
		return NULL;
	
	// Find a NULL pointer to a global variable
	ScriptVar* g;
	unsigned int u = 0;
	ITERATE_GLOBAL_VARS (u) {
		// Check that it doesn't exist already
		if (!g_GlobalVariables[u])
			break;
		
		if (!g_GlobalVariables[u]->name.icompare (name))
			r->ParserError ("tried to declare global scope variable %s twice", name.chars());
	}
	
	if (u == MAX_SCRIPT_VARIABLES)
		r->ParserError ("too many global variables!");
	
	g = new ScriptVar;
	g->index = u;
	g->name = name;
	g->statename = "";
	g->type = type;
	g->next = NULL;
	g->value =	(type == RETURNVAL_BOOLEAN) ? "false" :
			(type == RETURNVAL_INT) ? "0" : "";
	
	g_GlobalVariables[u] = g;
	return g;
}

/*void AssignScriptVariable (ScriptReader* r, str name, str value) {
	ScriptVar* g = FindScriptVariable (name);
	if (!g)
		r->ParserError ("global variable %s not declared", name.chars());
}*/

// Find a global variable by name
/* ScriptVar* FindScriptVariable (str name) {
	ScriptVar* g;
	ITERATE_SCRIPT_VARS (g)
		if (!g->name.compare (name))
			return g;
	
	return NULL;
}*/

unsigned int CountGlobalVars () {
	ScriptVar* var;
	unsigned int count = 0;
	unsigned int u = 0;
	ITERATE_GLOBAL_VARS (u) {
		if (g_GlobalVariables[u] != NULL)
			count++;
	}
	return count;
}