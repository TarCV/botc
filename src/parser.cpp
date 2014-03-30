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

#include "parser.h"
#include "events.h"
#include "commands.h"
#include "stringTable.h"
#include "list.h"
#include "lexer.h"
#include "dataBuffer.h"
#include "expression.h"

#define SCOPE(n) (m_scopeStack[m_scopeCursor - n])

static const StringList g_validZandronumVersions = {"1.2", "1.3", "2.0"};

// ============================================================================
//
BotscriptParser::BotscriptParser() :
	m_isReadOnly (false),
	m_mainBuffer (new DataBuffer),
	m_onenterBuffer (new DataBuffer),
	m_mainLoopBuffer (new DataBuffer),
	m_lexer (new Lexer),
	m_numStates (0),
	m_numEvents (0),
	m_currentMode (PARSERMODE_TopLevel),
	m_isStateSpawnDefined (false),
	m_gotMainLoop (false),
	m_scopeCursor (-1),
	m_isElseAllowed (false),
	m_highestGlobalVarIndex (0),
	m_highestStateVarIndex (0),
	m_zandronumVersion (10200), // 1.2
	m_defaultZandronumVersion (true) {}

// ============================================================================
//
BotscriptParser::~BotscriptParser()
{
	delete m_lexer;
}

// ============================================================================
//
void BotscriptParser::checkToplevel()
{
	if (m_currentMode != PARSERMODE_TopLevel)
		error ("%1-statements may only be defined at top level!", getTokenString());
}

// ============================================================================
//
void BotscriptParser::checkNotToplevel()
{
	if (m_currentMode == PARSERMODE_TopLevel)
		error ("%1-statements must not be defined at top level!", getTokenString());
}

