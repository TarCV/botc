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
#include "array.h"

#define MUST_TOPLEVEL if (g_CurMode != MODE_TOPLEVEL) \
	ParserError ("%s-statements may only be defined at top level!", token.chars());

#define MUST_NOT_TOPLEVEL if (g_CurMode == MODE_TOPLEVEL) \
	ParserError ("%s-statements may not be defined at top level!", token.chars());

#define SCOPE(n) scopestack[g_ScopeCursor - n]

int g_NumStates = 0;
int g_NumEvents = 0;
parsermode_e g_CurMode = MODE_TOPLEVEL;
str g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;
unsigned int g_ScopeCursor = 0;
DataBuffer* g_IfExpression = NULL;
bool g_CanElse = false;
str* g_UndefinedLabels[MAX_MARKS];
bool g_Neurosphere = false; // neurosphere-compat
array<constinfo_t> g_ConstInfo;

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via ObjWriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void ScriptReader::ParseBotScript (ObjWriter* w) {
	// Zero the entire block stack first
	for (int i = 0; i < MAX_SCOPE; i++)
		ZERO(scopestack[i]);
	
	for (int i = 0; i < MAX_MARKS; i++)
		g_UndefinedLabels[i] = NULL;
	
	while (Next()) {
		// Check if else is potentically valid
		if (token == "else" && !g_CanElse)
			ParserError ("else without preceding if");
		if (token != "else")
			g_CanElse = false;
		
		// ============================================================
		if (token == "state") {
			MUST_TOPLEVEL
			
			MustString ();
			
			// State name must be a word.
			if (token.first (" ") != token.len())
				ParserError ("state name must be a single word, got `%s`", token.chars());
			str statename = token;
			
			// stateSpawn is special - it *must* be defined. If we
			// encountered it, then mark down that we have it.
			if (-token == "statespawn")
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
		if (token == "event") {
			MUST_TOPLEVEL
			
			// Event definition
			MustString ();
			
			EventDef* e = FindEventByName (token);
			if (!e)
				ParserError ("bad event, got `%s`\n", token.chars());
			
			MustNext ("{");
			
			g_CurMode = MODE_EVENT;
			
			w->Write (DH_EVENT);
			w->Write (e->number);
			g_NumEvents++;
			continue;
		}
		
		// ============================================================
		if (token == "mainloop") {
			MUST_TOPLEVEL
			MustNext ("{");
			
			// Mode must be set before dataheader is written here!
			g_CurMode = MODE_MAINLOOP;
			w->Write (DH_MAINLOOP);
			continue;
		}
		
		// ============================================================
		if (token == "onenter" || token == "onexit") {
			MUST_TOPLEVEL
			bool onenter = token == "onenter";
			MustNext ("{");
			
			// Mode must be set before dataheader is written here,
			// because onenter goes to a separate buffer.
			g_CurMode = onenter ? MODE_ONENTER : MODE_ONEXIT;
			w->Write (onenter ? DH_ONENTER : DH_ONEXIT);
			continue;
		}
		
		// ============================================================
		if (token == "int" || token == "str" || token == "bool") {
			// For now, only globals are supported
			if (g_CurMode != MODE_TOPLEVEL || g_CurState.len())
				ParserError ("variables must only be global for now");
			
			type_e type =	(token == "int") ? TYPE_INT :
							(token == "str") ? TYPE_STRING :
							TYPE_BOOL;
			
			MustNext ();
			
			// Var name must not be a number
			if (token.isnumber())
				ParserError ("variable name must not be a number");
			
			str varname = token;
			ScriptVar* var = DeclareGlobalVariable (this, type, varname);
			
			if (!var)
				ParserError ("declaring %s variable %s failed",
					g_CurState.len() ? "state" : "global", varname.chars());
			
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// Goto
		if (token == "goto") {
			MUST_NOT_TOPLEVEL
			
			// Get the name of the label
			MustNext ();
			
			// Find the mark this goto statement points to
			unsigned int m = w->FindMark (token);
			
			// If not set, define it
			if (m == MAX_MARKS) {
				m = w->AddMark (token);
				g_UndefinedLabels[m] = new str (token);
			}
			
			// Add a reference to the mark.
			w->Write (DH_GOTO);
			w->AddReference (m);
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// If
		if (token == "if") {
			MUST_NOT_TOPLEVEL
			PushScope ();
			
			// Condition
			MustNext ("(");
			
			// Read the expression and write it.
			MustNext ();
			DataBuffer* c = ParseExpression (TYPE_INT);
			w->WriteBuffer (c);
			
			MustNext (")");
			MustNext ("{");
			
			// Add a mark - to here temporarily - and add a reference to it.
			// Upon a closing brace, the mark will be adjusted.
			unsigned int marknum = w->AddMark ("");
			
			// Use DH_IFNOTGOTO - if the expression is not true, we goto the mark
			// we just defined - and this mark will be at the end of the scope block.
			w->Write (DH_IFNOTGOTO);
			w->AddReference (marknum);
			
			// Store it
			SCOPE(0).mark1 = marknum;
			SCOPE(0).type = SCOPETYPE_IF;
			continue;
		}
		
		if (token == "else") {
			MUST_NOT_TOPLEVEL
			MustNext ("{");
			
			// Don't use PushScope as it resets the scope
			g_ScopeCursor++;
			if (g_ScopeCursor >= MAX_SCOPE)
				ParserError ("too deep scope");
			
			if (SCOPE(0).type != SCOPETYPE_IF)
				ParserError ("else without preceding if");
			
			// Write down to jump to the end of the else statement
			// Otherwise we have fall-throughs
			SCOPE(0).mark2 = w->AddMark ("");
			
			// Instruction to jump to the end after if block is complete
			w->Write (DH_GOTO);
			w->AddReference (SCOPE(0).mark2);
			
			// Move the ifnot mark here and set type to else
			w->MoveMark (SCOPE(0).mark1);
			SCOPE(0).type = SCOPETYPE_ELSE;
			continue;
		}
		
		// ============================================================
		// While
		if (token == "while") {
			MUST_NOT_TOPLEVEL
			PushScope ();
			
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
			w->Write (DH_IFNOTGOTO);
			w->AddReference (mark2);
			
			// Store the needed stuff
			SCOPE(0).mark1 = mark1;
			SCOPE(0).mark2 = mark2;
			SCOPE(0).type = SCOPETYPE_WHILE;
			continue;
		}
		
		// ============================================================
		// For loop
		if (token == "for") {
			MUST_NOT_TOPLEVEL
			PushScope ();
			
			// Initializer
			MustNext ("(");
			MustNext ();
			DataBuffer* init = ParseStatement (w);
			if (!init)
				ParserError ("bad statement for initializer of for");
			
			MustNext (";");
			
			// Condition
			MustNext ();
			DataBuffer* cond = ParseExpression (TYPE_INT);
			if (!cond)
				ParserError ("bad statement for condition of for");
			
			MustNext (";");
			
			// Incrementor
			MustNext ();
			DataBuffer* incr = ParseStatement (w);
			if (!incr)
				ParserError ("bad statement for incrementor of for");
			
			MustNext (")");
			MustNext ("{");
			
			// First, write out the initializer
			w->WriteBuffer (init);
			
			// Init two marks
			int mark1 = w->AddMark ("");
			int mark2 = w->AddMark ("");
			
			// Add the condition
			w->WriteBuffer (cond);
			w->Write (DH_IFNOTGOTO);
			w->AddReference (mark2);
			
			// Store the marks and incrementor
			SCOPE(0).mark1 = mark1;
			SCOPE(0).mark2 = mark2;
			SCOPE(0).buffer1 = incr;
			SCOPE(0).type = SCOPETYPE_FOR;
			continue;
		}
		
		// ============================================================
		// Do/while loop
		if (token == "do") {
			MUST_NOT_TOPLEVEL
			PushScope ();
			MustNext ("{");
			SCOPE(0).mark1 = w->AddMark ("");
			SCOPE(0).type = SCOPETYPE_DO;
			continue;
		}
		
		// ============================================================
		// Switch
		if (token == "switch") {
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
			PushScope ();
			MustNext ("(");
			MustNext ();
			w->WriteBuffer (ParseExpression (TYPE_INT));
			MustNext (")");
			MustNext ("{");
			SCOPE(0).type = SCOPETYPE_SWITCH;
			SCOPE(0).mark1 = w->AddMark (""); // end mark
			SCOPE(0).buffer1 = NULL; // default header
			continue;
		}
		
		// ============================================================
		if (token == "case") {
			// case is only allowed inside switch
			if (SCOPE(0).type != SCOPETYPE_SWITCH)
				ParserError ("case label outside switch");
			
			// Get the literal (Zandronum does not support expressions here)
			MustNumber ();
			int num = atoi (token.chars ());
			MustNext (":");
			
			for (int i = 0; i < MAX_CASE; i++)
				if (SCOPE(0).casenumbers[i] == num)
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
			w->Write (DH_CASEGOTO);
			w->Write (num);
			AddSwitchCase (w, NULL);
			SCOPE(0).casenumbers[SCOPE(0).casecursor] = num;
			continue;
		}
		
		if (token == "default") {
			if (SCOPE(0).type != SCOPETYPE_SWITCH)
				ParserError ("default label outside switch");
			
			if (SCOPE(0).buffer1)
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
			SCOPE(0).buffer1 = b;
			b->Write (DH_DROP);
			b->Write (DH_GOTO);
			AddSwitchCase (w, b);
			continue;
		}
		
		// ============================================================
		// Break statement.
		if (token == "break") {
			if (!g_ScopeCursor)
				ParserError ("unexpected `break`");
			
			w->Write (DH_GOTO);
			
			// switch and if use mark1 for the closing point,
			// for and while use mark2.
			switch (SCOPE(0).type) {
			case SCOPETYPE_IF:
			case SCOPETYPE_SWITCH:
				w->AddReference (SCOPE(0).mark1);
				break;
			case SCOPETYPE_FOR:
			case SCOPETYPE_WHILE:
				w->AddReference (SCOPE(0).mark2);
				break;
			default:
				ParserError ("unexpected `break`");
				break;
			}
			
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// Continue
		if (token == "continue") {
			MustNext (";");
			
			int curs;
			bool found = false;
			
			// Drop through the scope until we find a loop block
			for (curs = g_ScopeCursor; curs > 0 && !found; curs--) {
				switch (scopestack[curs].type) {
				case SCOPETYPE_FOR:
				case SCOPETYPE_WHILE:
				case SCOPETYPE_DO:
					w->Write (DH_GOTO);
					w->AddReference (scopestack[curs].mark1);
					found = true;
					break;
				default:
					break;
				}
			}
			
			// No loop blocks
			if (!found)
				ParserError ("`continue`-statement not inside a loop");
			
			continue;
		}
		
		// ============================================================
		// Label
		if (PeekNext() == ":") {
			MUST_NOT_TOPLEVEL
			
			// want no conflicts..
			if (IsKeyword (token))
				ParserError ("label name `%s` conflicts with keyword\n", token.chars());
			if (FindCommand (token))
				ParserError ("label name `%s` conflicts with command name\n", token.chars());
			if (FindGlobalVariable (token))
				ParserError ("label name `%s` conflicts with variable\n", token.chars());
			
			// See if a mark already exists for this label
			int mark = -1;
			for (int i = 0; i < MAX_MARKS; i++) {
				if (g_UndefinedLabels[i] && *g_UndefinedLabels[i] == token) {
					mark = i;
					w->MoveMark (i);
					
					// No longer undefinde
					delete g_UndefinedLabels[i];
					g_UndefinedLabels[i] = NULL;
				}
			}
			
			// Not found in unmarked lists, define it now
			if (mark == -1)
				w->AddMark (token);
			
			MustNext (":");
			continue;
		}
		
		// ============================================================
		if (token == "const") {
			constinfo_t info;
			
			// Get the type
			MustNext ();
			info.type = GetTypeByName (token);
			
			if (info.type == TYPE_UNKNOWN || info.type == TYPE_VOID)
				ParserError ("unknown type `%s` for constant", (char*)token);
			
			MustNext ();
			info.name = token;
			
			MustNext ("=");
			
			switch (info.type) {
			case TYPE_BOOL:
			case TYPE_INT:
				MustNumber (false);
				info.val = token;
				break;
			case TYPE_STRING:
				MustString ();
				info.val = token;
				break;
			case TYPE_UNKNOWN:
			case TYPE_VOID:
				break;
			}
			
			g_ConstInfo << info;
			
			MustNext (";");
			continue;
		}
		
		// ============================================================
		if (token == "}") {
			// Closing brace
			
			// If we're in the block stack, we're descending down from it now
			if (g_ScopeCursor > 0) {
				switch (SCOPE(0).type) {
				case SCOPETYPE_IF:
					// Adjust the closing mark.
					w->MoveMark (SCOPE(0).mark1);
					
					// We're returning from if, thus else can be next
					g_CanElse = true;
					break;
				case SCOPETYPE_ELSE:
					// else instead uses mark1 for itself (so if expression
					// fails, jump to else), mark2 means end of else
					w->MoveMark (SCOPE(0).mark2);
					break;
				case SCOPETYPE_FOR:
					// Write the incrementor at the end of the loop block
					w->WriteBuffer (SCOPE(0).buffer1);
					// fall-thru
				case SCOPETYPE_WHILE:
					// Write down the instruction to go back to the start of the loop
					w->Write (DH_GOTO);
					w->AddReference (SCOPE(0).mark1);
					
					// Move the closing mark here since we're at the end of the while loop
					w->MoveMark (SCOPE(0).mark2);
					break;
				case SCOPETYPE_DO: { 
					MustNext ("while");
					MustNext ("(");
					MustNext ();
					DataBuffer* expr = ParseExpression (TYPE_INT);
					MustNext (")");
					MustNext (";");
					
					// If the condition runs true, go back to the start.
					w->WriteBuffer (expr);
					w->Write (DH_IFGOTO);
					w->AddReference (SCOPE(0).mark1);
					break;
				}
				case SCOPETYPE_SWITCH: {
					// Switch closes. Move down to the record buffer of
					// the lower block.
					if (SCOPE(1).casecursor != -1)
						w->SwitchBuffer = SCOPE(1).casebuffers[SCOPE(1).casecursor];
					else
						w->SwitchBuffer = NULL;
					
					// If there was a default in the switch, write its header down now.
					// If not, write instruction to jump to the end of switch after
					// the headers (thus won't fall-through if no case matched)
					if (SCOPE(0).buffer1)
						w->WriteBuffer (SCOPE(0).buffer1);
					else {
						w->Write (DH_DROP);
						w->Write (DH_GOTO);
						w->AddReference (SCOPE(0).mark1);
					}
					
					// Go through all of the buffers we
					// recorded down and write them.
					for (unsigned int u = 0; u < MAX_CASE; u++) {
						if (!SCOPE(0).casebuffers[u])
							continue;
						
						w->MoveMark (SCOPE(0).casemarks[u]);
						w->WriteBuffer (SCOPE(0).casebuffers[u]);
					}
					
					// Move the closing mark here
					w->MoveMark (SCOPE(0).mark1);
					break;
				}
				case SCOPETYPE_UNKNOWN:
					break;
				}
				
				// Descend down the stack
				g_ScopeCursor--;
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
			
			if (PeekNext() == ";")
				MustNext (";");
			continue;
		}
		
		// Check if it's a command
		CommandDef* comm = FindCommand (token);
		if (comm) {
			w->GetCurrentBuffer()->Merge (ParseCommand (comm));
			MustNext (";");
			continue;
		}
		
		// ============================================================
		// If nothing else, parse it as a statement
		DataBuffer* b = ParseStatement (w);
		if (!b)
			ParserError ("unknown token `%s`", token.chars());
		
		w->WriteBuffer (b);
		MustNext (";");
	}
	
	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (g_CurMode != MODE_TOPLEVEL)
		ParserError ("script did not end at top level; did you forget a `}`?");
	
	// stateSpawn must be defined!
	if (!g_stateSpawnDefined)
		ParserError ("script must have a state named `stateSpawn`!");
	
	for (int i = 0; i < MAX_MARKS; i++)
		if (g_UndefinedLabels[i])
			ParserError ("label `%s` is referenced via `goto` but isn't defined\n", g_UndefinedLabels[i]->chars());
	
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
		if (token == ")") {
			if (curarg < comm->numargs)
				ParserError ("too few arguments passed to %s\n\tprototype: %s",
					comm->name.chars(), GetCommandPrototype (comm).chars());
			break;
			curarg++;
		}
		
		if (curarg >= comm->maxargs)
			ParserError ("too many arguments passed to %s\n\tprototype: %s",
				comm->name.chars(), GetCommandPrototype (comm).chars());
		
		r->Merge (ParseExpression (comm->argtypes[curarg]));
		MustNext ();
		
		if (curarg < comm->numargs - 1) {
			MustThis (",");
			MustNext ();
		} else if (curarg < comm->maxargs - 1) {
			// Can continue, but can terminate as well.
			if (token == ")") {
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
		r->Write (DH_PUSHNUMBER);
		r->Write (comm->defvals[curarg]);
		curarg++;
	}
	
	r->Write (DH_COMMAND);
	r->Write (comm->number);
	r->Write (comm->maxargs);
	
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
	case OPER_OR: return DH_ORLOGICAL;
	case OPER_AND: return DH_ANDLOGICAL;
	case OPER_BITWISEOR: return DH_ORBITWISE;
	case OPER_BITWISEEOR: return DH_EORBITWISE;
	case OPER_BITWISEAND: return DH_ANDBITWISE;
	}
	
	error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
DataBuffer* ScriptReader::ParseExpression (type_e reqtype) {
	DataBuffer* retbuf = new DataBuffer (64);
	
	// Parse first operand
	retbuf->Merge (ParseExprValue (reqtype));
	
	// Parse any and all operators we get
	int oper;
	while ((oper = ParseOperator (true)) != -1) {
		// We peeked the operator, move forward now
		Next ();
		
		// Can't be an assignement operator, those belong in assignments.
		if (IsAssignmentOperator (oper))
			ParserError ("assignment operator inside expression");
		
		// Parse the right operand.
		MustNext ();
		DataBuffer* rb = ParseExprValue (reqtype);
		
		if (oper == OPER_TERNARY) {
			// Ternary operator requires - naturally - a third operand.
			MustNext (":");
			MustNext ();
			DataBuffer* tb = ParseExprValue (reqtype);
			
			// It also is handled differently: there isn't a dataheader for ternary
			// operator. Instead, we abuse PUSHNUMBER and IFNOTGOTO for this.
			// Behold, big block of writing madness! :P
			int mark1 = retbuf->AddMark (""); // start of "else" case
			int mark2 = retbuf->AddMark (""); // end of expression
			retbuf->Write (DH_IFNOTGOTO); // if the first operand (condition)
			retbuf->AddMarkReference (mark1); // didn't eval true, jump into mark1
			retbuf->Merge (rb); // otherwise, perform second operand (true case)
			retbuf->Write (DH_GOTO); // afterwards, jump to the end, which is
			retbuf->AddMarkReference (mark2); // marked by mark2.
			retbuf->MoveMark (mark1); // move mark1 at the end of the true case
			retbuf->Merge (tb); // perform third operand (false case)
			retbuf->MoveMark (mark2); // move the ending mark2 here
		} else {
			// Write to buffer
			retbuf->Merge (rb);
			retbuf->Write (DataHeaderByOperator (NULL, oper));
		}
	}
	
	return retbuf;
}

// ============================================================================
// Parses an operator string. Returns the operator number code.
#define ISNEXT(char) (!PeekNext (peek ? 1 : 0) == char)
int ScriptReader::ParseOperator (bool peek) {
	str oper;
	if (peek)
		oper += PeekNext ();
	else
		oper += token;
	
	if (-oper == "strlen")
		return OPER_STRLEN;
	
	// Check one-char operators
	bool equalsnext = ISNEXT ("=");
	
	int o =	(oper == "=" && !equalsnext) ? OPER_ASSIGN :
		(oper == ">" && !equalsnext && !ISNEXT (">")) ? OPER_GREATERTHAN :
		(oper == "<" && !equalsnext && !ISNEXT ("<")) ? OPER_LESSTHAN :
		(oper == "&" && !ISNEXT ("&")) ? OPER_BITWISEAND :
		(oper == "|" && !ISNEXT ("|")) ? OPER_BITWISEOR :
		(oper == "+" && !equalsnext) ? OPER_ADD :
		(oper == "-" && !equalsnext) ? OPER_SUBTRACT :
		(oper == "*" && !equalsnext) ? OPER_MULTIPLY :
		(oper == "/" && !equalsnext) ? OPER_DIVIDE :
		(oper == "%" && !equalsnext) ? OPER_MODULUS :
		(oper == "^") ? OPER_BITWISEEOR :
		(oper == "?") ? OPER_TERNARY :
		-1;
	
	if (o != -1) {
		return o;
	}
	
	// Two-char operators
	oper += PeekNext (peek ? 1 : 0);
	equalsnext = PeekNext (peek ? 2 : 1) == ("=");
	
	o =	(oper == "+=") ? OPER_ASSIGNADD :
		(oper == "-=") ? OPER_ASSIGNSUB :
		(oper == "*=") ? OPER_ASSIGNMUL :
		(oper == "/=") ? OPER_ASSIGNDIV :
		(oper == "%=") ? OPER_ASSIGNMOD :
		(oper == "==") ? OPER_EQUALS :
		(oper == "!=") ? OPER_NOTEQUALS :
		(oper == ">=") ? OPER_GREATERTHANEQUALS :
		(oper == "<=") ? OPER_LESSTHANEQUALS :
		(oper == "&&") ? OPER_AND :
		(oper == "||") ? OPER_OR :
		(oper == "<<" && !equalsnext) ? OPER_LEFTSHIFT :
		(oper == ">>" && !equalsnext) ? OPER_RIGHTSHIFT :
		-1;
	
	if (o != -1) {
		MustNext ();
		return o;
	}
	
	// Three-char opers
	oper += PeekNext (peek ? 2 : 1);
	o =	oper == "<<=" ? OPER_ASSIGNLEFTSHIFT :
		oper == ">>=" ? OPER_ASSIGNRIGHTSHIFT :
		-1;
	
	if (o != -1) {
		MustNext ();
		MustNext ();
	}
	
	return o;
}

// ============================================================================
str ScriptReader::ParseFloat () {
	MustNumber (true);
	str floatstring = token;
	
	// Go after the decimal point
	if (PeekNext () == ".") {
		Next (".");
		MustNumber (false);
		floatstring += ".";
		floatstring += token;
	}
	
	return floatstring;
}

// ============================================================================
// Parses a value in the expression and returns the data needed to push
// it, contained in a data buffer. A value can be either a variable, a command,
// a literal or an expression.
DataBuffer* ScriptReader::ParseExprValue (type_e reqtype) {
	DataBuffer* b = new DataBuffer(16);
	
	ScriptVar* g;
	
	// Prefixing "!" means negation.
	bool negate = (token == "!");
	if (negate) // Jump past the "!"
		Next ();
	
	// Handle strlen
	if (token == "strlen") {
		MustNext ("(");
		MustNext ();
		
		// By this token we should get a string constant.
		constinfo_t* constant = FindConstant (token);
		if (!constant || constant->type != TYPE_STRING)
			ParserError ("strlen only works with const str");
		
		if (reqtype != TYPE_INT)
			ParserError ("strlen returns int but %s is expected\n", (char*)GetTypeName (reqtype));
		
		b->Write (DH_PUSHNUMBER);
		b->Write (constant->val.len ());
		
		MustNext (")");
	} else if (token == "(") {
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
	} else if (constinfo_t* constant = FindConstant (token)) {
		// Type check
		if (reqtype != constant->type)
			ParserError ("constant `%s` is %s, expression requires %s\n",
				(char*)constant->name, (char*)GetTypeName (constant->type),
				(char*)GetTypeName (reqtype));
		
		switch (constant->type) {
		case TYPE_BOOL:
		case TYPE_INT:
			b->Write (DH_PUSHNUMBER);
			b->Write (atoi (constant->val));
			break;
		case TYPE_STRING:
			b->WriteString (constant->val);
			break;
		case TYPE_VOID:
		case TYPE_UNKNOWN:
			break;
		}
	} else if ((g = FindGlobalVariable (token))) {
		// Global variable
		b->Write (DH_PUSHGLOBALVAR);
		b->Write (g->index);
	} else {
		// If nothing else, check for literal
		switch (reqtype) {
		case TYPE_VOID:
		case TYPE_UNKNOWN:
			ParserError ("unknown identifier `%s` (expected keyword, function or variable)", token.chars());
			break;
		case TYPE_BOOL:
		case TYPE_INT: {
			MustNumber (true);
			
			// All values are written unsigned - thus we need to write the value's
			// absolute value, followed by an unary minus for negatives.
			b->Write (DH_PUSHNUMBER);
			
			long v = atol (token);
			b->Write (static_cast<word> (abs (v)));
			if (v < 0)
				b->Write (DH_UNARYMINUS);
			break;
		}
		case TYPE_STRING:
			// PushToStringTable either returns the string index of the
			// string if it finds it in the table, or writes it to the
			// table and returns it index if it doesn't find it there.
			MustString (true);
			b->WriteString (token);
			break;
		}
	}
	
	// Negate it now if desired
	if (negate)
		b->Write (DH_NEGATELOGICAL);
	
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
	DataBuffer* expr = ParseExpression (var->type);
	
	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	if (oper == OPER_ASSIGNLEFTSHIFT || oper == OPER_ASSIGNRIGHTSHIFT) {
		retbuf->Write (global ? DH_PUSHGLOBALVAR : DH_PUSHLOCALVAR);
		retbuf->Write (var->index);
		retbuf->Merge (expr);
		retbuf->Write ((oper == OPER_ASSIGNLEFTSHIFT) ? DH_LSHIFT : DH_RSHIFT);
		retbuf->Write (global ? DH_ASSIGNGLOBALVAR : DH_ASSIGNLOCALVAR);
		retbuf->Write (var->index);
	} else {
		retbuf->Merge (expr);
		long dh = DataHeaderByOperator (var, oper);
		retbuf->Write (dh);
		retbuf->Write (var->index);
	}
	
	return retbuf;
}

void ScriptReader::PushScope () {
	g_ScopeCursor++;
	if (g_ScopeCursor >= MAX_SCOPE)
		ParserError ("too deep scope");
	
	ScopeInfo* info = &SCOPE(0);
	info->type = SCOPETYPE_UNKNOWN;
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
	if (FindConstant (token)) // There should not be constants here.
		ParserError ("invalid use for constant\n");
	
	// If it's a variable, expect assignment.
	if (ScriptVar* var = FindGlobalVariable (token))
		return ParseAssignment (var);
	
	return NULL;
}

void ScriptReader::AddSwitchCase (ObjWriter* w, DataBuffer* b) {
	ScopeInfo* info = &SCOPE(0);
	
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

constinfo_t* FindConstant (str token) {
	for (uint i = 0; i < g_ConstInfo.size(); i++)
		if (g_ConstInfo[i].name == token)
			return &g_ConstInfo[i];
	return NULL;
}