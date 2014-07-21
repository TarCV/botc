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

#ifndef BOTC_PARSER_H
#define BOTC_PARSER_H

#include <stdio.h>
#include "main.h"
#include "commands.h"
#include "lexerScanner.h"
#include "tokens.h"

class DataBuffer;
class Lexer;
class Variable;

// ============================================================================
// Mark types
//
named_enum MarkType : char
{
	MARK_Label,
	MARK_If,
	MARK_Internal, // internal structures
};

// ============================================================================
// Scope types
//
named_enum ScopeType : char
{
	SCOPE_Unknown,
	SCOPE_If,
	SCOPE_While,
	SCOPE_For,
	SCOPE_Do,
	SCOPE_Switch,
	SCOPE_Else,
};

named_enum AssignmentOperator : char
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

named_enum Writability : char
{
	WRITE_Mutable,		// normal read-many-write-many variable
	WRITE_Const,		// write-once const variable
	WRITE_Constexpr,	// const variable whose value is known to compiler
};

// =============================================================================
//
// Parser mode: where is the parser at?
//
named_enum class ParserMode
{
	TopLevel,	// at top level
	Event,		// inside event definition
	MainLoop,	// inside mainloop
	Onenter,	// inside onenter
	Onexit,		// inside onexit
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

	inline bool isGlobal() const
	{
		return statename.isEmpty();
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
	PROPERTY (public, bool, isReadOnly, setReadOnly, STOCK_WRITE)

public:
	BotscriptParser();
	~BotscriptParser();
	void					parseBotscript (String fileName);
	DataBuffer*				parseCommand (CommandInfo* comm);
	DataBuffer*				parseAssignment (Variable* var);
	AssignmentOperator		parseAssignmentOperator();
	String					parseFloat();
	void					pushScope (bool noreset = false);
	DataBuffer*				parseStatement();
	void					addSwitchCase (DataBuffer* b);
	void					checkToplevel();
	void					checkNotToplevel();
	bool					tokenIs (Token a);
	String					getTokenString();
	String					describePosition() const;
	void					writeToFile (String outfile);
	Variable*				findVariable (const String& name);
	bool					isInGlobalState() const;
	void					suggestHighestVarIndex (bool global, int index);
	int						getHighestVarIndex (bool global);

	inline ScopeInfo& scope (int offset)
	{
		return m_scopeStack[m_scopeCursor - offset];
	}

	inline int numEvents() const
	{
		return m_numEvents;
	}

	inline int numStates() const
	{
		return m_numStates;
	}

private:
	// The main buffer - the contents of this is what we
	// write to file after parsing is complete
	DataBuffer*		m_mainBuffer;

	// onenter buffer - the contents of the onenter {} block
	// is buffered here and is merged further at the end of state
	DataBuffer*		m_onenterBuffer;

	// Mainloop buffer - the contents of the mainloop {} block
	// is buffered here and is merged further at the end of state
	DataBuffer*		m_mainLoopBuffer;

	// Switch buffer - switch case data is recorded to this
	// buffer initially, instead of into main buffer.
	DataBuffer*		m_switchBuffer;

	Lexer*			m_lexer;
	int				m_numStates;
	int				m_numEvents;
	ParserMode		m_currentMode;
	String			m_currentState;
	bool			m_isStateSpawnDefined;
	bool			m_gotMainLoop;
	int				m_scopeCursor;
	bool			m_isElseAllowed;
	int				m_highestGlobalVarIndex;
	int				m_highestStateVarIndex;
	int				m_numWrittenBytes;
	List<ScopeInfo>	m_scopeStack;
	int				m_zandronumVersion;
	bool			m_defaultZandronumVersion;

	DataBuffer*		currentBuffer();
	void			parseStateBlock();
	void			parseEventBlock();
	void			parseMainloop();
	void			parseOnEnterExit();
	void			parseVar();
	void			parseGoto();
	void			parseIf();
	void			parseElse();
	void			parseWhileBlock();
	void			parseForBlock();
	void			parseDoBlock();
	void			parseSwitchBlock();
	void			parseSwitchCase();
	void			parseSwitchDefault();
	void			parseBreak();
	void			parseContinue();
	void			parseBlockEnd();
	void			parseLabel();
	void			parseEventdef();
	void			parseFuncdef();
	void			parseUsing();
	void			writeMemberBuffers();
	void			writeStringTable();
	DataBuffer*		parseExpression (DataType reqtype, bool fromhere = false);
	DataHeader		getAssigmentDataHeader (AssignmentOperator op, Variable* var);
};

#endif // BOTC_PARSER_H
