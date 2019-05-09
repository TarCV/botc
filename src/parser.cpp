/*
	Copyright 2012-2014 Teemu Piippo
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

#include <cstring>
#include <cerrno>
#include <cassert>
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

// _________________________________________________________________________________________________
//
BotscriptParser::BotscriptParser() :
	m_isReadOnly (false),
	m_mainBuffer (new DataBuffer),
	m_onenterBuffer (new DataBuffer),
	m_mainLoopBuffer (new DataBuffer),
	m_switchBuffer (nullptr),
	m_lexer (new Lexer),
	m_numEvents (0),
	m_currentMode{ ParserMode::TopLevel },
	m_isStateSpawnDefined (false),
	m_gotMainLoop (false),
	m_scopeCursor (-1),
	m_isElseAllowed (false),
	m_highestGlobalVarIndex (0),
	m_highestStateVarIndex (0) {}

// _________________________________________________________________________________________________
//
BotscriptParser::~BotscriptParser()
{
	delete m_lexer;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::checkToplevel()
{
	if (m_currentMode.last() != ParserMode::TopLevel)
		error ("%1-statements may only be defined at top level!", getTokenString());
}

// _________________________________________________________________________________________________
//
void BotscriptParser::checkTopOrStatelevel()
{
	if (m_currentMode.last() != ParserMode::TopLevel && m_currentMode.last() != ParserMode::State)
		error("%1-statements may only be defined at top or state levels!", getTokenString());
}

// _________________________________________________________________________________________________
//
void BotscriptParser::checkNotToplevel()
{
	if (m_currentMode.last() == ParserMode::TopLevel && m_currentMode.last() != ParserMode::State)
		error ("%1-statements must not be defined at top or state levels!", getTokenString());
}

// _________________________________________________________________________________________________
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
		if (tokenIs (Token::Else) and m_isElseAllowed == false)
			error ("else without preceding if");

		if (tokenIs (Token::Else) == false)
			m_isElseAllowed = false;

		switch (m_lexer->token()->type)
		{
			case Token::State:
				parseStateBlock();
				break;

			case Token::Event:
				parseEventBlock();
				break;

			case Token::Mainloop:
				parseMainloop();
				break;

			case Token::Onenter:
			case Token::Onexit:
				parseOnEnterExit();
				break;

			case Token::Int:
			case Token::Const:
			case Token::Str:
			case Token::Bool:
				parseVar();
				break;

			case Token::If:
				parseIf();
				break;

			case Token::Else:
				parseElse();
				break;

			case Token::While:
				parseWhileBlock();
				break;

			case Token::For:
				parseForBlock();
				break;

			case Token::Do:
				parseDoBlock();
				break;

			case Token::Switch:
				parseSwitchBlock();
				break;

			case Token::Case:
				parseSwitchCase();
				break;

			case Token::Default:
				parseSwitchDefault();
				break;

			case Token::Break:
				parseBreak();
				break;

			case Token::Continue:
				parseContinue();
				break;

			case Token::BraceEnd:
				parseBlockEnd();
				break;

			case Token::Eventdef:
				parseEventdef();
				break;

			case Token::Funcdef:
                parseFuncdef(false);
				break;

		    case Token::BuiltinDef:
		        parseBuiltinDef();
		        break;

			case Token::Semicolon:
				break;

			default:
			{
				// Check if it's a command
				CommandInfo* comm = findCommandByName (getTokenString());

				if (comm)
				{
					currentBuffer()->mergeAndDestroy (parseCommand (comm));
					m_lexer->mustGetNext (Token::Semicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				m_lexer->skip (-1);
				DataBuffer* b = parseStatement();

				if (b == null)
				{
					m_lexer->next();
					error ("unknown token `%1`", getTokenString());
				}

				currentBuffer()->mergeAndDestroy (b);
				m_lexer->mustGetNext (Token::Semicolon);
				break;
			}
		}
	}

	// Script file ended. Do some last checks and write the last things to main buffer
	if (m_currentMode.last() != ParserMode::TopLevel)
		error ("script did not end at top level; a `}` is missing somewhere");

	if (isReadOnly() == false)
	{
		// stateSpawn must be defined!
		if (m_isStateSpawnDefined == false)
			error ("script must have a state named `stateSpawn`!");

		// Dump the last state's onenter and mainloop
		writeMemberBuffers();

		// String table
		writeStringTable();
	}
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseStateBlock()
{
	checkToplevel();

	m_lexer->mustGetNext (Token::Symbol);
	String statename = getTokenString();

	// State name must be a word.
	if (statename.firstIndexOf (" ") != -1)
		error ("state name must be a single word, got `%1`", statename);

	State *state = null;

	if (m_lexer->peekNextType(Token::BraceStart)) {
		// This is an actual definition, not just a declaration
		m_lexer->mustGetNext(Token::BraceStart);
		m_currentMode.append(ParserMode::State);

		// stateSpawn is special - it *must* be defined. If we
		// encountered it, then mark down that we have it.
		if (statename.toLowercase() == "statespawn")
			m_isStateSpawnDefined = true;

		state = getStateByName(statename);
		if (state->isdeclared) {
			error("State %1 is already defined, cann't redefine", statename);
		}
		if (state == nullptr) {
			state = new State(m_knownStates.size());
			state->name = statename;
			m_knownStates.append(*state);
		}

		state->isdeclared = true;

		currentBuffer()->writeHeader(DataHeader::StateName);
		currentBuffer()->writeString(statename);
		currentBuffer()->writeHeader(DataHeader::StateIndex);
		currentBuffer()->writeDWord(state->index);

		m_currentState = statename;
		m_gotMainLoop = false;

		pushScope();
		SCOPE(0).type = SCOPE_State;
	}
	else {
		m_lexer->mustGetNext(Token::Semicolon);

		state = getStateByName(statename);
		if (state == nullptr) {
			state = new State(m_knownStates.size());
			state->name = statename;
			state->isdeclared = false;
			m_knownStates.append(*state);
		}
		else {
			warning("State %1 is already defined or declared", statename);
		}
	}
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseEventBlock()
{
	checkTopOrStatelevel();
	m_lexer->mustGetNext (Token::Symbol);

	EventDefinition* e = findEventByName (getTokenString());

	if (e == null)
		error ("bad event, got `%1`\n", getTokenString());

	m_lexer->mustGetNext (Token::BraceStart);
	m_currentMode.append(ParserMode::Event);
	currentBuffer()->writeHeader (DataHeader::Event);
	currentBuffer()->writeDWord (e->number);
	m_numEvents++;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseMainloop()
{
	checkTopOrStatelevel();
	m_lexer->mustGetNext (Token::BraceStart);

	m_currentMode.append(ParserMode::MainLoop);
	m_gotMainLoop = true;
	m_mainLoopBuffer->writeHeader (DataHeader::MainLoop);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseOnEnterExit()
{
	checkTopOrStatelevel();
	bool onenter = (tokenIs (Token::Onenter));
	m_lexer->mustGetNext (Token::BraceStart);
	m_currentMode.append(onenter ? ParserMode::Onenter : ParserMode::Onexit);
	currentBuffer()->writeHeader (onenter ? DataHeader::OnEnter : DataHeader::OnExit);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseVar()
{
	Variable* var = new Variable;
	var->origin = m_lexer->describeCurrentPosition();
	var->isarray = false;
	m_lexer->skip(-1);
	bool isconst = m_lexer->next (Token::Const);
	m_lexer->mustGetAnyOf ({Token::Int,Token::Str,Token::Bool});

	DataType vartype = (tokenIs (Token::Int)) ? TYPE_Int
					 : (tokenIs (Token::Str)) ? TYPE_String
					 : TYPE_Bool;

	if (m_lexer->next (Token::Const))
	{
		if (isconst)
			error ("duplicate const in variable declaration");

		isconst = true;
	}

	m_lexer->mustGetNext (Token::Symbol);
	String name = getTokenString();

	if (m_lexer->next (Token::BracketStart))
	{
		m_lexer->mustGetNext (Token::BracketEnd);
		var->isarray = true;

		if (isconst)
			error ("arrays cannot be const");
	}

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
	{
		if (var->name == name)
		{
			error ("Variable $%1 is already declared on this scope; declared at %2",
				var->name, var->origin);
		}
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
		m_lexer->mustGetNext (Token::Assign);
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
			error ("const variables must be constexpr (deductible at compile-time)");
		}
	}

	// Assign an index for the variable if it is not constexpr. Constexpr
	// variables can simply be substituted out for their value when used
	// so they need no index.
	if (var->writelevel != WRITE_Constexpr)
	{
		bool isglobal = isInGlobalState();
		var->index = isglobal ? SCOPE(0).globalVarIndexBase++ : SCOPE(0).localVarIndexBase++;

		if ((isglobal == true and var->index >= Limits::MaxGlobalVars) or
			(isglobal == false and var->index >= Limits::MaxStateVars))
		{
			error ("too many %1 variables", isglobal ? "global" : "state-local");
		}
	}

	if (isInGlobalState()) {
		assert(var->isGlobal());
		SCOPE(0).globalVariables << var;
	} else {
		var->statename = m_currentState;
		assert(!var->isGlobal());
		SCOPE(0).localVariables << var;
	}

	suggestHighestVarIndex (isInGlobalState(), var->index);
	m_lexer->mustGetNext (Token::Semicolon);
	print ("Declared %3 variable #%1 $%2\n", var->index, var->name, isInGlobalState() ? "global" : 
"state-local");
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseIf()
{
	checkNotToplevel();
	pushScope();

	// Condition
	m_lexer->mustGetNext (Token::ParenStart);

	// Read the expression and write it.
	DataBuffer* c = parseExpression (TYPE_Bool);
	currentBuffer()->mergeAndDestroy (c);

	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::BraceStart);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	ByteMark* mark = currentBuffer()->addMark ("");

	// Use DataHeader::IfNotGoto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	currentBuffer()->writeHeader (DataHeader::IfNotGoto);
	currentBuffer()->addReference (mark);

	// Store it
	SCOPE (0).mark1 = mark;
	SCOPE (0).type = SCOPE_If;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseElse()
{
	checkNotToplevel();
	m_lexer->mustGetNext (Token::BraceStart);
	pushScope (true);

	if (SCOPE (0).type != SCOPE_If)
		error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = currentBuffer()->addMark ("");

	// Instruction to jump to the end after if block is complete
	currentBuffer()->writeHeader (DataHeader::Goto);
	currentBuffer()->addReference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	currentBuffer()->adjustMark (SCOPE (0).mark1);
	SCOPE (0).type = SCOPE_Else;
}

// _________________________________________________________________________________________________
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
	m_lexer->mustGetNext (Token::ParenStart);
	DataBuffer* expr = parseExpression (TYPE_Int);
	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::BraceStart);

	// write condition
	currentBuffer()->mergeAndDestroy (expr);

	// Instruction to go to the end if it fails
	currentBuffer()->writeHeader (DataHeader::IfNotGoto);
	currentBuffer()->addReference (mark2);

	// Store the needed stuff
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).type = SCOPE_While;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseForBlock()
{
	checkNotToplevel();
	pushScope();

	// Initializer
	m_lexer->mustGetNext (Token::ParenStart);
	DataBuffer* init = parseStatement();

	if (init == null)
		error ("bad statement for initializer of for");

	m_lexer->mustGetNext (Token::Semicolon);

	// Condition
	DataBuffer* cond = parseExpression (TYPE_Int);

	if (cond == null)
		error ("bad statement for condition of for");

	m_lexer->mustGetNext (Token::Semicolon);

	// Incrementor
	DataBuffer* incr = parseStatement();

	if (incr == null)
		error ("bad statement for incrementor of for");

	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::BraceStart);

	// First, write out the initializer
	currentBuffer()->mergeAndDestroy (init);

	// Init two marks
	ByteMark* mark1 = currentBuffer()->addMark ("");
	ByteMark* mark2 = currentBuffer()->addMark ("");

	// Add the condition
	currentBuffer()->mergeAndDestroy (cond);
	currentBuffer()->writeHeader (DataHeader::IfNotGoto);
	currentBuffer()->addReference (mark2);

	// Store the marks and incrementor
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).buffer1 = incr;
	SCOPE (0).type = SCOPE_For;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseDoBlock()
{
	checkNotToplevel();
	pushScope();
	m_lexer->mustGetNext (Token::BraceStart);
	SCOPE (0).mark1 = currentBuffer()->addMark ("");
	SCOPE (0).type = SCOPE_Do;
}

// _________________________________________________________________________________________________
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
	m_lexer->mustGetNext (Token::ParenStart);
	currentBuffer()->mergeAndDestroy (parseExpression (TYPE_Int));
	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::BraceStart);
	SCOPE (0).type = SCOPE_Switch;
	SCOPE (0).mark1 = currentBuffer()->addMark (""); // end mark
	SCOPE (0).buffer1 = null; // default header
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseSwitchCase()
{
	// case is only allowed inside switch
	if (SCOPE (0).type != SCOPE_Switch)
		error ("case label outside switch");

	// Get a literal value for the case block. Zandronum does not support
	// expressions here.
	bool isNegative = m_lexer->next(Token::Minus);
	m_lexer->mustGetNext (Token::Number);
	int num = m_lexer->token()->text.toLong();
	if (isNegative) {
		num = -num;
	}
	m_lexer->mustGetNext (Token::Colon);

	for (const CaseInfo& info : SCOPE(0).cases)
	{
		if (info.number == num)
			error ("multiple case %1 labels in one switch", num);
	}

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
	m_switchBuffer = nullptr;
	currentBuffer()->writeHeader (DataHeader::CaseGoto);
	currentBuffer()->writeDWord (num);
	addSwitchCase (null);
	SCOPE (0).casecursor->number = num;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseSwitchDefault()
{
	if (SCOPE (0).type != SCOPE_Switch)
		error ("default label outside switch");

	if (SCOPE (0).buffer1 != null)
		error ("multiple default labels in one switch");

	m_lexer->mustGetNext (Token::Colon);

	// The default header is buffered into buffer1, since
	// it has to be the last of the case headers
	//
	// Since the expression is pushed into the switch
	// and is only popped when case succeeds, we have
	// to pop it with DataHeader::Drop manually if we end up in
	// a default.
	DataBuffer* buf = new DataBuffer;
	SCOPE (0).buffer1 = buf;
	buf->writeHeader (DataHeader::Drop);
	buf->writeHeader (DataHeader::Goto);
	addSwitchCase (buf);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseBreak()
{
	if (m_scopeCursor == 0)
		error ("unexpected `break`");

	currentBuffer()->writeHeader (DataHeader::Goto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case SCOPE_If:
		case SCOPE_Switch:
			currentBuffer()->addReference (SCOPE (0).mark1);
			break;

		case SCOPE_For:
		case SCOPE_While:
			currentBuffer()->addReference (SCOPE (0).mark2);
			break;

		default:
			error ("unexpected `break`");
			break;
	}

	m_lexer->mustGetNext (Token::Semicolon);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseContinue()
{
	m_lexer->mustGetNext (Token::Semicolon);

	int curs;
	bool found = false;

	// Fall through the scope until we find a loop block
	for (curs = m_scopeCursor; curs > 0 and !found; curs--)
	{
		switch (m_scopeStack[curs].type)
		{
			case SCOPE_For:
			case SCOPE_While:
			case SCOPE_Do:
				currentBuffer()->writeHeader (DataHeader::Goto);
				currentBuffer()->addReference (m_scopeStack[curs].mark1);
				found = true;
				break;

			default:
				break;
		}
	}

	// No loop blocks
	if (found == false)
		error ("`continue`-statement not inside a loop");
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseBlockEnd()
{
	// Closing brace
	// If we're in the block stack, we're descending down from it now
	if (m_scopeCursor > 0 && SCOPE(0).type != SCOPE_State) // states are special case
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
				currentBuffer()->writeHeader (DataHeader::Goto);
				currentBuffer()->addReference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				currentBuffer()->adjustMark (SCOPE (0).mark2);
				break;
			}

			case SCOPE_Do:
			{
				m_lexer->mustGetNext (Token::While);
				m_lexer->mustGetNext (Token::ParenStart);
				DataBuffer* expr = parseExpression (TYPE_Int);
				m_lexer->mustGetNext (Token::ParenEnd);
				m_lexer->mustGetNext (Token::Semicolon);

				// If the condition runs true, go back to the start.
				currentBuffer()->mergeAndDestroy (expr);
				currentBuffer()->writeHeader (DataHeader::IfGoto);
				currentBuffer()->addReference (SCOPE (0).mark1);
				break;
			}

			case SCOPE_Switch:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != null)
					m_switchBuffer = SCOPE (1).casecursor->data;
				else
					m_switchBuffer = nullptr;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					currentBuffer()->mergeAndDestroy (SCOPE (0).buffer1);
				else
				{
					currentBuffer()->writeHeader (DataHeader::Drop);
					currentBuffer()->writeHeader (DataHeader::Goto);
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
			default:
				break;
		}

		// Descend down the stack
		m_scopeCursor--;
		return;
	}

	if (m_currentMode.last() == ParserMode::State) {
		// write the current state's onenter and
		// mainloop buffers to file now
		if (m_currentState.isEmpty() == false)
			writeMemberBuffers();

		if (SCOPE(0).type == SCOPE_State) {
			// Descend down the stack
			m_scopeCursor--;
		}
	} else {
		DataHeader dataheader =
			(m_currentMode.last() == ParserMode::Event) ? DataHeader::EndEvent
			: (m_currentMode.last() == ParserMode::MainLoop) ? DataHeader::EndMainLoop
			: (m_currentMode.last() == ParserMode::Onenter) ? DataHeader::EndOnEnter
			: (m_currentMode.last() == ParserMode::Onexit) ? DataHeader::EndOnExit
			: DataHeader::NumValues;

		if (dataheader == DataHeader::NumValues)
			error("unexpected `}`");

		// Data header must be written before mode is changed because
		// onenter and mainloop go into special buffers, and we want
		// the closing data headers into said buffers too.
		currentBuffer()->writeHeader(dataheader);
	}

	assert(m_currentMode.size() > 1);
	m_currentMode.pop();
	assert(m_currentMode.last() == ParserMode::TopLevel || m_currentMode.last() == ParserMode::State);

	m_lexer->next (Token::Semicolon);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseEventdef()
{
	EventDefinition* e = new EventDefinition;

	m_lexer->mustGetNext (Token::Number);
	e->number = getTokenString().toLong();
	m_lexer->mustGetNext (Token::Colon);
	m_lexer->mustGetNext (Token::Symbol);
	e->name = m_lexer->token()->text;
	m_lexer->mustGetNext (Token::ParenStart);
	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::Semicolon);
	addEvent (e);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseFuncdef(bool isBuiltin)
{
	CommandInfo* comm = new CommandInfo;
	comm->origin = m_lexer->describeCurrentPosition();

	// Return value
	m_lexer->mustGetAnyOf ({Token::Int,Token::Void,Token::Bool,Token::Str});
	comm->returnvalue = getTypeByName (m_lexer->token()->text); // TODO
	ASSERT_NE (comm->returnvalue, TYPE_Unknown);

	// Number
	m_lexer->mustGetNext (Token::Number);
	comm->number = m_lexer->token()->text.toLong();
	comm->isbuiltin = isBuiltin;
	m_lexer->mustGetNext (Token::Colon);

	// Name
	m_lexer->mustGetNext (Token::Symbol);
	comm->name = m_lexer->token()->text;

	// Arguments
	m_lexer->mustGetNext (Token::ParenStart);
	comm->minargs = 0;

	while (m_lexer->peekNextType (Token::ParenEnd) == false)
	{
		if (comm->args.isEmpty() == false)
			m_lexer->mustGetNext (Token::Comma);

		CommandArgument arg;
		m_lexer->mustGetAnyOf ({Token::Int,Token::Bool,Token::Str});
		DataType type = getTypeByName (m_lexer->token()->text); // TODO
		ASSERT_NE (type, TYPE_Unknown)
		ASSERT_NE (type, TYPE_Void)
		arg.type = type;

		m_lexer->mustGetNext (Token::Symbol);
		arg.name = m_lexer->token()->text;

		// If this is an optional parameter, we need the default value.
		if (comm->minargs < comm->args.size() or m_lexer->peekNextType (Token::Assign))
		{
			m_lexer->mustGetNext (Token::Assign);

			switch (type)
			{
				case TYPE_Int:
				case TYPE_Bool:
					m_lexer->mustGetNext (Token::Number);
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

	m_lexer->mustGetNext (Token::ParenEnd);
	m_lexer->mustGetNext (Token::Semicolon);

	addCommandDefinition(comm);
}

// _________________________________________________________________________________________________
//
void BotscriptParser::parseBuiltinDef()
{
    parseFuncdef(true);
}

// _________________________________________________________________________________________________
//
// Parses a command call
//
DataBuffer* BotscriptParser::parseCommand (CommandInfo* comm)
{
	DataBuffer* r = new DataBuffer (64);

	if ((m_currentMode.last() == ParserMode::TopLevel || m_currentMode.last() == ParserMode::State) and comm->returnvalue == TYPE_Void)
		error ("command call at top or state level");

	m_lexer->mustGetNext (Token::ParenStart);
	m_lexer->mustGetNext (Token::Any);

	int curarg = 0;

	for (;;)
	{
		if (tokenIs (Token::ParenEnd))
		{
			if (curarg < comm->minargs)
			{
				error ("too few arguments passed to %1\n\tusage is: %2",
					comm->name, comm->signature());
			}

			break;
		}

		if (curarg >= comm->args.size())
		{
			error ("too many arguments (%3) passed to %1\n\tusage is: %2",
				comm->name, comm->signature());
		}

		r->mergeAndDestroy (parseExpression (comm->args[curarg].type, true));
		m_lexer->mustGetNext (Token::Any);

		if (curarg < comm->minargs - 1)
		{
			m_lexer->tokenMustBe (Token::Comma);
			m_lexer->mustGetNext (Token::Any);
		}
		else if (curarg < comm->args.size() - 1)
		{
			// Can continue, but can terminate as well.
			if (tokenIs (Token::ParenEnd))
			{
				curarg++;
				break;
			}
			else
			{
				m_lexer->tokenMustBe (Token::Comma);
				m_lexer->mustGetNext (Token::Any);
			}
		}

		curarg++;
	}

	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->args.size())
	{
		r->writeHeader (DataHeader::PushNumber);
		r->writeDWord (comm->args[curarg].defvalue);
		curarg++;
	}

	if (comm->isbuiltin) {
		r->writeDWord(comm->number);
	} else {
		r->writeHeader(DataHeader::Command);
		r->writeDWord(comm->number);
		r->writeDWord(comm->args.size());
	}

	return r;
}

// _________________________________________________________________________________________________
//
String BotscriptParser::parseFloat()
{
	m_lexer->tokenMustBe (Token::Number);
	String floatstring = getTokenString();
	Lexer::TokenInfo tok;

	// Go after the decimal point
	if (m_lexer->peekNext (&tok) and tok.type ==Token::Dot)
	{
		m_lexer->skip();
		m_lexer->mustGetNext (Token::Number);
		floatstring += ".";
		floatstring += getTokenString();
	}

	return floatstring;
}

// _________________________________________________________________________________________________
//
// Parses an assignment operator.
//
AssignmentOperator BotscriptParser::parseAssignmentOperator()
{
	const List<Token> tokens =
	{
		Token::Assign,
		Token::AddAssign,
		Token::SubAssign,
		Token::MultiplyAssign,
		Token::DivideAssign,
		Token::ModulusAssign,
		Token::DoublePlus,
		Token::DoubleMinus,
	};

	m_lexer->mustGetAnyOf (tokens);

	switch (m_lexer->tokenType())
	{
		case Token::Assign:			return ASSIGNOP_Assign;
		case Token::AddAssign:		return ASSIGNOP_Add;
		case Token::SubAssign:		return ASSIGNOP_Subtract;
		case Token::MultiplyAssign:	return ASSIGNOP_Multiply;
		case Token::DivideAssign:	return ASSIGNOP_Divide;
		case Token::ModulusAssign:	return ASSIGNOP_Modulus;
		case Token::DoublePlus:		return ASSIGNOP_Increase;
		case Token::DoubleMinus:	return ASSIGNOP_Decrease;
		default: break;
	}

	error ("WTF bad operator token %1", m_lexer->DescribeToken (m_lexer->token()));
	return (AssignmentOperator) 0;
}

// _________________________________________________________________________________________________
//
const struct AssignmentDataHeaderInfo
{
	AssignmentOperator	op;
	DataHeader			local;
	DataHeader			global;
	DataHeader			array;
}
AssignmentDataHeaders[] =
{
	{
		ASSIGNOP_Assign,
		DataHeader::AssignLocalVar,
		DataHeader::AssignGlobalVar,
		DataHeader::AssignGlobalArray
	},
	{
		ASSIGNOP_Add,
		DataHeader::AddLocalVar,
		DataHeader::AddGlobalVar,
		DataHeader::AddGlobalArray
	},
	{
		ASSIGNOP_Subtract,
		DataHeader::SubtractLocalVar,
		DataHeader::SubtractGlobalVar,
		DataHeader::SubtractGlobalArray
	},
	{
		ASSIGNOP_Multiply,
		DataHeader::MultiplyLocalVar,
		DataHeader::MultiplyGlobalVar,
		DataHeader::MultiplyGlobalArray
	},
	{
		ASSIGNOP_Divide,
		DataHeader::DivideLocalVar,
		DataHeader::DivideGlobalVar,
		DataHeader::DivideGlobalArray
	},
	{
		ASSIGNOP_Modulus,
		DataHeader::ModLocalVar,
		DataHeader::ModGlobalVar,
		DataHeader::ModGlobalArray
	},
	{
		ASSIGNOP_Increase,
		DataHeader::IncreaseLocalVar,
		DataHeader::IncreaseGlobalVar,
		DataHeader::IncreaseGlobalArray
	},
	{
		ASSIGNOP_Decrease,
		DataHeader::DecreaseLocalVar,
		DataHeader::DecreaseGlobalVar,
		DataHeader::DecreaseGlobalArray
	},
};

DataHeader BotscriptParser::getAssigmentDataHeader (AssignmentOperator op, Variable* var)
{
	for (const auto& a : AssignmentDataHeaders)
	{
		if (a.op != op)
			continue;

		if (var->isarray)
			return a.array;

		if (var->isGlobal())
			return a.global;

		return a.local;
	}

	error ("WTF: couldn't find data header for operator #%1", op);
	return (DataHeader) 0;
}

State * BotscriptParser::getStateByName(const String& name)
{
	std::function<bool(State const&)> lambda = [&name](const State& state) -> bool { return state.name == name; };
	return m_knownStates.find(lambda);
}

// _________________________________________________________________________________________________
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
		m_lexer->mustGetNext (Token::BracketStart);
		Expression expr (this, m_lexer, TYPE_Int);
		expr.getResult()->convertToBuffer();
		arrayindex = expr.getResult()->buffer()->clone();
		m_lexer->mustGetNext (Token::BracketEnd);
	}

	// Get an operator
	AssignmentOperator oper = parseAssignmentOperator();

	if (m_currentMode.last() == ParserMode::TopLevel || m_currentMode.last() == ParserMode::State)
		error ("can't alter variables at top or state level");

	if (var->isarray)
		retbuf->mergeAndDestroy (arrayindex);

	// Parse the right operand
	if (oper != ASSIGNOP_Increase and oper != ASSIGNOP_Decrease)
	{
		DataBuffer* expr = parseExpression (var->type);
		retbuf->mergeAndDestroy (expr);
	}

#if 0
	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	retbuf->WriteDWord (var->IsGlobal() ? DataHeader::PushGlobalVar : DataHeader::PushLocalVar);
	retbuf->WriteDWord (var->index);
	retbuf->MergeAndDestroy (expr);
	retbuf->WriteDWord ((oper == OPER_ASSIGNLEFTSHIFT) ? DataHeader::LeftShift : 
DataHeader::RightShift);
	retbuf->WriteDWord (var->IsGlobal() ? DataHeader::AssignGlobalVar : DataHeader::AssignLocalVar);
	retbuf->WriteDWord (var->index);
#endif

	retbuf->writeHeader (getAssigmentDataHeader (oper, var));
	retbuf->writeDWord (var->index);
	return retbuf;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::pushScope (bool noreset)
{
	m_scopeCursor++;

	if (m_scopeStack.size() < m_scopeCursor + 1)
	{
		ScopeInfo newscope;
		m_scopeStack << newscope;
		noreset = false;
	}

	if (not noreset)
	{
		ScopeInfo* info = &SCOPE (0);
		info->type = SCOPE_Unknown;
		info->mark1 = null;
		info->mark2 = null;
		info->buffer1 = null;
		info->cases.clear();
		info->casecursor = null;
	}

	// Reset variable stuff in any case
	SCOPE(0).globalVarIndexBase = (m_scopeCursor == 0) ? 0 : SCOPE(1).globalVarIndexBase;
	SCOPE(0).localVarIndexBase = (m_scopeCursor == 0) ? 0 : SCOPE(1).localVarIndexBase;

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
		delete var;

	SCOPE(0).localVariables.clear();
	SCOPE(0).globalVariables.clear();
}

// _________________________________________________________________________________________________
//
DataBuffer* BotscriptParser::parseExpression (DataType reqtype, bool fromhere)
{
	// hehe
	if (fromhere)
		m_lexer->skip (-1);

	Expression expr (this, m_lexer, TYPE_ToBeDecided);
	ASSERT_NE(expr.getResult()->valueType(), TYPE_ToBeDecided)
	if (expr.getResult()->valueType() != reqtype) {
		error("Incompatible data type");
	}
	expr.getResult()->convertToBuffer();

	// The buffer will be destroyed once the function ends so we need to
	// clone it now.
	return expr.getResult()->buffer()->clone();
}

// _________________________________________________________________________________________________
//
DataBuffer* BotscriptParser::parseStatement()
{
	// If it's a variable, expect assignment.
	if (m_lexer->next (Token::Symbol))
	{
		Variable* var = findVariable (getTokenString());

		if (var == null)
			error ("unknown variable $%1", var->name);

		return parseAssignment (var);
	}

	return null;
}

// _________________________________________________________________________________________________
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
	List<CaseInfo> &cases = SCOPE(0).cases;
	cases << casedata;
	info->casecursor = &*(cases.end() - 1);
}

// _________________________________________________________________________________________________
//
bool BotscriptParser::tokenIs (Token a)
{
	return (m_lexer->tokenType() == a);
}

// _________________________________________________________________________________________________
//
String BotscriptParser::getTokenString()
{
	return m_lexer->token()->text;
}

// _________________________________________________________________________________________________
//
String BotscriptParser::describePosition() const
{
	Lexer::TokenInfo* tok = m_lexer->token();
	return tok->file + ":" + String (tok->line) + ":" + String (tok->column);
}

// _________________________________________________________________________________________________
//
//	Where are we writing to right now?
//
DataBuffer* BotscriptParser::currentBuffer()
{
	if (m_switchBuffer != nullptr)
		return m_switchBuffer;

	if (m_currentMode.last() == ParserMode::MainLoop)
		return m_mainLoopBuffer;

	if (m_currentMode.last() == ParserMode::Onenter)
		return m_onenterBuffer;

	return m_mainBuffer;
}

// _________________________________________________________________________________________________
//
void BotscriptParser::writeMemberBuffers()
{
	// If there was no mainloop defined, write a dummy one now.
	if (m_gotMainLoop == false)
	{
		m_mainLoopBuffer->writeHeader (DataHeader::MainLoop);
		m_mainLoopBuffer->writeHeader (DataHeader::EndMainLoop);
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

// _________________________________________________________________________________________________
//
// Write string table
//
void BotscriptParser::writeStringTable()
{
	int stringcount = countStringsInTable();

	if (stringcount == 0)
		return;

	// Write header
	m_mainBuffer->writeHeader (DataHeader::StringList);
	m_mainBuffer->writeDWord (stringcount);

	// Write all strings
	for (int i = 0; i < stringcount; i++)
		m_mainBuffer->writeString (getStringTable()[i]);
}

// _________________________________________________________________________________________________
//
// Write the compiled bytecode to a file
//
void BotscriptParser::writeToFile (String outfile)
{
	FILE* fp = fopen (outfile, "wb");

	if (fp == null)
		error ("couldn't open %1 for writing: %2", outfile, std::strerror (errno));

	// First, resolve references
	for (MarkReference* ref : m_mainBuffer->references())
	{
		for (int i = 0; i < 4; ++i)
			m_mainBuffer->buffer()[ref->pos + i] = (ref->target->pos >> (8 * i)) & 0xFF;
	}

	// Then, dump the main buffer to the file
	fwrite (m_mainBuffer->buffer(), 1, m_mainBuffer->writtenSize(), fp);
	print ("-- %1 byte%s1 written to %2\n", m_mainBuffer->writtenSize(), outfile);
	fclose (fp);
}

// _________________________________________________________________________________________________
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

// _________________________________________________________________________________________________
//
// Is the parser currently in global state (i.e. not in any specific state)?
//
bool BotscriptParser::isInGlobalState() const
{
	return m_currentState.isEmpty();
}

// _________________________________________________________________________________________________
//
void BotscriptParser::suggestHighestVarIndex (bool global, int index)
{
	if (global)
		m_highestGlobalVarIndex = max (m_highestGlobalVarIndex, index);
	else
		m_highestStateVarIndex = max (m_highestStateVarIndex, index);
}

// _________________________________________________________________________________________________
//
int BotscriptParser::getHighestVarIndex (bool global)
{
	if (global)
		return m_highestGlobalVarIndex;

	return m_highestStateVarIndex;
}
