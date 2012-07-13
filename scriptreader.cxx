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
#include "string.h"
#include "str.h"
#include "common.h"
#include "scriptreader.h"

bool IsWhitespace (char c) {
	// These characters are invisible, thus considered whitespace
	if (c <= 32 || c == 127 || c == 255)
		return true;
	
	return false;
}

ScriptReader::ScriptReader (str path) {
	fp = fopen (path, "r");
	CHECK_FILE (fp, path, "reading");
	
	curline = 1;
	pos = 0;
	token = "";
}

ScriptReader::~ScriptReader () {
	fclose (fp);
}

char ScriptReader::ReadChar () {
	char* c = (char*)malloc (sizeof (char));
	if (!fread (c, sizeof (char), 1, fp))
		return 0;
	
	// If we just read a new line, increase current line number counter.
	if (c[0] == '\n')
		curline++;
	
	return c[0];
}

// true if was found, false if not.
bool ScriptReader::Next () {
	str tmp = "";
	bool quote = false;
	tokenquoted = false;
	while (!feof (fp)) {
		char c = ReadChar ();
		
		// Quotation mark.
		if (c == '"') {
			// End-quote ends the token.
			if (quote)
				break;
			
			tokenquoted = true;
			quote = !quote;
			continue;
		}
		
		if (IsWhitespace (c) && !quote) {
			// Don't break if we haven't gathered anything yet.
			if (tmp.len())
				break;
		} else {
			tmp += c;
		}
	}
	
	// If we got nothing here, read failed.
	if (!tmp.len()) {
		token = "";
		return false;
	}
	
	pos++;
	token = tmp;
	return true;
}

// Returns the next token without advancing the cursor.
str ScriptReader::PeekNext () {
	// Store current position
	int cpos = ftell (fp);
	
	// Advance on the token.
	if (!Next ())
		return "";
	
	str tmp = token;
	
	// Restore position
	fseek (fp, cpos, SEEK_SET);
	pos--;
	
	return tmp;
}

void ScriptReader::Seek (unsigned int n, int origin) {
	switch (origin) {
	case SEEK_SET:
		fseek (fp, 0, SEEK_SET);
		pos = 0;
		break;
	case SEEK_CUR:
		break;
	case SEEK_END:
		printf ("ScriptReader::Seek: SEEK_END not yet supported.\n");
		break;
	}
	
	for (unsigned int i = 0; i < n+1; i++)
		Next();
}

void ScriptReader::MustNext (const char* c) {
	if (!Next()) {
		if (strlen (c))
			ParseError ("expected `%s`, reached end of file instead\n", c);
		else
			ParseError ("expected a token, reached end of file instead\n");
	}
	
	if (strlen (c) && token.compare (c) != 0) {
		ParseError ("expected `%s`, got `%s` instead", c, token.chars());
	}
}

void ScriptReader::ParseError (const char* message, ...) {
	PERFORM_FORMAT (message, outmessage);
	error ("Parse error on line %d:\n%s\n", curline, outmessage);
}