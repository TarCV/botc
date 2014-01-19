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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "str.h"
#include "commands.h"
#include "lexer.h"

static list<command_info*> g_commands;

// ============================================================================
//
void add_command_definition (command_info* comm)
{
	// Ensure that there is no conflicts
	for (command_info* it : g_commands)
		if (it->number == comm->number)
			error ("Attempted to redefine command #%1 (%2) as %3",
				g_commands[comm->number]->name, comm->name);

	g_commands << comm;
}

// ============================================================================
// Finds a command by name
command_info* find_command_by_name (string fname)
{
	for (command_info* comm : g_commands)
	{
		if (fname.to_uppercase() == comm->name.to_uppercase())
			return comm;
	}

	return null;
}

// ============================================================================
// Returns the prototype of the command
string get_command_signature (command_info* comm)
{
	string text;
	text += GetTypeName (comm->returnvalue);
	text += ' ';
	text += comm->name;

	if (comm->maxargs != 0)
		text += ' ';

	text += '(';

	bool hasoptionals = false;

	for (int i = 0; i < comm->maxargs; i++)
	{
		if (i == comm->numargs)
		{
			hasoptionals = true;
			text += '[';
		}

		if (i)
			text += ", ";

		text += GetTypeName (comm->args[i].type);
		text += ' ';
		text += comm->args[i].name;

		if (i >= comm->numargs)
		{
			text += " = ";

			bool is_string = comm->args[i].type == TYPE_STRING;

			if (is_string)
				text += '"';

			text += string::from_number (comm->args[i].defvalue);

			if (is_string)
				text += '"';
		}
	}

	if (hasoptionals)
		text += ']';

	text += ')';
	return text;
}

const list<command_info*> get_commands()
{
	return g_commands;
}
