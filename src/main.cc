/*
	Copyright (c) 2012-2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "main.h"
#include "scriptreader.h"
#include "object_writer.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"
#include "data_buffer.h"
#include "object_writer.h"
#include "parser.h"
#include "lexer.h"

// List of keywords
const string_list g_Keywords =
{
	"bool",
	"break",
	"case",
	"continue",
	"const",
	"default",
	"do",
	"else",
	"event",
	"for",
	"goto",
	"if",
	"int",
	"mainloop",
	"onenter",
	"onexit",
	"state",
	"switch",
	"str"
	"void",
	"while",

	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	"enum", // Would enum actually be useful? I think so.
	"func", // Would function support need external support from zandronum?
	"return",
};

// databuffer global variable
int g_NextMark = 0;

int main (int argc, char** argv)
{
	try
	{
		// Intepret command-line parameters:
		// -l: list commands
		// I guess there should be a better way to do this.
		if (argc == 2 && !strcmp (argv[1], "-l"))
		{
			command_info* comm;
			init_commands();
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
		header = format ("%1 version %2.%3", APPNAME, VERSION_MAJOR, VERSION_MINOR);

		for (int i = 0; i < (header.len() / 2) - 1; ++i)
			headerline += "-=";

		headerline += '-';
		print ("%1\n%2\n", header, headerline);

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

		// If we'd end up writing into an existing file,
		// ask the user if we want to overwrite it
		if (fexists (outfile))
		{
			// Additional warning if the paths are the same
			string warning;
#ifdef FILE_CASEINSENSITIVE

			if (+outfile == +string (argv[1]))
#else
			if (outfile == argv[1])
#endif
			{
				warning = "\nWARNING: Output file is the same as the input file. ";
				warning += "Answering yes here will destroy the source!\n";
				warning += "Continue nevertheless?";
			}

			printf ("output file `%s` already exists! overwrite?%s (y/n) ", outfile.chars(), warning.chars());

			char ans;
			fgets (&ans, 1, stdin);

			if (ans != 'y')
			{
				printf ("abort\n");
				exit (1);
			}
		}

		// Read definitions
		printf ("Reading definitions...\n");
		init_events();
		init_commands();

		// Prepare reader and writer
		botscript_parser* r = new botscript_parser;
		object_writer* w = new object_writer;

		// We're set, begin parsing :)
		printf ("Parsing script...\n");
		r->parse_botscript (argv[1], w);
		printf ("Script parsed successfully.\n");

		// Parse done, print statistics and write to file
		int globalcount = g_GlobalVariables.size();
		int stringcount = num_strings_in_table();
		int NumMarks = w->MainBuffer->count_marks();
		int NumRefs = w->MainBuffer->count_references();
		print ("%1 / %2 strings written\n", stringcount, g_max_stringlist_size);
		print ("%1 / %2 global variables\n", globalcount, g_max_global_vars);
		print ("%1 / %2 bytecode marks\n", NumMarks, MAX_MARKS); // TODO: nuke
		print ("%1 / %2 bytecode references\n", NumRefs, MAX_MARKS); // TODO: nuke
		print ("%1 / %2 events\n", g_NumEvents, g_max_events);
		print ("%1 state%s1\n", g_NumStates);

		w->write_to_file (outfile);

		// Clear out the junk
		delete r;
		delete w;

		// Done!
		exit (0);
	}
	catch (script_error& e)
	{
		lexer* lx = lexer::get_main_lexer();
		string fileinfo;

		if (lx != null && lx->has_valid_token())
		{
			lexer::token* tk = lx->get_token();
			fileinfo = format ("%1:%2:%3: ", tk->file, tk->line, tk->column);
		}

		fprint (stderr, "%1error: %2\n", fileinfo, e.what());
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

	if (extdot >= s.len() - 4)
		s -= (s.len() - extdot);

	s += ".o";
	return s;
}

// ============================================================================
// Is the given argument a reserved keyword?
bool IsKeyword (string s)
{
	for (int u = 0; u < NumKeywords(); u++)
		if (s.to_uppercase() == g_Keywords[u].to_uppercase())
			return true;

	return false;
}

int NumKeywords()
{
	return sizeof (g_Keywords) / sizeof (const char*);
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
