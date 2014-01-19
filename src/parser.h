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
#include "main.h"
#include "commands.h"
#include "object_writer.h"
#include "lexer_scanner.h"
#include "tokens.h"

#define MAX_FILESTACK 8
#define MAX_SCOPE 32
#define MAX_CASE 64

class lexer;
class script_variable;

// Operators
enum operator_e
{
	OPER_ADD,
	OPER_SUBTRACT,
	OPER_MULTIPLY,
	OPER_DIVIDE,
	OPER_MODULUS,
	OPER_ASSIGN,
	OPER_ASSIGNADD,
	OPER_ASSIGNSUB,
	OPER_ASSIGNMUL,
	OPER_ASSIGNDIV,
	OPER_ASSIGNMOD, // -- 10
	OPER_EQUALS,
	OPER_NOTEQUALS,
	OPER_LESSTHAN,
	OPER_GREATERTHAN,
	OPER_LESSTHANEQUALS,
	OPER_GREATERTHANEQUALS,
	OPER_LEFTSHIFT,
	OPER_RIGHTSHIFT,
	OPER_ASSIGNLEFTSHIFT,
	OPER_ASSIGNRIGHTSHIFT, // -- 20
	OPER_OR,
	OPER_AND,
	OPER_BITWISEOR,
	OPER_BITWISEAND,
	OPER_BITWISEEOR,
	OPER_TERNARY,
	OPER_STRLEN,
};

struct operator_info
{
	operator_e		opercode;
	e_data_header	dataheader;
	e_token			token;
};

// Mark types
enum marktype_e
{
	e_label_mark,
	e_if_mark,
	e_internal_mark, // internal structures
};

// Block types
enum scopetype_e
{
	e_unknown_scope,
	e_if_scope,
	e_while_scope,
	e_for_scope,
	e_do_scope,
	e_switch_scope,
	e_else_scope,
};

// ============================================================================
// Meta-data about blocks
struct ScopeInfo
{
	int mark1;
	int mark2;
	scopetype_e type;
	data_buffer* buffer1;

	// switch-related stuff
	// Which case are we at?
	short casecursor;

	// Marks to case-blocks
	int casemarks[MAX_CASE];

	// Numbers of the case labels
	int casenumbers[MAX_CASE];

	// actual case blocks
	data_buffer* casebuffers[MAX_CASE];

	// What is the current buffer of the block?
	data_buffer* recordbuffer;
};

// ============================================================================
struct constant_info
{
	string name;
	type_e type;
	string val;
};

// ============================================================================
// The script reader reads the script, parses it and tells the object writer
// the bytecode it needs to write to file.
class botscript_parser
{
	public:
		// ====================================================================
		// TODO: make private
		FILE* fp[MAX_FILESTACK];
		string filepath[MAX_FILESTACK];
		int fc;

		int pos[MAX_FILESTACK];
		int curline[MAX_FILESTACK];
		int curchar[MAX_FILESTACK];
		ScopeInfo scopestack[MAX_SCOPE];
		long savedpos[MAX_FILESTACK]; // filepointer cursor position
		int commentmode;
		long prevpos;
		string prevtoken;

		// ====================================================================
		// METHODS
		botscript_parser();
		~botscript_parser();
		void parse_botscript (string file_name, object_writer* w);
		data_buffer* ParseCommand (command_info* comm);
		data_buffer* parse_expression (type_e reqtype);
		data_buffer* parse_assignment (script_variable* var);
		int parse_operator (bool peek = false);
		data_buffer* parse_expr_value (type_e reqtype);
		string parse_float ();
		void push_scope ();
		data_buffer* parse_statement ();
		void add_switch_case (data_buffer* b);
		void check_toplevel();
		void check_not_toplevel();
		bool token_is (e_token a);
		string token_string();
		string describe_position() const;

	private:
		lexer*			m_lx;
		object_writer*	m_writer;

		void parse_state_block();
		void parse_event_block();
		void parse_mainloop();
		void parse_on_enter_exit();
		void parse_variable_declaration();
		void parse_goto();
		void parse_if();
		void parse_else();
		void parse_while_block();
		void parse_for_block();
		void parse_do_block();
		void parse_switch_block();
		void parse_switch_case();
		void parse_switch_default();
		void parse_break();
		void parse_continue();
		void parse_block_end();
		void parse_const();
		void parse_label();
		void parse_eventdef();
		void parse_funcdef();
};

constant_info* find_constant (const string& tok);

#endif // BOTC_PARSER_H
