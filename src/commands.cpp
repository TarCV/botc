/*
	Copyright 2012-2014 Teemu Piippo
	Copyright 2019-2020 TarCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from this
	   software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stringClass.h"
#include "commands.h"
#include "lexer.h"

static List<CommandInfo*> Commands;

// _________________________________________________________________________________________________
//
void addCommandDefinition (CommandInfo* comm)
{
	// Ensure that there is no conflicts
	for (CommandInfo* it : Commands)
	{
		if (it->number == comm->number && it->isbuiltin == comm->isbuiltin)
		{
			error ("Attempted to redefine command #%1 (%2) as %3",
				   Commands[comm->number]->name, comm->name);
		}
	}

	Commands << comm;
}

// _________________________________________________________________________________________________
// Finds a command by name
CommandInfo* findCommandByName (String fname)
{
	for (CommandInfo* comm : Commands)
	{
		if (fname.toUppercase() == comm->name.toUppercase())
			return comm;
	}

	return null;
}

// _________________________________________________________________________________________________
//
// Returns the prototype of the command
//
String CommandInfo::signature()
{
	String text;
	text += dataTypeName (returnvalue);
	text += ' ';
	text += name;

	if (args.isEmpty() == false)
		text += ' ';

	text += '(';

	bool hasoptionals = false;

	for (int i = 0; i < args.size(); i++)
	{
		if (i == minargs)
		{
			hasoptionals = true;
			text += '[';
		}

		if (i)
			text += ", ";

		text += dataTypeName (args[i].type);
		text += ' ';
		text += args[i].name;

		if (i >= minargs)
		{
			bool isString = args[i].type == TYPE_String;
			text += " = ";

			if (isString)
				text += '"';

			text += String::fromNumber (args[i].defvalue);

			if (isString)
				text += '"';
		}
	}

	if (hasoptionals)
		text += ']';

	text += ')';
	return text;
}

const List<CommandInfo*>& getCommands()
{
	return Commands;
}
