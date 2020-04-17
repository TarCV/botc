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
#include <vector>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <filesystem>
#include <ciso646>

using std::string;
using std::vector;

using std::filesystem::path;

static int LineNumber;
static std::string CurrentFile;
static auto const null = nullptr;

#define NAMED_ENUM_MACRO "named_enum"

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
struct NamedEnumInfo
{
	string			name;
	vector<string>	enumerators;
	bool			scoped;
	string			underlyingtype;
	bool			valuedefs;

	NamedEnumInfo() :
		scoped (false),
		valuedefs (false) {}

	//
	// Generate a string containing a C++ stub declaration for this enum.
	//
	string makeStub() const
	{
		return string ("enum ")
			+ (scoped ? "class " : "")
			+ name
			+ (underlyingtype.size() ? (" : " + underlyingtype) : "")
			+ ";";
	}

	string enumeratorString (string const& e) const
	{
		if (scoped)
			return name + "::" + e;
		else
			return e;
	}
};

// =============================================================================
//
void Normalize (string& a)
{
	char const* start;
	char const* end;

	for (start = &a.c_str()[0]; isspace (*start); ++start)
		;

	for (end = start; not isspace (*end) and *end != '\0'; ++end)
		;

	a = a.substr (start - a.c_str(), end - start);
}

// =============================================================================
//
class OutputFile
{
	string _buffer;
	string _filepath;

public:
	OutputFile (string const& filepath) :
		_filepath (filepath)
	{
		FILE* readhandle = fopen (_filepath.c_str(), "r");

		if (readhandle)
		{
			char line[1024];

			if (fgets (line, sizeof line, readhandle) == null)
				Error ("I/O error while reading %s", _filepath.c_str());

			if (strncmp (line, "// ", 3) == 0)
			{
				// Get rid of the newline at the end
				char* cp;
				for (cp = &line[3]; *cp != '\0' and *cp != '\n'; ++cp)
					;

				*cp = '\0';
			}

			fclose (readhandle);
		}
	}

	void append (char const* fmtstr, ...)
	{
		char buf[1024];
		va_list va;
		va_start (va, fmtstr);
		vsprintf (buf, fmtstr, va);
		va_end (va);
		_buffer += buf;
	}

    void appendString (const string &str) {
	    _buffer += str;
	}

	void writeToDisk()
	{
		FILE* handle = fopen (_filepath.c_str(), "w");

		if (not handle)
			Error ("couldn't open %s for writing", _filepath.c_str());

		fwrite (_buffer.c_str(), 1, _buffer.size(), handle);
		fclose (handle);
		fprintf (stdout, "Wrote output file %s.\n", _filepath.c_str());
	}
};

// =============================================================================
//
void SkipWhitespace (char*& cp, const char *end)
{
	while (cp < end && isspace (*cp))
	{
		if (*cp == '\n')
			LineNumber++;

		++cp;
	}

	if (strncmp (cp, "//", 2) == 0)
	{
		while (cp < end && *(++cp) != '\n')
			;

		LineNumber++;
		SkipWhitespace (cp, end);
	}
}

string getbasename(const char *str) {
    return path(str).filename().string();
}

