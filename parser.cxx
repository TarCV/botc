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
	ParserError ("%s-statements may only be defined at top level!", token.chars());

#define MUST_NOT_TOPLEVEL if (g_CurMode == MODE_TOPLEVEL) \
	ParserError ("%s-statements may not be defined at top level!", token.chars());

int g_NumStates = 0;
int g_NumEvents = 0;
int g_CurMode = MODE_TOPLEVEL;
str g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;
unsigned int g_BlockStackCursor = 0;
DataBuffer* g_IfExpression = NULL;

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via ObjWriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void ScriptReader::ParseBotScript (ObjWriter* w) {
	while (Next()) {
		// ============================================================
		if (!token.compare ("state")) {
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
		
		// ============================================================
		if (!token.compare ("event")) {
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
		
		// ============================================================
		if (!token.compare ("mainloop")) {
			MUST_TOPLEVEL
			MustNext ("{");
			
			// Mode must be set before dataheader is written here!
			g_CurMode = MODE_MAINLOOP;
			w->Write (DH_MAINLOOP);
			continue;
		}
		
		// ============================================================
		if (!token.compare ("onenter") || !token.compare ("onexit")) {
			MUST_TOPLEVEL
			bool onenter = !token.compare ("onenter");
			MustNext ("{");
			
			// Mode must be set before dataheader is written here,
			// because onenter goes to a separate buffer.
			g_CurMode = onenter ? MODE_ONENTER : MODE_ONEXIT;
			w->Write (onenter ? DH_ONENTER : DH_ONEXIT);
			continue;
		}
		
		// ============================================================
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
		
		// ============================================================
		// Goto
		if (!token.compare ("goto")) {
			MUST_NOT_TOPLEVEL
			
			// Get the name of the label
			MustNext ();
			
			// Find the mark this goto statement points to
			// TODO: This should define the mark instead of bombing
			// out if the mark isn't found!
			unsigned int m = w->FindMark (token);
			if (m == MAX_MARKS)
				ParserError ("unknown label `%s`!", token.chars());
			
			// Add a reference to the mark.
			w->Write<word> (DH_GOTO);
			w->AddReference (m);
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// If
		if (!token.compare ("if")) {
			MUST_NOT_TOPLEVEL
			PushBlockStack ();
			
			// Condition
			MustNext ("(");
			
			// Read the expression and write it.
			// TODO: This should be storing it into a variable first, so
			// that else statements would be possible!
			MustNext ();
			DataBuffer* c = ParseExpression (TYPE_INT);
			w->WriteBuffer (c);
			
			MustNext (")");
			MustNext ("{");
			
			// Add a mark - to here temporarily - and add a reference to it.
			// Upon a closing brace, the mark will be adjusted.
			unsigned int marknum = w->AddMark ("");
			
			// Use DH_IFNOTGOTO - if the expression is not true, we goto the mark
			// we just defined - and this mark will be at the end of the block.
			w->Write<word> (DH_IFNOTGOTO);
			w->AddReference (marknum);
			
			// Store it in the block stack
			blockstack[g_BlockStackCursor].mark1 = marknum;
			blockstack[g_BlockStackCursor].type = BLOCKTYPE_IF;
			continue;
		}
		
		// ============================================================
		// While
		if (!token.compare ("while")) {
			MUST_NOT_TOPLEVEL
			PushBlockStack ();
			
			// While loops need two marks - one at the start of the loop and one at the
			// end. The condition is checked at the very start of the loop, if it fails,
			// we use goto to skip to the end of the loop. At the end, we loop back to
			// the beginning with a go-to statement.
			unsigned int mark1 = w->AddMark (""); // start
			unsigned int mark2 = w->AddMark (""); // end
			
			// Condition
			MustNext ("(");
			MustNext ();
			DataBuffer* expr = ParseExpression (TYPE_INT);
			MustNext (")");
			MustNext ("{");
			
			// Write condition
			w->WriteBuffer (expr);
			
			// Instruction to go to the end if it fails
			w->Write<word> (DH_IFNOTGOTO);
			w->AddReference (mark2);
			
			// Store the needed stuff
			blockstack[g_BlockStackCursor].mark1 = mark1;
			blockstack[g_BlockStackCursor].mark2 = mark2;
			blockstack[g_BlockStackCursor].type = BLOCKTYPE_WHILE;
			continue;
		}
		
		// ============================================================
		// For loop
		if (!token.compare ("for")) {
			MUST_NOT_TOPLEVEL
			PushBlockStack ();
			
			// Initializer
			MustNext ("(");
			MustNext ();
			DataBuffer* init = ParseStatement (w);
			MustNext (";");
			
			// Condition
			MustNext ();
			DataBuffer* cond = ParseExpression (TYPE_INT);
			MustNext (";");
			
			// Incrementor
			MustNext ();
			DataBuffer* incr = ParseStatement (w);
			MustNext (")");
			MustNext ("{");
			
			// First, write out the initializer
			w->WriteBuffer (init);
			
			// Init two marks
			int mark1 = w->AddMark ("");
			int mark2 = w->AddMark ("");
			
			// Add the condition
			w->WriteBuffer (cond);
			w->Write<word> (DH_IFNOTGOTO);
			w->AddReference (mark2);
			
			// Store the marks and incrementor
			blockstack[g_BlockStackCursor].mark1 = mark1;
			blockstack[g_BlockStackCursor].mark2 = mark2;
			blockstack[g_BlockStackCursor].buffer1 = incr;
			blockstack[g_BlockStackCursor].type = BLOCKTYPE_FOR;
			continue;
		}
		
		// ============================================================
		// Do/while loop
		if (!token.compare ("do")) {
			MUST_NOT_TOPLEVEL
			PushBlockStack ();
			MustNext ("{");
			blockstack[g_BlockStackCursor].mark1 = w->AddMark ("");
			blockstack[g_BlockStackCursor].type = BLOCKTYPE_DO;
			continue;
		}
		
		// ============================================================
		// Switch
		if (!token.compare ("switch")) {
			/* This goes a bit tricky. switch is structured in the
			 * bytecode followingly:
			 * (expression)
			 * case a: goto casemark1
			 * case b: goto casemark2
			 * case c: goto casemark3
			 * goto mark1 // jump to end if no matches
			 * casemark1: ...
			 * casemark2: ...
			 * casemark3: ...
			 * mark1: // end mark
			 */
			
			MUST_NOT_TOPLEVEL
			PushBlockStack ();
			MustNext ("(");
			MustNext ();
			w->WriteBuffer (ParseExpression (TYPE_INT));
			MustNext (")");
			MustNext ("{");
			blockstack[g_BlockStackCursor].type = BLOCKTYPE_SWITCH;
			blockstack[g_BlockStackCursor].mark1 = w->AddMark (""); // end mark
			blockstack[g_BlockStackCursor].buffer1 = NULL; // default header
			continue;
		}
		
		// ============================================================
		if (!token.compare ("case")) {
			// case is only allowed inside switch
			BlockInformation* info = &blockstack[g_BlockStackCursor];
			if (info->type != BLOCKTYPE_SWITCH)
				ParserError ("case label outside switch");
			
			// Get the literal (Zandronum does not support expressions here)
			MustNumber ();
			int num = atoi (token.chars ());
			MustNext (":");
			
			for (int i = 0; i < MAX_CASE; i++)
				if (info->casenumbers[i] == num)
					ParserError ("multiple case %d labels in one switch", num);
			
			// Write down the expression and case-go-to. This builds
			// the case tree. The closing event will write the actual
			// blocks and move the marks appropriately.
			//	AddSwitchCase will add the reference to the mark
			// for the case block that this heralds, and takes care
			// of buffering setup and stuff like that.
			//	NULL the switch buffer for the case-go-to statement,
			// we want it all under the switch, not into the case-buffers.
			w->SwitchBuffer = NULL;
			w->Write<word> (DH_CASEGOTO);
			w->Write<word> (num);
			AddSwitchCase (w, NULL);
			info->casenumbers[info->casecursor] = num;
			continue;
		}
		
		if (!token.compare ("default")) {
			BlockInformation* info = &blockstack[g_BlockStackCursor];
			if (info->type != BLOCKTYPE_SWITCH)
				ParserError ("default label outside switch");
			
			if (info->buffer1)
				ParserError ("multiple default labels in one switch");
			
			MustNext (":");
			
			// The default header is buffered into buffer1, since
			// it has to be the last of the case headers
			//
			// Since the expression is pushed into the switch
			// and is only popped when case succeeds, we have
			// to pop it with DH_DROP manually if we end up in
			// a default.
			DataBuffer* b = new DataBuffer;
			info->buffer1 = b;
			b->Write<word> (DH_DROP);
			b->Write<word> (DH_GOTO);
			AddSwitchCase (w, b);
			continue;
		}
		
		// ============================================================
		// Break statement.
		if (!token.compare ("break")) {
			if (!g_BlockStackCursor)
				ParserError ("unexpected `break`");
			
			BlockInformation* info = &blockstack[g_BlockStackCursor];
			
			w->Write<word> (DH_GOTO);
			
			// switch and if use mark1 for the closing point,
			// for and while use mark2.
			switch (info->type) {
			case BLOCKTYPE_IF:
			case BLOCKTYPE_SWITCH:
				w->AddReference (info->mark1);
				break;
			case BLOCKTYPE_FOR:
			case BLOCKTYPE_WHILE:
				w->AddReference (info->mark2);
				break;
			default:
				ParserError ("unexpected `break`");
				break;
			}
			
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// Label
		if (!PeekNext().compare (":")) {
			MUST_NOT_TOPLEVEL
			
			if (IsKeyword (token))
				ParserError ("label name `%s` conflicts with keyword\n", token.chars());
			if (FindCommand (token))
				ParserError ("label name `%s` conflicts with command name\n", token.chars());
			if (FindGlobalVariable (token))
				ParserError ("label name `%s` conflicts with variable\n", token.chars());
			
			w->AddMark (token);
			MustNext (":");
			continue;
		}
		
		// ============================================================
		if (!token.compare ("}")) {
			// Closing brace
			
			// If we're in the block stack, we're descending down from it now
			if (g_BlockStackCursor > 0) {
				BlockInformation* info = &blockstack[g_BlockStackCursor];
				switch (info->type) {
				case BLOCKTYPE_IF:
					// Adjust the closing mark.
					w->MoveMark (info->mark1);
					break;
				case BLOCKTYPE_FOR:
					// Write the incrementor at the end of the loop block
					w->WriteBuffer (info->buffer1);
					// fall-thru
				case BLOCKTYPE_WHILE:
					// Write down the instruction to go back to the start of the loop
					w->Write (DH_GOTO);
					w->AddReference (info->mark1);
					
					// Move the closing mark here since we're at the end of the while loop
					w->MoveMark (info->mark2);
					break;
				case BLOCKTYPE_DO: { 
					MustNext ("while");
					MustNext ("(");
					MustNext ();
					DataBuffer* expr = ParseExpression (TYPE_INT);
					MustNext (")");
					MustNext (";");
					
					// If the condition runs true, go back to the start.
					w->WriteBuffer (expr);
					w->Write<long> (DH_IFGOTO);
					w->AddReference (info->mark1);
					break;
				}
				case BLOCKTYPE_SWITCH: {
					// Switch closes. Move down to the record buffer of
					// the lower block.
					BlockInformation* previnfo = &blockstack[g_BlockStackCursor - 1];
					if (previnfo->casecursor != -1)
						w->SwitchBuffer = previnfo->casebuffers[previnfo->casecursor];
					else
						w->SwitchBuffer = NULL;
					
					// If there was a default in the switch, write its header down now.
					// If not, write instruction to jump to the end of switch after
					// the headers (thus won't fall-through if no case matched)
					if (info->buffer1)
						w->WriteBuffer (info->buffer1);
					else {
						w->Write<word> (DH_DROP);
						w->Write<word> (DH_GOTO);
						w->AddReference (info->mark1);
					}
					
					// Go through all of the buffers we
					// recorded down and write them.
					for (unsigned int u = 0; u < MAX_CASE; u++) {
						if (!info->casebuffers[u])
							continue;
						
						w->MoveMark (info->casemarks[u]);
						w->WriteBuffer (info->casebuffers[u]);
					}
					
					// Move the closing mark here
					w->MoveMark (info->mark1);
				}
				}
				
				// Descend down the stack
				g_BlockStackCursor--;
				continue;
			}
			
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
		
		// ============================================================
		// If nothing else, parse it as a statement (which is either
		// assignment or expression)
		DataBuffer* b = ParseStatement (w);
		w->WriteBuffer (b);
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
	
	MustNext ("(");
	MustNext ();
	
	int curarg = 0;
	while (1) {
		if (!token.compare (")")) {
			if (curarg < comm->numargs - 1)
				ParserError ("too few arguments passed to %s\n", comm->name.chars());
			break;
			curarg++;
		}
		
		if (curarg >= comm->maxargs)
			ParserError ("too many arguments passed to %s\n", comm->name.chars());
		
		r->Merge (ParseExpression (comm->argtypes[curarg]));
		MustNext ();
		
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
	case OPER_ASSIGNLEFTSHIFT:
	case OPER_ASSIGNRIGHTSHIFT:
	case OPER_ASSIGN:
		return true;
	}
	return false;
}

// ============================================================================
// Finds an operator's corresponding dataheader
static word DataHeaderByOperator (ScriptVar* var, int oper) {
	if (IsAssignmentOperator (oper)) {
		if (!var)
			error ("operator %d requires left operand to be a variable\n", oper);
		
		// TODO: At the moment, vars only are global
		// OPER_ASSIGNLEFTSHIFT and OPER_ASSIGNRIGHTSHIFT do not
		// have data headers, instead they are expanded out in
		// the operator parser
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
	case OPER_EQUALS: return DH_EQUALS;
	case OPER_NOTEQUALS: return DH_NOTEQUALS;
	case OPER_LESSTHAN: return DH_LESSTHAN;
	case OPER_GREATERTHAN: return DH_GREATERTHAN;
	case OPER_LESSTHANEQUALS: return DH_LESSTHANEQUALS;
	case OPER_GREATERTHANEQUALS: return DH_GREATERTHANEQUALS;
	case OPER_LEFTSHIFT: return DH_LSHIFT;
	case OPER_RIGHTSHIFT: return DH_RSHIFT;
	}
	
	error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
DataBuffer* ScriptReader::ParseExpression (int reqtype) {
	DataBuffer* retbuf = new DataBuffer (64);
	
	// Parse first operand
	retbuf->Merge (ParseExprValue (reqtype));
	
	// Parse any and all operators we get
	int oper;
	while ((oper = ParseOperator (true)) != -1) {
		// We peeked the operator, move forward now
		MustNext();
		
		// Can't be an assignement operator, those belong in assignments.
		if (IsAssignmentOperator (oper))
			ParserError ("assignment operator inside expression");
		
		// Parse the right operand,
		MustNext ();
		DataBuffer* rb = ParseExprValue (reqtype);
		
		// Write to buffer
		retbuf->Merge (rb);
		long dh = DataHeaderByOperator (NULL, oper);
		retbuf->Write<word> (dh);
	}
	
	return retbuf;
}

// ============================================================================
// Parses an operator string. Returns the operator number code.
#define ISNEXT(char) !PeekNext (peek ? 1 : 0).compare (char)
int ScriptReader::ParseOperator (bool peek) {
	str oper;
	if (peek)
		oper += PeekNext ();
	else
		oper += token;
	
	// Check one-char operators
	bool equalsnext = ISNEXT ("=");
	bool morenext = ISNEXT (">");
	bool lessnext = ISNEXT ("<");
	
	int o =	(!oper.compare ("=") && !equalsnext) ? OPER_ASSIGN :
		(!oper.compare (">") && !equalsnext && !morenext) ? OPER_GREATERTHAN :
		(!oper.compare ("<") && !equalsnext && !lessnext) ? OPER_LESSTHAN :
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
	equalsnext = !PeekNext (peek ? 2 : 1).compare ("=");
	
	o =	!oper.compare ("+=") ? OPER_ASSIGNADD :
		!oper.compare ("-=") ? OPER_ASSIGNSUB :
		!oper.compare ("*=") ? OPER_ASSIGNMUL :
		!oper.compare ("/=") ? OPER_ASSIGNDIV :
		!oper.compare ("%=") ? OPER_ASSIGNMOD :
		!oper.compare ("==") ? OPER_EQUALS :
		!oper.compare ("!=") ? OPER_NOTEQUALS :
		!oper.compare (">=") ? OPER_GREATERTHANEQUALS :
		!oper.compare ("<=") ? OPER_LESSTHANEQUALS :
		(!oper.compare ("<<") && !equalsnext) ? OPER_LEFTSHIFT :
		(!oper.compare (">>") && !equalsnext) ? OPER_RIGHTSHIFT :
		-1;
	
	if (o != -1) {
		MustNext ();
		return o;
	}
	
	// Three-char opers
	oper += PeekNext (peek ? 2 : 1);
	o =	!oper.compare ("<<=") ? OPER_ASSIGNLEFTSHIFT :
		!oper.compare (">>=") ? OPER_ASSIGNRIGHTSHIFT :
		-1;
	
	if (o != -1) {
		MustNext ();
		MustNext ();
	}
	
	return o;
}

// ============================================================================
// Parses a value in the expression and returns the data needed to push
// it, contained in a data buffer. A value can be either a variable, a command,
// a literal or an expression.
DataBuffer* ScriptReader::ParseExprValue (int reqtype) {
	DataBuffer* b = new DataBuffer(16);
	
	ScriptVar* g;
	
	// Prefixing "!" means negation.
	bool negate = !token.compare ("!");
	if (negate) // Jump past the "!"
		MustNext ();
	
	if (!token.compare ("(")) {
		// Expression
		MustNext ();
		DataBuffer* c = ParseExpression (reqtype);
		b->Merge (c);
		MustNext (")");
	} else if (CommandDef* comm = FindCommand (token)) {
		delete b;
		
		// Command
		if (reqtype && comm->returnvalue != reqtype)
			ParserError ("%s returns an incompatible data type", comm->name.chars());
		b = ParseCommand (comm);
	} else if ((g = FindGlobalVariable (token)) && reqtype != TYPE_STRING) {
		// Global variable
		b->Write<word> (DH_PUSHGLOBALVAR);
		b->Write<word> (g->index);
	} else {
		// If nothing else, check for literal
		switch (reqtype) {
		case TYPE_VOID:
			ParserError ("unknown identifier `%s` (expected keyword, function or variable)", token.chars());
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
			MustString (true);
			b->Write<word> (DH_PUSHSTRINGINDEX);
			b->Write<word> (PushToStringTable (token.chars()));
			break;
		}
	}
	
	// Negate it now if desired
	if (negate)
		b->Write<word> (DH_NEGATELOGICAL);
	
	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
DataBuffer* ScriptReader::ParseAssignment (ScriptVar* var) {
	bool global = !var->statename.len ();
	
	// Get an operator
	MustNext ();
	int oper = ParseOperator ();
	if (!IsAssignmentOperator (oper))
		ParserError ("expected assignment operator");
	
	if (g_CurMode == MODE_TOPLEVEL) // TODO: lift this restriction
		ParserError ("can't alter variables at top level");
	
	// Parse the right operand
	MustNext ();
	DataBuffer* retbuf = new DataBuffer;
	DataBuffer* expr = ParseExpression (TYPE_INT);
	
	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	if (oper == OPER_ASSIGNLEFTSHIFT || oper == OPER_ASSIGNRIGHTSHIFT) {
		retbuf->Write<word> (global ? DH_PUSHGLOBALVAR : DH_PUSHLOCALVAR);
		retbuf->Write<word> (var->index);
		retbuf->Merge (expr);
		retbuf->Write<word> ((oper == OPER_ASSIGNLEFTSHIFT) ? DH_LSHIFT : DH_RSHIFT);
		retbuf->Write<word> (global ? DH_ASSIGNGLOBALVAR : DH_ASSIGNLOCALVAR);
		retbuf->Write<word> (var->index);
	} else {
		retbuf->Merge (expr);
		long dh = DataHeaderByOperator (var, oper);
		retbuf->Write<word> (dh);
		retbuf->Write<word> (var->index);
	}
	
	return retbuf;
}

void ScriptReader::PushBlockStack () {
	g_BlockStackCursor++;
	BlockInformation* info = &blockstack[g_BlockStackCursor];
	info->type = 0;
	info->mark1 = 0;
	info->mark2 = 0;
	info->buffer1 = NULL;
	info->casecursor = -1;
	for (int i = 0; i < MAX_CASE; i++) {
		info->casemarks[i] = MAX_MARKS;
		info->casebuffers[i] = NULL;
		info->casenumbers[i] = -1;
	}
}

DataBuffer* ScriptReader::ParseStatement (ObjWriter* w) {
	// If it's a variable, expect assignment.
	if (ScriptVar* var = FindGlobalVariable (token)) {
		DataBuffer* b = ParseAssignment (var);
		return b;
	}
	
	// If it's not a keyword, parse it as an expression.
	DataBuffer* b = ParseExpression (TYPE_VOID);
	return b;
}

void ScriptReader::AddSwitchCase (ObjWriter* w, DataBuffer* b) {
	BlockInformation* info = &blockstack[g_BlockStackCursor];
	
	info->casecursor++;
	if (info->casecursor >= MAX_CASE)
		ParserError ("too many cases in one switch");
	
	// Init a mark for the case buffer
	int m = w->AddMark ("");
	info->casemarks[info->casecursor] = m;
	
	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (b)
		b->AddMarkReference (m);
	else
		w->AddReference (m);
	
	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	info->casebuffers[info->casecursor] = w->SwitchBuffer = new DataBuffer;
}