// ============================================================================
//
// Main compiler code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
//
void BotscriptParser::parseBotscript (String fileName)
{
	// Lex and preprocess the file
	m_lexer->processFile (fileName);
	pushScope();

	while (m_lexer->next())
	{
		// Check if else is potentically valid
		if (tokenIs (TK_Else) && m_isElseAllowed == false)
			error ("else without preceding if");

		if (tokenIs (TK_Else) == false)
			m_isElseAllowed = false;

		switch (m_lexer->token()->type)
		{
			case TK_State:
				parseStateBlock();
				break;

			case TK_Event:
				parseEventBlock();
				break;

			case TK_Mainloop:
				parseMainloop();
				break;

			case TK_Onenter:
			case TK_Onexit:
				parseOnEnterExit();
				break;

			case TK_Var:
				parseVar();
				break;

			case TK_If:
				parseIf();
				break;

			case TK_Else:
				parseElse();
				break;

			case TK_While:
				parseWhileBlock();
				break;

			case TK_For:
				parseForBlock();
				break;

			case TK_Do:
				parseDoBlock();
				break;

			case TK_Switch:
				parseSwitchBlock();
				break;

			case TK_Case:
				parseSwitchCase();
				break;

			case TK_Default:
				parseSwitchDefault();
				break;

			case TK_Break:
				parseBreak();
				break;

			case TK_Continue:
				parseContinue();
				break;

			case TK_BraceEnd:
				parseBlockEnd();
				break;

			case TK_Eventdef:
				parseEventdef();
				break;

			case TK_Funcdef:
				parseFuncdef();
				break;

			case TK_Semicolon:
				break;

			case TK_Using:
				parseUsing();
				break;

			default:
			{
				// Check if it's a command
				CommandInfo* comm = findCommandByName (getTokenString());

				if (comm)
				{
					currentBuffer()->mergeAndDestroy (parseCommand (comm));
					m_lexer->mustGetNext (TK_Semicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				m_lexer->skip (-1);
				DataBuffer* b = parseStatement();

				if (b == false)
				{
					m_lexer->next();
					error ("unknown token `%1`", getTokenString());
				}

				currentBuffer()->mergeAndDestroy (b);
				m_lexer->mustGetNext (TK_Semicolon);
				break;
			}
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (m_currentMode != PARSERMODE_TopLevel)
		error ("script did not end at top level; a `}` is missing somewhere");

	if (isReadOnly() == false)
	{
		// stateSpawn must be defined!
		if (m_isStateSpawnDefined == false)
			error ("script must have a state named `stateSpawn`!");

		if (m_defaultZandronumVersion)
		{
			print ("\n");
			print ("note: use the 'using' directive to define a target Zandronum version\n");
			print ("usage: using zandronum <version>, possible versions: %1\n", g_validZandronumVersions);
			print ("\n");
		}

		// Dump the last state's onenter and mainloop
		writeMemberBuffers();

		// String table
		writeStringTable();
	}
}

// ============================================================================
//
void BotscriptParser::parseStateBlock()
{
	checkToplevel();
	m_lexer->mustGetNext (TK_String);
	String statename = getTokenString();

	// State name must be a word.
	if (statename.firstIndexOf (" ") != -1)
		error ("state name must be a single word, got `%1`", statename);

	// stateSpawn is special - it *must* be defined. If we
	// encountered it, then mark down that we have it.
	if (statename.toLowercase() == "statespawn")
		m_isStateSpawnDefined = true;

	// Must end in a colon
	m_lexer->mustGetNext (TK_Colon);

	// write the previous state's onenter and
	// mainloop buffers to file now
	if (m_currentState.isEmpty() == false)
		writeMemberBuffers();

	currentBuffer()->writeDWord (DH_StateName);
	currentBuffer()->writeString (statename);
	currentBuffer()->writeDWord (DH_StateIndex);
	currentBuffer()->writeDWord (m_numStates);

	m_numStates++;
	m_currentState = statename;
	m_gotMainLoop = false;
}

// ============================================================================
//
void BotscriptParser::parseEventBlock()
{
	checkToplevel();
	m_lexer->mustGetNext (TK_String);

	EventDefinition* e = findEventByName (getTokenString());

	if (e == null)
		error ("bad event, got `%1`\n", getTokenString());

	m_lexer->mustGetNext (TK_BraceStart);
	m_currentMode = PARSERMODE_Event;
	currentBuffer()->writeDWord (DH_Event);
	currentBuffer()->writeDWord (e->number);
	m_numEvents++;
}

// ============================================================================
//
void BotscriptParser::parseMainloop()
{
	checkToplevel();
	m_lexer->mustGetNext (TK_BraceStart);

	m_currentMode = PARSERMODE_MainLoop;
	m_mainLoopBuffer->writeDWord (DH_MainLoop);
}

// ============================================================================
//
void BotscriptParser::parseOnEnterExit()
{
	checkToplevel();
	bool onenter = (tokenIs (TK_Onenter));
	m_lexer->mustGetNext (TK_BraceStart);

	m_currentMode = onenter ? PARSERMODE_Onenter : PARSERMODE_Onexit;
	currentBuffer()->writeDWord (onenter ? DH_OnEnter : DH_OnExit);
}

// ============================================================================
//
void BotscriptParser::parseVar()
{
	Variable* var = new Variable;
	var->origin = m_lexer->describeCurrentPosition();
	var->isarray = false;
	const bool isconst = m_lexer->next (TK_Const);
	m_lexer->mustGetAnyOf ({TK_Int,TK_Str,TK_Void});

	DataType vartype =	(tokenIs (TK_Int)) ? TYPE_Int :
					(tokenIs (TK_Str)) ? TYPE_String :
					TYPE_Bool;

	m_lexer->mustGetNext (TK_DollarSign);
	m_lexer->mustGetNext (TK_Symbol);
	String name = getTokenString();

	if (m_lexer->next (TK_BracketStart))
	{
		m_lexer->mustGetNext (TK_BracketEnd);
		var->isarray = true;

		if (isconst)
			error ("arrays cannot be const");
	}

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
	{
		if (var->name == name)
			error ("Variable $%1 is already declared on this scope; declared at %2",
				var->name, var->origin);
	}

	var->name = name;
	var->statename = "";
	var->type = vartype;

	if (isconst == false)
	{
		var->writelevel = WRITE_Mutable;
	}
	else
	{
		m_lexer->mustGetNext (TK_Assign);
		Expression expr (this, m_lexer, vartype);

		// If the expression was constexpr, we know its value and thus
		// can store it in the variable.
		if (expr.getResult()->isConstexpr())
		{
			var->writelevel = WRITE_Constexpr;
			var->value = expr.getResult()->value();
		}
		else
		{
			// TODO: might need a VM-wise oninit for this...
			error ("const variables must be constexpr");
		}
	}

	// Assign an index for the variable if it is not constexpr. Constexpr
	// variables can simply be substituted out for their value when used
	// so they need no index.
	if (var->writelevel != WRITE_Constexpr)
	{
		bool isglobal = isInGlobalState();
		var->index = isglobal ? SCOPE(0).globalVarIndexBase++ : SCOPE(0).localVarIndexBase++;

		if ((isglobal == true && var->index >= gMaxGlobalVars) ||
			(isglobal == false && var->index >= gMaxStateVars))
		{
			error ("too many %1 variables", isglobal ? "global" : "state-local");
		}
	}

	if (isInGlobalState())
		SCOPE(0).globalVariables << var;
	else
		SCOPE(0).localVariables << var;

	suggestHighestVarIndex (isInGlobalState(), var->index);
	m_lexer->mustGetNext (TK_Semicolon);
	print ("Declared %3 variable #%1 $%2\n", var->index, var->name, isInGlobalState() ? "global" : "state-local");
}

// ============================================================================
//
void BotscriptParser::parseIf()
{
	checkNotToplevel();
	pushScope();

	// Condition
	m_lexer->mustGetNext (TK_ParenStart);

	// Read the expression and write it.
	DataBuffer* c = parseExpression (TYPE_Int);
	currentBuffer()->mergeAndDestroy (c);

	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_BraceStart);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	ByteMark* mark = currentBuffer()->addMark ("");

	// Use DH_IfNotGoto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	currentBuffer()->writeDWord (DH_IfNotGoto);
	currentBuffer()->addReference (mark);

	// Store it
	SCOPE (0).mark1 = mark;
	SCOPE (0).type = SCOPE_If;
}

// ============================================================================
//
void BotscriptParser::parseElse()
{
	checkNotToplevel();
	m_lexer->mustGetNext (TK_BraceStart);
	pushScope (eNoReset);

	if (SCOPE (0).type != SCOPE_If)
		error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = currentBuffer()->addMark ("");

	// Instruction to jump to the end after if block is complete
	currentBuffer()->writeDWord (DH_Goto);
	currentBuffer()->addReference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	currentBuffer()->adjustMark (SCOPE (0).mark1);
	SCOPE (0).type = SCOPE_Else;
}

// ============================================================================
//
void BotscriptParser::parseWhileBlock()
{
	checkNotToplevel();
	pushScope();

	// While loops need two marks - one at the start of the loop and one at the
	// end. The condition is checked at the very start of the loop, if it fails,
	// we use goto to skip to the end of the loop. At the end, we loop back to
	// the beginning with a go-to statement.
	ByteMark* mark1 = currentBuffer()->addMark (""); // start
	ByteMark* mark2 = currentBuffer()->addMark (""); // end

	// Condition
	m_lexer->mustGetNext (TK_ParenStart);
	DataBuffer* expr = parseExpression (TYPE_Int);
	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_BraceStart);

	// write condition
	currentBuffer()->mergeAndDestroy (expr);

	// Instruction to go to the end if it fails
	currentBuffer()->writeDWord (DH_IfNotGoto);
	currentBuffer()->addReference (mark2);

	// Store the needed stuff
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).type = SCOPE_While;
}

// ============================================================================
//
void BotscriptParser::parseForBlock()
{
	checkNotToplevel();
	pushScope();

	// Initializer
	m_lexer->mustGetNext (TK_ParenStart);
	DataBuffer* init = parseStatement();

	if (init == null)
		error ("bad statement for initializer of for");

	m_lexer->mustGetNext (TK_Semicolon);

	// Condition
	DataBuffer* cond = parseExpression (TYPE_Int);

	if (cond == null)
		error ("bad statement for condition of for");

	m_lexer->mustGetNext (TK_Semicolon);

	// Incrementor
	DataBuffer* incr = parseStatement();

	if (incr == null)
		error ("bad statement for incrementor of for");

	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_BraceStart);

	// First, write out the initializer
	currentBuffer()->mergeAndDestroy (init);

	// Init two marks
	ByteMark* mark1 = currentBuffer()->addMark ("");
	ByteMark* mark2 = currentBuffer()->addMark ("");

	// Add the condition
	currentBuffer()->mergeAndDestroy (cond);
	currentBuffer()->writeDWord (DH_IfNotGoto);
	currentBuffer()->addReference (mark2);

	// Store the marks and incrementor
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).buffer1 = incr;
	SCOPE (0).type = SCOPE_For;
}

