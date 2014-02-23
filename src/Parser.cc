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

#include "Parser.h"
#include "Events.h"
#include "Commands.h"
#include "StringTable.h"
#include "Containers.h"
#include "Lexer.h"
#include "DataBuffer.h"
#include "Expression.h"

#define SCOPE(n) (mScopeStack[mScopeCursor - n])

// ============================================================================
//
BotscriptParser::BotscriptParser() :
	mIsReadOnly (false),
	mMainBuffer (new DataBuffer),
	mOnEnterBuffer (new DataBuffer),
	mMainLoopBuffer (new DataBuffer),
	mLexer (new Lexer),
	mNumStates (0),
	mNumEvents (0),
	mCurrentMode (PARSERMODE_TopLevel),
	mStateSpawnDefined (false),
	mGotMainLoop (false),
	mScopeCursor (-1),
	mCanElse (false),
	mHighestGlobalVarIndex (0),
	mHighestStateVarIndex (0) {}

// ============================================================================
//
BotscriptParser::~BotscriptParser()
{
	delete mLexer;
}

// ============================================================================
//
void BotscriptParser::CheckToplevel()
{
	if (mCurrentMode != PARSERMODE_TopLevel)
		Error ("%1-statements may only be defined at top level!", GetTokenString());
}

// ============================================================================
//
void BotscriptParser::CheckNotToplevel()
{
	if (mCurrentMode == PARSERMODE_TopLevel)
		Error ("%1-statements must not be defined at top level!", GetTokenString());
}

