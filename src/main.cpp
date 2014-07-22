/*
	Copyright 2012-2014 Teemu Piippo
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
#include "stringTable.h"
#include "dataBuffer.h"
#include "parser.h"
#include "lexer.h"
#include "hginfo.h"

int main (int argc, char** argv)
{
	try
	{
		// Intepret command-line parameters:
		// -l: list commands
		// I guess there should be a better way to do this.
		if (argc == 2 and String (argv[1]) == "-l")
		{
			BotscriptParser parser;
			parser.setReadOnly (true);
			parser.parseBotscript ("botc_defs.bts");

			for (CommandInfo* comm : getCommands())
				print ("%1\n", comm->signature());

			exit (0);
		}

		if (argc == 2 and String (argv[1]) == "-v")
		{
			// Print header
			String header;
			header = format (APPNAME " %1", versionString (true));

#ifdef DEBUG
			header += " (debug build)";
#endif

			print ("%1\n", header);
			exit (0);
		}

		if (argc < 2)
		{
			fprintf (stderr, APPNAME " %s\n", versionString (false).c_str());
			fprintf (stderr, "usage: %s <infile> [outfile] # compiles botscript\n", argv[0]);
			fprintf (stderr, "       %s -l                 # lists commands\n", argv[0]);
			fprintf (stderr, "       %s -v                 # displays version info\n", argv[0]);
			exit (1);
		}

		String outfile;

		if (argc < 3)
			outfile = makeObjectFileName (argv[1]);
		else
			outfile = argv[2];

		// Prepare reader and writer
		BotscriptParser* parser = new BotscriptParser;

		// We're set, begin parsing :)
		print ("Parsing script...\n");
		parser->parseBotscript (argv[1]);
		print ("Script parsed successfully.\n");

		// Parse done, print statistics and write to file
		int globalcount = parser->getHighestVarIndex (true) + 1;
		int statelocalcount = parser->getHighestVarIndex (false) + 1;
		int stringcount = countStringsInTable();
		print ("%1 / %2 strings\n", stringcount, Limits::MaxStringlistSize);
		print ("%1 / %2 global variable indices\n", globalcount, Limits::MaxGlobalVars);
		print ("%1 / %2 state variable indices\n", statelocalcount, Limits::MaxStateVars);
		print ("%1 / %2 events\n", parser->numEvents(), Limits::MaxEvents);
		print ("%1 state%s1\n", parser->numStates());

		parser->writeToFile (outfile);
		delete parser;
		return 0;
	}
	catch (std::exception& e)
	{
		fprintf (stderr, "error: %s\n", e.what());
		return 1;
	}
}

// ============================================================================
//
// Mutates given filename to an object filename
//
String makeObjectFileName (String s)
{
	// Locate the extension and chop it out
	int extdot = s.lastIndexOf (".");

	if (extdot >= s.length() - 4)
		s -= (s.length() - extdot);

	s += ".o";
	return s;
}

// ============================================================================
//
DataType getTypeByName (String token)
{
	token = token.toLowercase();
	return	(token == "int") ? TYPE_Int
		  : (token == "str") ? TYPE_String
		  : (token == "void") ? TYPE_Void
		  : (token == "bool") ? TYPE_Bool
		  : TYPE_Unknown;
}


// ============================================================================
//
// Inverse operation - type name by value
//
String dataTypeName (DataType type)
{
	switch (type)
	{
		case TYPE_Int: return "int"; break;
		case TYPE_String: return "str"; break;
		case TYPE_Void: return "void"; break;
		case TYPE_Bool: return "bool"; break;
		case TYPE_Unknown: return "???"; break;
	}

	return "";
}

// =============================================================================
//
String makeVersionString (int major, int minor, int patch)
{
	String ver = String::fromNumber (major);
	ver += "." + String::fromNumber (minor);

	if (patch != 0)
		ver += "." + patch;

	return ver;
}

// =============================================================================
//
String versionString (bool longform)
{
	String result = makeVersionString (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

#ifdef SVN_REVISION_STRING
	if (longform)
		result += "-" SVN_REVISION_STRING;
#else
	(void) longform; // shuts up GCC
#endif

	return result;
}