// ============================================================================
//
void BotscriptParser::parseDoBlock()
{
	checkNotToplevel();
	pushScope();
	m_lexer->mustGetNext (TK_BraceStart);
	SCOPE (0).mark1 = currentBuffer()->addMark ("");
	SCOPE (0).type = SCOPE_Do;
}

// ============================================================================
//
void BotscriptParser::parseSwitchBlock()
{
	// This gets a bit tricky. switch is structured in the
	// bytecode followingly:
	//
	// (expression)
	// case a: goto casemark1
	// case b: goto casemark2
	// case c: goto casemark3
	// goto mark1 // jump to end if no matches
	// casemark1: ...
	// casemark2: ...
	// casemark3: ...
	// mark1: // end mark

	checkNotToplevel();
	pushScope();
	m_lexer->mustGetNext (TK_ParenStart);
	currentBuffer()->mergeAndDestroy (parseExpression (TYPE_Int));
	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_BraceStart);
	SCOPE (0).type = SCOPE_Switch;
	SCOPE (0).mark1 = currentBuffer()->addMark (""); // end mark
	SCOPE (0).buffer1 = null; // default header
}

// ============================================================================
//
void BotscriptParser::parseSwitchCase()
{
	// case is only allowed inside switch
	if (SCOPE (0).type != SCOPE_Switch)
		error ("case label outside switch");

	// Get a literal value for the case block. Zandronum does not support
	// expressions here.
	m_lexer->mustGetNext (TK_Number);
	int num = m_lexer->token()->text.toLong();
	m_lexer->mustGetNext (TK_Colon);

	for (const CaseInfo& info : SCOPE(0).cases)
		if (info.number == num)
			error ("multiple case %1 labels in one switch", num);

	// Write down the expression and case-go-to. This builds
	// the case tree. The closing event will write the actual
	// blocks and move the marks appropriately.
	//
	// AddSwitchCase will add the reference to the mark
	// for the case block that this heralds, and takes care
	// of buffering setup and stuff like that.
	//
	// We null the switch buffer for the case-go-to statement as
	// we want it all under the switch, not into the case-buffers.
	m_switchBuffer = null;
	currentBuffer()->writeDWord (DH_CaseGoto);
	currentBuffer()->writeDWord (num);
	addSwitchCase (null);
	SCOPE (0).casecursor->number = num;
}

