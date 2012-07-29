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

#define __PARSER_CXX__

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "str.h"
#include "objwriter.h"
#include "scriptreader.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"

#define MUST_TOPLEVEL if (g_CurMode != MODE_TOPLEVEL) \
	ParserError ("%ss may only be defined at top level!", token.chars());

int g_NumStates = 0;
int g_NumEvents = 0;
int g_CurMode = MODE_TOPLEVEL;
str g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via ObjWriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void ScriptReader::BeginParse (ObjWriter* w) {
	while (Next()) {
		printf ("BeginParse: token: `%s`\n", token.chars());
		if (!token.icompare ("state")) {
			MUST_TOPLEVEL
			
			MustString ();
			
			// State name must be a word.
			if (token.first (" ") != token.len())
				ParserError ("state name must be a single word! got `%s`", token.chars());
			str statename = token;
			
			// stateSpawn is special - it *must* be defined. If we
			// encountered it, then mark down that we have it.
			if (!token.icompare ("statespawn"))
				g_stateSpawnDefined = true;
			
			// Must end in a colon
			MustNext (":");
			
			// Write the previous state's onenter and
			// mainloop buffers to file now
			if (g_CurState.len())
				w->WriteBuffers();
			
			w->Write (DH_STATENAME);
			w->WriteString (statename);
			w->Write (DH_STATEIDX);
			w->Write (g_NumStates);
			
			g_NumStates++;
			g_CurState = token;
			g_GotMainLoop = false;
			continue;
		}
		
		if (!token.icompare ("event")) {
			MUST_TOPLEVEL
			
			// Event definition
			MustString ();
			
			EventDef* e = FindEventByName (token);
			if (!e)
				ParserError ("bad event! got `%s`\n", token.chars());
			
			MustNext ("{");
			
			g_CurMode = MODE_EVENT;
			
			w->Write (DH_EVENT);
			w->Write<word> (e->number);
			g_NumEvents++;
			continue;
		}
		
		if (!token.icompare ("mainloop")) {
			MUST_TOPLEVEL
			MustNext ("{");
			
			// Mode must be set before dataheader is written here!
			g_CurMode = MODE_MAINLOOP;
			w->Write (DH_MAINLOOP);
			continue;
		}
		
		if (!token.icompare ("onenter") || !token.icompare ("onexit")) {
			MUST_TOPLEVEL
			bool onenter = !token.compare ("onenter");
			MustNext ("{");
			
			// Mode must be set before dataheader is written here,
			// because onenter goes to a separate buffer.
			g_CurMode = onenter ? MODE_ONENTER : MODE_ONEXIT;
			w->Write (onenter ? DH_ONENTER : DH_ONEXIT);
			continue;
		}
		
		if (!token.compare ("var")) {
			// For now, only globals are supported
			if (g_CurMode != MODE_TOPLEVEL || g_CurState.len())
				ParserError ("variables must only be global for now");
			
			MustNext ();
			
			// Var name must not be a number
			if (token.isnumber())
				ParserError ("variable name must not be a number");
			
			str varname = token;
			ScriptVar* var = DeclareGlobalVariable (this, varname);
			
			if (!var)
				ParserError ("declaring %s variable %s failed",
					g_CurState.len() ? "state" : "global", varname.chars());
			
			MustNext (";");
			continue;
		}
		
		if (!token.compare ("}")) {
			printf ("parse closing brace\n");
			// Closing brace
			int dataheader =	(g_CurMode == MODE_EVENT) ? DH_ENDEVENT :
						(g_CurMode == MODE_MAINLOOP) ? DH_ENDMAINLOOP :
						(g_CurMode == MODE_ONENTER) ? DH_ENDONENTER :
						(g_CurMode == MODE_ONEXIT) ? DH_ENDONEXIT : -1;
			
			if (dataheader == -1)
				ParserError ("unexpected `}`");
			
			// Data header must be written before mode is changed because
			// onenter and mainloop go into special buffers, and we want
			// the closing data headers into said buffers too.
			w->Write (dataheader);
			g_CurMode = MODE_TOPLEVEL;
			
			if (!PeekNext().compare (";"))
				MustNext (";");
			continue;
		}
		
		// If it's a variable, expect assignment.
		if (ScriptVar* var = FindGlobalVariable (token)) {
			DataBuffer* b = ParseAssignment (var);
			printf ("current token after assignment: `%s`\n", token.chars());
			MustNext (";");
			w->WriteBuffer (b);
			delete b;
			continue;
		}
		
		// If it's not a keyword, parse it as an expression.
		printf ("token length: %d, first char: %c [%d]\n", token.len(), token.chars()[0], token.chars()[0]);
		DataBuffer* b = ParseExpression (TYPE_VOID);
		w->WriteBuffer (b);
		delete b;
		printf ("expression done! current token is %s\n", token.chars());
		MustNext (";");
	}
	
	if (g_CurMode != MODE_TOPLEVEL)
		ParserError ("script did not end at top level; did you forget a `}`?");
	
	// stateSpawn must be defined!
	if (!g_stateSpawnDefined)
		ParserError ("script must have a state named `stateSpawn`!");
	
	// Dump the last state's onenter and mainloop
	w->WriteBuffers ();
	
	// String table
	w->WriteStringTable ();
}

