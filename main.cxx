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

#define __MAIN_CXX__

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
#include "array.h"

#include "bots.h"
#include "botcommands.h"

// List of keywords
const char* g_Keywords[] = {
	"break",
	"case",
	"continue",
	"default",
	"do",
	"else",
	"event",
	"for",
	"goto",
	"if",
	"mainloop",
	"onenter",
	"onexit",
	"state",
	"switch",
	"var"
	"while",
	
	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	"enum", // Would enum actually be useful? I think so.
	"func", // Would function support need external support from zandronum?
	"return",
};

// databuffer global variable
int g_NextMark = 0;

int main (int argc, char** argv) {
	// Intepret command-line parameters:
	// -l: list commands
	// I guess there should be a better way to do this.
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
	header.appendformat ("%s version %d.%d", APPNAME, VERSION_MAJOR, VERSION_MINOR);
	
	// Include revision if non-zero
	if (VERSION_REVISION)
		header.appendformat (".%d", VERSION_REVISION);
	
	headerline *= (header.len() / 2) - 1;
	headerline += '-';
	printf ("%s\n%s\n", header.chars(), headerline.chars());
	
	if (argc < 2) {
		fprintf (stderr, "usage: %s <infile> [outfile] # compiles botscript\n", argv[0]);
		fprintf (stderr, "       %s -l                 # lists commands\n", argv[0]);
		exit (1);
	}
	
	// A word should always be exactly 4 bytes. The above list command
	// doesn't need it, but the rest of the program does.
	if (sizeof (word) != 4)
		error ("%s expects a word (uint32_t) to be 4 bytes in size, is %d\n",
			APPNAME, sizeof (word));
	
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
	
	// Prepare reader and writer
	ScriptReader* r = new ScriptReader (argv[1]);
	ObjWriter* w = new ObjWriter (outfile);
	
	// We're set, begin parsing :)
	printf ("Parsing script...\n");
	r->ParseBotScript (w);
	printf ("Script parsed successfully.\n");
	
	// Parse done, print statistics and write to file
	unsigned int globalcount = g_GlobalVariables.size();
	unsigned int stringcount = CountStringTable ();
	int NumMarks = w->MainBuffer->CountMarks ();
	int NumRefs = w->MainBuffer->CountReferences ();
	printf ("%u / %u strings written\n", stringcount, MAX_LIST_STRINGS);
	printf ("%u / %u global variables\n", globalcount, MAX_SCRIPT_VARIABLES);
	printf ("%d / %d bytecode marks\n", NumMarks, MAX_MARKS);
	printf ("%d / %d bytecode references\n", NumRefs, MAX_MARKS);
	printf ("%d / %d events\n", g_NumEvents, MAX_NUM_EVENTS);
	printf ("%d state%s\n", g_NumStates, PLURAL (g_NumStates));
	
	w->WriteToFile ();
	
	// Clear out the junk
	delete r;
	delete w;
	
	// Done!
	exit (0);
}

// ============================================================================
// Utility functions

// ============================================================================
// Does the given file exist?
bool fexists (char* path) {
	if (FILE* test = fopen (path, "r")) {
		fclose (test);
		return true;
	}
	return false;
}

// ============================================================================
// Generic error
void error (const char* text, ...) {
	PERFORM_FORMAT (text, c);
	fprintf (stderr, "error: %s", c);
	exit (1);
}

// ============================================================================
// Mutates given filename to an object filename
char* ObjectFileName (str s) {
	// Locate the extension and chop it out
	unsigned int extdot = s.last (".");
	if (extdot >= s.len()-4)
		s -= (s.len() - extdot);
	
	s += ".o";
	return s.chars();
}

// ============================================================================
// Is the given argument a reserved keyword?
bool IsKeyword (str s) {
	for (unsigned int u = 0; u < NumKeywords (); u++)
		if (!s.icompare (g_Keywords[u]))
			return true;
	return false;
}

unsigned int NumKeywords () {
	return sizeof (g_Keywords) / sizeof (const char*);
}