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

// TODO: get rid of this too?
#define MAX_CASE 64

// TODO: get rid of this
#define MAX_MARKS 512

class DataBuffer;
class Lexer;
class Variable;

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
named_enum MarkType
{
	MARK_Label,
	MARK_If,
	MARK_Internal, // internal structures
};

// ============================================================================
// Scope types
//
named_enum ScopeType
{
	SCOPE_Unknown,
	SCOPE_If,
	SCOPE_While,
	SCOPE_For,
	SCOPE_Do,
	SCOPE_Switch,
	SCOPE_Else,
};

named_enum AssignmentOperator
{
	ASSIGNOP_Assign,
	ASSIGNOP_Add,
	ASSIGNOP_Subtract,
	ASSIGNOP_Multiply,
	ASSIGNOP_Divide,
	ASSIGNOP_Modulus,
	ASSIGNOP_Increase,
	ASSIGNOP_Decrease,
};

named_enum Writability
{
	WRITE_Mutable,		// normal read-many-write-many variable
	WRITE_Const,		// write-once const variable
	WRITE_Constexpr,	// const variable whose value is known to compiler
};

// =============================================================================
//
// Parser mode: where is the parser at?
//
named_enum ParserMode
{
	PARSERMODE_TopLevel,	// at top level
	PARSERMODE_Event,		// inside event definition
	PARSERMODE_MainLoop,	// inside mainloop
	PARSERMODE_Onenter,		// inside onenter
	PARSERMODE_Onexit,		// inside onexit
};

// ============================================================================
//
struct Variable
{
	String			name;
	String			statename;
	DataType		type;
	int				index;
	Writability		writelevel;
	int				value;
	String			origin;
	bool			isarray;

	inline bool IsGlobal() const
	{
		return statename.IsEmpty();
	}
};

// ============================================================================
//
struct CaseInfo
{
	ByteMark*		mark;
	int				number;
	DataBuffer*		data;
};

// ============================================================================
//
// Meta-data about scopes
//
struct ScopeInfo
{
	ByteMark*					mark1;
	ByteMark*					mark2;
	ScopeType					type;
	DataBuffer*					buffer1;
	int							globalVarIndexBase;
	int							globalArrayIndexBase;
	int							localVarIndexBase;

	// switch-related stuff
	List<CaseInfo>::Iterator	casecursor;
	List<CaseInfo>				cases;
	List<Variable*>				localVariables;
	List<Variable*>				globalVariables;
	List<Variable*>				globalArrays;
};

// ============================================================================
//
class BotscriptParser
{
	PROPERTY (public, bool, ReadOnly, BOOL_OPS, STOCK_WRITE)

	public:
		enum EReset
		{
			eNoReset,
			SCOPE_Reset,
		};

		BotscriptParser();
		~BotscriptParser();
		void					ParseBotscript (String fileName);
		DataBuffer*				ParseCommand (CommandInfo* comm);
		DataBuffer*				ParseAssignment (Variable* var);
		AssignmentOperator		ParseAssignmentOperator();
		String					ParseFloat();
		void					PushScope (EReset reset = SCOPE_Reset);
		DataBuffer*				ParseStatement();
		void					AddSwitchCase (DataBuffer* b);
		void					CheckToplevel();
		void					CheckNotToplevel();
		bool					TokenIs (EToken a);
		String					GetTokenString();
		String					DescribePosition() const;
		void					WriteToFile (String outfile);
		Variable*				FindVariable (const String& name);
		bool					IsInGlobalState() const;
		void					SuggestHighestVarIndex (bool global, int index);
		int						GetHighestVarIndex (bool global);

		inline ScopeInfo& GSCOPE_t (int offset)
		{
			return mScopeStack[mScopeCursor - offset];
		}

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
		ParserMode				mCurrentMode;
		String					mCurrentState;
		bool					mStateSpawnDefined;
		bool					mGotMainLoop;
		int						mScopeCursor;
		bool					mCanElse;
		List<UndefinedLabel>	mUndefinedLabels;
		int						mHighestGlobalVarIndex;
		int						mHighestStateVarIndex;

		// How many bytes have we written to file?
		int						mNumWrittenBytes;

		// Scope data
		List<ScopeInfo>			mScopeStack;

		DataBuffer*	buffer();
		void			ParseStateBlock();
		void			ParseEventBlock();
		void			ParseMainloop();
		void			ParseOnEnterExit();
		void			ParseVar();
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
		void			ParseLabel();
		void			ParseEventdef();
		void			ParseFuncdef();
		void			writeMemberBuffers();
		void			WriteStringTable();
		DataBuffer*		ParseExpression (DataType reqtype, bool fromhere = false);
		DataHeader		GetAssigmentDataHeader (AssignmentOperator op, Variable* var);
};

#endif // BOTC_PARSER_H