// ============================================================================
//
void BotscriptParser::parseSwitchDefault()
{
	if (SCOPE (0).type != SCOPE_Switch)
		error ("default label outside switch");

	if (SCOPE (0).buffer1 != null)
		error ("multiple default labels in one switch");

	m_lexer->mustGetNext (TK_Colon);

	// The default header is buffered into buffer1, since
	// it has to be the last of the case headers
	//
	// Since the expression is pushed into the switch
	// and is only popped when case succeeds, we have
	// to pop it with DH_Drop manually if we end up in
	// a default.
	DataBuffer* buf = new DataBuffer;
	SCOPE (0).buffer1 = buf;
	buf->writeDWord (DH_Drop);
	buf->writeDWord (DH_Goto);
	addSwitchCase (buf);
}

// ============================================================================
//
void BotscriptParser::parseBreak()
{
	if (m_scopeCursor == 0)
		error ("unexpected `break`");

	currentBuffer()->writeDWord (DH_Goto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case SCOPE_If:
		case SCOPE_Switch:
		{
			currentBuffer()->addReference (SCOPE (0).mark1);
		} break;

		case SCOPE_For:
		case SCOPE_While:
		{
			currentBuffer()->addReference (SCOPE (0).mark2);
		} break;

		default:
		{
			error ("unexpected `break`");
		} break;
	}

	m_lexer->mustGetNext (TK_Semicolon);
}

// ============================================================================
//
void BotscriptParser::parseContinue()
{
	m_lexer->mustGetNext (TK_Semicolon);

	int curs;
	bool found = false;

	// Fall through the scope until we find a loop block
	for (curs = m_scopeCursor; curs > 0 && !found; curs--)
	{
		switch (m_scopeStack[curs].type)
		{
			case SCOPE_For:
			case SCOPE_While:
			case SCOPE_Do:
			{
				currentBuffer()->writeDWord (DH_Goto);
				currentBuffer()->addReference (m_scopeStack[curs].mark1);
				found = true;
			} break;

			default:
				break;
		}
	}

	// No loop blocks
	if (found == false)
		error ("`continue`-statement not inside a loop");
}

