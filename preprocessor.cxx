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
#include "scriptreader.h"

/* Since the preprocessor is *called* from ReadChar and I don't want
 * to worry about recursive preprocessing, the preprocessor uses its
 * own bare-bones variant of the function for file reading.
 */
char ScriptReader::PPReadChar () {
	char* c = (char*)malloc (sizeof (char));
	if (!fread (c, sizeof (char), 1, fp[fc]))
		return 0;
	curchar[fc]++;
	return c[0];
}

void ScriptReader::PPMustChar (char c) {
	char d = PPReadChar ();
	if (c != d) {
		ParserError ("expected `%c`, got `%d`", c, d);
	}
}

// ============================================================================
// Reads a word until whitespace
str ScriptReader::PPReadWord (char &term) {
	str word;
	while (1) {
		char c = PPReadChar();
		if (feof (fp[fc]) || (IsCharWhitespace (c) && word.len ())) {
			term = c;
			break;
		}
		word += c;
	}
	return word;
}

// ============================================================================
// Preprocess any directives found in the script file
void ScriptReader::PreprocessDirectives () {
	size_t spos = ftell (fp[fc]);
	if (!DoDirectivePreprocessing ())
		fseek (fp[fc], spos, SEEK_SET);
}

/* ============================================================================
 * Returns true if the pre-processing was successful, false if not.
 * If pre-processing was successful, the file pointer remains where
 * it was, if not, it's pushed back to where it was before preprocessing
 * took place and is parsed normally.
 */
bool ScriptReader::DoDirectivePreprocessing () {
	char trash;
	// Directives start with a pound sign
	if (PPReadChar() != '#')
		return false;
	
	// Read characters until next whitespace to
	// build the name of the directive
	str directive = PPReadWord (trash);
	
	// Now check the directive name against known names
	if (!directive.icompare ("include")) {
		// #include-directive
		char terminator;
		str file = PPReadWord (terminator);
		
		if (!file.len())
			ParserError ("expected file name for #include, got nothing instead");
		OpenFile (file);
		return true;
	}
	
	ParserError ("unknown directive `#%s`!", directive.chars());
	return false;
}