// ============================================================================
//
// Main compiler code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
//
void BotscriptParser::ParseBotscript (String fileName)
{
	// Lex and preprocess the file
	mLexer->ProcessFile (fileName);
	PushScope();

	while (mLexer->Next())
	{
		// Check if else is potentically valid
		if (TokenIs (TK_Else) && mCanElse == false)
			Error ("else without preceding if");

		if (TokenIs (TK_Else) == false)
			mCanElse = false;

		switch (mLexer->Token()->type)
		{
			case TK_State:
				ParseStateBlock();
				break;

			case TK_Event:
				ParseEventBlock();
				break;

			case TK_Mainloop:
				ParseMainloop();
				break;

			case TK_Onenter:
			case TK_Onexit:
				ParseOnEnterExit();
				break;

			case TK_Var:
				ParseVar();
				break;

			case TK_If:
				ParseIf();
				break;

			case TK_Else:
				ParseElse();
				break;

			case TK_While:
				ParseWhileBlock();
				break;

			case TK_For:
				ParseForBlock();
				break;

			case TK_Do:
				ParseDoBlock();
				break;

			case TK_Switch:
				ParseSwitchBlock();
				break;

			case TK_Case:
				ParseSwitchCase();
				break;

			case TK_Default:
				ParseSwitchDefault();
				break;

			case TK_Break:
				ParseBreak();
				break;

			case TK_Continue:
				ParseContinue();
				break;

			case TK_BraceEnd:
				ParseBlockEnd();
				break;

			case TK_Eventdef:
				ParseEventdef();
				break;

			case TK_Funcdef:
				ParseFuncdef();
				break;

			case TK_Semicolon:
				break;

			default:
			{
				// Check if it's a command
				CommandInfo* comm = FindCommandByName (GetTokenString());

				if (comm)
				{
					buffer()->MergeAndDestroy (ParseCommand (comm));
					mLexer->MustGetNext (TK_Semicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				mLexer->Skip (-1);
				DataBuffer* b = ParseStatement();

				if (b == false)
				{
					mLexer->Next();
					Error ("unknown token `%1`", GetTokenString());
				}

				buffer()->MergeAndDestroy (b);
				mLexer->MustGetNext (TK_Semicolon);
				break;
			}
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (mCurrentMode != PARSERMODE_TopLevel)
		Error ("script did not end at top level; a `}` is missing somewhere");

	if (IsReadOnly() == false)
	{
		// stateSpawn must be defined!
		if (mStateSpawnDefined == false)
			Error ("script must have a state named `stateSpawn`!");

		// Dump the last state's onenter and mainloop
		writeMemberBuffers();

		// String table
		WriteStringTable();
	}
}

// ============================================================================
//
void BotscriptParser::ParseStateBlock()
{
	CheckToplevel();
	mLexer->MustGetNext (TK_String);
	String statename = GetTokenString();

	// State name must be a word.
	if (statename.FirstIndexOf (" ") != -1)
		Error ("state name must be a single word, got `%1`", statename);

	// stateSpawn is special - it *must* be defined. If we
	// encountered it, then mark down that we have it.
	if (statename.ToLowercase() == "statespawn")
		mStateSpawnDefined = true;

	// Must end in a colon
	mLexer->MustGetNext (TK_Colon);

	// write the previous state's onenter and
	// mainloop buffers to file now
	if (mCurrentState.IsEmpty() == false)
		writeMemberBuffers();

	buffer()->WriteDWord (DH_StateName);
	buffer()->WriteString (statename);
	buffer()->WriteDWord (DH_StateIndex);
	buffer()->WriteDWord (mNumStates);

	mNumStates++;
	mCurrentState = statename;
	mGotMainLoop = false;
}

// ============================================================================
//
void BotscriptParser::ParseEventBlock()
{
	CheckToplevel();
	mLexer->MustGetNext (TK_String);

	EventDefinition* e = FindEventByName (GetTokenString());

	if (e == null)
		Error ("bad event, got `%1`\n", GetTokenString());

	mLexer->MustGetNext (TK_BraceStart);
	mCurrentMode = PARSERMODE_Event;
	buffer()->WriteDWord (DH_Event);
	buffer()->WriteDWord (e->number);
	mNumEvents++;
}

// ============================================================================
//
void BotscriptParser::ParseMainloop()
{
	CheckToplevel();
	mLexer->MustGetNext (TK_BraceStart);

	mCurrentMode = PARSERMODE_MainLoop;
	mMainLoopBuffer->WriteDWord (DH_MainLoop);
}

// ============================================================================
//
void BotscriptParser::ParseOnEnterExit()
{
	CheckToplevel();
	bool onenter = (TokenIs (TK_Onenter));
	mLexer->MustGetNext (TK_BraceStart);

	mCurrentMode = onenter ? PARSERMODE_Onenter : PARSERMODE_Onexit;
	buffer()->WriteDWord (onenter ? DH_OnEnter : DH_OnExit);
}

// ============================================================================
//
void BotscriptParser::ParseVar()
{
	Variable* var = new Variable;
	var->origin = mLexer->DescribeCurrentPosition();
	var->isarray = false;
	const bool isconst = mLexer->Next (TK_Const);
	mLexer->MustGetAnyOf ({TK_Int,TK_Str,TK_Void});

	DataType vartype =	(TokenIs (TK_Int)) ? TYPE_Int :
					(TokenIs (TK_Str)) ? TYPE_String :
					TYPE_Bool;

	mLexer->MustGetNext (TK_Symbol);
	String name = GetTokenString();

	if (mLexer->Next (TK_BracketStart))
	{
		mLexer->MustGetNext (TK_BracketEnd);
		var->isarray = true;

		if (isconst)
			Error ("arrays cannot be const");
	}

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
	{
		if (var->name == name)
			Error ("Variable $%1 is already declared on this scope; declared at %2",
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
		mLexer->MustGetNext (TK_Assign);
		Expression expr (this, mLexer, vartype);

		// If the expression was constexpr, we know its value and thus
		// can store it in the variable.
		if (expr.Result()->IsConstexpr())
		{
			var->writelevel = WRITE_Constexpr;
			var->value = expr.Result()->Value();
		}
		else
		{
			// TODO: might need a VM-wise oninit for this...
			Error ("const variables must be constexpr");
		}
	}

	// Assign an index for the variable if it is not constexpr. Constexpr
	// variables can simply be substituted out for their value when used
	// so they need no index.
	if (var->writelevel != WRITE_Constexpr)
	{
		bool isglobal = IsInGlobalState();
		var->index = isglobal ? SCOPE(0).globalVarIndexBase++ : SCOPE(0).localVarIndexBase++;

		if ((isglobal == true && var->index >= gMaxGlobalVars) ||
			(isglobal == false && var->index >= gMaxStateVars))
		{
			Error ("too many %1 variables", isglobal ? "global" : "state-local");
		}
	}

	if (IsInGlobalState())
		SCOPE(0).globalVariables << var;
	else
		SCOPE(0).localVariables << var;

	SuggestHighestVarIndex (IsInGlobalState(), var->index);
	mLexer->MustGetNext (TK_Semicolon);
	Print ("Declared %3 variable #%1 $%2\n", var->index, var->name, IsInGlobalState() ? "global" : "state-local");
}

// ============================================================================
//
void BotscriptParser::ParseIf()
{
	CheckNotToplevel();
	PushScope();

	// Condition
	mLexer->MustGetNext (TK_ParenStart);

	// Read the expression and write it.
	DataBuffer* c = ParseExpression (TYPE_Int);
	buffer()->MergeAndDestroy (c);

	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_BraceStart);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	ByteMark* mark = buffer()->AddMark ("");

	// Use DH_IfNotGoto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	buffer()->WriteDWord (DH_IfNotGoto);
	buffer()->AddReference (mark);

	// Store it
	SCOPE (0).mark1 = mark;
	SCOPE (0).type = SCOPE_If;
}

// ============================================================================
//
void BotscriptParser::ParseElse()
{
	CheckNotToplevel();
	mLexer->MustGetNext (TK_BraceStart);
	PushScope (eNoReset);

	if (SCOPE (0).type != SCOPE_If)
		Error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = buffer()->AddMark ("");

	// Instruction to jump to the end after if block is complete
	buffer()->WriteDWord (DH_Goto);
	buffer()->AddReference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	buffer()->AdjustMark (SCOPE (0).mark1);
	SCOPE (0).type = SCOPE_Else;
}

// ============================================================================
//
void BotscriptParser::ParseWhileBlock()
{
	CheckNotToplevel();
	PushScope();

	// While loops need two marks - one at the start of the loop and one at the
	// end. The condition is checked at the very start of the loop, if it fails,
	// we use goto to skip to the end of the loop. At the end, we loop back to
	// the beginning with a go-to statement.
	ByteMark* mark1 = buffer()->AddMark (""); // start
	ByteMark* mark2 = buffer()->AddMark (""); // end

	// Condition
	mLexer->MustGetNext (TK_ParenStart);
	DataBuffer* expr = ParseExpression (TYPE_Int);
	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_BraceStart);

	// write condition
	buffer()->MergeAndDestroy (expr);

	// Instruction to go to the end if it fails
	buffer()->WriteDWord (DH_IfNotGoto);
	buffer()->AddReference (mark2);

	// Store the needed stuff
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).type = SCOPE_While;
}

// ============================================================================
//
void BotscriptParser::ParseForBlock()
{
	CheckNotToplevel();
	PushScope();

	// Initializer
	mLexer->MustGetNext (TK_ParenStart);
	DataBuffer* init = ParseStatement();

	if (init == null)
		Error ("bad statement for initializer of for");

	mLexer->MustGetNext (TK_Semicolon);

	// Condition
	DataBuffer* cond = ParseExpression (TYPE_Int);

	if (cond == null)
		Error ("bad statement for condition of for");

	mLexer->MustGetNext (TK_Semicolon);

	// Incrementor
	DataBuffer* incr = ParseStatement();

	if (incr == null)
		Error ("bad statement for incrementor of for");

	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_BraceStart);

	// First, write out the initializer
	buffer()->MergeAndDestroy (init);

	// Init two marks
	ByteMark* mark1 = buffer()->AddMark ("");
	ByteMark* mark2 = buffer()->AddMark ("");

	// Add the condition
	buffer()->MergeAndDestroy (cond);
	buffer()->WriteDWord (DH_IfNotGoto);
	buffer()->AddReference (mark2);

	// Store the marks and incrementor
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).buffer1 = incr;
	SCOPE (0).type = SCOPE_For;
}