// ============================================================================
//
void BotscriptParser::parseBlockEnd()
{
	// Closing brace
	// If we're in the block stack, we're descending down from it now
	if (m_scopeCursor > 0)
	{
		switch (SCOPE (0).type)
		{
			case SCOPE_If:
			{
				// Adjust the closing mark.
				currentBuffer()->adjustMark (SCOPE (0).mark1);

				// We're returning from `if`, thus `else` follow
				m_isElseAllowed = true;
				break;
			}

			case SCOPE_Else:
			{
				// else instead uses mark1 for itself (so if expression
				// fails, jump to else), mark2 means end of else
				currentBuffer()->adjustMark (SCOPE (0).mark2);
				break;
			}

			case SCOPE_For:
			{	// write the incrementor at the end of the loop block
				currentBuffer()->mergeAndDestroy (SCOPE (0).buffer1);
			}
			case SCOPE_While:
			{	// write down the instruction to go back to the start of the loop
				currentBuffer()->writeDWord (DH_Goto);
				currentBuffer()->addReference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				currentBuffer()->adjustMark (SCOPE (0).mark2);
				break;
			}

			case SCOPE_Do:
			{
				m_lexer->mustGetNext (TK_While);
				m_lexer->mustGetNext (TK_ParenStart);
				DataBuffer* expr = parseExpression (TYPE_Int);
				m_lexer->mustGetNext (TK_ParenEnd);
				m_lexer->mustGetNext (TK_Semicolon);

				// If the condition runs true, go back to the start.
				currentBuffer()->mergeAndDestroy (expr);
				currentBuffer()->writeDWord (DH_IfGoto);
				currentBuffer()->addReference (SCOPE (0).mark1);
				break;
			}

			case SCOPE_Switch:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != SCOPE (1).cases.begin() - 1)
					m_switchBuffer = SCOPE (1).casecursor->data;
				else
					m_switchBuffer = null;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					currentBuffer()->mergeAndDestroy (SCOPE (0).buffer1);
				else
				{
					currentBuffer()->writeDWord (DH_Drop);
					currentBuffer()->writeDWord (DH_Goto);
					currentBuffer()->addReference (SCOPE (0).mark1);
				}

				// Go through all of the buffers we
				// recorded down and write them.
				for (CaseInfo& info : SCOPE (0).cases)
				{
					currentBuffer()->adjustMark (info.mark);
					currentBuffer()->mergeAndDestroy (info.data);
				}

				// Move the closing mark here
				currentBuffer()->adjustMark (SCOPE (0).mark1);
				break;
			}

			case SCOPE_Unknown:
				break;
		}

		// Descend down the stack
		m_scopeCursor--;
		return;
	}

	int dataheader =	(m_currentMode == PARSERMODE_Event) ? DH_EndEvent :
						(m_currentMode == PARSERMODE_MainLoop) ? DH_EndMainLoop :
						(m_currentMode == PARSERMODE_Onenter) ? DH_EndOnEnter :
						(m_currentMode == PARSERMODE_Onexit) ? DH_EndOnExit : -1;

	if (dataheader == -1)
		error ("unexpected `}`");

	// Data header must be written before mode is changed because
	// onenter and mainloop go into special buffers, and we want
	// the closing data headers into said buffers too.
	currentBuffer()->writeDWord (dataheader);
	m_currentMode = PARSERMODE_TopLevel;
	m_lexer->next (TK_Semicolon);
}

// =============================================================================
//
void BotscriptParser::parseEventdef()
{
	EventDefinition* e = new EventDefinition;

	m_lexer->mustGetNext (TK_Number);
	e->number = getTokenString().toLong();
	m_lexer->mustGetNext (TK_Colon);
	m_lexer->mustGetNext (TK_Symbol);
	e->name = m_lexer->token()->text;
	m_lexer->mustGetNext (TK_ParenStart);
	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_Semicolon);
	addEvent (e);
}

// =============================================================================
//
void BotscriptParser::parseFuncdef()
{
	CommandInfo* comm = new CommandInfo;
	comm->origin = m_lexer->describeCurrentPosition();

	// Return value
	m_lexer->mustGetAnyOf ({TK_Int,TK_Void,TK_Bool,TK_Str});
	comm->returnvalue = getTypeByName (m_lexer->token()->text); // TODO
	assert (comm->returnvalue != -1);

	// Number
	m_lexer->mustGetNext (TK_Number);
	comm->number = m_lexer->token()->text.toLong();
	m_lexer->mustGetNext (TK_Colon);

	// Name
	m_lexer->mustGetNext (TK_Symbol);
	comm->name = m_lexer->token()->text;

	// Arguments
	m_lexer->mustGetNext (TK_ParenStart);
	comm->minargs = 0;

	while (m_lexer->peekNextType (TK_ParenEnd) == false)
	{
		if (comm->args.isEmpty() == false)
			m_lexer->mustGetNext (TK_Comma);

		CommandArgument arg;
		m_lexer->mustGetAnyOf ({TK_Int,TK_Bool,TK_Str});
		DataType type = getTypeByName (m_lexer->token()->text); // TODO
		assert (type != -1 && type != TYPE_Void);
		arg.type = type;

		m_lexer->mustGetNext (TK_Symbol);
		arg.name = m_lexer->token()->text;

		// If this is an optional parameter, we need the default value.
		if (comm->minargs < comm->args.size() || m_lexer->peekNextType (TK_Assign))
		{
			m_lexer->mustGetNext (TK_Assign);

			switch (type)
			{
				case TYPE_Int:
				case TYPE_Bool:
					m_lexer->mustGetNext (TK_Number);
					break;

				case TYPE_String:
					error ("string arguments cannot have default values");

				case TYPE_Unknown:
				case TYPE_Void:
					break;
			}

			arg.defvalue = m_lexer->token()->text.toLong();
		}
		else
			comm->minargs++;

		comm->args << arg;
	}

	m_lexer->mustGetNext (TK_ParenEnd);
	m_lexer->mustGetNext (TK_Semicolon);
	addCommandDefinition (comm);
}

