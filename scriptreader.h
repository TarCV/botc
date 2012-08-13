/*
 *	botc source code
 *	Copyright (C) 2012 Santeri `Dusk` Piippo
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	
 *	1. Redistributions of source code must retain the above copyright notice,
 *	   this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright notice,
 *	   this list of conditions and the following disclaimer in the documentation
 *	   and/or other materials provided with the distribution.
 *	3. Neither the name of the developer nor the names of its contributors may
 *	   be used to endorse or promote products derived from this software without
 *	   specific prior written permission.
 *	4. Redistributions in any form must be accompanied by information on how to
 *	   obtain complete source code for the software and any accompanying
 *	   software that uses the software. The source code must either be included
 *	   in the distribution or be available for no more than the cost of
 *	   distribution plus a nominal fee, and must be freely redistributable
 *	   under reasonable conditions. For an executable file, complete source
 *	   code means the source code for all modules it contains. It does not
 *	   include source code for modules or files that typically accompany the
 *	   major components of the operating system on which the executable file
 *	   runs.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SCRIPTREADER_H__
#define __SCRIPTREADER_H__

#include <stdio.h>
#include "str.h"
#include "objwriter.h"
#include "commands.h"

#define MAX_FILESTACK 8
#define MAX_STRUCTSTACK 32
#define MAX_CASE 64

class ScriptVar;

// ============================================================================
// Meta-data about blocks
struct BlockInformation {
	unsigned int mark1;
	unsigned int mark2;
	unsigned int type;
	DataBuffer* buffer1;
	
	// switch-related stuff
	// Which case are we at?
	short casecursor;
	
	// Marks to case-blocks
	int casemarks[MAX_CASE];
	
	// Numbers of the case labels
	int casenumbers[MAX_CASE];
	
	// actual case blocks
	DataBuffer* casebuffers[MAX_CASE];
	
	// What is the current buffer of the block?
	DataBuffer* recordbuffer;
};

// ============================================================================
// The script reader reads the script, parses it and tells the object writer
// the bytecode it needs to write to file.
class ScriptReader {
public:
	// ====================================================================
	// MEMBERS
	FILE* fp[MAX_FILESTACK];
	char* filepath[MAX_FILESTACK];
	int fc;
	
	unsigned int pos[MAX_FILESTACK];
	unsigned int curline[MAX_FILESTACK];
	unsigned int curchar[MAX_FILESTACK];
	BlockInformation blockstack[MAX_STRUCTSTACK];
	long savedpos[MAX_FILESTACK]; // filepointer cursor position
	str token;
	int commentmode;
	long prevpos;
	str prevtoken;
	
	// ====================================================================
	// METHODS
	// scriptreader.cxx:
	ScriptReader (str path);
	~ScriptReader ();
	void OpenFile (str path);
	void CloseFile (unsigned int u = MAX_FILESTACK);
	char ReadChar ();
	char PeekChar (int offset = 0);
	bool Next (bool peek = false);
	void Prev ();
	str PeekNext (int offset = 0);
	void Seek (unsigned int n, int origin);
	void MustNext (const char* c = "");
	void MustThis (const char* c);
	void MustString (bool gotquote = false);
	void MustNumber (bool fromthis = false);
	void MustBool ();
	bool BoolValue ();
	
	void ParserError (const char* message, ...);
	void ParserWarning (const char* message, ...);
	
	// parser.cxx:
	void ParseBotScript (ObjWriter* w);
	DataBuffer* ParseCommand (CommandDef* comm);
	DataBuffer* ParseExpression (int reqtype);
	DataBuffer* ParseAssignment (ScriptVar* var);
	int ParseOperator (bool peek = false);
	DataBuffer* ParseExprValue (int reqtype);
	void PushBlockStack ();
	
	// preprocessor.cxx:
	void PreprocessDirectives ();
	void PreprocessMacros ();
	DataBuffer* ParseStatement (ObjWriter* w);
	void AddSwitchCase (ObjWriter* w, DataBuffer* b);
	
private:
	bool atnewline;
	char c;
	void ParserMessage (const char* header, char* message);
	
	bool DoDirectivePreprocessing ();
	char PPReadChar ();
	void PPMustChar (char c);
	str PPReadWord (char &term);
};

enum {
	TYPE_VOID = 0,
	TYPE_INT,
	TYPE_STRING,
	TYPE_FLOAT
};

// Operators
enum {
	OPER_ADD,
	OPER_SUBTRACT,
	OPER_MULTIPLY,
	OPER_DIVIDE,
	OPER_MODULUS,
	OPER_ASSIGN,
	OPER_ASSIGNADD,
	OPER_ASSIGNSUB,
	OPER_ASSIGNMUL,
	OPER_ASSIGNDIV,
	OPER_ASSIGNMOD, // -- 10
	OPER_EQUALS,
	OPER_NOTEQUALS,
	OPER_LESSTHAN,
	OPER_GREATERTHAN,
	OPER_LESSTHANEQUALS,
	OPER_GREATERTHANEQUALS,
	OPER_LEFTSHIFT,
	OPER_RIGHTSHIFT,
	OPER_ASSIGNLEFTSHIFT,
	OPER_ASSIGNRIGHTSHIFT, // -- 20
	OPER_OR,
	OPER_AND,
	OPER_BITWISEOR,
	OPER_BITWISEAND,
	OPER_BITWISEEOR,
};

// Mark types
enum {
	MARKTYPE_LABEL,
	MARKTYPE_IF,
	MARKTYPE_INTERNAL, // internal structures
};

// Block types
enum {
	BLOCKTYPE_IF,
	BLOCKTYPE_WHILE,
	BLOCKTYPE_FOR,
	BLOCKTYPE_DO,
	BLOCKTYPE_SWITCH,
};

#endif // __SCRIPTREADER_H__