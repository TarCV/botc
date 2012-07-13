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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#include "str.h"
#include "scriptreader.h"
#include "objwriter.h"
#include "parser.h"

#include "bots.h"
#include "botcommands.h"

int main (int argc, char** argv) {
	if (argc != 3) {
		fprintf (stderr, "usage: %s: <infile> <outFile>\n", argv[0]);
		exit (1);
	}
	
	// Print header
	str header;
	str headerline = "=-";
	header.appendformat ("%s version %d.%d.%d", APPNAME, VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
	headerline.repeat ((header.len()/2)-1);
	printf ("%s\n%s\n", header.chars(), headerline.chars());
	
	// Prepare reader and writer
	str infile = argv[1];
	str outfile = argv[2];
	ScriptReader *r = new ScriptReader (infile);
	ObjWriter *w = new ObjWriter (outfile);
	
	// We're set, begin parsing :)
	r->BeginParse (w);
	
	// Clear out the junk afterwards
	delete r;
	delete w;
	
	// Print statistics
	printf ("%d states written\n", g_NumStates);
	printf ("%d events written\n", g_NumEvents);
}

void error (const char* text, ...) {
	PERFORM_FORMAT (text, c);
	fprintf (stderr, "error: %s", c);
	exit (1);
}