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

#include "Main.h"
#include "Events.h"
#include "Commands.h"
#include "StringTable.h"
#include "DataBuffer.h"
#include "Parser.h"
#include "Lexer.h"
#include "GitInformation.h"

int main (int argc, char** argv)
{
	try
	{
		// Intepret command-line parameters:
		// -l: list commands
		// I guess there should be a better way to do this.
		if (argc == 2 && String (argv[1]) == "-l")
		{
			Print ("Begin list of commands:\n");
			Print ("------------------------------------------------------\n");

			BotscriptParser parser;
			parser.SetReadOnly (true);
			parser.ParseBotscript ("botc_defs.bts");

			for (CommandInfo* comm : GetCommands())
				Print ("%1\n", comm->GetSignature());

			Print ("------------------------------------------------------\n");
			Print ("End of command list\n");
			exit (0);
		}

		// Print header
		String header;
		String headerline;
		header = Format (APPNAME " version %1", GetVersionString (true));

#ifdef DEBUG
		header += " (debug build)";
#endif

		for (int i = 0; i < header.Length() / 2; ++i)
			headerline += "-=";

		headerline += '-';
		Print ("%2\n\n%1\n\n%2\n\n", header, headerline);

		if (argc < 2)
		{
			fprintf (stderr, "usage: %s <infile> [outfile] # compiles botscript\n", argv[0]);
			fprintf (stderr, "       %s -l                 # lists commands\n", argv[0]);
			exit (1);
		}

		String outfile;

		if (argc < 3)
			outfile = MakeObjectFileName (argv[1]);
		else
			outfile = argv[2];

		// Prepare reader and writer
		BotscriptParser* parser = new BotscriptParser;

		// We're set, begin parsing :)
		Print ("Parsing script...\n");
		parser->ParseBotscript (argv[1]);
		Print ("Script parsed successfully.\n");

		// Parse done, print statistics and write to file
		int globalcount = parser->GetHighestVarIndex (true) + 1;
		int statelocalcount = parser->GetHighestVarIndex (false) + 1;
		int stringcount = CountStringsInTable();
		Print ("%1 / %2 strings\n", stringcount, gMaxStringlistSize);
		Print ("%1 / %2 global variable indices\n", globalcount, gMaxGlobalVars);
		Print ("%1 / %2 state variable indices\n", statelocalcount, gMaxGlobalVars);
		Print ("%1 / %2 events\n", parser->GetNumEvents(), gMaxEvents);
		Print ("%1 state%s1\n", parser->GetNumStates());

		parser->WriteToFile (outfile);
		delete parser;
		return 0;
	}
	catch (std::exception& e)
	{
		PrintTo (stderr, "error: %1\n", e.what());
		return 1;
	}
}

// ============================================================================
//
// Mutates given filename to an object filename
//
String MakeObjectFileName (String s)
{
	// Locate the extension and chop it out
	int extdot = s.LastIndexOf (".");

	if (extdot >= s.Length() - 4)
		s -= (s.Length() - extdot);

	s += ".o";
	return s;
}

// ============================================================================
//
DataType GetTypeByName (String t)
{
	t = t.ToLowercase();
	return	(t == "int") ? TYPE_Int :
			(t == "str") ? TYPE_String :
			(t == "void") ? TYPE_Void :
			(t == "bool") ? TYPE_Bool :
			TYPE_Unknown;
}


// ============================================================================
//
// Inverse operation - type name by value
//
String DataTypeName (DataType type)
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
String MakeVersionString (int major, int minor, int patch)
{
	String ver = Format ("%1.%2", major, minor);

	if (patch != 0)
	{
		ver += ".";
		ver += patch;
	}

	return ver;
}

// =============================================================================
//
String GetVersionString (bool longform)
{
	String tag (GIT_DESCRIPTION);
	String version = tag;

	if (longform && tag.EndsWith ("-pre"))
		version += "-" + String (GIT_HASH).Mid (0, 8);

	return version;
}