// ============================================================================
//
void BotscriptParser::ParseDoBlock()
{
	CheckNotToplevel();
	PushScope();
	mLexer->MustGetNext (TK_BraceStart);
	SCOPE (0).mark1 = buffer()->AddMark ("");
	SCOPE (0).type = SCOPE_Do;
}

// ============================================================================
//
void BotscriptParser::ParseSwitchBlock()
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

	CheckNotToplevel();
	PushScope();
	mLexer->MustGetNext (TK_ParenStart);
	buffer()->MergeAndDestroy (ParseExpression (TYPE_Int));
	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_BraceStart);
	SCOPE (0).type = SCOPE_Switch;
	SCOPE (0).mark1 = buffer()->AddMark (""); // end mark
	SCOPE (0).buffer1 = null; // default header
}

// ============================================================================
//
void BotscriptParser::ParseSwitchCase()
{
	// case is only allowed inside switch
	if (SCOPE (0).type != SCOPE_Switch)
		Error ("case label outside switch");

	// Get a literal value for the case block. Zandronum does not support
	// expressions here.
	mLexer->MustGetNext (TK_Number);
	int num = mLexer->Token()->text.ToLong();
	mLexer->MustGetNext (TK_Colon);

	for (const CaseInfo& info : SCOPE(0).cases)
		if (info.number == num)
			Error ("multiple case %1 labels in one switch", num);

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
	mSwitchBuffer = null;
	buffer()->WriteDWord (DH_CaseGoto);
	buffer()->WriteDWord (num);
	AddSwitchCase (null);
	SCOPE (0).casecursor->number = num;
}

