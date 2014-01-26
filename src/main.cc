/*
	Copyright 2012-2014 Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "main.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"
#include "data_buffer.h"
#include "parser.h"
#include "lexer.h"
#include "gitinfo.h"

int main (int argc, char** argv)
{
	try
	{
		// Intepret command-line parameters:
		// -l: list commands
		// I guess there should be a better way to do this.
		if (argc == 2 && !strcmp (argv[1], "-l"))
		{
			printf ("Begin list of commands:\n");
			printf ("------------------------------------------------------\n");

			for (command_info* comm : get_commands())
				print ("%1\n", get_command_signature (comm));

			printf ("------------------------------------------------------\n");
			printf ("End of command list\n");
			exit (0);
		}

		// Print header
		string header;
		string headerline;
		header = format (APPNAME " version %1", get_version_string (e_long_form));

#ifdef DEBUG
		header += " (debug build)";
#endif

		for (int i = 0; i < header.length() / 2; ++i)
			headerline += "-=";

		headerline += '-';
		print ("%2\n\n%1\n\n%2\n\n", header, headerline);

		if (argc < 2)
		{
			fprintf (stderr, "usage: %s <infile> [outfile] # compiles botscript\n", argv[0]);
			fprintf (stderr, "       %s -l                 # lists commands\n", argv[0]);
			exit (1);
		}

		string outfile;

		if (argc < 3)
			outfile = ObjectFileName (argv[1]);
		else
			outfile = argv[2];

		// Prepare reader and writer
		botscript_parser* parser = new botscript_parser;

		// We're set, begin parsing :)
		print ("Parsing script...\n");
		parser->parse_botscript (argv[1]);
		print ("Script parsed successfully.\n");

		// Parse done, print statistics and write to file
		int globalcount = g_GlobalVariables.size();
		int stringcount = num_strings_in_table();
		print ("%1 / %2 strings written\n", stringcount, g_max_stringlist_size);
		print ("%1 / %2 global variables\n", globalcount, g_max_global_vars);
		print ("%1 / %2 events\n", parser->get_num_events(), g_max_events);
		print ("%1 state%s1\n", parser->get_num_states());

		parser->write_to_file (outfile);

		// Clear out the junk
		delete parser;

		// Done!
		exit (0);
	}
	catch (script_error& e)
	{
		fprint (stderr, "error: %1\n", e.what());
	}
}

// ============================================================================
// Utility functions

// ============================================================================
// Does the given file exist?
bool fexists (string path)
{
	if (FILE* test = fopen (path, "r"))
	{
		fclose (test);
		return true;
	}

	return false;
}

// ============================================================================
// Mutates given filename to an object filename
string ObjectFileName (string s)
{
	// Locate the extension and chop it out
	int extdot = s.last (".");

	if (extdot >= s.length() - 4)
		s -= (s.length() - extdot);

	s += ".o";
	return s;
}

// ============================================================================
type_e GetTypeByName (string t)
{
	t = t.to_lowercase();
	return	(t == "int") ? TYPE_INT :
			(t == "str") ? TYPE_STRING :
			(t == "void") ? TYPE_VOID :
			(t == "bool") ? TYPE_BOOL :
			TYPE_UNKNOWN;
}


// ============================================================================
// Inverse operation - type name by value
string GetTypeName (type_e type)
{
	switch (type)
	{
		case TYPE_INT: return "int"; break;
		case TYPE_STRING: return "str"; break;
		case TYPE_VOID: return "void"; break;
		case TYPE_BOOL: return "bool"; break;
		case TYPE_UNKNOWN: return "???"; break;
	}

	return "";
}
// =============================================================================
//

string make_version_string (int major, int minor, int patch)
{
	string ver = format ("%1.%2", major, minor);

	if (patch != 0)
	{
		ver += ".";
		ver += patch;
	}

	return ver;
}

// =============================================================================
//
string get_version_string (form_length_e len)
{
	string tag (GIT_DESCRIPTION);
	string version = tag;

	if (tag.ends_with ("-pre") && len == e_long_form)
		version += "-" + string (GIT_HASH).mid (0, 8);

	return version;
}