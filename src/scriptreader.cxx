#include "main.h"
#include "scriptreader.h"

#define STORE_POSITION \
	bool stored_atnewline = atnewline; \
	unsigned int stored_curline = curline[fc]; \
	unsigned int stored_curchar = curchar[fc];

#define RESTORE_POSITION \
	atnewline = stored_atnewline; \
	curline[fc] = stored_curline; \
	curchar[fc] = stored_curchar;

// ============================================================================
ScriptReader::ScriptReader (string path) {
	token = "";
	prevtoken = "";
	prevpos = 0;
	fc = -1;
	
	for (unsigned int u = 0; u < MAX_FILESTACK; u++)
		fp[u] = null;
	
	OpenFile (path);
	commentmode = 0;
}

// ============================================================================
ScriptReader::~ScriptReader () {
	// If comment mode is 2 by the time the file ended, the
	// comment was left unterminated. 1 is no problem, since
	// it's terminated by newlines anyway.
	if (commentmode == 2)
		ParserError ("unterminated `/*`-style comment");
	
	for (unsigned int u = 0; u < MAX_FILESTACK; u++) {
		if (fp[u]) {
			ParserWarning ("file idx %u remained open after parsing", u);
			CloseFile (u);
		}
	}
}

// ============================================================================
// Opens a file and pushes its pointer to stack
void ScriptReader::OpenFile (string path) {
	if (fc+1 >= MAX_FILESTACK) 
		ParserError ("supposed to open file `%s` but file stack is full! do you have recursive `#include` directives?",
			path.chars());
	
	// Save the position first.
	if (fc != -1) {
		savedpos[fc] = ftell (fp[fc]);
	}
	
	fc++;
	
	fp[fc] = fopen (path, "r");
	if (!fp[fc]) {
		ParserError ("couldn't open %s for reading!\n", path.chars ());
		exit (1);
	}
	
	fseek (fp[fc], 0, SEEK_SET);
	filepath[fc] = path.chars();
	curline[fc] = 1;
	curchar[fc] = 1;
	pos[fc] = 0;
	atnewline = 0;
}

// ============================================================================
// Closes the current file
void ScriptReader::CloseFile (unsigned int u) {
	if (u >= MAX_FILESTACK)
		u = fc;
	
	if (!fp[u])
		return;
	
	fclose (fp[u]);
	fp[u] = null;
	fc--;
	
	if (fc != -1)
		fseek (fp[fc], savedpos[fc], SEEK_SET);
}

// ============================================================================
char ScriptReader::ReadChar () {
	if (feof (fp[fc]))
		return 0;
	
	char c;
	if (!fread (&c, 1, 1, fp[fc]))
		return 0;
	
	// We're at a newline, thus next char read will begin the next line
	if (atnewline) {
		atnewline = false;
		curline[fc]++;
		curchar[fc] = 0; // gets incremented to 1
	}
	
	if (c == '\n') {
		atnewline = true;
		
		// Check for pre-processor directives
		PreprocessDirectives ();
	}
	
	curchar[fc]++;
	return c;
}

// ============================================================================
// Peeks the next character
char ScriptReader::PeekChar (int offset) {
	// Store current position
	long curpos = ftell (fp[fc]);
	STORE_POSITION
	
	// Forward by offset
	fseek (fp[fc], offset, SEEK_CUR);
	
	// Read the character
	char* c = (char*)malloc (sizeof (char));
	
	if (!fread (c, sizeof (char), 1, fp[fc])) {
		fseek (fp[fc], curpos, SEEK_SET);
		return 0;
	}
	
	// Rewind back
	fseek (fp[fc], curpos, SEEK_SET);
	RESTORE_POSITION
	
	return c[0];
}

