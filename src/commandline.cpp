/*
	Copyright 2014 Teemu Piippo
	Copyright 2019-2020 TarCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from this
	   software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include "commandline.h"

// _________________________________________________________________________________________________
//
CommandLine::~CommandLine()
{
	for (CommandLineOption* opt : _options)
		delete opt;
}

// _________________________________________________________________________________________________
//
StringList CommandLine::process (int const argc, char* argv[])
{
	StringList args;

	for (int argn = 1; argn < argc; ++argn)
	{
		String const arg (argv[argn]);

		if (arg == "--")
		{
			// terminating option - write everything to args and be done with it.
			while (++argn < argc)
				args << argv[argn];

			break;
		}

		if (arg[0] != '-' or arg[1] == '\0')
		{
			// non-option argument - pass to result
			args << arg;
			continue;
		}

		if (arg[1] != '-' and arg[1] != '\0')
		{
			// short-form options
			for (int i = 1; i < arg.length(); ++i)
			{
				CommandLineOption** optionptr = _options.find ([&](CommandLineOption* const& a)
				{
					return arg[i] == a->shortform();
				});

				if (not optionptr)
					error ("unknown option -%1", arg[i]);

				CommandLineOption* option = *optionptr;

				if (option->isOfType<bool>())
				{
					// Bool options need no parameters
					option->handleValue ("");
				}
				else
				{
					// Ensure we got a valid parameter coming up
					if (argn == argc - 1)
						error ("option -%1 requires a parameter", option->describe());

					if (i != arg.length() - 1)
					{
						error ("option %1 requires a parameter but has option -%2 stacked on",
							option->describe(), arg[i + 1]);
					}

					option->handleValue (argv[++argn]);
				}
			}
		}
		else
		{
			// long-form options
			String name, value;
			{
				int idx = arg.firstIndexOf ("=", 2);

				if (idx != -1)
				{
					name = arg.mid (2, idx);
					value = arg.mid (idx + 1);
				}
				else
				{
					name = arg.mid (2);
				}
			}

			CommandLineOption** optionptr = _options.find ([&](CommandLineOption* const& a)
			{
				return a->longform() == name;
			});

			if (not optionptr)
				error ("unknown option --%1", name);

			if ((*optionptr)->isOfType<bool>() and not value.isEmpty())
				error ("option %1 does not take a value", (*optionptr)->describe());
 
			(*optionptr)->handleValue (value);
		}
	}

	return args;
}

// _________________________________________________________________________________________________
//
String CommandLine::describeOptions() const
{
	StringList lines;

	for (CommandLineOption* option : _options)
	{
		String line ("  ");

		if (option->shortform() != '\0')
			line += String ({'-', option->shortform(), '\0'});
		else
			line += "  ";

		line += "  ";

		if (not option->longform().isEmpty())
		{
			line += "--" + option->longform();
			String description (option->describeArgument());

			if (not description.isEmpty())
				line += "=" + description;
		}

		lines << line;
	}

	int maxlength (0);

	for (String& line : lines)
		maxlength = max (maxlength, line.length());

	maxlength += 2;
	ASSERT_EQ (lines.size(), _options.size());

	for (int i = 0; i < lines.size(); ++i)
	{
		while (lines[i].length() < maxlength)
			lines[i] += " ";

		lines[i] += _options[i]->description();
	}

	StringList extralines;

	for (CommandLineOption* option : _options)
	{
		String extradata = option->describeExtra();

		if (not extradata.isEmpty())
		{
			if (extralines.isEmpty())
				extralines << "";

			extralines << extradata;
		}
	}

	lines.merge (extralines);
	String result (lines.join ("\n"));
	return result;
}

// _________________________________________________________________________________________________
//
void CommandLineOption::handleValue (const String& a)
{
	if (isOfType<bool>())
	{
		pointer().setValue (true);
	}
	elif (isOfType<int>())
	{
		bool ok;
		pointer().setValue (a.toLong (&ok));

		if (not ok)
			error ("bad integral value passed to %1", describe());
	}
	elif (isOfType<String>())
	{
		pointer().setValue (a);
	}
	elif (isOfType<double>())
	{
		bool ok;
		pointer().setValue (a.toDouble (&ok));

		if (not ok)
			error ("bad floating-point value passed to %1", describe());
	}
	else
	{
		error ("OMGWTFBBQ: %1 has an invalid type index", describe());
	}
}

// _________________________________________________________________________________________________
//
String CommandLineOption::describeArgument() const
{
	if (isOfType<int>())
		return "INTEGER";
	elif (isOfType<String>())
		return "STRING";
	elif (isOfType<double>())
		return "FLOAT";

	return "";
}
