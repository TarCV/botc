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
#include "Variables.h"
#include "Containers.h"
#include "Lexer.h"
#include "DataBuffer.h"

#define SCOPE(n) (mScopeStack[mScopeCursor - n])

// ============================================================================
//
BotscriptParser::BotscriptParser() :
	mReadOnly (false),
	mLexer (new Lexer) {}

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
	if (mCurrentMode != ETopLevelMode)
		Error ("%1-statements may only be defined at top level!", GetTokenString());
}

// ============================================================================
//
void BotscriptParser::CheckNotToplevel()
{
	if (mCurrentMode == ETopLevelMode)
		Error ("%1-statements must not be defined at top level!", GetTokenString());
}

// ============================================================================
// Main Parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void BotscriptParser::ParseBotscript (String fileName)
{
	// Lex and preprocess the file
	mLexer->ProcessFile (fileName);

	mCurrentMode = ETopLevelMode;
	mNumStates = 0;
	mNumEvents = 0;
	mScopeCursor = 0;
	mStateSpawnDefined = false;
	mGotMainLoop = false;
	mIfExpression = null;
	mCanElse = false;

	// Zero the entire block stack first
	// TODO: this shouldn't be necessary
	for (int i = 0; i < MAX_SCOPE; i++)
		ZERO (mScopeStack[i]);

	while (mLexer->GetNext())
	{
		// Check if else is potentically valid
		if (TokenIs (tkElse) && mCanElse == false)
			Error ("else without preceding if");

		if (TokenIs (tkElse) == false)
			mCanElse = false;

		switch (mLexer->GetToken()->type)
		{
			case tkState:
				ParseStateBlock();
				break;

			case tkEvent:
				ParseEventBlock();
				break;

			case tkMainloop:
				ParseMainloop();
				break;

			case tkOnenter:
			case tkOnexit:
				ParseOnEnterExit();
				break;

			case tkInt:
			case tkStr:
			case tkVoid:
				ParseVariableDeclaration();
				break;

			case tkGoto:
				ParseGoto();
				break;

			case tkIf:
				ParseIf();
				break;

			case tkElse:
				ParseElse();
				break;

			case tkWhile:
				ParseWhileBlock();
				break;

			case tkFor:
				ParseForBlock();
				break;

			case tkDo:
				ParseDoBlock();
				break;

			case tkSwitch:
				ParseSwitchBlock();
				break;

			case tkCase:
				ParseSwitchCase();
				break;

			case tkDefault:
				ParseSwitchDefault();
				break;

			case tkBreak:
				ParseBreak();
				break;

			case tkContinue:
				ParseContinue();
				break;

			case tkBraceEnd:
				ParseBlockEnd();
				break;

			case tkConst:
				ParseConst();
				break;

			case tkEventdef:
				ParseEventdef();
				break;

			case tkFuncdef:
				ParseFuncdef();
				break;

			default:
			{
				// Check for labels
				Lexer::Token next;

				if (TokenIs (tkSymbol) &&
					mLexer->PeekNext (&next) &&
					next.type == tkColon)
				{
					ParseLabel();
					break;
				}

				// Check if it's a command
				CommandInfo* comm = FindCommandByName (GetTokenString());

				if (comm)
				{
					buffer()->MergeAndDestroy (ParseCommand (comm));
					mLexer->MustGetNext (tkSemicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				DataBuffer* b = ParseStatement();

				if (b == false)
					Error ("unknown token `%1`", GetTokenString());

				buffer()->MergeAndDestroy (b);
				mLexer->MustGetNext (tkSemicolon);
			}
			break;
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (mCurrentMode != ETopLevelMode)
		Error ("script did not end at top level; a `}` is missing somewhere");

	if (IsReadOnly() == false)
	{
		// stateSpawn must be defined!
		if (mStateSpawnDefined == false)
			Error ("script must have a state named `stateSpawn`!");

		// Ensure no goto target is left undefined
		if (mUndefinedLabels.IsEmpty() == false)
		{
			StringList names;

			for (UndefinedLabel& undf : mUndefinedLabels)
				names << undf.name;

			Error ("labels `%1` are referenced via `goto` but are not defined\n", names);
		}

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
	mLexer->MustGetNext (tkString);
	String statename = GetTokenString();

	// State name must be a word.
	if (statename.FirstIndexOf (" ") != -1)
		Error ("state name must be a single word, got `%1`", statename);

	// stateSpawn is special - it *must* be defined. If we
	// encountered it, then mark down that we have it.
	if (-statename == "statespawn")
		mStateSpawnDefined = true;

	// Must end in a colon
	mLexer->MustGetNext (tkColon);

	// write the previous state's onenter and
	// mainloop buffers to file now
	if (mCurrentState.IsEmpty() == false)
		writeMemberBuffers();

	buffer()->WriteDWord (dhStateName);
	buffer()->WriteString (statename);
	buffer()->WriteDWord (dhStateIndex);
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
	mLexer->MustGetNext (tkString);

	EventDefinition* e = FindEventByName (GetTokenString());

	if (e == null)
		Error ("bad event, got `%1`\n", GetTokenString());

	mLexer->MustGetNext (tkBraceStart);
	mCurrentMode = EEventMode;
	buffer()->WriteDWord (dhEvent);
	buffer()->WriteDWord (e->number);
	mNumEvents++;
}

// ============================================================================
//
void BotscriptParser::ParseMainloop()
{
	CheckToplevel();
	mLexer->MustGetNext (tkBraceStart);

	mCurrentMode = EMainLoopMode;
	mMainLoopBuffer->WriteDWord (dhMainLoop);
}

// ============================================================================
//
void BotscriptParser::ParseOnEnterExit()
{
	CheckToplevel();
	bool onenter = (TokenIs (tkOnenter));
	mLexer->MustGetNext (tkBraceStart);

	mCurrentMode = onenter ? EOnenterMode : EOnexitMode;
	buffer()->WriteDWord (onenter ? dhOnEnter : dhOnExit);
}

// ============================================================================
//
void BotscriptParser::ParseVariableDeclaration()
{
	// For now, only globals are supported
	if (mCurrentMode != ETopLevelMode || mCurrentState.IsEmpty() == false)
		Error ("variables must only be global for now");

	EType type =	(TokenIs (tkInt)) ? EIntType :
					(TokenIs (tkStr)) ? EStringType :
					EBoolType;

	mLexer->MustGetNext();
	String varname = GetTokenString();

	// Var name must not be a number
	if (varname.IsNumeric())
		Error ("variable name must not be a number");

	ScriptVariable* var = DeclareGlobalVariable (type, varname);
	(void) var;
	mLexer->MustGetNext (tkSemicolon);
}

// ============================================================================
//
void BotscriptParser::ParseGoto()
{
	CheckNotToplevel();

	// Get the name of the label
	mLexer->MustGetNext();

	// Find the mark this goto statement points to
	String target = GetTokenString();
	ByteMark* mark = buffer()->FindMarkByName (target);

	// If not set, define it
	if (mark == null)
	{
		UndefinedLabel undf;
		undf.name = target;
		undf.target = buffer()->AddMark (target);
		mUndefinedLabels << undf;
	}

	// Add a reference to the mark.
	buffer()->WriteDWord (dhGoto);
	buffer()->AddReference (mark);
	mLexer->MustGetNext (tkSemicolon);
}

// ============================================================================
//
void BotscriptParser::ParseIf()
{
	CheckNotToplevel();
	PushScope();

	// Condition
	mLexer->MustGetNext (tkParenStart);

	// Read the expression and write it.
	mLexer->MustGetNext();
	DataBuffer* c = ParseExpression (EIntType);
	buffer()->MergeAndDestroy (c);

	mLexer->MustGetNext (tkParenEnd);
	mLexer->MustGetNext (tkBraceStart);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	ByteMark* mark = buffer()->AddMark ("");

	// Use dhIfNotGoto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	buffer()->WriteDWord (dhIfNotGoto);
	buffer()->AddReference (mark);

	// Store it
	SCOPE (0).mark1 = mark;
	SCOPE (0).type = eIfScope;
}

// ============================================================================
//
void BotscriptParser::ParseElse()
{
	CheckNotToplevel();
	mLexer->MustGetNext (tkBraceStart);

	// Don't use PushScope as it resets the scope
	mScopeCursor++;

	if (mScopeCursor >= MAX_SCOPE)
		Error ("too deep scope");

	if (SCOPE (0).type != eIfScope)
		Error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = buffer()->AddMark ("");

	// Instruction to jump to the end after if block is complete
	buffer()->WriteDWord (dhGoto);
	buffer()->AddReference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	buffer()->AdjustMark (SCOPE (0).mark1);
	SCOPE (0).type = eElseScope;
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
	mLexer->MustGetNext (tkParenStart);
	mLexer->MustGetNext();
	DataBuffer* expr = ParseExpression (EIntType);
	mLexer->MustGetNext (tkParenEnd);
	mLexer->MustGetNext (tkBraceStart);

	// write condition
	buffer()->MergeAndDestroy (expr);

	// Instruction to go to the end if it fails
	buffer()->WriteDWord (dhIfNotGoto);
	buffer()->AddReference (mark2);

	// Store the needed stuff
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).type = eWhileScope;
}

// ============================================================================
//
void BotscriptParser::ParseForBlock()
{
	CheckNotToplevel();
	PushScope();

	// Initializer
	mLexer->MustGetNext (tkParenStart);
	mLexer->MustGetNext();
	DataBuffer* init = ParseStatement();

	if (init == null)
		Error ("bad statement for initializer of for");

	mLexer->MustGetNext (tkSemicolon);

	// Condition
	mLexer->MustGetNext();
	DataBuffer* cond = ParseExpression (EIntType);

	if (cond == null)
		Error ("bad statement for condition of for");

	mLexer->MustGetNext (tkSemicolon);

	// Incrementor
	mLexer->MustGetNext();
	DataBuffer* incr = ParseStatement();

	if (incr == null)
		Error ("bad statement for incrementor of for");

	mLexer->MustGetNext (tkParenEnd);
	mLexer->MustGetNext (tkBraceStart);

	// First, write out the initializer
	buffer()->MergeAndDestroy (init);

	// Init two marks
	ByteMark* mark1 = buffer()->AddMark ("");
	ByteMark* mark2 = buffer()->AddMark ("");

	// Add the condition
	buffer()->MergeAndDestroy (cond);
	buffer()->WriteDWord (dhIfNotGoto);
	buffer()->AddReference (mark2);

	// Store the marks and incrementor
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).buffer1 = incr;
	SCOPE (0).type = eForScope;
}

// ============================================================================
//
void BotscriptParser::ParseDoBlock()
{
	CheckNotToplevel();
	PushScope();
	mLexer->MustGetNext (tkBraceStart);
	SCOPE (0).mark1 = buffer()->AddMark ("");
	SCOPE (0).type = eDoScope;
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
	mLexer->MustGetNext (tkParenStart);
	mLexer->MustGetNext();
	buffer()->MergeAndDestroy (ParseExpression (EIntType));
	mLexer->MustGetNext (tkParenEnd);
	mLexer->MustGetNext (tkBraceStart);
	SCOPE (0).type = eSwitchScope;
	SCOPE (0).mark1 = buffer()->AddMark (""); // end mark
	SCOPE (0).buffer1 = null; // default header
}

// ============================================================================
//
void BotscriptParser::ParseSwitchCase()
{
	// case is only allowed inside switch
	if (SCOPE (0).type != eSwitchScope)
		Error ("case label outside switch");

	// Get the literal (Zandronum does not support expressions here)
	mLexer->MustGetNext (tkNumber);
	int num = mLexer->GetToken()->text.ToLong();
	mLexer->MustGetNext (tkColon);

	for (int i = 0; i < MAX_CASE; i++)
		if (SCOPE (0).casenumbers[i] == num)
			Error ("multiple case %d labels in one switch", num);

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
	buffer()->WriteDWord (dhCaseGoto);
	buffer()->WriteDWord (num);
	AddSwitchCase (null);
	SCOPE (0).casenumbers[SCOPE (0).casecursor] = num;
}

// ============================================================================
//
void BotscriptParser::ParseSwitchDefault()
{
	if (SCOPE (0).type != eSwitchScope)
		Error ("default label outside switch");

	if (SCOPE (0).buffer1)
		Error ("multiple default labels in one switch");

	mLexer->MustGetNext (tkColon);

	// The default header is buffered into buffer1, since
	// it has to be the last of the case headers
	//
	// Since the expression is pushed into the switch
	// and is only popped when case succeeds, we have
	// to pop it with dhDrop manually if we end up in
	// a default.
	DataBuffer* b = new DataBuffer;
	SCOPE (0).buffer1 = b;
	b->WriteDWord (dhDrop);
	b->WriteDWord (dhGoto);
	AddSwitchCase (b);
}

// ============================================================================
//
void BotscriptParser::ParseBreak()
{
	if (mScopeCursor == 0)
		Error ("unexpected `break`");

	buffer()->WriteDWord (dhGoto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case eIfScope:
		case eSwitchScope:
		{
			buffer()->AddReference (SCOPE (0).mark1);
		} break;

		case eForScope:
		case eWhileScope:
		{
			buffer()->AddReference (SCOPE (0).mark2);
		} break;

		default:
		{
			Error ("unexpected `break`");
		} break;
	}

	mLexer->MustGetNext (tkSemicolon);
}

// ============================================================================
//
void BotscriptParser::ParseContinue()
{
	mLexer->MustGetNext (tkSemicolon);

	int curs;
	bool found = false;

	// Fall through the scope until we find a loop block
	for (curs = mScopeCursor; curs > 0 && !found; curs--)
	{
		switch (mScopeStack[curs].type)
		{
			case eForScope:
			case eWhileScope:
			case eDoScope:
			{
				buffer()->WriteDWord (dhGoto);
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
			case eIfScope:
				// Adjust the closing mark.
				buffer()->AdjustMark (SCOPE (0).mark1);

				// We're returning from if, thus else can be next
				mCanElse = true;
				break;

			case eElseScope:
				// else instead uses mark1 for itself (so if expression
				// fails, jump to else), mark2 means end of else
				buffer()->AdjustMark (SCOPE (0).mark2);
				break;

			case eForScope:
				// write the incrementor at the end of the loop block
				buffer()->MergeAndDestroy (SCOPE (0).buffer1);
			case eWhileScope:
				// write down the instruction to go back to the start of the loop
				buffer()->WriteDWord (dhGoto);
				buffer()->AddReference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				buffer()->AdjustMark (SCOPE (0).mark2);
				break;

			case eDoScope:
			{
				mLexer->MustGetNext (tkWhile);
				mLexer->MustGetNext (tkParenStart);
				mLexer->MustGetNext();
				DataBuffer* expr = ParseExpression (EIntType);
				mLexer->MustGetNext (tkParenEnd);
				mLexer->MustGetNext (tkSemicolon);

				// If the condition runs true, go back to the start.
				buffer()->MergeAndDestroy (expr);
				buffer()->WriteDWord (dhIfGoto);
				buffer()->AddReference (SCOPE (0).mark1);
				break;
			}

			case eSwitchScope:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != -1)
					mSwitchBuffer = SCOPE (1).casebuffers[SCOPE (1).casecursor];
				else
					mSwitchBuffer = null;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					buffer()->MergeAndDestroy (SCOPE (0).buffer1);
				else
				{
					buffer()->WriteDWord (dhDrop);
					buffer()->WriteDWord (dhGoto);
					buffer()->AddReference (SCOPE (0).mark1);
				}

				// Go through all of the buffers we
				// recorded down and write them.
				for (int u = 0; u < MAX_CASE; u++)
				{
					if (SCOPE (0).casebuffers[u] == null)
						continue;

					buffer()->AdjustMark (SCOPE (0).casemarks[u]);
					buffer()->MergeAndDestroy (SCOPE (0).casebuffers[u]);
				}

				// Move the closing mark here
				buffer()->AdjustMark (SCOPE (0).mark1);
				break;
			}

			case eUnknownScope:
				break;
		}

		// Descend down the stack
		mScopeCursor--;
		return;
	}

	int dataheader =	(mCurrentMode == EEventMode) ? dhEndEvent :
						(mCurrentMode == EMainLoopMode) ? dhEndMainLoop :
						(mCurrentMode == EOnenterMode) ? dhEndOnEnter :
						(mCurrentMode == EOnexitMode) ? dhEndOnExit : -1;

	if (dataheader == -1)
		Error ("unexpected `}`");

	// Data header must be written before mode is changed because
	// onenter and mainloop go into special buffers, and we want
	// the closing data headers into said buffers too.
	buffer()->WriteDWord (dataheader);
	mCurrentMode = ETopLevelMode;
	mLexer->GetNext (tkSemicolon);
}

// ============================================================================
//
void BotscriptParser::ParseConst()
{
	ConstantInfo info;

	// Get the type
	mLexer->MustGetNext();
	String typestring = GetTokenString();
	info.type = GetTypeByName (typestring);

	if (info.type == EUnknownType || info.type == EVoidType)
		Error ("unknown type `%1` for constant", typestring);

	mLexer->MustGetNext();
	info.name = GetTokenString();

	mLexer->MustGetNext (tkAssign);

	switch (info.type)
	{
		case EBoolType:
		case EIntType:
		{
			mLexer->MustGetNext (tkNumber);
		} break;

		case EStringType:
		{
			mLexer->MustGetNext (tkString);
		} break;

		case EUnknownType:
		case EVoidType:
			break;
	}

	info.val = mLexer->GetToken()->text;
	mConstants << info;

	mLexer->MustGetNext (tkSemicolon);
}

// ============================================================================
//
void BotscriptParser::ParseLabel()
{
	CheckNotToplevel();
	String labelName = GetTokenString();
	ByteMark* mark = null;

	// want no conflicts..
	if (FindCommandByName (labelName))
		Error ("label name `%1` conflicts with command name\n", labelName);

	if (FindGlobalVariable (labelName))
		Error ("label name `%1` conflicts with variable\n", labelName);

	// See if a mark already exists for this label
	for (UndefinedLabel& undf : mUndefinedLabels)
	{
		if (undf.name != labelName)
			continue;

		mark = undf.target;
		buffer()->AdjustMark (mark);

		// No longer undefined
		mUndefinedLabels.Remove (undf);
		break;
	}

	// Not found in unmarked lists, define it now
	if (mark == null)
		buffer()->AddMark (labelName);

	mLexer->MustGetNext (tkColon);
}

// =============================================================================
//
void BotscriptParser::ParseEventdef()
{
	EventDefinition* e = new EventDefinition;

	mLexer->MustGetNext (tkNumber);
	e->number = GetTokenString().ToLong();
	mLexer->MustGetNext (tkColon);
	mLexer->MustGetNext (tkSymbol);
	e->name = mLexer->GetToken()->text;
	mLexer->MustGetNext (tkParenStart);
	mLexer->MustGetNext (tkParenEnd);
	mLexer->MustGetNext (tkSemicolon);
	AddEvent (e);
}

// =============================================================================
//
void BotscriptParser::ParseFuncdef()
{
	CommandInfo* comm = new CommandInfo;

	// Number
	mLexer->MustGetNext (tkNumber);
	comm->number = mLexer->GetToken()->text.ToLong();

	mLexer->MustGetNext (tkColon);

	// Name
	mLexer->MustGetNext (tkSymbol);
	comm->name = mLexer->GetToken()->text;

	mLexer->MustGetNext (tkColon);

	// Return value
	mLexer->MustGetAnyOf ({tkInt, tkVoid, tkBool, tkStr});
	comm->returnvalue = GetTypeByName (mLexer->GetToken()->text); // TODO
	assert (comm->returnvalue != -1);

	mLexer->MustGetNext (tkColon);

	// Num args
	mLexer->MustGetNext (tkNumber);
	comm->numargs = mLexer->GetToken()->text.ToLong();

	mLexer->MustGetNext (tkColon);

	// Max args
	mLexer->MustGetNext (tkNumber);
	comm->maxargs = mLexer->GetToken()->text.ToLong();

	// Argument types
	int curarg = 0;

	while (curarg < comm->maxargs)
	{
		CommandArgument arg;
		mLexer->MustGetNext (tkColon);
		mLexer->MustGetAnyOf ({tkInt, tkBool, tkStr});
		EType type = GetTypeByName (mLexer->GetToken()->text);
		assert (type != -1 && type != EVoidType);
		arg.type = type;

		mLexer->MustGetNext (tkParenStart);
		mLexer->MustGetNext (tkSymbol);
		arg.name = mLexer->GetToken()->text;

		// If this is an optional parameter, we need the default value.
		if (curarg >= comm->numargs)
		{
			mLexer->MustGetNext (tkAssign);

			switch (type)
			{
				case EIntType:
				case EBoolType:
					mLexer->MustGetNext (tkNumber);
					break;

				case EStringType:
					mLexer->MustGetNext (tkString);
					break;

				case EUnknownType:
				case EVoidType:
					break;
			}

			arg.defvalue = mLexer->GetToken()->text.ToLong();
		}

		mLexer->MustGetNext (tkParenEnd);
		comm->args << arg;
		curarg++;
	}

	mLexer->MustGetNext (tkSemicolon);
	AddCommandDefinition (comm);
}

// ============================================================================
// Parses a command call
DataBuffer* BotscriptParser::ParseCommand (CommandInfo* comm)
{
	DataBuffer* r = new DataBuffer (64);

	if (mCurrentMode == ETopLevelMode)
		Error ("command call at top level");

	mLexer->MustGetNext (tkParenStart);
	mLexer->MustGetNext();

	int curarg = 0;

	while (1)
	{
		if (TokenIs (tkParenEnd))
		{
			if (curarg < comm->numargs)
				Error ("too few arguments passed to %1\n\tusage is: %2",
					comm->name, comm->GetSignature());

			break;
			curarg++;
		}

		if (curarg >= comm->maxargs)
			Error ("too many arguments passed to %1\n\tusage is: %2",
				comm->name, comm->GetSignature());

		r->MergeAndDestroy (ParseExpression (comm->args[curarg].type));
		mLexer->MustGetNext();

		if (curarg < comm->numargs - 1)
		{
			mLexer->TokenMustBe (tkComma);
			mLexer->MustGetNext();
		}
		else if (curarg < comm->maxargs - 1)
		{
			// Can continue, but can terminate as well.
			if (TokenIs (tkParenEnd))
			{
				curarg++;
				break;
			}
			else
			{
				mLexer->TokenMustBe (tkComma);
				mLexer->MustGetNext();
			}
		}

		curarg++;
	}

	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->maxargs)
	{
		r->WriteDWord (dhPushNumber);
		r->WriteDWord (comm->args[curarg].defvalue);
		curarg++;
	}

	r->WriteDWord (dhCommand);
	r->WriteDWord (comm->number);
	r->WriteDWord (comm->maxargs);

	return r;
}

// ============================================================================
// Is the given operator an assignment operator?
//
static bool IsAssignmentOperator (int oper)
{
	switch (oper)
	{
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
//
static word GetDataHeaderByOperator (ScriptVariable* var, int oper)
{
	if (IsAssignmentOperator (oper))
	{
		if (var == null)
			Error ("operator %d requires left operand to be a variable\n", oper);

		// TODO: At the moment, vars only are global
		// OPER_ASSIGNLEFTSHIFT and OPER_ASSIGNRIGHTSHIFT do not
		// have data headers, instead they are expanded out in
		// the operator Parser
		switch (oper)
		{
			case OPER_ASSIGNADD: return dhAddGlobalVar;
			case OPER_ASSIGNSUB: return dhSubtractGlobalVar;
			case OPER_ASSIGNMUL: return dhMultiplyGlobalVar;
			case OPER_ASSIGNDIV: return dhDivideGlobalVar;
			case OPER_ASSIGNMOD: return dhModGlobalVar;
			case OPER_ASSIGN: return dhAssignGlobalVar;

			default: Error ("bad assignment operator!\n");
		}
	}

	switch (oper)
	{
		case OPER_ADD: return dhAdd;
		case OPER_SUBTRACT: return dhSubtract;
		case OPER_MULTIPLY: return dhMultiply;
		case OPER_DIVIDE: return dhDivide;
		case OPER_MODULUS: return dhModulus;
		case OPER_EQUALS: return dhEquals;
		case OPER_NOTEQUALS: return dhNotEquals;
		case OPER_LESSTHAN: return dhLessThan;
		case OPER_GREATERTHAN: return dhGreaterThan;
		case OPER_LESSTHANEQUALS: return dhAtMost;
		case OPER_GREATERTHANEQUALS: return dhAtLeast;
		case OPER_LEFTSHIFT: return dhLeftShift;
		case OPER_RIGHTSHIFT: return dhRightShift;
		case OPER_OR: return dhOrLogical;
		case OPER_AND: return dhAndLogical;
		case OPER_BITWISEOR: return dhOrBitwise;
		case OPER_BITWISEEOR: return dhEorBitwise;
		case OPER_BITWISEAND: return dhAndBitwise;
	}

	Error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
//
DataBuffer* BotscriptParser::ParseExpression (EType reqtype)
{
	DataBuffer* retbuf = new DataBuffer (64);

	// Parse first operand
	retbuf->MergeAndDestroy (ParseExprValue (reqtype));

	// Parse any and all operators we get
	int oper;

	while ( (oper = ParseOperator (true)) != -1)
	{
		// We peeked the operator, move forward now
		mLexer->Skip();

		// Can't be an assignement operator, those belong in assignments.
		if (IsAssignmentOperator (oper))
			Error ("assignment operator inside expression");

		// Parse the right operand.
		mLexer->MustGetNext();
		DataBuffer* rb = ParseExprValue (reqtype);

		if (oper == OPER_TERNARY)
		{
			// Ternary operator requires - naturally - a third operand.
			mLexer->MustGetNext (tkColon);
			mLexer->MustGetNext();
			DataBuffer* tb = ParseExprValue (reqtype);

			// It also is handled differently: there isn't a dataheader for ternary
			// operator. Instead, we abuse PUSHNUMBER and IFNOTGOTO for this.
			// Behold, big block of writing madness! :P
			ByteMark* mark1 = retbuf->AddMark (""); // start of "else" case
			ByteMark* mark2 = retbuf->AddMark (""); // end of expression
			retbuf->WriteDWord (dhIfNotGoto); // if the first operand (condition)
			retbuf->AddReference (mark1); // didn't eval true, jump into mark1
			retbuf->MergeAndDestroy (rb); // otherwise, perform second operand (true case)
			retbuf->WriteDWord (dhGoto); // afterwards, jump to the end, which is
			retbuf->AddReference (mark2); // marked by mark2.
			retbuf->AdjustMark (mark1); // move mark1 at the end of the true case
			retbuf->MergeAndDestroy (tb); // perform third operand (false case)
			retbuf->AdjustMark (mark2); // move the ending mark2 here
		}
		else
		{
			// write to buffer
			retbuf->MergeAndDestroy (rb);
			retbuf->WriteDWord (GetDataHeaderByOperator (null, oper));
		}
	}

	return retbuf;
}

// ============================================================================
// Parses an operator string. Returns the operator number code.
//
#define ISNEXT(C) (mLexer->PeekNextString (peek ? 1 : 0) == C)

int BotscriptParser::ParseOperator (bool peek)
{
	String oper;

	if (peek)
		oper += mLexer->PeekNextString();
	else
		oper += GetTokenString();

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

	if (o != -1)
	{
		return o;
	}

	// Two-char operators
	oper += mLexer->PeekNextString (peek ? 1 : 0);
	equalsnext = mLexer->PeekNextString (peek ? 2 : 1) == ("=");

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

	if (o != -1)
	{
		mLexer->MustGetNext();
		return o;
	}

	// Three-char opers
	oper += mLexer->PeekNextString (peek ? 2 : 1);
	o =	oper == "<<=" ? OPER_ASSIGNLEFTSHIFT :
		oper == ">>=" ? OPER_ASSIGNRIGHTSHIFT :
		-1;

	if (o != -1)
	{
		mLexer->MustGetNext();
		mLexer->MustGetNext();
	}

	return o;
}

// ============================================================================
//
String BotscriptParser::ParseFloat()
{
	mLexer->TokenMustBe (tkNumber);
	String floatstring = GetTokenString();
	Lexer::Token tok;

	// Go after the decimal point
	if (mLexer->PeekNext (&tok) && tok.type == tkDot)
	{
		mLexer->Skip();
		mLexer->MustGetNext (tkNumber);
		floatstring += ".";
		floatstring += GetTokenString();
	}

	return floatstring;
}

// ============================================================================
// Parses a value in the expression and returns the data needed to push
// it, contained in a data buffer. A value can be either a variable, a command,
// a literal or an expression.
//
DataBuffer* BotscriptParser::ParseExprValue (EType reqtype)
{
	DataBuffer* b = new DataBuffer (16);
	ScriptVariable* g;

	// Prefixing "!" means negation.
	bool negate = TokenIs (tkExclamationMark);

	if (negate) // Jump past the "!"
		mLexer->Skip();

	if (TokenIs (tkParenStart))
	{
		// Expression
		mLexer->MustGetNext();
		DataBuffer* c = ParseExpression (reqtype);
		b->MergeAndDestroy (c);
		mLexer->MustGetNext (tkParenEnd);
	}
	else if (CommandInfo* comm = FindCommandByName (GetTokenString()))
	{
		delete b;

		// Command
		if (reqtype && comm->returnvalue != reqtype)
			Error ("%1 returns an incompatible data type", comm->name);

		b = ParseCommand (comm);
	}
	else if (ConstantInfo* constant = FindConstant (GetTokenString()))
	{
		// Type check
		if (reqtype != constant->type)
			Error ("constant `%1` is %2, expression requires %3\n",
				constant->name, GetTypeName (constant->type),
				GetTypeName (reqtype));

		switch (constant->type)
		{
			case EBoolType:
			case EIntType:
			{
				b->WriteDWord (dhPushNumber);
				b->WriteDWord (constant->val.ToLong());
				break;
			}

			case EStringType:
			{
				b->WriteStringIndex (constant->val);
				break;
			}

			case EVoidType:
			case EUnknownType:
				break;
		}
	}
	else if ((g = FindGlobalVariable (GetTokenString())))
	{
		// Global variable
		b->WriteDWord (dhPushGlobalVar);
		b->WriteDWord (g->index);
	}
	else
	{
		// If nothing else, check for literal
		switch (reqtype)
		{
			case EVoidType:
			case EUnknownType:
			{
				Error ("unknown identifier `%1` (expected keyword, function or variable)", GetTokenString());
				break;
			}

			case EBoolType:
			case EIntType:
			{
				mLexer->TokenMustBe (tkNumber);

				// All values are written unsigned - thus we need to write the value's
				// absolute value, followed by an unary minus for negatives.
				b->WriteDWord (dhPushNumber);

				long v = GetTokenString().ToLong();
				b->WriteDWord (static_cast<word> (abs (v)));

				if (v < 0)
					b->WriteDWord (dhUnaryMinus);

				break;
			}

			case EStringType:
			{
				// PushToStringTable either returns the string index of the
				// string if it finds it in the table, or writes it to the
				// table and returns it index if it doesn't find it there.
				mLexer->TokenMustBe (tkString);
				b->WriteStringIndex (GetTokenString());
				break;
			}
		}
	}

	// Negate it now if desired
	if (negate)
		b->WriteDWord (dhNegateLogical);

	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
//
DataBuffer* BotscriptParser::ParseAssignment (ScriptVariable* var)
{
	// Get an operator
	mLexer->MustGetNext();
	int oper = ParseOperator();

	if (IsAssignmentOperator (oper) == false)
		Error ("expected assignment operator");

	if (mCurrentMode == ETopLevelMode)
		Error ("can't alter variables at top level");

	// Parse the right operand
	mLexer->MustGetNext();
	DataBuffer* retbuf = new DataBuffer;
	DataBuffer* expr = ParseExpression (var->type);

	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	if (oper == OPER_ASSIGNLEFTSHIFT || oper == OPER_ASSIGNRIGHTSHIFT)
	{
		retbuf->WriteDWord (var->IsGlobal() ? dhPushGlobalVar : dhPushLocalVar);
		retbuf->WriteDWord (var->index);
		retbuf->MergeAndDestroy (expr);
		retbuf->WriteDWord ((oper == OPER_ASSIGNLEFTSHIFT) ? dhLeftShift : dhRightShift);
		retbuf->WriteDWord (var->IsGlobal() ? dhAssignGlobalVar : dhAssignLocalVar);
		retbuf->WriteDWord (var->index);
	}
	else
	{
		retbuf->MergeAndDestroy (expr);
		long dh = GetDataHeaderByOperator (var, oper);
		retbuf->WriteDWord (dh);
		retbuf->WriteDWord (var->index);
	}

	return retbuf;
}

// ============================================================================
//
void BotscriptParser::PushScope()
{
	mScopeCursor++;

	if (mScopeCursor >= MAX_SCOPE)
		Error ("too deep scope");

	ScopeInfo* info = &SCOPE (0);
	info->type = eUnknownScope;
	info->mark1 = null;
	info->mark2 = null;
	info->buffer1 = null;
	info->casecursor = -1;

	for (int i = 0; i < MAX_CASE; i++)
	{
		info->casemarks[i] = null;
		info->casebuffers[i] = null;
		info->casenumbers[i] = -1;
	}
}

// ============================================================================
//
DataBuffer* BotscriptParser::ParseStatement()
{
	if (FindConstant (GetTokenString())) // There should not be constants here.
		Error ("invalid use for constant\n");

	// If it's a variable, expect assignment.
	if (ScriptVariable* var = FindGlobalVariable (GetTokenString()))
		return ParseAssignment (var);

	return null;
}

// ============================================================================
//
void BotscriptParser::AddSwitchCase (DataBuffer* b)
{
	ScopeInfo* info = &SCOPE (0);

	info->casecursor++;

	if (info->casecursor >= MAX_CASE)
		Error ("too many cases in one switch");

	// Init a mark for the case buffer
	ByteMark* casemark = buffer()->AddMark ("");
	info->casemarks[info->casecursor] = casemark;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (b)
		b->AddReference (casemark);
	else
		buffer()->AddReference (casemark);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	info->casebuffers[info->casecursor] = mSwitchBuffer = new DataBuffer;
}

// ============================================================================
//
ConstantInfo* BotscriptParser::FindConstant (const String& tok)
{
	for (int i = 0; i < mConstants.Size(); i++)
		if (mConstants[i].name == tok)
			return &mConstants[i];

	return null;
}

// ============================================================================
//
bool BotscriptParser::TokenIs (EToken a)
{
	return (mLexer->GetTokenType() == a);
}

// ============================================================================
//
String BotscriptParser::GetTokenString()
{
	return mLexer->GetToken()->text;
}

// ============================================================================
//
String BotscriptParser::DescribePosition() const
{
	Lexer::Token* tok = mLexer->GetToken();
	return tok->file + ":" + String (tok->line) + ":" + String (tok->column);
}

// ============================================================================
//
DataBuffer* BotscriptParser::buffer()
{
	if (mSwitchBuffer != null)
		return mSwitchBuffer;

	if (mCurrentMode == EMainLoopMode)
		return mMainLoopBuffer;

	if (mCurrentMode == EOnenterMode)
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
		mMainLoopBuffer->WriteDWord (dhMainLoop);
		mMainLoopBuffer->WriteDWord (dhEndMainLoop);
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
	mMainBuffer->WriteDWord (dhStringList);
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
	FILE* fp = fopen (outfile, "w");

	if (fp == null)
		Error ("couldn't open %1 for writing: %2", outfile, strerror (errno));

	// First, resolve references
	for (MarkReference* ref : mMainBuffer->GetReferences())
	{
		// Substitute the placeholder with the mark position
		for (int v = 0; v < 4; v++)
			mMainBuffer->GetBuffer()[ref->pos + v] = ((ref->target->pos) << (8 * v)) & 0xFF;

		Print ("reference at %1 resolved to mark at %2\n", ref->pos, ref->target->pos);
	}

	// Then, dump the main buffer to the file
	fwrite (mMainBuffer->GetBuffer(), 1, mMainBuffer->GetWrittenSize(), fp);
	Print ("-- %1 byte%s1 written to %2\n", mMainBuffer->GetWrittenSize(), outfile);
	fclose (fp);
}