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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#include "str.h"
#include "scriptreader.h"
#include "objwriter.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"

#include "bots.h"
#include "botcommands.h"

bool fexists (char* path) {
	if (FILE* test = fopen (path, "r")) {
		fclose (test);
		return true;
	}
	return false;
}

int main (int argc, char** argv) {
	// Intepret command-line parameters:
	// -l: list commands
	if (argc == 2 && !strcmp (argv[1], "-l")) {
		ReadCommands ();
		printf ("Begin list of commands:\n");
		printf ("------------------------------------------------------\n");
		
		CommandDef* comm;
		ITERATE_COMMANDS (comm)
			printf ("%s\n", GetCommandPrototype (comm).chars());
		
		printf ("------------------------------------------------------\n");
		printf ("End of command list\n");
		exit (0);
	}
	
	// Print header
	str header;
	str headerline = "-=";
	header.appendformat ("%s version %d.%d.%d", APPNAME, VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
	headerline.repeat ((header.len()/2)-1);
	headerline += '-';
	printf ("%s\n%s\n", header.chars(), headerline.chars());
	
	if (argc < 2) {
		fprintf (stderr, "usage: %s: <infile> [outfile]\n", argv[0]);
		exit (1);
	}
	
	str outfile;
	if (argc < 3)
		outfile = ObjectFileName (argv[1]);
	else
		outfile = argv[2];
	
	// If we'd end up writing into an existing file,
	// ask the user if we want to overwrite it
	if (fexists (outfile)) {
		// Additional warning if the paths are the same
		str warning;
#ifdef FILE_CASEINSENSITIVE
		if (!outfile.icompare (argv[1]))
#else
		if (!outfile.compare (argv[1]))
#endif
		{
			warning = "\nWARNING: Output file is the same as the input file. ";
			warning += "Answering yes here will destroy the source!\n";
			warning += "Continue nevertheless?";
		}
		printf ("output file `%s` already exists! overwrite?%s (y/n) ", outfile.chars(), warning.chars());
		
		char ans;
		fgets (&ans, 2, stdin);
		if (ans != 'y') {
			printf ("abort\n");
			exit (1);
		}
	}
	
	// Read definitions
	printf ("Reading definitions...\n");
	ReadEvents ();
	ReadCommands ();
	
	// Init stuff
	InitStringTable ();
	InitVariables ();
	
	// Prepare reader and writer
	ScriptReader *r = new ScriptReader (argv[1]);
	ObjWriter *w = new ObjWriter (outfile);
	
	// We're set, begin parsing :)
	printf ("Parsing script..\n");
	r->BeginParse (w);
	
	// Parse done, print statistics and write to file
	unsigned int globalcount = CountGlobalVars ();
	printf ("%u global variable%s\n", globalcount, PLURAL (globalcount));
	printf ("%d state%s written\n", g_NumStates, PLURAL (g_NumStates));
	printf ("%d event%s written\n", g_NumEvents, PLURAL (g_NumEvents));
	w->WriteToFile ();
	
	// Clear out the junk
	printf ("clear r\n");
	delete r;
	printf ("clear w\n");
	delete w;
	printf ("done!\n");
}

void error (const char* text, ...) {
	PERFORM_FORMAT (text, c);
	fprintf (stderr, "error: %s", c);
	exit (1);
}

char* ObjectFileName (str s) {
	// Locate the extension and chop it out
	unsigned int extdot = s.last (".");
	if (extdot >= s.len()-4)
		s.trim (s.len() - extdot);
	
	s += ".o";
	return s.chars();
}