// ============================================================================
// Parses a command call
DataBuffer* ScriptReader::ParseCommand (CommandDef* comm) {
	DataBuffer* r = new DataBuffer(64);
	if (g_CurMode == MODE_TOPLEVEL)
		ParserError ("command call at top level");
	
	printf ("\n\n\n=====================================\nBEGIN PARSING COMMAND\n");
	printf ("token: %s\n", token.chars());
	MustNext ("(");
	MustNext ();
	
	int curarg = 0;
	while (1) {
		printf ("at argument %d\n", curarg);
		printf ("next token: %s\n", token.chars());
		
		if (!token.compare (")")) {
			printf ("closing command with token `%s`\n", token.chars());
			if (curarg < comm->numargs - 1)
				ParserError ("too few arguments passed to %s\n", comm->name.chars());
			break;
			curarg++;
		}
		
		if (curarg >= comm->maxargs)
			ParserError ("too many arguments passed to %s\n", comm->name.chars());
		
		r->Merge (ParseExpression (comm->argtypes[curarg]));
		MustNext ();
		printf ("after expression, token is `%s`\n", token.chars());
		
		if (curarg < comm->numargs - 1) {
			MustThis (",");
			MustNext ();
		} else if (curarg < comm->maxargs - 1) {
			// Can continue, but can terminate as well.
			if (!token.compare (")")) {
				curarg++;
				break;
			} else {
				MustThis (",");
				MustNext ();
			}
		}
		
		curarg++;
	}
	
	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->maxargs) {
		r->Write<word> (DH_PUSHNUMBER);
		r->Write<word> (comm->defvals[curarg]);
		curarg++;
	}
	
	r->Write<word> (DH_COMMAND);
	r->Write<word> (comm->number);
	r->Write<word> (comm->maxargs);
	
	printf ("command complete\n");
	return r;
}

// ============================================================================
// Is the given operator an assignment operator?
static bool IsAssignmentOperator (int oper) {
	switch (oper) {
	case OPER_ASSIGNADD:
	case OPER_ASSIGNSUB:
	case OPER_ASSIGNMUL:
	case OPER_ASSIGNDIV:
	case OPER_ASSIGNMOD:
	case OPER_ASSIGN:
		return true;
	}
	return false;
}