// =============================================================================
//
int main (int argc, char* argv[])
{
	try
	{
		vector<NamedEnumInfo>	namedEnumerations;
		vector<string>			filesToInclude;

		if (argc < 3)
		{
			fprintf (stderr, "usage: %s input [input [input [...]]] output\n", argv[0]);
			return EXIT_FAILURE;
		}

		OutputFile header (argv[argc - 2]);
		OutputFile source (argv[argc - 1]);

		for (int i = 1; i < argc - 2; ++ i)
		{
			FILE* fp = fopen (argv[i], "r");
			char* buf;

			if (fp == NULL)
			{
				CurrentFile = "";
				Error ("could not open %s for reading: %s",
					argv[i], strerror (errno));
			}

			CurrentFile = argv[i];
			LineNumber = 1;
			fseek (fp, 0, SEEK_END);
			long int filesize = ftell (fp);
			rewind (fp);

			try
			{
				buf = new char[filesize];
			}
			catch (std::bad_alloc)
			{
				Error ("out of memory: could not allocate %ld bytes for opening %s\n",
					filesize, argv[i]);
			}

			long readbytes = fread(buf, 1, filesize, fp);
			if (readbytes < filesize) {
				if (feof(fp)) {
					filesize = readbytes;
				} else {
					Error("filesystem error: could not read %ld bytes from %s (read %ld bytes instead, error - %s)\n",
						  filesize, argv[i], readbytes, strerror(ferror(fp)), feof(fp));
				}
			}

			char* const end = &buf[0] + filesize;

			for (char* cp = &buf[0]; cp < end; ++cp)
			{
				SkipWhitespace (cp, end);

				if (strncmp (cp, "#define ", strlen ("#define ")) == 0)
				{
					while (cp < end && *cp != '\n')
						cp++;

					continue;
				}

				if ((cp != &buf[0] && isspace (* (cp - 1)) == false) ||
					strncmp (cp, NAMED_ENUM_MACRO " ", strlen (NAMED_ENUM_MACRO " ")) != 0)
				{
					continue;
				}

				cp += strlen (NAMED_ENUM_MACRO " ");
				SkipWhitespace (cp, end);

				NamedEnumInfo nenum;
				auto& enumname = nenum.name;
				auto& enumerators = nenum.enumerators;

				// See if it's a scoped enum
				if (strncmp (cp, "class ", strlen ("class ")) == 0)
				{
					nenum.scoped = true;
					cp += strlen ("class ");
				}

				if (isalpha (*cp) == false && *cp != '_')
					Error ("anonymous " NAMED_ENUM_MACRO);

				while (isalnum (*cp) || *cp == '_')
					enumname += *cp++;

				SkipWhitespace (cp, end);

				// We need an underlying type if this is not a scoped enum
				if (*cp == ':')
				{
					SkipWhitespace (cp, end);

					for (++cp; *cp != '\0' and *cp != '{'; ++cp)
						nenum.underlyingtype += *cp;

					if (not nenum.underlyingtype.size())
						Error ("underlying type left empty");

					Normalize (nenum.underlyingtype);
				}

				if (not nenum.scoped and not nenum.underlyingtype.size())
				{
					Error (NAMED_ENUM_MACRO " %s must be forward-declarable and thus must either "
						"be scoped (" NAMED_ENUM_MACRO " class) or define an underlying type "
						"(enum A : int)", nenum.name.c_str());
				}

				if (*cp++ != '{')
					Error ("expected '{' after " NAMED_ENUM_MACRO);

				for (;;)
				{
					SkipWhitespace (cp, end);

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

					SkipWhitespace (cp, end);

					if (*cp == '=')
					{
						// until I figure out how to deal with the duplicate value issue
						// I should probably use a map but not today.
						Error ("cannot have named enums that define enumerator values");

						nenum.valuedefs = true;

						while (*cp != ',' && *cp != '\0')
							cp++;

						if (*cp == '\0')
						{
							Error ("unexpected EOF while processing " NAMED_ENUM_MACRO " %s ",
								nenum.name.c_str());
						}
					}

					if (*cp == ',')
						SkipWhitespace (++cp, end);

					enumerators.push_back (enumerator);
				}

				SkipWhitespace (cp, end);

				if (*cp != ';')
					Error ("expected ';'");

				if (enumerators.size() > 0)
				{
					namedEnumerations.push_back (nenum);
					filesToInclude.push_back (argv[i]);
				}
				else
				{
					printf ("warning: " NAMED_ENUM_MACRO " %s left empty\n", nenum.name.c_str());
				}
			}
		}

		header.append ("#pragma once\n\n");

		for (NamedEnumInfo& e : namedEnumerations)
			header.append ("%s\n", e.makeStub().c_str());

		header.append ("\n");

		for (NamedEnumInfo& e : namedEnumerations)
			header.append ("const char* Get%sString (%s value);\n", e.name.c_str(), e.name.c_str());

		header.append ("\n");

		// MakeFormatArgument overloads so enums can be passed to that
		for (NamedEnumInfo& e : namedEnumerations)
			header.append ("String MakeFormatArgument (%s value);\n", e.name.c_str());

		std::sort (filesToInclude.begin(), filesToInclude.end());
		auto pos = std::unique (filesToInclude.begin(), filesToInclude.end());
		filesToInclude.resize (std::distance (filesToInclude.begin(), pos));

		for (string const& a : filesToInclude) {
            source.append("#include \"");
            source.appendString(getbasename(a.c_str()));
            source.append("\"\n");
        }

		source.append ("#include \"");
		source.appendString(getbasename(argv[argc - 2]));
		source.append ("\"\n");

		for (NamedEnumInfo& e : namedEnumerations)
		{
			if (not e.valuedefs)
			{
				source.append ("\nstatic const char* %sNames[] =\n{\n", e.name.c_str());

				for (const string& a : e.enumerators)
					source.append ("\t\"%s\",\n", e.enumeratorString (a).c_str());

				source.append ("};\n\n");

				source.append ("const char* Get%sString (%s value)\n"
					"{\n"
					"\treturn %sNames[long (value)];\n"
					"}\n",
					e.name.c_str(), e.name.c_str(), e.name.c_str());
			}
			else
			{
				source.append ("const char* Get%sString (%s value)\n"
					"{\n"
					"\tswitch (value)\n"
					"\t{\n", e.name.c_str(), e.name.c_str());

				for (const string& a : e.enumerators)
				{
					source.append ("\t\tcase %s: return \"%s\";\n",
						e.enumeratorString (a).c_str(), e.enumeratorString (a).c_str());
				}

				source.append ("\t\tdefault: return (\"[[[unknown "
					"value passed to Get%sString]]]\");\n\t}\n}\n", e.name.c_str());
			}

			source.append ("String MakeFormatArgument (%s value)\n{\n"
				"\treturn Get%sString (value);\n}\n", e.name.c_str(), e.name.c_str());
		}

		source.writeToDisk();
		header.writeToDisk();
	}
	catch (std::string a)
	{
		if (CurrentFile.size() > 0)
		{
			fprintf (stderr, "%s: %s:%d: error: %s\n",
				argv[0], CurrentFile.c_str(), LineNumber, a.c_str());
		}
		else
		{
			fprintf (stderr, "%s: error: %s\n", argv[0], a.c_str());
		}
		return 1;
	}

	return 0;
}