// ============================================================================
//
void BotscriptParser::ParseSwitchDefault()
{
	if (SCOPE (0).type != SCOPE_Switch)
		Error ("default label outside switch");

	if (SCOPE (0).buffer1 != null)
		Error ("multiple default labels in one switch");

	mLexer->MustGetNext (TK_Colon);

	// The default header is buffered into buffer1, since
	// it has to be the last of the case headers
	//
	// Since the expression is pushed into the switch
	// and is only popped when case succeeds, we have
	// to pop it with DH_Drop manually if we end up in
	// a default.
	DataBuffer* buf = new DataBuffer;
	SCOPE (0).buffer1 = buf;
	buf->WriteDWord (DH_Drop);
	buf->WriteDWord (DH_Goto);
	AddSwitchCase (buf);
}

// ============================================================================
//
void BotscriptParser::ParseBreak()
{
	if (mScopeCursor == 0)
		Error ("unexpected `break`");

	buffer()->WriteDWord (DH_Goto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case SCOPE_If:
		case SCOPE_Switch:
		{
			buffer()->AddReference (SCOPE (0).mark1);
		} break;

		case SCOPE_For:
		case SCOPE_While:
		{
			buffer()->AddReference (SCOPE (0).mark2);
		} break;

		default:
		{
			Error ("unexpected `break`");
		} break;
	}

	mLexer->MustGetNext (TK_Semicolon);
}

// ============================================================================
//
void BotscriptParser::ParseContinue()
{
	mLexer->MustGetNext (TK_Semicolon);

	int curs;
	bool found = false;

	// Fall through the scope until we find a loop block
	for (curs = mScopeCursor; curs > 0 && !found; curs--)
	{
		switch (mScopeStack[curs].type)
		{
			case SCOPE_For:
			case SCOPE_While:
			case SCOPE_Do:
			{
				buffer()->WriteDWord (DH_Goto);
				buffer()->AddReference (mScopeStack[curs].mark1);
				found = true;
			} break;

			default:
				break;
		}
	}

	// No loop blocks
	if (found == false)
		Error ("`continue`-statement not inside a loop");
}

