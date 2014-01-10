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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "scriptreader.h"
#include "str.h"
#include "commands.h"

CommandDef* g_CommDef;

// ============================================================================
// Reads command definitions from commands.def and stores them to memory.
void ReadCommands () {
	ScriptReader* r = new ScriptReader ("commands.def");
	g_CommDef = null;
	CommandDef* curdef = g_CommDef;
	unsigned int numCommDefs = 0;
	
	while (r->PeekNext().len()) {
		CommandDef* comm = new CommandDef;
		comm->next = null;
		
		// Number
		r->MustNumber ();
		comm->number = r->token.to_long();
		
		r->MustNext (":");
		
		// Name
		r->MustNext ();
		comm->name = r->token;
		if (IsKeyword (comm->name))
			r->ParserError ("command name `%s` conflicts with keyword", comm->name.chars());
		
		r->MustNext (":");
		
		// Return value
		r->MustNext ();
		comm->returnvalue = GetTypeByName (r->token);
		if (comm->returnvalue == -1)
			r->ParserError ("bad return value type `%s` for command %s", r->token.chars(), comm->name.chars());
		
		r->MustNext (":");
		
		// Num args
		r->MustNumber ();
		comm->numargs = r->token.to_long();
		
		r->MustNext (":");
		
		// Max args
		r->MustNumber ();
		comm->maxargs = r->token.to_long();
		
		if (comm->maxargs > MAX_MAXARGS)
			r->ParserError ("maxargs (%d) greater than %d!", comm->maxargs, MAX_MAXARGS);
		
		// Argument types
		int curarg = 0;
		while (curarg < comm->maxargs) {
			r->MustNext (":");
			r->MustNext ();
			
			type_e type = GetTypeByName (r->token);
			if (type == -1)
				r->ParserError ("bad argument %d type `%s`", curarg, r->token.chars());
			if (type == TYPE_VOID)
				r->ParserError ("void is not a valid argument type!");
			comm->argtypes[curarg] = type;
			
			r->MustNext ("(");
			r->MustNext ();
			
			// - 1 because of terminating null character
			if (r->token.len() > MAX_ARGNAMELEN - 1)
				r->ParserWarning ("argument name is too long (%d, max is %d)",
					r->token.len(), MAX_ARGNAMELEN - 1);
			
			strncpy (comm->argnames[curarg], r->token.chars(), MAX_ARGNAMELEN);
			comm->argnames[curarg][MAX_ARGNAMELEN-1] = 0;
			
			// If this is an optional parameter, we need the default value.
			if (curarg >= comm->numargs) {
				r->MustNext ("=");
				switch (type) {
				case TYPE_INT:
				case TYPE_BOOL:
					r->MustNumber();
					break;
				case TYPE_STRING:
					r->MustString();
					break;
				case TYPE_UNKNOWN:
				case TYPE_VOID:
					break;
				}
				
				comm->defvals[curarg] = r->token.to_long();
			}
			
			r->MustNext (")");
			curarg++;
		}
		
		if (!g_CommDef)
			g_CommDef = comm;
		
		if (!curdef) {
			curdef = comm;
		} else {
			curdef->next = comm;
			curdef = comm;
		}
		numCommDefs++;
	}
	
	if (!numCommDefs)
		r->ParserError ("no commands defined!\n");
	
	r->CloseFile ();
	delete r;
	printf ("%d command definitions read.\n", numCommDefs);
}

// ============================================================================
// Finds a command by name
CommandDef* FindCommand (string fname) {
	CommandDef* comm;
	ITERATE_COMMANDS (comm) {
		if (fname.to_uppercase() == comm->name.to_uppercase())
			return comm;
	}
	
	return null;
}

// ============================================================================
// Returns the prototype of the command
string GetCommandPrototype (CommandDef* comm) {
	string text;
	text += GetTypeName (comm->returnvalue);
	text += ' ';
	text += comm->name;
	text += '(';
	
	bool hasOptionalArguments = false;
	for (int i = 0; i < comm->maxargs; i++) {
		if (i == comm->numargs) {
			hasOptionalArguments = true;
			text += '[';
		}
		
		if (i)
			text += ", ";
		
		text += GetTypeName (comm->argtypes[i]);
		text += ' ';
		text += comm->argnames[i];
		
		if (i >= comm->numargs) {
			text += '=';
			
			bool isString = comm->argtypes[i] == TYPE_STRING;
			if (isString) text += '"';
			
			char defvalstring[8];
			sprintf (defvalstring, "%d", comm->defvals[i]);
			text += defvalstring;
			
			if (isString) text += '"';
		}
	}
	
	if (hasOptionalArguments)
		text += ']';
	text += ')';
	return text;
}