#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "str.h"
#include "scriptreader.h"

/* Since the preprocessor is *called* from ReadChar and I don't want
 * to worry about recursive preprocessing, the preprocessor uses its
 * own bare-bones variant of the function for file reading.
 */
char ScriptReader::PPReadChar () {
	char c;
	if (!fread (&c, sizeof (char), 1, fp[fc]))
		return 0;
	curchar[fc]++;
	return c;
}

void ScriptReader::PPMustChar (char c) {
	char d = PPReadChar ();
	if (c != d)
		ParserError ("expected `%c`, got `%d`", c, d);
}

// ============================================================================
// Reads a word until whitespace
string ScriptReader::PPReadWord (char &term) {
	string word;
	while (1) {
		char c = PPReadChar();
		if (feof (fp[fc]) || (isspace (c) && word.len ())) {
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
	string directive = PPReadWord (trash);
	
	// Now check the directive name against known names
	if (directive == "include") {
		// #include-directive
		char terminator;
		string file = PPReadWord (terminator);
		
		if (!file.len())
			ParserError ("expected file name for #include, got nothing instead");
		OpenFile (file);
		return true;
	} else if (directive == "neurosphere") {
		// #neurosphere - activates neurosphere compatibility, aka stuff
		// that is still WIP and what main zandronum does not yet support.
		// Most end users should never need this.
		g_Neurosphere = true;
		return true;
	}
	
	ParserError ("unknown directive `#%s`!", directive.chars());
	return false;
}