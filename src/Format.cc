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

#include <cstdio>
#include "Main.h"
#include "Format.h"
#include "Lexer.h"

// =============================================================================
//
static void FormatError (String fmtstr, const String errdescribe, int pos)
{
	fmtstr.Replace ("\n", " ");
	fmtstr.Replace ("\t", " ");
	String errmsg ("With format string:\n" + fmtstr + "\n");

	for (int x = 0; x < pos; ++x)
		errmsg += "-";

	errmsg += "^\n" + errdescribe;
	throw std::logic_error (errmsg.STDString());
}

// =============================================================================
//
String FormatArgs (const String& fmtstr, const std::vector<String>& args)
{
	String fmt = fmtstr;
	String out;
	int pos = 0;

	while ((pos = fmt.FirstIndexOf ("%", pos)) != -1)
	{
		if (fmt[pos + 1] == '%')
		{
			fmt.Replace (pos, 2, "%");
			pos++;
			continue;
		}

		int ofs = 1;
		char mod = '\0';

		// handle modifiers
		if (fmt[pos + ofs] == 's' || fmt[pos + ofs] == 'x' || fmt[pos + ofs] == 'd')
		{
			mod = fmt[pos + ofs];
			ofs++;
		}

		if (!isdigit (fmt[pos + ofs]))
			FormatError (fmtstr, "bad format string, expected digit with optional "
				"modifier after '%%'", pos);

		int i = fmt[pos + ofs]  - '0';

		if (i > static_cast<signed> (args.size()))
			FormatError (fmtstr, String ("Format argument #") + i + " used but not defined.", pos);

		String replacement = args[i - 1];

		switch (mod)
		{
			case 's': replacement = (replacement == "1") ? "" : "s";		break;
			case 'd': replacement.SPrintf ("%d", replacement[0]);			break;
			case 'x': replacement.SPrintf ("0x%X", replacement.ToLong());	break;
			default: break;
		}

		fmt.Replace (pos, 1 + ofs, replacement);
		pos += replacement.Length();
	}

	return fmt;
}

// =============================================================================
//
void Error (String msg)
{
	Lexer* lx = Lexer::GetCurrentLexer();
	String fileinfo;

	if (lx != null && lx->HasValidToken())
	{
		Lexer::TokenInfo* tk = lx->Token();
		fileinfo = Format ("%1:%2:%3: ", tk->file, tk->line, tk->column);
	}

	throw std::runtime_error (fileinfo + msg);
}