// ============================================================================
//
void BotscriptParser::ParseBlockEnd()
{
	// Closing brace
	// If we're in the block stack, we're descending down from it now
	if (mScopeCursor > 0)
	{
		switch (SCOPE (0).type)
		{
			case SCOPE_If:
			{
				// Adjust the closing mark.
				buffer()->AdjustMark (SCOPE (0).mark1);

				// We're returning from `if`, thus `else` follow
				mCanElse = true;
				break;
			}

			case SCOPE_Else:
			{
				// else instead uses mark1 for itself (so if expression
				// fails, jump to else), mark2 means end of else
				buffer()->AdjustMark (SCOPE (0).mark2);
				break;
			}

			case SCOPE_For:
			{	// write the incrementor at the end of the loop block
				buffer()->MergeAndDestroy (SCOPE (0).buffer1);
			}
			case SCOPE_While:
			{	// write down the instruction to go back to the start of the loop
				buffer()->WriteDWord (DH_Goto);
				buffer()->AddReference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				buffer()->AdjustMark (SCOPE (0).mark2);
				break;
			}

			case SCOPE_Do:
			{
				mLexer->MustGetNext (TK_While);
				mLexer->MustGetNext (TK_ParenStart);
				DataBuffer* expr = ParseExpression (TYPE_Int);
				mLexer->MustGetNext (TK_ParenEnd);
				mLexer->MustGetNext (TK_Semicolon);

				// If the condition runs true, go back to the start.
				buffer()->MergeAndDestroy (expr);
				buffer()->WriteDWord (DH_IfGoto);
				buffer()->AddReference (SCOPE (0).mark1);
				break;
			}

			case SCOPE_Switch:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != SCOPE (1).cases.begin() - 1)
					mSwitchBuffer = SCOPE (1).casecursor->data;
				else
					mSwitchBuffer = null;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					buffer()->MergeAndDestroy (SCOPE (0).buffer1);
				else
				{
					buffer()->WriteDWord (DH_Drop);
					buffer()->WriteDWord (DH_Goto);
					buffer()->AddReference (SCOPE (0).mark1);
				}

				// Go through all of the buffers we
				// recorded down and write them.
				for (CaseInfo& info : SCOPE (0).cases)
				{
					buffer()->AdjustMark (info.mark);
					buffer()->MergeAndDestroy (info.data);
				}

				// Move the closing mark here
				buffer()->AdjustMark (SCOPE (0).mark1);
				break;
			}

			case SCOPE_Unknown:
				break;
		}

		// Descend down the stack
		mScopeCursor--;
		return;
	}

	int dataheader =	(mCurrentMode == PARSERMODE_Event) ? DH_EndEvent :
						(mCurrentMode == PARSERMODE_MainLoop) ? DH_EndMainLoop :
						(mCurrentMode == PARSERMODE_Onenter) ? DH_EndOnEnter :
						(mCurrentMode == PARSERMODE_Onexit) ? DH_EndOnExit : -1;

	if (dataheader == -1)
		Error ("unexpected `}`");

	// Data header must be written before mode is changed because
	// onenter and mainloop go into special buffers, and we want
	// the closing data headers into said buffers too.
	buffer()->WriteDWord (dataheader);
	mCurrentMode = PARSERMODE_TopLevel;
	mLexer->Next (TK_Semicolon);
}

// =============================================================================
//
void BotscriptParser::ParseEventdef()
{
	EventDefinition* e = new EventDefinition;

	mLexer->MustGetNext (TK_Number);
	e->number = GetTokenString().ToLong();
	mLexer->MustGetNext (TK_Colon);
	mLexer->MustGetNext (TK_Symbol);
	e->name = mLexer->Token()->text;
	mLexer->MustGetNext (TK_ParenStart);
	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_Semicolon);
	AddEvent (e);
}