// ============================================================================
//
// Parses a using statement
//
void BotscriptParser::parseUsing()
{
	checkToplevel();
	m_lexer->mustGetSymbol ("zandronum");
	String versionText;

	while (m_lexer->next() && (m_lexer->tokenType() == TK_Number || m_lexer->tokenType() == TK_Dot))
		versionText += getTokenString();

	// Note: at this point the lexer's pointing at the token after the version.
	if (versionText.isEmpty())
		error ("expected version string, got `%1`", getTokenString());
	if (g_validZandronumVersions.contains (versionText) == false)
		error ("unknown version string `%2`: valid versions: `%1`\n", g_validZandronumVersions, versionText);

	StringList versionTokens = versionText.split (".");
	m_zandronumVersion = versionTokens[0].toLong() * 10000 + versionTokens[1].toLong() * 100;
	m_defaultZandronumVersion = false;
	m_lexer->tokenMustBe (TK_Semicolon);
}

// ============================================================================/
//
// Parses a command call
//
DataBuffer* BotscriptParser::parseCommand (CommandInfo* comm)
{
	DataBuffer* r = new DataBuffer (64);

	if (m_currentMode == PARSERMODE_TopLevel && comm->returnvalue == TYPE_Void)
		error ("command call at top level");

	m_lexer->mustGetNext (TK_ParenStart);
	m_lexer->mustGetNext (TK_Any);

	int curarg = 0;

	for (;;)
	{
		if (tokenIs (TK_ParenEnd))
		{
			if (curarg < comm->minargs)
				error ("too few arguments passed to %1\n\tusage is: %2",
					comm->name, comm->signature());

			break;
		}

		if (curarg >= comm->args.size())
			error ("too many arguments (%3) passed to %1\n\tusage is: %2",
				comm->name, comm->signature());

		r->mergeAndDestroy (parseExpression (comm->args[curarg].type, true));
		m_lexer->mustGetNext (TK_Any);

		if (curarg < comm->minargs - 1)
		{
			m_lexer->tokenMustBe (TK_Comma);
			m_lexer->mustGetNext (TK_Any);
		}
		else if (curarg < comm->args.size() - 1)
		{
			// Can continue, but can terminate as well.
			if (tokenIs (TK_ParenEnd))
			{
				curarg++;
				break;
			}
			else
			{
				m_lexer->tokenMustBe (TK_Comma);
				m_lexer->mustGetNext (TK_Any);
			}
		}

		curarg++;
	}

	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->args.size())
	{
		r->writeDWord (DH_PushNumber);
		r->writeDWord (comm->args[curarg].defvalue);
		curarg++;
	}

	r->writeDWord (DH_Command);
	r->writeDWord (comm->number);
	r->writeDWord (comm->args.size());

	return r;
}

// ============================================================================
//
String BotscriptParser::parseFloat()
{
	m_lexer->tokenMustBe (TK_Number);
	String floatstring = getTokenString();
	Lexer::TokenInfo tok;

	// Go after the decimal point
	if (m_lexer->peekNext (&tok) && tok.type ==TK_Dot)
	{
		m_lexer->skip();
		m_lexer->mustGetNext (TK_Number);
		floatstring += ".";
		floatstring += getTokenString();
	}

	return floatstring;
}

// ============================================================================
//
// Parses an assignment operator.
//
AssignmentOperator BotscriptParser::parseAssignmentOperator()
{
	const List<ETokenType> tokens =
	{
		TK_Assign,
		TK_AddAssign,
		TK_SubAssign,
		TK_MultiplyAssign,
		TK_DivideAssign,
		TK_ModulusAssign,
		TK_DoublePlus,
		TK_DoubleMinus,
	};

	m_lexer->mustGetAnyOf (tokens);

	switch (m_lexer->tokenType())
	{
		case TK_Assign:			return ASSIGNOP_Assign;
		case TK_AddAssign:		return ASSIGNOP_Add;
		case TK_SubAssign:		return ASSIGNOP_Subtract;
		case TK_MultiplyAssign:	return ASSIGNOP_Multiply;
		case TK_DivideAssign:	return ASSIGNOP_Divide;
		case TK_ModulusAssign:	return ASSIGNOP_Modulus;
		case TK_DoublePlus:		return ASSIGNOP_Increase;
		case TK_DoubleMinus:	return ASSIGNOP_Decrease;
		default: break;
	}

	assert (false);
	return (AssignmentOperator) 0;
}

// ============================================================================
//
struct AssignmentDataHeaderInfo
{
	AssignmentOperator	op;
	DataHeader			local;
	DataHeader			global;
	DataHeader			array;
};