// ============================================================================
// Read a token from the file buffer. Returns true if token was found, false if not.
bool ScriptReader::Next (bool peek) {
	prevpos = ftell (fp[fc]);
	string tmp = "";
	
	while (1) {
		// Check end-of-file
		if (feof (fp[fc])) {
			// If we're just peeking, we shouldn't
			// actually close anything.. 
			if (peek)
				break;
			
			CloseFile ();
			if (fc == -1)
				break;
		}
		
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
				fseek (fp[fc], ftell (fp[fc]) - 1, SEEK_SET);
			else
				tmp += c;
			break;
		}
		
		if (isspace (c)) {
			// Don't break if we haven't gathered anything yet.
			if (tmp.len())
				break;
		} else {
			tmp += c;
		}
	}
	
	// If we got nothing here, read failed. This should
	// only happen in the case of EOF.
	if (!tmp.len()) {
		token = "";
		return false;
	}
	
	pos[fc]++;
	prevtoken = token;
	token = tmp;
	return true;
}

// ============================================================================
// Returns the next token without advancing the cursor.
string ScriptReader::PeekNext (int offset) {
	// Store current information
	string storedtoken = token;
	int cpos = ftell (fp[fc]);
	STORE_POSITION
	
	// Advance on the token.
	while (offset >= 0) {
		if (!Next (true))
			return "";
		offset--;
	}
	
	string tmp = token;
	
	// Restore position
	fseek (fp[fc], cpos, SEEK_SET);
	pos[fc]--;
	token = storedtoken;
	RESTORE_POSITION
	return tmp;
}

// ============================================================================
void ScriptReader::Seek (unsigned int n, int origin) {
	switch (origin) {
	case SEEK_SET:
		fseek (fp[fc], 0, SEEK_SET);
		pos[fc] = 0;
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

// ============================================================================
void ScriptReader::MustNext (const char* c) {
	if (!Next()) {
		if (strlen (c))
			ParserError ("expected `%s`, reached end of file instead\n", c);
		else
			ParserError ("expected a token, reached end of file instead\n");
	}
	
	if (strlen (c))
		MustThis (c);
}

// ============================================================================
void ScriptReader::MustThis (const char* c) {
	if (token != c)
		ParserError ("expected `%s`, got `%s` instead", c, token.chars());
}

// ============================================================================
void ScriptReader::ParserError (const char* message, ...) {
	char buf[512];
	va_list va;
	va_start (va, message);
	sprintf (buf, message, va);
	va_end (va);
	ParserMessage ("\nError: ", buf);
	exit (1);
}

// ============================================================================
void ScriptReader::ParserWarning (const char* message, ...) {
	char buf[512];
	va_list va;
	va_start (va, message);
	sprintf (buf, message, va);
	va_end (va);
	ParserMessage ("Warning: ", buf);
}

// ============================================================================
void ScriptReader::ParserMessage (const char* header, string message) {
	if (fc >= 0 && fc < MAX_FILESTACK)
		fprintf (stderr, "%s%s:%u:%u: %s\n",
			header, filepath[fc].c_str(), curline[fc], curchar[fc], message.c_str());
	else
		fprintf (stderr, "%s%s\n", header, message.c_str());
}

// ============================================================================
// if gotquote == 1, the current token already holds the quotation mark.
void ScriptReader::MustString (bool gotquote) {
	if (gotquote)
		MustThis ("\"");
	else
		MustNext ("\"");
	
	string string;
	// Keep reading characters until we find a terminating quote.
	while (1) {
		// can't end here!
		if (feof (fp[fc]))
			ParserError ("unterminated string");
		
		char c = ReadChar ();
		if (c == '"')
			break;
		
		string += c;
	}
	
	token = string;
}

// ============================================================================
void ScriptReader::MustNumber (bool fromthis) {
	if (!fromthis)
		MustNext ();
	
	string num = token;
	if (num == "-") {
		MustNext ();
		num += token;
	}
	
	// "true" and "false" are valid numbers
	if (-token == "true")
		token = "1";
	else if (-token == "false")
		token = "0";
	else {
		bool ok;
		int num = token.to_long (&ok);

		if (!ok)
			ParserError ("expected a number, got `%s`", token.chars());

		token = num;
	}
}