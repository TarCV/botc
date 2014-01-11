/*
	Copyright (c) 2013-2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "containers.h"
#include "bots.h"
#include "object_writer.h"
#include "stringtable.h"
#include "variables.h"
#include "parser.h"

list<script_variable> g_GlobalVariables;
list<script_variable> g_LocalVariables;

// ============================================================================
// Tries to declare a new global-scope variable. Returns pointer
// to new global variable, null if declaration failed.
script_variable* DeclareGlobalVariable (botscript_parser* r, type_e type, string name)
{
	// Unfortunately the VM does not support string variables so yeah.
	if (type == TYPE_STRING)
		error ("variables cannot be string\n");

	// Check that the variable is valid
	if (FindCommand (name))
		error ("name of variable-to-be `%s` conflicts with that of a command", name.chars());

	if (IsKeyword (name))
		error ("name of variable-to-be `%s` is a keyword", name.chars());

	if (g_GlobalVariables.size() >= MAX_SCRIPT_VARIABLES)
		error ("too many global variables!");

	for (int i = 0; i < g_GlobalVariables.size(); i++)
		if (g_GlobalVariables[i].name == name)
			error ("attempted redeclaration of global variable `%s`", name.chars());

	script_variable g;
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
script_variable* FindGlobalVariable (string name)
{
	for (int i = 0; i < g_GlobalVariables.size(); i++)
	{
		script_variable* g = &g_GlobalVariables[i];

		if (g->name == name)
			return g;
	}

	return null;
}

// ============================================================================
// Count all declared global variables
int CountGlobalVars ()
{
	return g_GlobalVariables.size();
}
