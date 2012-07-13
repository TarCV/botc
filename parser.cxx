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
 *	3. Neither the name of the Skulltag Development Team nor the names of its
 *	   contributors may be used to endorse or promote products derived from this
 *	   software without specific prior written permission.
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

#define TOKEN_CHARS token.chars()
#define TOKEN_IS(s) !token.compare (s)
#define MUST_TOPLEVEL if (g_CurMode != MODE_TOPLEVEL) \
	ParseError ("%ss may only be defined at top level!", token.chars());

int g_NumStates = 0;
int g_NumEvents = 0;
int g_CurMode = MODE_TOPLEVEL;
str g_CurState = "";

void ScriptReader::BeginParse (ObjWriter* w) {
	while (Next()) {
		// printf ("got token %s\n", token.chars());
		if (TOKEN_IS ("#include")) {
			MustNext ();
			// First ensure that the file can be opened
			FILE* newfile = fopen (token.chars(), "r");
			if (!newfile)
				ParseError ("couldn't open included file `%s`!", token.chars());
			fclose (newfile);
			ScriptReader* newreader = new ScriptReader (token.chars());
			newreader->BeginParse (w);
		} else if (TOKEN_IS ("state")) {
			MUST_TOPLEVEL
			
			MustNext ();
			
			str statename = token;
			
			// State name must be a word.
			if (statename.first (" ") != statename.len())
				ParseError ("state name must be a single word! got `%s`", (char*)statename);
			
			// Must end in a colon
			MustNext (":");
			
			w->Write (DH_STATENAME);
			w->Write (statename.len());
			w->WriteString (statename);
			w->Write (DH_STATEIDX);
			w->Write (g_NumStates);
			
			g_NumStates++;
			g_CurState = statename;
		} else if (TOKEN_IS ("event")) {
			MUST_TOPLEVEL
			
			// Event definition
			MustNext ();
			
			EventDef* e = FindEventByName (token);
			if (!e)
				ParseError ("bad event! got `%s`\n", token.chars());
			
			MustNext ("{");
			
			g_CurMode = MODE_EVENT;
			
			w->Write (DH_EVENT);
			w->Write<long> (e->number);
			g_NumEvents++;
		} else if (TOKEN_IS ("}")) {
			// Closing brace..
			switch (g_CurMode) {
			case MODE_EVENT:
				// Brace closes event.
				w->Write (DH_ENDEVENT);
				g_CurMode = MODE_TOPLEVEL;
				break;
			default:
				ParseError ("unexpected `}`");
			}
		} else {
			ParseError ("unknown keyword `%s`!", TOKEN_CHARS);
		}
	}
	
	if (g_CurMode != MODE_TOPLEVEL)
		ParseError ("script did not end at top level! did you forget a `}`?\n");
	
	/*
	// State
	w->WriteState ("stateSpawn");
	
	w->Write (DH_ONENTER);
	w->Write (DH_ENDONENTER);
	
	w->Write (DH_MAINLOOP);
	w->Write (DH_ENDMAINLOOP);
	
	w->Write (DH_EVENT);
	w->Write (BOTEVENT_PLAYER_FIREDSSG);
	w->Write (DH_COMMAND);
	w->Write (BOTCMD_BEGINJUMPING);
	w->Write (0);
	w->Write (DH_ENDEVENT);
	*/
}