const AssignmentDataHeaderInfo gAssignmentDataHeaders[] =
{
	{ ASSIGNOP_Assign,		DH_AssignLocalVar,		DH_AssignGlobalVar,		DH_AssignGlobalArray },
	{ ASSIGNOP_Add,			DH_AddLocalVar,			DH_AddGlobalVar,		DH_AddGlobalArray },
	{ ASSIGNOP_Subtract,	DH_SubtractLocalVar,	DH_SubtractGlobalVar,	DH_SubtractGlobalArray },
	{ ASSIGNOP_Multiply,	DH_MultiplyLocalVar,	DH_MultiplyGlobalVar,	DH_MultiplyGlobalArray },
	{ ASSIGNOP_Divide,		DH_DivideLocalVar,		DH_DivideGlobalVar,		DH_DivideGlobalArray },
	{ ASSIGNOP_Modulus,		DH_ModLocalVar,			DH_ModGlobalVar,		DH_ModGlobalArray },
	{ ASSIGNOP_Increase,	DH_IncreaseLocalVar,	DH_IncreaseGlobalVar,	DH_IncreaseGlobalArray },
	{ ASSIGNOP_Decrease,	DH_DecreaseLocalVar,	DH_DecreaseGlobalVar,	DH_DecreaseGlobalArray },
};

DataHeader BotscriptParser::getAssigmentDataHeader (AssignmentOperator op, Variable* var)
{
	for (const auto& a : gAssignmentDataHeaders)
	{
		if (a.op != op)
			continue;

		if (var->isarray)
			return a.array;

		if (var->IsGlobal())
			return a.global;

		return a.local;
	}

	error ("WTF: couldn't find data header for operator #%1", op);
	return (DataHeader) 0;
}

// ============================================================================
//
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
//
DataBuffer* BotscriptParser::parseAssignment (Variable* var)
{
	DataBuffer* retbuf = new DataBuffer;
	DataBuffer* arrayindex = null;

	if (var->writelevel != WRITE_Mutable)
		error ("cannot alter read-only variable $%1", var->name);

	if (var->isarray)
	{
		m_lexer->mustGetNext (TK_BracketStart);
		Expression expr (this, m_lexer, TYPE_Int);
		expr.getResult()->convertToBuffer();
		arrayindex = expr.getResult()->buffer()->clone();
		m_lexer->mustGetNext (TK_BracketEnd);
	}

	// Get an operator
	AssignmentOperator oper = parseAssignmentOperator();

	if (m_currentMode == PARSERMODE_TopLevel)
		error ("can't alter variables at top level");

	if (var->isarray)
		retbuf->mergeAndDestroy (arrayindex);

	// Parse the right operand
	if (oper != ASSIGNOP_Increase && oper != ASSIGNOP_Decrease)
	{
		DataBuffer* expr = parseExpression (var->type);
		retbuf->mergeAndDestroy (expr);
	}

#if 0
	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	retbuf->WriteDWord (var->IsGlobal() ? DH_PushGlobalVar : DH_PushLocalVar);
	retbuf->WriteDWord (var->index);
	retbuf->MergeAndDestroy (expr);
	retbuf->WriteDWord ((oper == OPER_ASSIGNLEFTSHIFT) ? DH_LeftShift : DH_RightShift);
	retbuf->WriteDWord (var->IsGlobal() ? DH_AssignGlobalVar : DH_AssignLocalVar);
	retbuf->WriteDWord (var->index);
#endif

	DataHeader dh = getAssigmentDataHeader (oper, var);
	retbuf->writeDWord (dh);
	retbuf->writeDWord (var->index);
	return retbuf;
}

// ============================================================================
//
void BotscriptParser::pushScope (EReset reset)
{
	m_scopeCursor++;

	if (m_scopeStack.size() < m_scopeCursor + 1)
	{
		ScopeInfo newscope;
		m_scopeStack << newscope;
		reset = SCOPE_Reset;
	}

	if (reset == SCOPE_Reset)
	{
		ScopeInfo* info = &SCOPE (0);
		info->type = SCOPE_Unknown;
		info->mark1 = null;
		info->mark2 = null;
		info->buffer1 = null;
		info->cases.clear();
		info->casecursor = info->cases.begin() - 1;
	}

	// Reset variable stuff in any case
	SCOPE(0).globalVarIndexBase = (m_scopeCursor == 0) ? 0 : SCOPE(1).globalVarIndexBase;
	SCOPE(0).localVarIndexBase = (m_scopeCursor == 0) ? 0 : SCOPE(1).localVarIndexBase;

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
		delete var;

	SCOPE(0).localVariables.clear();
	SCOPE(0).globalVariables.clear();
}

// ============================================================================
//
DataBuffer* BotscriptParser::parseExpression (DataType reqtype, bool fromhere)
{
	// hehe
	if (fromhere == true)
		m_lexer->skip (-1);

	Expression expr (this, m_lexer, reqtype);
	expr.getResult()->convertToBuffer();

	// The buffer will be destroyed once the function ends so we need to
	// clone it now.
	return expr.getResult()->buffer()->clone();
}

