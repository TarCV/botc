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

#define __COMMANDS_CXX__
#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "scriptreader.h"
#include "str.h"
#include "commands.h"

CommandDef* g_CommDef;

void ReadCommands () {
	ScriptReader* r = new ScriptReader ((char*)"commands.def");
	g_CommDef = NULL;
	CommandDef* curdef = g_CommDef;
	unsigned int numCommDefs = 0;
	
	while (r->Next()) {
		CommandDef* comm = new CommandDef;
		
		int n = 0;
		str t = "";
		for (unsigned int u = 0; u < r->token.len(); u++) {
			// If we're at the last character of the string, we need
			// to both add the character to t and check it. Thus,
			// we do the addition, exceptionally, here.
			if (u == r->token.len() - 1 && r->token[u] != ':')
				t += r->token[u];
			
			if (r->token[u] == ':' || u == r->token.len() - 1) {
				int i = atoi (t.chars());
				switch (n) {
				case 0:
					// Number
					comm->number = i;
					break;
				case 1:
					// Name
					comm->name = t;
					break;
				case 2:
					// Return value
					t.tolower();
					if (!t.compare ("int"))
						comm->returnvalue = RETURNVAL_INT;
					else if (!t.compare ("str"))
						comm->returnvalue = RETURNVAL_STRING;
					else if (!t.compare ("void"))
						comm->returnvalue = RETURNVAL_VOID;
					else if (!t.compare ("bool"))
						comm->returnvalue = RETURNVAL_BOOLEAN;
					else
						r->ParseError ("bad return value type `%s`", t.chars());
					break;
				case 3:
					// Num args
					comm->numargs = i;
					break;
				case 4:
					// Max args
					comm->maxargs = i;
					break;
				}
				
				n++;
				t = "";
			} else {
				t += r->token[u];
			}
		}
		
		comm->next = NULL;
		
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
	
	/*
	CommandDef* c;
	ITERATE_COMMANDS (c) {
		printf ("[%u] %s: %d, %d arguments, return type %d\n",
			c->number, c->name.chars(), c->numargs, c->maxargs, c->returnvalue);
	}
	*/
	
	delete r;
	printf ("%d command definitions read.\n", numCommDefs);
}