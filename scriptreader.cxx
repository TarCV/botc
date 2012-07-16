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
#include "string.h"
#include "str.h"
#include "common.h"
#include "scriptreader.h"

static bool IsWhitespace (char c) {
	// These characters are invisible, thus considered whitespace
	if (c <= 32 || c == 127 || c == 255)
		return true;
	
	return false;
}

ScriptReader::ScriptReader (str path) {
	if (!(fp = fopen (path, "r"))) {
		error ("couldn't open %s for reading!\n", path.chars ());
		exit (1);
	}
	
	filepath = path;
	curline = 1;
	curchar = 1;
	pos = 0;
	token = "";
	atnewline = false;
	commentmode = 0;
}

ScriptReader::~ScriptReader () {
	FinalChecks ();
	fclose (fp);
}

char ScriptReader::ReadChar () {
	char* c = (char*)malloc (sizeof (char));
	if (!fread (c, sizeof (char), 1, fp))
		return 0;
	
	// We're at a newline, thus next char read will begin the next line
	if (atnewline) {
		atnewline = false;
		curline++;
		curchar = 0; // gets incremented to 1
	}
	
	if (c[0] == '\n')
		atnewline = true;
	
	curchar++;
	return c[0];
}

char ScriptReader::PeekChar (int offset) {
	// Store current position
	long curpos = ftell (fp);
	
	// Forward by offset
	fseek (fp, offset, SEEK_CUR);
	
	// Read the character
	char* c = (char*)malloc (sizeof (char));
	if (!fread (c, sizeof (char), 1, fp))
		return 0;
	
	// Rewind back
	fseek (fp, curpos, SEEK_SET);
	
	return c[0];
}

// true if was found, false if not.
bool ScriptReader::Next () {
	str tmp = "";
	
	while (!feof (fp)) {
		// Check if the next token possibly starts a comment.
		if (PeekChar () == '/' && !tmp.len()) {
			char c2 = PeekChar (1);
			// C++-style comment
			if (c2 == '/')
				commentmode = 1;
			else if (c2 == '*')
				commentmode = 2;
			
			// We don't need to actually read in the
			// comment characters, since they will get
			// ignored due to comment mode anyway.
		}
		
		c = ReadChar ();
		
		// If this is a comment we're reading, check if this character
		// gets the comment terminated, otherwise ignore it.
		if (commentmode > 0) {
			if (commentmode == 1 && c == '\n') {
				// C++-style comments are terminated by a newline
				commentmode = 0;
				continue;
			} else if (commentmode == 2 && c == '*') {
				// C-style comments are terminated by a `*/`
				if (PeekChar() == '/') {
					commentmode = 0;
					// Now the char has to be read in since we
					// no longer are reading a comment
					ReadChar ();
				}
			}
			
			// Otherwise, ignore it.
			continue;
		}
		
		// Non-alphanumber characters (sans underscore) break the word too.
		// If there was prior data, the delimeter pushes the cursor back so
		// that the next character will be the same delimeter. If there isn't,
		// the delimeter itself is included (and thus becomes a token itself.)
		if ((c >= 33 && c <= 47) ||
			(c >= 58 && c <= 64) ||
			(c >= 91 && c <= 96 && c != '_') ||
			(c >= 123 && c <= 126)) {
			if (tmp.len())
				fseek (fp, ftell (fp) - 1, SEEK_SET);
			else
				tmp += c;
			break;
		}
		
		if (IsWhitespace (c)) {
			// Don't break if we haven't gathered anything yet.
			if (tmp.len())
				break;
		} else {
			tmp += c;
		}
	}
	
	// If we got nothing here, read failed. This should
	// only hapen in the case of EOF.
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
			ParserError ("expected `%s`, reached end of file instead\n", c);
		else
			ParserError ("expected a token, reached end of file instead\n");
	}
	
	if (strlen (c) && token.compare (c) != 0) {
		ParserError ("expected `%s`, got `%s` instead", c, token.chars());
	}
}

void ScriptReader::ParserError (const char* message, ...) {
	PERFORM_FORMAT (message, outmessage);
	ParserMessage ("\nParse error\n", outmessage);
	exit (1);
}

void ScriptReader::ParserWarning (const char* message, ...) {
	PERFORM_FORMAT (message, outmessage);
	ParserMessage ("Warning: ", outmessage);
}

void ScriptReader::ParserMessage (const char* header, char* message) {
	fprintf (stderr, "%sIn file %s, at line %u, col %u: %s\n",
		header, filepath.chars(), curline, curchar, message);
}

void ScriptReader::MustString () {
	MustNext ("\"");
	
	str string;
	// Keep reading characters until we find a terminating quote.
	while (1) {
		// can't end here!
		if (feof (fp))
			ParserError ("unterminated string");
		
		char c = ReadChar ();
		if (c == '"')
			break;
		
		string += c;
	}
	
	token = string;
}

void ScriptReader::MustNumber () {
	MustNext ();
	if (!token.isnumber())
		ParserError ("expected a number, got `%s`", token.chars());
}

void ScriptReader::MustBool () {
	MustNext();
	if (!token.compare ("0") || !token.compare ("1") ||
	    !token.compare ("true") || !token.compare ("false") ||
	    !token.compare ("yes") || !token.compare ("no")) {
			return;
	}
	
	ParserError ("expected a boolean value, got `%s`", token.chars());
}

bool ScriptReader::BoolValue () {
	return (!token.compare ("1") || !token.compare ("true") || !token.compare ("yes"));
}

void ScriptReader::MustValue (int type) {
	switch (type) {
	case RETURNVAL_INT: MustNumber (); break;
	case RETURNVAL_STRING: MustString (); break;
	case RETURNVAL_BOOLEAN: MustBool (); break;
	}
}

// Checks to be performed at the end of file
void ScriptReader::FinalChecks () {
	// If comment mode is 2 by the time the file ended, the
	// comment was left unterminated. 1 is no problem, since
	// it's terminated by newlines anyway.
	if (commentmode == 2)
		ParserError ("unterminated `/*`-style comment");
}