// ============================================================================
//
DataBuffer* BotscriptParser::parseStatement()
{
	// If it's a variable, expect assignment.
	if (m_lexer->next (TK_DollarSign))
	{
		m_lexer->mustGetNext (TK_Symbol);
		Variable* var = findVariable (getTokenString());

		if (var == null)
			error ("unknown variable $%1", var->name);

		return parseAssignment (var);
	}

	return null;
}

// ============================================================================
//
void BotscriptParser::addSwitchCase (DataBuffer* casebuffer)
{
	ScopeInfo* info = &SCOPE (0);
	CaseInfo casedata;

	// Init a mark for the case buffer
	ByteMark* casemark = currentBuffer()->addMark ("");
	casedata.mark = casemark;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (casebuffer != null)
		casebuffer->addReference (casemark);
	else
		currentBuffer()->addReference (casemark);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	casedata.data = m_switchBuffer = new DataBuffer;
	SCOPE(0).cases << casedata;
	info->casecursor++;
}

// ============================================================================
//
bool BotscriptParser::tokenIs (ETokenType a)
{
	return (m_lexer->tokenType() == a);
}

// ============================================================================
//
String BotscriptParser::getTokenString()
{
	return m_lexer->token()->text;
}

// ============================================================================
//
String BotscriptParser::describePosition() const
{
	Lexer::TokenInfo* tok = m_lexer->token();
	return tok->file + ":" + String (tok->line) + ":" + String (tok->column);
}

// ============================================================================
//
DataBuffer* BotscriptParser::currentBuffer()
{
	if (m_switchBuffer != null)
		return m_switchBuffer;

	if (m_currentMode == PARSERMODE_MainLoop)
		return m_mainLoopBuffer;

	if (m_currentMode == PARSERMODE_Onenter)
		return m_onenterBuffer;

	return m_mainBuffer;
}

// ============================================================================
//
void BotscriptParser::writeMemberBuffers()
{
	// If there was no mainloop defined, write a dummy one now.
	if (m_gotMainLoop == false)
	{
		m_mainLoopBuffer->writeDWord (DH_MainLoop);
		m_mainLoopBuffer->writeDWord (DH_EndMainLoop);
	}

	// Write the onenter and mainloop buffers, in that order in particular.
	for (DataBuffer** bufp : List<DataBuffer**> ({&m_onenterBuffer, &m_mainLoopBuffer}))
	{
		currentBuffer()->mergeAndDestroy (*bufp);

		// Clear the buffer afterwards for potential next state
		*bufp = new DataBuffer;
	}

	// Next state definitely has no mainloop yet
	m_gotMainLoop = false;
}

// ============================================================================
//
// Write string table
//
void BotscriptParser::writeStringTable()
{
	int stringcount = countStringsInTable();

	if (stringcount == 0)
		return;

	// Write header
	m_mainBuffer->writeDWord (DH_StringList);
	m_mainBuffer->writeDWord (stringcount);

	// Write all strings
	for (int i = 0; i < stringcount; i++)
		m_mainBuffer->writeString (getStringTable()[i]);
}

// ============================================================================
//
// Write the compiled bytecode to a file
//
void BotscriptParser::writeToFile (String outfile)
{
	FILE* fp = fopen (outfile, "wb");

	if (fp == null)
		error ("couldn't open %1 for writing: %2", outfile, strerror (errno));

	// First, resolve references
	for (MarkReference* ref : m_mainBuffer->references())
		for (int i = 0; i < 4; ++i)
			m_mainBuffer->buffer()[ref->pos + i] = (ref->target->pos >> (8 * i)) & 0xFF;

	// Then, dump the main buffer to the file
	fwrite (m_mainBuffer->buffer(), 1, m_mainBuffer->writtenSize(), fp);
	print ("-- %1 byte%s1 written to %2\n", m_mainBuffer->writtenSize(), outfile);
	fclose (fp);
}

// ============================================================================
//
// Attempt to find the variable by the given name. Looks from current scope
// downwards.
//
Variable* BotscriptParser::findVariable (const String& name)
{
	for (int i = m_scopeCursor; i >= 0; --i)
	{
		for (Variable* var : m_scopeStack[i].globalVariables + m_scopeStack[i].localVariables)
		{
			if (var->name == name)
				return var;
		}
	}

	return null;
}

// ============================================================================
//
// Is the parser currently in global state (i.e. not in any specific state)?
//
bool BotscriptParser::isInGlobalState() const
{
	return m_currentState.isEmpty();
}

// ============================================================================
//
void BotscriptParser::suggestHighestVarIndex (bool global, int index)
{
	if (global)
		m_highestGlobalVarIndex = max (m_highestGlobalVarIndex, index);
	else
		m_highestStateVarIndex = max (m_highestStateVarIndex, index);
}

// ============================================================================
//
int BotscriptParser::getHighestVarIndex (bool global)
{
	if (global)
		return m_highestGlobalVarIndex;

	return m_highestStateVarIndex;
}
