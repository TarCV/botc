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

#ifndef BOTC_PARSER_H
#define BOTC_PARSER_H

#include <stdio.h>
#include "Main.h"
#include "Commands.h"
#include "LexerScanner.h"
#include "Tokens.h"

// TODO: get rid of this
#define MAX_SCOPE 32

// TODO: get rid of this too?
#define MAX_CASE 64

// TODO: get rid of this
#define MAX_MARKS 512

class DataBuffer;
class Lexer;
class ScriptVariable;

// ============================================================================
//
struct UndefinedLabel
{
	String		name;
	ByteMark*	target;
};

// ============================================================================
// Mark types
//
enum eMarkType
{
	eLabelMark,
	eIfMark,
	eInternalMark, // internal structures
};

// ============================================================================
// Scope types
//
enum EScopeType
{
	eUnknownScope,
	eIfScope,
	eWhileScope,
	eForScope,
	eDoScope,
	eSwitchScope,
	eElseScope,
};

enum EAssignmentOperator
{
	EAssign,
	EAssignAdd,
	EAssignSub,
	EAssignMul,
	EAssignDiv,
	EAssignMod,
};

// ============================================================================
// Meta-data about scopes
//
struct ScopeInfo
{
	ByteMark*		mark1;
	ByteMark*		mark2;
	EScopeType		type;
	DataBuffer*		buffer1;

	// switch-related stuff
	// Which case are we at?
	int				casecursor;

	// Marks to case-blocks
	ByteMark*		casemarks[MAX_CASE];

	// Numbers of the case labels
	int				casenumbers[MAX_CASE];

	// actual case blocks
	DataBuffer*	casebuffers[MAX_CASE];

	// What is the current buffer of the block?
	DataBuffer*	recordbuffer;
};

// ============================================================================
//
struct ConstantInfo
{
	String name;
	EType type;
	String val;
};

// ============================================================================
//
class BotscriptParser
{
	PROPERTY (public, bool, ReadOnly, BOOL_OPS, STOCK_WRITE)

	public:
		// ====================================================================
		// METHODS
		BotscriptParser();
		~BotscriptParser();
		ConstantInfo*			FindConstant (const String& tok);
		void					ParseBotscript (String fileName);
		DataBuffer*				ParseCommand (CommandInfo* comm);
		DataBuffer*				ParseAssignment (ScriptVariable* var);
		EAssignmentOperator		ParseAssignmentOperator ();
		String					ParseFloat();
		void					PushScope();
		DataBuffer*				ParseStatement();
		void					AddSwitchCase (DataBuffer* b);
		void					CheckToplevel();
		void					CheckNotToplevel();
		bool					TokenIs (EToken a);
		String					GetTokenString();
		String					DescribePosition() const;
		void					WriteToFile (String outfile);

		inline int GetNumEvents() const
		{
			return mNumEvents;
		}

		inline int GetNumStates() const
		{
			return mNumStates;
		}

	private:
		// The main buffer - the contents of this is what we
		// write to file after parsing is complete
		DataBuffer*				mMainBuffer;

		// onenter buffer - the contents of the onenter{} block
		// is buffered here and is merged further at the end of state
		DataBuffer*				mOnEnterBuffer;

		// Mainloop buffer - the contents of the mainloop{} block
		// is buffered here and is merged further at the end of state
		DataBuffer*				mMainLoopBuffer;

		// Switch buffer - switch case data is recorded to this
		// buffer initially, instead of into main buffer.
		DataBuffer*				mSwitchBuffer;

		Lexer*					mLexer;
		int						mNumStates;
		int						mNumEvents;
		EParserMode				mCurrentMode;
		String					mCurrentState;
		bool					mStateSpawnDefined;
		bool					mGotMainLoop;
		int						mScopeCursor;
		DataBuffer*				mIfExpression;
		bool					mCanElse;
		List<UndefinedLabel>	mUndefinedLabels;
		List<ConstantInfo>		mConstants;

		// How many bytes have we written to file?
		int						mNumWrittenBytes;

		// Scope data
		// TODO: make a List
		ScopeInfo				mScopeStack[MAX_SCOPE];

		DataBuffer*	buffer();
		void			ParseStateBlock();
		void			ParseEventBlock();
		void			ParseMainloop();
		void			ParseOnEnterExit();
		void			ParseVariableDeclaration();
		void			ParseGoto();
		void			ParseIf();
		void			ParseElse();
		void			ParseWhileBlock();
		void			ParseForBlock();
		void			ParseDoBlock();
		void			ParseSwitchBlock();
		void			ParseSwitchCase();
		void			ParseSwitchDefault();
		void			ParseBreak();
		void			ParseContinue();
		void			ParseBlockEnd();
		void			ParseConst();
		void			ParseLabel();
		void			ParseEventdef();
		void			ParseFuncdef();
		void			writeMemberBuffers();
		void			WriteStringTable();
		DataBuffer*		ParseExpression (EType reqtype, bool fromhere = false);
		EDataHeader		GetAssigmentDataHeader (EAssignmentOperator op, ScriptVariable* var);
};

#endif // BOTC_PARSER_H
