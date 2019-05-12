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
#include "commandline.h"
#include "enumstrings.h"

#ifdef SVN_REVISION_STRING
#define FULL_VERSION_STRING VERSION_STRING "-" SVN_REVISION_STRING;
#else
#define FULL_VERSION_STRING VERSION_STRING;
#endif

int main (int argc, char** argv)
{
	try
	{
		Verbosity verboselevel (Verbosity::None);
		bool listcommands (false);
		bool sendhelp (false);

		CommandLine cmdline;
		cmdline.addOption (listcommands, 'l', "listfunctions", "List available functions");
		cmdline.addOption (sendhelp, 'h', "help", "Print help text");
		cmdline.addEnumeratedOption (verboselevel, 'V', "verbose", "Output more information");
		StringList args = cmdline.process (argc, argv);

		if (sendhelp)
		{
			// Print header
			String header = APPNAME " " FULL_VERSION_STRING;
#ifdef DEBUG
			header += " (debug build)";
#endif
			printTo (stderr, "%1\n", header);
			printTo (stderr, "usage: %1 [OPTIONS] SOURCE [OUTPUT]\n\n", argv[0]);
			printTo (stderr, "Options:\n" + cmdline.describeOptions() + "\n");
			return EXIT_SUCCESS;
		}

		if (listcommands)
		{
			BotscriptParser parser;
			parser.setReadOnly (true);
			parser.parseBotscript ("botc_defs.bts");

			for (CommandInfo* comm : getCommands())
				print ("%1\n", comm->signature());

			return EXIT_SUCCESS;
		}

		if (not within (args.size(), 1, 2))
		{
			printTo (stderr, "%1: need an input file.\nUse `%1 --help` for more information\n",
				argv[0]);
			return EXIT_FAILURE;
		}

		String outfile;

		if (args.size() == 1)
			outfile = makeObjectFileName (args[0]);
		else
			outfile = args[1];

		// Prepare reader and writer
		BotscriptParser* parser = new BotscriptParser;

		// We're set, begin parsing :)
		print ("Parsing script...\n");
		parser->parseBotscript (args[0]);
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
		return EXIT_SUCCESS;
	}
	catch (std::exception& e)
	{
		fprintf (stderr, "error: %s\n", e.what());
		return EXIT_FAILURE;
	}
}

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
//
DataType getTypeByName (String token)
{
	token = token.toLowercase();
	return	(token == "int") ? TYPE_Int
		  : (token == "str") ? TYPE_String
		  : (token == "void") ? TYPE_Void
		  : (token == "bool") ? TYPE_Bool
		  : (token == "state") ? TYPE_State
		  : (token == "array") ? TYPE_Array
		  : TYPE_Unknown;
}


// _________________________________________________________________________________________________
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
		case TYPE_State: return "state"; break;
		case TYPE_Array: return "array"; break;
		case TYPE_Unknown: return "???"; break;
	}

	return "";
}

// _________________________________________________________________________________________________
//
String makeVersionString (int major, int minor, int patch)
{
	String ver = String::fromNumber (major);
	ver += "." + String::fromNumber (minor);

	if (patch != 0)
		ver += String (".") + patch;

	return ver;
}

// _________________________________________________________________________________________________
//
String versionString()
{
	return VERSION_STRING;
}