// =============================================================================
//
void BotscriptParser::ParseFuncdef()
{
	CommandInfo* comm = new CommandInfo;
	comm->origin = mLexer->DescribeCurrentPosition();

	// Return value
	mLexer->MustGetAnyOf ({TK_Int,TK_Void,TK_Bool,TK_Str});
	comm->returnvalue = GetTypeByName (mLexer->Token()->text); // TODO
	assert (comm->returnvalue != -1);

	// Number
	mLexer->MustGetNext (TK_Number);
	comm->number = mLexer->Token()->text.ToLong();
	mLexer->MustGetNext (TK_Colon);

	// Name
	mLexer->MustGetNext (TK_Symbol);
	comm->name = mLexer->Token()->text;

	// Arguments
	mLexer->MustGetNext (TK_ParenStart);
	comm->minargs = 0;

	while (mLexer->PeekNextType (TK_ParenEnd) == false)
	{
		if (comm->args.IsEmpty() == false)
			mLexer->MustGetNext (TK_Comma);

		CommandArgument arg;
		mLexer->MustGetAnyOf ({TK_Int,TK_Bool,TK_Str});
		DataType type = GetTypeByName (mLexer->Token()->text); // TODO
		assert (type != -1 && type != TYPE_Void);
		arg.type = type;

		mLexer->MustGetNext (TK_Symbol);
		arg.name = mLexer->Token()->text;

		// If this is an optional parameter, we need the default value.
		if (comm->minargs < comm->args.Size() || mLexer->PeekNextType (TK_Assign))
		{
			mLexer->MustGetNext (TK_Assign);

			switch (type)
			{
				case TYPE_Int:
				case TYPE_Bool:
					mLexer->MustGetNext (TK_Number);
					break;

				case TYPE_String:
					Error ("string arguments cannot have default values");

				case TYPE_Unknown:
				case TYPE_Void:
					break;
			}

			arg.defvalue = mLexer->Token()->text.ToLong();
		}
		else
			comm->minargs++;

		comm->args << arg;
	}

	mLexer->MustGetNext (TK_ParenEnd);
	mLexer->MustGetNext (TK_Semicolon);
	AddCommandDefinition (comm);
}

// ============================================================================
// Parses a command call
DataBuffer* BotscriptParser::ParseCommand (CommandInfo* comm)
{
	DataBuffer* r = new DataBuffer (64);

	if (mCurrentMode == PARSERMODE_TopLevel && comm->returnvalue == TYPE_Void)
		Error ("command call at top level");

	mLexer->MustGetNext (TK_ParenStart);
	mLexer->MustGetNext (TK_Any);

	int curarg = 0;

	for (;;)
	{
		if (TokenIs (TK_ParenEnd))
		{
			if (curarg < comm->minargs)
				Error ("too few arguments passed to %1\n\tusage is: %2",
					comm->name, comm->GetSignature());

			break;
		}

		if (curarg >= comm->args.Size())
			Error ("too many arguments (%3) passed to %1\n\tusage is: %2",
				comm->name, comm->GetSignature());

		r->MergeAndDestroy (ParseExpression (comm->args[curarg].type, true));
		mLexer->MustGetNext (TK_Any);

		if (curarg < comm->minargs - 1)
		{
			mLexer->TokenMustBe (TK_Comma);
			mLexer->MustGetNext (TK_Any);
		}
		else if (curarg < comm->args.Size() - 1)
		{
			// Can continue, but can terminate as well.
			if (TokenIs (TK_ParenEnd))
			{
				curarg++;
				break;
			}
			else
			{
				mLexer->TokenMustBe (TK_Comma);
				mLexer->MustGetNext (TK_Any);
			}
		}

		curarg++;
	}

	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->args.Size())
	{
		r->WriteDWord (DH_PushNumber);
		r->WriteDWord (comm->args[curarg].defvalue);
		curarg++;
	}

	r->WriteDWord (DH_Command);
	r->WriteDWord (comm->number);
	r->WriteDWord (comm->args.Size());

	return r;
}

// ============================================================================
//
String BotscriptParser::ParseFloat()
{
	mLexer->TokenMustBe (TK_Number);
	String floatstring = GetTokenString();
	Lexer::TokenInfo tok;

	// Go after the decimal point
	if (mLexer->PeekNext (&tok) && tok.type ==TK_Dot)
	{
		mLexer->Skip();
		mLexer->MustGetNext (TK_Number);
		floatstring += ".";
		floatstring += GetTokenString();
	}

	return floatstring;
}

