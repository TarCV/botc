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

#define __PARSER_CXX__

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "str.h"
#include "objwriter.h"
#include "scriptreader.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"

#define MUST_TOPLEVEL if (g_CurMode != MODE_TOPLEVEL) \
	ParserError ("%ss may only be defined at top level!", token.chars());

int g_NumStates = 0;
int g_NumEvents = 0;
int g_CurMode = MODE_TOPLEVEL;
str g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;

void ScriptReader::BeginParse (ObjWriter* w) {
	while (Next()) {
		if (!token.icompare ("state")) {
			MUST_TOPLEVEL
			
			MustString ();
			
			// State name must be a word.
			if (token.first (" ") != token.len())
				ParserError ("state name must be a single word! got `%s`", token.chars());
			str statename = token;
			
			// stateSpawn is special - it *must* be defined. If we
			// encountered it, then mark down that we have it.
			if (!token.icompare ("statespawn"))
				g_stateSpawnDefined = true;
			
			// Must end in a colon
			MustNext (":");
			
			// Write the previous state's onenter and
			// mainloop buffers to file now
			if (g_CurState.len())
				w->WriteBuffers();
			
			w->Write (DH_STATENAME);
			w->WriteString (statename);
			w->Write (DH_STATEIDX);
			w->Write (g_NumStates);
			
			g_NumStates++;
			g_CurState = token;
			g_GotMainLoop = false;
			continue;
		}
		
		if (!token.icompare ("event")) {
			MUST_TOPLEVEL
			
			// Event definition
			MustString ();
			
			EventDef* e = FindEventByName (token);
			if (!e)
				ParserError ("bad event! got `%s`\n", token.chars());
			
			MustNext ("{");
			
			g_CurMode = MODE_EVENT;
			
			w->Write (DH_EVENT);
			w->Write<long> (e->number);
			g_NumEvents++;
			continue;
		}
		
		if (!token.icompare ("mainloop")) {
			MUST_TOPLEVEL
			MustNext ("{");
			
			// Mode must be set before dataheader is written here!
			g_CurMode = MODE_MAINLOOP;
			w->Write (DH_MAINLOOP);
			continue;
		}
		
		if (!token.icompare ("onenter") || !token.icompare ("onexit")) {
			MUST_TOPLEVEL
			bool onenter = !token.compare ("onenter");
			MustNext ("{");
			
			// Mode must be set before dataheader is written here,
			// because onenter goes to a separate buffer.
			g_CurMode = onenter ? MODE_ONENTER : MODE_ONEXIT;
			w->Write (onenter ? DH_ONENTER : DH_ONEXIT);
			continue;
		}
		
		if (!token.compare ("var")) {
			// For now, only globals are supported
			if (g_CurMode != MODE_TOPLEVEL || g_CurState.len())
				ParserError ("variables must only be global for now");
			
			MustNext ();
			
			// Var name must not be a number
			if (token.isnumber())
				ParserError ("variable name must not be a number");
			
			str varname = token;
			ScriptVar* var = DeclareGlobalVariable (this, varname);
			
			if (!var)
				ParserError ("declaring %s variable %s failed",
					g_CurState.len() ? "state" : "global", varname.chars());
			
			MustNext (";");
			continue;
		}
		
		if (!token.compare ("}")) {
			// Closing brace
			int dataheader =	(g_CurMode == MODE_EVENT) ? DH_ENDEVENT :
						(g_CurMode == MODE_MAINLOOP) ? DH_ENDMAINLOOP :
						(g_CurMode == MODE_ONENTER) ? DH_ENDONENTER :
						(g_CurMode == MODE_ONEXIT) ? DH_ENDONEXIT : -1;
			
			if (dataheader == -1)
				ParserError ("unexpected `}`");
			
			// Data header must be written before mode is changed because
			// onenter and mainloop go into buffers, and we want the closing
			// data headers into said buffers too.
			w->Write (dataheader);
			g_CurMode = MODE_TOPLEVEL;
			
			if (!PeekNext().compare (";"))
				MustNext (";");
			continue;
		}
		// Check global variables
		ScriptVar* g = FindGlobalVariable (token);
		if (g) {
			// Not in top level, unfortunately..
			if (g_CurMode == MODE_TOPLEVEL)
				ParserError ("can't alter variables at top level");
			
			// Build operator string. Only '=' is one
			// character, others are two.
			MustNext ();
			str oper = token;
			if (token.compare ("=") != 0) {
				MustNext ();
				oper += token;
			}
			
			// Unary operators
			if (!oper.compare ("++")) {
				w->Write<long> (DH_INCGLOBALVAR);
				w->Write<long> (g->index);
			} else if (!oper.compare ("--")) {
				w->Write<long> (DH_DECGLOBALVAR);
				w->Write<long> (g->index);
			} else {
				// Binary operators
				// And only with numbers for now too.
				// TODO: make a proper expression parser!
				MustNumber();
				
				int val = atoi (token.chars());
				w->Write<long> (DH_PUSHNUMBER);
				w->Write<long> (val);
				
				int h =	!oper.compare("=") ? DH_ASSIGNGLOBALVAR :
					!oper.compare("+=") ? DH_ADDGLOBALVAR :
					!oper.compare("-=") ? DH_SUBGLOBALVAR :
					!oper.compare("*=") ? DH_MULGLOBALVAR :
					!oper.compare("/=") ? DH_DIVGLOBALVAR :
					!oper.compare("%=") ? DH_MODGLOBALVAR : -1;
				
				if (h == -1)
					ParserError ("bad operator `%s`!", oper.chars());
				
				w->Write<long> (h);
				w->Write<long> (g->index);
			}
				
			MustNext (";");
			continue;
		}
		
		// Check if it's a command.
		CommandDef* comm = GetCommandByName (token);
		if (comm) { 
			ParseCommand (comm, w);
			continue;
		}
		
		ParserError ("unknown keyword `%s`", token.chars());
	}
	
	if (g_CurMode != MODE_TOPLEVEL)
		ParserError ("script did not end at top level; did you forget a `}`?");
	
	// stateSpawn must be defined!
	if (!g_stateSpawnDefined)
		ParserError ("script must have a state named `stateSpawn`!");
	
	// Dump the last state's onenter and mainloop
	w->WriteBuffers();
	
	// If we added strings here, we need to write a list of them.
	unsigned int stringcount = CountStringTable ();
	if (stringcount) {
		w->Write<long> (DH_STRINGLIST);
		w->Write<long> (stringcount);
		for (unsigned int a = 0; a < stringcount; a++)
			w->WriteString (g_StringTable[a]);
	}
	
	printf ("%u string%s written\n", stringcount, PLURAL (stringcount));
}

