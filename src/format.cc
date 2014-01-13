/*
	Copyright (c) 2014, Santeri Piippo
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

#include <cstdio>
#include "main.h"
#include "format.h"

static void draw_pos (const string& fmt, int pos)
{
	string rep (fmt);
	rep.replace ("\n", "↵");
	rep.replace ("\t", "⇥");

	fprintf (stderr, "%s\n", rep.chars());

	for (int x = 0; x < pos; ++x)
		fprintf (stderr, "-");

	fprintf (stderr, "^\n");
}

string format_args (const list<format_arg>& args)
{
	const string& fmtstr = args[0].as_string();
	assert (args.size() >= 1);

	if (args.size() == 1)
		return args[0].as_string();

	string fmt = fmtstr;
	string out;
	int pos = 0;

	while ((pos = fmt.first ("%", pos)) != -1)
	{
		if (fmt[pos + 1] == '%')
		{
			fmt.replace (pos, 2, "%");
			pos++;
			continue;
		}

		int ofs = 1;
		char mod = '\0';

		// handle modifiers
		if (fmt[pos + ofs] == 's' || fmt[pos + ofs] == 'x')
		{
			mod = fmt[ pos + ofs ];
			ofs++;
		}

		if (!isdigit (fmt[pos + ofs]))
		{
			fprintf (stderr, "bad format string, expected digit with optional "
					 "modifier after '%%':\n");
			draw_pos (fmt, pos);
			return fmt;
		}

		int i = fmt[pos + ofs]  - '0';

		if (i >= args.size())
		{
			fprintf (stderr, "format arg #%d used but not defined: %s\n", i, fmtstr.chars());
			return fmt;
		}

		string repl = args[i].as_string();

		if (mod == 's')
		{
			repl = (repl == "1") ? "" : "s";
		}

		elif (mod == 'd')
		{
			repl.sprintf ("%d", repl[0]);
		}
		elif (mod == 'x')
		{
			// modifier x: reinterpret the argument as hex
			repl.sprintf ("0x%X", strtol (repl.chars(), null, 10));
		}

		fmt.replace (pos, 1 + ofs, repl);
		pos += repl.length();
	}

	return fmt;
}

void print_args (FILE* fp, const list<format_arg>& args)
{
	string out = format_args (args);
	fprintf (fp, "%s", out.chars());
}