/*
	Copyright (c) 2012-2014, Santeri Piippo
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "scriptreader.h"
#include "str.h"
#include "commands.h"
#include "lexer.h"

static list<command_info*> g_commands;

// ============================================================================
// Reads command definitions from commands.def and stores them to memory.
void init_commands ()
{
	lexer lx;
	lx.process_file ("commands.def");

	while (lx.get_next())
	{
		command_info* comm = new command_info;

		// Number
		lx.must_be (tk_number);
		comm->number = lx.get_token()->text.to_long();

		lx.must_get_next (tk_colon);

		// Name
		lx.must_get_next (tk_symbol);
		comm->name = lx.get_token()->text;

		if (IsKeyword (comm->name))
			error ("command name `%1` conflicts with keyword", comm->name);

		lx.must_get_next (tk_colon);

		// Return value
		lx.must_get_any_of ({tk_int, tk_void, tk_bool, tk_str});
		comm->returnvalue = GetTypeByName (lx.get_token()->text); // TODO
		assert (comm->returnvalue != -1);

		lx.must_get_next (tk_colon);

		// Num args
		lx.must_get_next (tk_number);
		comm->numargs = lx.get_token()->text.to_long();

		lx.must_get_next (tk_colon);

		// Max args
		lx.must_get_next (tk_number);
		comm->maxargs = lx.get_token()->text.to_long();

		// Argument types
		int curarg = 0;

		while (curarg < comm->maxargs)
		{
			command_argument arg;
			lx.must_get_next (tk_colon);
			lx.must_get_any_of ({tk_int, tk_bool, tk_str});
			type_e type = GetTypeByName (lx.get_token()->text);
			assert (type != -1 && type != TYPE_VOID);
			arg.type = type;

			lx.must_get_next (tk_paren_start);
			lx.must_get_next (tk_symbol);
			arg.name = lx.get_token()->text;

			// If this is an optional parameter, we need the default value.
			if (curarg >= comm->numargs)
			{
				lx.must_get_next (tk_assign);

				switch (type)
				{
					case TYPE_INT:
					case TYPE_BOOL:
						lx.must_get_next (tk_number);
						break;

					case TYPE_STRING:
						lx.must_get_next (tk_string);
						break;

					case TYPE_UNKNOWN:
					case TYPE_VOID:
						break;
				}

				arg.defvalue = lx.get_token()->text.to_long();
			}

			lx.must_get_next (tk_paren_end);
			comm->args << arg;
			curarg++;
		}

		g_commands << comm;
	}

	if (g_commands.is_empty())
		error ("no commands defined!\n");

	print ("%1 command definitions read.\n", g_commands.size());
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
			text += '=';

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