void ScriptReader::ParseCommand (CommandDef* comm, ObjWriter* w) {
	// If this was defined at top-level, we stop right at square one!
	if (g_CurMode == MODE_TOPLEVEL)
		ParserError ("no commands allowed at top level!");
	
	MustNext ("(");
	int curarg = 0;
	while (1) {
		if (curarg >= comm->maxargs) {
			if (!PeekNext().compare (","))
				ParserError ("got `,` while expecting command-terminating `)`, are you passing too many parameters? (max %d)",
					comm->maxargs);
			MustNext (")");
			curarg++;
			break;
		}
		
		if (!PeekNext().len())
			ParserError ("unexpected end-of-file, unterminated command");
		
		// If we get a ")" now, the user probably gave too few parameters
		if (!PeekNext().compare (")"))
			ParserError ("unexpected `)`, did you pass too few parameters? (need %d)", comm->numargs);
		
		// Argument may be using a variable
		ScriptVar* g = FindGlobalVariable (PeekNext ());
		if (g && comm->argtypes[curarg] != RETURNVAL_STRING) {
			// Advance cursor past the var name
			Next();
			
			w->Write<long> (DH_PUSHGLOBALVAR);
			w->Write<long> (g->index);
		} else {
			// Check for raw value
			switch (comm->argtypes[curarg]) {
			case RETURNVAL_INT:
				MustNumber();
				w->Write<long> (DH_PUSHNUMBER);
				w->Write<long> (atoi (token.chars ()));
				break;
			case RETURNVAL_BOOLEAN:
				MustBool();
				w->Write<long> (DH_PUSHNUMBER);
				w->Write<long> (BoolValue ());
				break;
			case RETURNVAL_STRING:
				MustString();
				w->Write<long> (DH_PUSHSTRINGINDEX);
				w->Write<long> (PushToStringTable (token.chars()));
				break;
			}
		}
		
		if (curarg < comm->numargs - 1) {
			MustNext (",");
		} else if (curarg < comm->maxargs - 1) {
			// Can continue, but can terminate as well.
			if (!PeekNext ().compare (")")) {
				MustNext (")");
				curarg++;
				break;
			} else
				MustNext (",");
		}
		
		curarg++;
	}
	MustNext (";");
	
	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->maxargs) {
		w->Write<long> (DH_PUSHNUMBER);
		w->Write<long> (comm->defvals[curarg]);
		curarg++;
	}
	
	w->Write<long> (DH_COMMAND);
	w->Write<long> (comm->number);
	w->Write<long> (comm->maxargs);
}