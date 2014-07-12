/*
	Copyright 2014 Teemu Piippo
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

#include <string>
#include <deque>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

using std::string;
using std::deque;

static int gLineNumber;
static std::string gCurrentFile;

// =============================================================================
//
struct NamedEnumInfo
{
	string			name;
	deque<string>	enumerators;
};

// =============================================================================
//
void SkipWhitespace (char*& cp)
{
	while (isspace (*cp))
	{
		if (*cp == '\n')
			gLineNumber++;

		++cp;
	}

	if (strncmp (cp, "//", 2) == 0)
	{
		while (*(++cp) != '\n')
			;

		gLineNumber++;
		SkipWhitespace (cp);
	}
}

// =============================================================================
//
void Error (const char* fmt, ...)
{
	char buf[1024];
	va_list va;
	va_start (va, fmt);
	vsprintf (buf, fmt, va);
	va_end (va);
	throw std::string (buf);
}

// =============================================================================
//
int main (int argc, char* argv[])
{
	try
	{
		deque<NamedEnumInfo>	namedEnumerations;
		deque<string>			filesToInclude;

		if (argc < 3)
		{
			fprintf (stderr, "usage: %s input [input [input [...]]] output\n", argv[0]);
			return EXIT_FAILURE;
		}

		for (int i = 1; i < argc - 1; ++ i)
		{
			gCurrentFile = argv[i];
			FILE* fp = fopen (argv[i], "r");
			char* buf;
			gLineNumber = 1;

			if (fp == NULL)
			{
				fprintf (stderr, "could not open %s for writing: %s\n",
					argv[i], strerror (errno));
				exit (EXIT_FAILURE);
			}

			fseek (fp, 0, SEEK_END);
			long int filesize = ftell (fp);
			rewind (fp);

			try
			{
				buf = new char[filesize];
			}
			catch (std::bad_alloc)
			{
				Error ("could not allocate %ld bytes for %s\n", filesize, argv[i]);
			}

			if (static_cast<long> (fread (buf, 1, filesize, fp)) < filesize)
				Error ("could not read %ld bytes from %s\n", filesize, argv[i]);

			char* const end = &buf[0] + filesize;

			for (char* cp = &buf[0]; cp < end; ++cp)
			{
				SkipWhitespace (cp);

				if (strncmp (cp, "#define ", strlen ("#define ")) == 0)
				{
					while (cp < end && *cp != '\n')
						cp++;

					continue;
				}

				if ((cp != &buf[0] && isspace (* (cp - 1)) == false) ||
					strncmp (cp, "named_enum ", strlen ("named_enum ")) != 0)
				{
					continue;
				}

				cp += strlen ("named_enum ");
				SkipWhitespace (cp);

				NamedEnumInfo nenum;
				auto& enumname = nenum.name;
				auto& enumerators = nenum.enumerators;

				if (isalpha (*cp) == false && *cp != '_')
					Error ("anonymous named_enum");

				while (isalnum (*cp) || *cp == '_')
					enumname += *cp++;

				SkipWhitespace (cp);

				if (*cp++ != '{')
					Error ("expected '{' after named_enum");

				for (;;)
				{
					SkipWhitespace (cp);

					if (*cp == '}')
					{
						cp++;
						break;
					}

					if (isalpha (*cp) == false && *cp != '_')
						Error ("expected identifier, got '%c'", *cp);

					std::string enumerator;

					while (isalnum (*cp) || *cp == '_')
						enumerator += *cp++;

					SkipWhitespace (cp);

					if (*cp == ',')
						SkipWhitespace (++cp);

					if (*cp == '=')
						Error ("named enums must not have defined values");

					enumerators.push_back (enumerator);
				}

				SkipWhitespace (cp);

				if (*cp != ';')
					Error ("expected ';'");

				if (enumerators.size() > 0)
				{
					namedEnumerations.push_back (nenum);
					filesToInclude.push_back (argv[i]);
				}
			}
		}

		FILE* fp;

		if ((fp = fopen (argv[argc - 1], "w")) == NULL)
			Error ("couldn't open %s for writing: %s", argv[argc - 1], strerror (errno));

		fprintf (fp, "#pragma once\n");

		std::sort (filesToInclude.begin(), filesToInclude.end());
		auto pos = std::unique (filesToInclude.begin(), filesToInclude.end());
		filesToInclude.resize (std::distance (filesToInclude.begin(), pos));

		for (const string& a : filesToInclude)
			fprintf (fp, "#include \"%s\"\n", basename (a.c_str()));

		for (NamedEnumInfo& e : namedEnumerations)
		{
			fprintf (fp, "\nstatic const char* g_%sNames[] =\n{\n", e.name.c_str());

			for (const string& a : e.enumerators)
				fprintf (fp, "\t\"%s\",\n", a.c_str());

			fprintf (fp, "};\n\n");

			fprintf (fp, "inline const char* get%sString (%s a)\n"
				"{\n"
				"\treturn g_%sNames[a];\n"
				"}\n",
				e.name.c_str(), e.name.c_str(), e.name.c_str());
		}

		printf ("Wrote named enumerations to %s\n", argv[argc - 1]);
		fclose (fp);
	}
	catch (std::string a)
	{
		fprintf (stderr, "%s:%d: error: %s\n", gCurrentFile.c_str(), gLineNumber, a.c_str());
		return 1;
	}

	return 0;
}