// ============================================================================
// Finds an operator's corresponding dataheader
static long DataHeaderByOperator (ScriptVar* var, int oper) {
	if (IsAssignmentOperator (oper)) {
		if (!var)
			error ("operator %d requires left operand to be a variable\n", oper);
		
		// TODO: At the moment, vars only are global
		switch (oper) {
		case OPER_ASSIGNADD: return DH_ADDGLOBALVAR;
		case OPER_ASSIGNSUB: return DH_SUBGLOBALVAR;
		case OPER_ASSIGNMUL: return DH_MULGLOBALVAR;
		case OPER_ASSIGNDIV: return DH_DIVGLOBALVAR;
		case OPER_ASSIGNMOD: return DH_MODGLOBALVAR;
		case OPER_ASSIGN: return DH_ASSIGNGLOBALVAR;
		default: error ("bad assignment operator!!\n");
		}
	}
	
	switch (oper) {
	case OPER_ADD: return DH_ADD;
	case OPER_SUBTRACT: return DH_SUBTRACT;
	case OPER_MULTIPLY: return DH_MULTIPLY;
	case OPER_DIVIDE: return DH_DIVIDE;
	case OPER_MODULUS: return DH_MODULUS;
	}
	
	error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
DataBuffer* ScriptReader::ParseExpression (int reqtype) {
	printf ("begin parsing expression. this token is `%s`, next token is `%s`\n",
		token.chars(), PeekNext().chars());
	DataBuffer* retbuf = new DataBuffer (64);
	
	DataBuffer* lb = NULL;
	
	// If it's a variable, note it down - we need to do special checks with it later.
	ScriptVar* var = FindGlobalVariable (token);
	
	lb = ParseExprValue (reqtype);
	printf ("done\n");
	
	// Get an operator
	printf ("parse operator at token %s\n", token.chars());
	int oper = ParseOperator (true);
	printf ("operator parsed: token is now %s\n", token.chars());
	printf ("got %d\n", oper);
	
	// No operator found - stop here.
	if (oper == -1) {
		retbuf->Merge (lb);
		printf ("expression complete without operator, stopping at `%s`\n", token.chars());
		return retbuf;
	}
	
	// We peeked the operator, move forward now
	MustNext();
	
	// Can't be an assignement operator, those belong in assignments.
	if (IsAssignmentOperator (oper))
		ParserError ("assignment operator inside expressions");
	
	// Parse the right operand,
	printf ("parse right operand\n");
	MustNext ();
	DataBuffer* rb = ParseExprValue (reqtype);
	printf ("done\n");
	
	retbuf->Merge (rb);
	retbuf->Merge (lb);
	
	long dh = DataHeaderByOperator (var, oper);
	retbuf->Write<word> (dh);
	
	printf ("expression complete\n");
	return retbuf;
}

// ============================================================================
// Parses an operator string. Returns the operator number code.
int ScriptReader::ParseOperator (bool peek) {
	str oper;
	if (peek)
		oper += PeekNext ();
	else
		oper += token;
	
	// Check one-char operators
	int o =	!oper.compare ("=") ? OPER_ASSIGN :
		!oper.compare ("+") ? OPER_ADD :
		!oper.compare ("-") ? OPER_SUBTRACT :
		!oper.compare ("*") ? OPER_MULTIPLY :
		!oper.compare ("/") ? OPER_DIVIDE :
		!oper.compare ("%") ? OPER_MODULUS :
		-1;
	
	if (o != -1) {
		return o;
	}
	
	// Two-char operators
	oper += PeekNext (peek ? 1 : 0);
	
	o =	!oper.compare ("+=") ? OPER_ASSIGNADD :
		!oper.compare ("-=") ? OPER_ASSIGNSUB :
		!oper.compare ("*=") ? OPER_ASSIGNMUL :
		!oper.compare ("/=") ? OPER_ASSIGNDIV :
		!oper.compare ("%=") ? OPER_ASSIGNMOD :
		-1;
	
	if (o != -1)
		MustNext ();
	
	return o;
}

// ============================================================================
// Parses a value in the expression and returns the data needed to push
// it, contained in a data buffer. A value can be either a variable, a command,
// a literal or an expression.
DataBuffer* ScriptReader::ParseExprValue (int reqtype) {
	printf ("parse expr value `%s` with requirement type %d\n", token.chars(), reqtype);
	DataBuffer* b = new DataBuffer(16);
	
	ScriptVar* g;
	
	if (!token.compare ("(")) {
		printf ("value is an expression\n");
		// Expression
		MustNext ();
		DataBuffer* c = ParseExpression (reqtype);
		b->Merge (c);
		MustNext (")");
	} else if (CommandDef* comm = FindCommand (token)) {
		printf ("value is a command\n");
		delete b;
		
		// Command
		if (reqtype && comm->returnvalue != reqtype)
			ParserError ("%s returns an incompatible data type", comm->name.chars());
		b = ParseCommand (comm);
	} else if ((g = FindGlobalVariable (token)) && reqtype != TYPE_STRING) {
		printf ("value is a global var\n");
		// Global variable
		b->Write<word> (DH_PUSHGLOBALVAR);
		b->Write<word> (g->index);
	} else {
		printf ("value is a literal\n");
		// If nothing else, check for literal
		switch (reqtype) {
		case TYPE_VOID:
			ParserError ("bad syntax");
			break;
		case TYPE_INT: {
			MustNumber (true);
			
			// All values are written unsigned - thus we need to write the value's
			// absolute value, followed by an unary minus if it was negative.
			b->Write<word> (DH_PUSHNUMBER);
			long v = atoi (token.chars ());
			b->Write<word> (static_cast<word> (abs (v)));
			if (v < 0)
				b->Write<word> (DH_UNARYMINUS);
			break;
		}
		case TYPE_STRING:
			// PushToStringTable either returns the string index of the
			// string if it finds it in the table, or writes it to the
			// table and returns it index if it doesn't find it there.
			printf ("value is a string literal\n");
			MustString (true);
			b->Write<word> (DH_PUSHSTRINGINDEX);
			b->Write<word> (PushToStringTable (token.chars()));
			break;
		}
	}
	printf ("value parsed: current token is `%s`\n", token.chars());
	
	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
DataBuffer* ScriptReader::ParseAssignment (ScriptVar* var) {
	printf ("ASSIGNMENT: this token is `%s`, next token is `%s`\n",
		token.chars(), PeekNext().chars());
	
	// Get an operator
	printf ("parse assignment operator at token %s\n", token.chars());
	
	MustNext ();
	int oper = ParseOperator ();
	if (!IsAssignmentOperator (oper))
		ParserError ("expected assignment operator");
	printf ("got %d\n", oper);
	
	if (g_CurMode == MODE_TOPLEVEL) // TODO: lift this restriction
		ParserError ("can't alter variables at top level");
	
	// Parse the right operand,
	printf ("parse right operand\n");
	MustNext ();
	DataBuffer* retbuf = ParseExprValue (TYPE_INT);
	
	long dh = DataHeaderByOperator (var, oper);
	retbuf->Write<word> (dh);
	retbuf->Write<word> (var->index);
	
	printf ("assignment complete\n");
	return retbuf;
}