// ============================================================================
//
// Parses an assignment operator.
//
AssignmentOperator BotscriptParser::ParseAssignmentOperator()
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

	mLexer->MustGetAnyOf (tokens);

	switch (mLexer->TokenType())
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

DataHeader BotscriptParser::GetAssigmentDataHeader (AssignmentOperator op, Variable* var)
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

	Error ("WTF: couldn't find data header for operator #%1", op);
	return (DataHeader) 0;
}

// ============================================================================
//
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
//
DataBuffer* BotscriptParser::ParseAssignment (Variable* var)
{
	DataBuffer* retbuf = new DataBuffer;
	DataBuffer* arrayindex = null;

	if (var->writelevel != WRITE_Mutable)
		Error ("cannot alter read-only variable $%1", var->name);

	if (var->isarray)
	{
		mLexer->MustGetNext (TK_BracketStart);
		Expression expr (this, mLexer, TYPE_Int);
		expr.Result()->ConvertToBuffer();
		arrayindex = expr.Result()->Buffer()->Clone();
		mLexer->MustGetNext (TK_BracketEnd);
	}

	// Get an operator
	AssignmentOperator oper = ParseAssignmentOperator();

	if (mCurrentMode == PARSERMODE_TopLevel)
		Error ("can't alter variables at top level");

	// Parse the right operand
	if (oper != ASSIGNOP_Increase && oper != ASSIGNOP_Decrease)
	{
		DataBuffer* expr = ParseExpression (var->type);
		retbuf->MergeAndDestroy (expr);
	}

	if (var->isarray)
		retbuf->MergeAndDestroy (arrayindex);

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

	DataHeader dh = GetAssigmentDataHeader (oper, var);
	retbuf->WriteDWord (dh);
	retbuf->WriteDWord (var->index);
	return retbuf;
}

// ============================================================================
//
void BotscriptParser::PushScope (EReset reset)
{
	mScopeCursor++;

	if (mScopeStack.Size() < mScopeCursor + 1)
	{
		ScopeInfo newscope;
		mScopeStack << newscope;
		reset = SCOPE_Reset;
	}

	if (reset == SCOPE_Reset)
	{
		ScopeInfo* info = &SCOPE (0);
		info->type = SCOPE_Unknown;
		info->mark1 = null;
		info->mark2 = null;
		info->buffer1 = null;
		info->cases.Clear();
		info->casecursor = info->cases.begin() - 1;
	}

	// Reset variable stuff in any case
	SCOPE(0).globalVarIndexBase = (mScopeCursor == 0) ? 0 : SCOPE(1).globalVarIndexBase;
	SCOPE(0).localVarIndexBase = (mScopeCursor == 0) ? 0 : SCOPE(1).localVarIndexBase;

	for (Variable* var : SCOPE(0).globalVariables + SCOPE(0).localVariables)
		delete var;

	SCOPE(0).localVariables.Clear();
	SCOPE(0).globalVariables.Clear();
}

// ============================================================================
//
DataBuffer* BotscriptParser::ParseExpression (DataType reqtype, bool fromhere)
{
	// hehe
	if (fromhere == true)
		mLexer->Skip (-1);

	Expression expr (this, mLexer, reqtype);
	expr.Result()->ConvertToBuffer();

	// The buffer will be destroyed once the function ends so we need to
	// clone it now.
	return expr.Result()->Buffer()->Clone();
}

// ============================================================================
//
DataBuffer* BotscriptParser::ParseStatement()
{
	// If it's a variable, expect assignment.
	if (mLexer->Next (TK_DollarSign))
	{
		mLexer->MustGetNext (TK_Symbol);
		Variable* var = FindVariable (GetTokenString());

		if (var == null)
			Error ("unknown variable $%1", var->name);

		return ParseAssignment (var);
	}

	return null;
}

// ============================================================================
//
void BotscriptParser::AddSwitchCase (DataBuffer* casebuffer)
{
	ScopeInfo* info = &SCOPE (0);
	CaseInfo casedata;

	// Init a mark for the case buffer
	ByteMark* casemark = buffer()->AddMark ("");
	casedata.mark = casemark;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (casebuffer != null)
		casebuffer->AddReference (casemark);
	else
		buffer()->AddReference (casemark);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	casedata.data = mSwitchBuffer = new DataBuffer;
	SCOPE(0).cases << casedata;
	info->casecursor++;
}

