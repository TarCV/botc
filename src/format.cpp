/*
	Copyright 2012-2014 Teemu Piippo
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

#include <cstdio>
#include "main.h"
#include "format.h"
#include "lexer.h"

// _________________________________________________________________________________________________
//
// Throws an error while formatting the string
//
static void formatError (String fmtstr, const String errdescribe, int pos)
{
	fmtstr.replace ("\n", " ");
	fmtstr.replace ("\t", " ");
	String errmsg ("With format string:\n" + fmtstr + "\n");

	for (int x = 0; x < pos; ++x)
		errmsg += "-";

	errmsg += "^\n" + errdescribe;
	throw std::logic_error (errmsg.stdString());
}

// _________________________________________________________________________________________________
//
// Main formatter algorithm function. Processes @fmtstr with @args and returns
// the result.
//
String formatArgs (const String& fmtstr, const std::vector<String>& args)
{
	String fmt = fmtstr;
	String out;
	int pos = 0;

	while ((pos = fmt.firstIndexOf ("%", pos)) != -1)
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
		if (fmt[pos + ofs] == 's' or fmt[pos + ofs] == 'x' or fmt[pos + ofs] == 'd')
		{
			mod = fmt[pos + ofs];
			ofs++;
		}

		if (not isdigit (fmt[pos + ofs]))
		{
			formatError (fmtstr, "bad format string, expected digit with optional "
				"modifier after '%%'", pos);
		}

		int i = fmt[pos + ofs]  - '0';

#ifdef DEBUG
		if (i == 0)
			formatError (fmtstr, "%0 in format string", pos);
#endif

		if (i > int (args.size()))
			formatError (fmtstr, String ("Format argument #") + i + " used but not defined.", pos);

		String replacement = args[i - 1];

		switch (mod)
		{
			case 's': replacement = (replacement == "1") ? "" : "s";		break;
			case 'd': replacement.sprintf ("%d", replacement[0]);			break;
			case 'x': replacement.sprintf ("0x%X", replacement.toLong());	break;
			default: break;
		}

		fmt.replace (pos, 1 + ofs, replacement);
		pos += replacement.length();
	}

	return fmt;
}

// _________________________________________________________________________________________________
//
// Throws a runtime error with the message @msg. If a lexer is active, its
// position is printed as well.
//
void error (const String& msg)
{
	Lexer* lx = Lexer::GetCurrentLexer();
	String fileinfo;

	if (lx != null and lx->hasValidToken())
	{
		Lexer::TokenInfo* tk = lx->token();
		fileinfo = format ("%1:%2:%3: ", tk->file, tk->line, tk->column);
	}

    throw std::runtime_error ((fileinfo + msg).c_str());
}