// ============================================================================
//
bool BotscriptParser::TokenIs (ETokenType a)
{
	return (mLexer->TokenType() == a);
}

// ============================================================================
//
String BotscriptParser::GetTokenString()
{
	return mLexer->Token()->text;
}

// ============================================================================
//
String BotscriptParser::DescribePosition() const
{
	Lexer::TokenInfo* tok = mLexer->Token();
	return tok->file + ":" + String (tok->line) + ":" + String (tok->column);
}

// ============================================================================
//
DataBuffer* BotscriptParser::buffer()
{
	if (mSwitchBuffer != null)
		return mSwitchBuffer;

	if (mCurrentMode == PARSERMODE_MainLoop)
		return mMainLoopBuffer;

	if (mCurrentMode == PARSERMODE_Onenter)
		return mOnEnterBuffer;

	return mMainBuffer;
}

// ============================================================================
//
void BotscriptParser::writeMemberBuffers()
{
	// If there was no mainloop defined, write a dummy one now.
	if (mGotMainLoop == false)
	{
		mMainLoopBuffer->WriteDWord (DH_MainLoop);
		mMainLoopBuffer->WriteDWord (DH_EndMainLoop);
	}

	// Write the onenter and mainloop buffers, in that order in particular.
	for (DataBuffer** bufp : List<DataBuffer**> ({&mOnEnterBuffer, &mMainLoopBuffer}))
	{
		buffer()->MergeAndDestroy (*bufp);

		// Clear the buffer afterwards for potential next state
		*bufp = new DataBuffer;
	}

	// Next state definitely has no mainloop yet
	mGotMainLoop = false;
}

// ============================================================================
//
// Write string table
//
void BotscriptParser::WriteStringTable()
{
	int stringcount = CountStringsInTable();

	if (stringcount == 0)
		return;

	// Write header
	mMainBuffer->WriteDWord (DH_StringList);
	mMainBuffer->WriteDWord (stringcount);

	// Write all strings
	for (int i = 0; i < stringcount; i++)
		mMainBuffer->WriteString (GetStringTable()[i]);
}

// ============================================================================
//
// Write the compiled bytecode to a file
//
void BotscriptParser::WriteToFile (String outfile)
{
	FILE* fp = fopen (outfile, "wb");

	if (fp == null)
		Error ("couldn't open %1 for writing: %2", outfile, strerror (errno));

	// First, resolve references
	for (MarkReference* ref : mMainBuffer->References())
		for (int i = 0; i < 4; ++i)
			mMainBuffer->Buffer()[ref->pos + i] = (ref->target->pos >> (8 * i)) & 0xFF;

	// Then, dump the main buffer to the file
	fwrite (mMainBuffer->Buffer(), 1, mMainBuffer->WrittenSize(), fp);
	Print ("-- %1 byte%s1 written to %2\n", mMainBuffer->WrittenSize(), outfile);
	fclose (fp);
}

// ============================================================================
//
// Attempt to find the variable by the given name. Looks from current scope
// downwards.
//
Variable* BotscriptParser::FindVariable (const String& name)
{
	for (int i = mScopeCursor; i >= 0; --i)
	{
		for (Variable* var : mScopeStack[i].globalVariables + mScopeStack[i].localVariables)
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
bool BotscriptParser::IsInGlobalState() const
{
	return mCurrentState.IsEmpty();
}

// ============================================================================
//
void BotscriptParser::SuggestHighestVarIndex (bool global, int index)
{
	if (global)
		mHighestGlobalVarIndex = max (mHighestGlobalVarIndex, index);
	else
		mHighestStateVarIndex = max (mHighestStateVarIndex, index);
}

// ============================================================================
//
int BotscriptParser::GetHighestVarIndex (bool global)
{
	if (global)
		return mHighestGlobalVarIndex;

	return mHighestStateVarIndex;
}
