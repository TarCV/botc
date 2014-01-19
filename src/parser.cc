/*
	Copyright (c) 2012-2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "object_writer.h"
#include "parser.h"
#include "events.h"
#include "commands.h"
#include "stringtable.h"
#include "variables.h"
#include "containers.h"
#include "lexer.h"

#define SCOPE(n) scopestack[g_ScopeCursor - n]

// TODO: make these static
int g_NumStates = 0;
int g_NumEvents = 0;
parsermode_e g_current_mode = MODE_TOPLEVEL;
string g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;
int g_ScopeCursor = 0;
data_buffer* g_IfExpression = null;
bool g_CanElse = false;
static string* g_undefined_labels[MAX_MARKS]; // TODO: make a list
list<constant_info> g_ConstInfo;

// ============================================================================
//
botscript_parser::botscript_parser() :
	m_lx (new lexer) {}

// ============================================================================
//
botscript_parser::~botscript_parser()
{
	delete m_lx;
}

// ============================================================================
//
void botscript_parser::check_toplevel()
{
	if (g_current_mode != MODE_TOPLEVEL)
		error ("%1-statements may only be defined at top level!", token_string());
}

// ============================================================================
//
void botscript_parser::check_not_toplevel()
{
	if (g_current_mode == MODE_TOPLEVEL)
		error ("%1-statements must not be defined at top level!", token_string());
}

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void botscript_parser::parse_botscript (string file_name, object_writer* w)
{
	m_writer = w;

	// Lex and preprocess the file
	m_lx->process_file (file_name);

	// Zero the entire block stack first
	// TODO: this shouldn't be necessary
	for (int i = 0; i < MAX_SCOPE; i++)
		ZERO (scopestack[i]);

	for (int i = 0; i < MAX_MARKS; i++)
		g_undefined_labels[i] = null;

	while (m_lx->get_next())
	{
		// Check if else is potentically valid
		if (token_is (tk_else) && !g_CanElse)
			error ("else without preceding if");

		if (!token_is (tk_else))
			g_CanElse = false;

		switch (m_lx->get_token()->type)
		{
			case tk_state:
				parse_state_block();
				break;

			case tk_event:
				parse_event_block();
				break;

			case tk_mainloop:
				parse_mainloop();
				break;

			case tk_onenter:
			case tk_onexit:
				parse_on_enter_exit();
				break;

			case tk_int:
			case tk_str:
			case tk_void:
				parse_variable_declaration();
				break;

			case tk_goto:
				parse_goto();
				break;

			case tk_if:
				parse_if();
				break;

			case tk_else:
				parse_else();
				break;

			case tk_while:
				parse_while_block();
				break;

			case tk_for:
				parse_for_block();
				break;

			case tk_do:
				parse_do_block();
				break;

			case tk_switch:
				parse_switch_block();
				break;

			case tk_case:
				parse_switch_case();
				break;

			case tk_default:
				parse_switch_default();
				break;

			case tk_break:
				parse_break();
				break;

			case tk_continue:
				parse_continue();
				break;

			case tk_brace_end:
				parse_block_end();
				break;

			case tk_const:
				parse_const();
				break;

			case tk_eventdef:
				parse_eventdef();
				break;

			case tk_funcdef:
				parse_funcdef();
				break;

			default:
			{
				// Check for labels
				lexer::token next;

				if (token_is (tk_symbol) &&
					m_lx->peek_next (&next) &&
					next.type == tk_colon)
				{
					parse_label();
					break;
				}

				// Check if it's a command
				command_info* comm = find_command_by_name (token_string());

				if (comm)
				{
					m_writer->get_current_buffer()->merge (ParseCommand (comm));
					m_lx->must_get_next (tk_semicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				data_buffer* b = parse_statement();

				if (!b)
					error ("unknown token `%1`", token_string());

				m_writer->write_buffer (b);
				m_lx->must_get_next (tk_semicolon);
			}
			break;
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (g_current_mode != MODE_TOPLEVEL)
		error ("script did not end at top level; a `}` is missing somewhere");

	// stateSpawn must be defined!
	if (!g_stateSpawnDefined)
		error ("script must have a state named `stateSpawn`!");

	for (int i = 0; i < MAX_MARKS; i++)
		if (g_undefined_labels[i])
			error ("label `%1` is referenced via `goto` but isn't defined\n", g_undefined_labels[i]);

	// Dump the last state's onenter and mainloop
	m_writer->write_member_buffers();

	// String table
	m_writer->write_string_table();
}

// ============================================================================
//
void botscript_parser::parse_state_block()
{
	check_toplevel();
	m_lx->must_get_next (tk_string);
	string statename = token_string();

	// State name must be a word.
	if (statename.first (" ") != -1)
		error ("state name must be a single word, got `%1`", statename);

	// stateSpawn is special - it *must* be defined. If we
	// encountered it, then mark down that we have it.
	if (-statename == "statespawn")
		g_stateSpawnDefined = true;

	// Must end in a colon
	m_lx->must_get_next (tk_colon);

	// write the previous state's onenter and
	// mainloop buffers to file now
	if (g_CurState.is_empty() == false)
		m_writer->write_member_buffers();

	m_writer->write (dh_state_name);
	m_writer->write_string (statename);
	m_writer->write (dh_state_index);
	m_writer->write (g_NumStates);

	g_NumStates++;
	g_CurState = statename;
	g_GotMainLoop = false;
}

// ============================================================================
//
void botscript_parser::parse_event_block()
{
	check_toplevel();
	m_lx->must_get_next (tk_string);

	event_info* e = find_event_by_name (token_string());

	if (!e)
		error ("bad event, got `%1`\n", token_string());

	m_lx->must_get_next (tk_brace_start);
	g_current_mode = MODE_EVENT;
	m_writer->write (dh_event);
	m_writer->write (e->number);
	g_NumEvents++;
}

// ============================================================================
//
void botscript_parser::parse_mainloop()
{
	check_toplevel();
	m_lx->must_get_next (tk_brace_start);

	// Mode must be set before dataheader is written here!
	g_current_mode = MODE_MAINLOOP;
	m_writer->write (dh_main_loop);
}

// ============================================================================
//
void botscript_parser::parse_on_enter_exit()
{
	check_toplevel();
	bool onenter = (token_is (tk_onenter));
	m_lx->must_get_next (tk_brace_start);

	// Mode must be set before dataheader is written here,
	// because onenter goes to a separate buffer.
	g_current_mode = onenter ? MODE_ONENTER : MODE_ONEXIT;
	m_writer->write (onenter ? dh_on_enter : dh_on_exit);
}

// ============================================================================
//
void botscript_parser::parse_variable_declaration()
{
	// For now, only globals are supported
	if (g_current_mode != MODE_TOPLEVEL || g_CurState.is_empty() == false)
		error ("variables must only be global for now");

	type_e type =	(token_is (tk_int)) ? TYPE_INT :
					(token_is (tk_str)) ? TYPE_STRING :
					TYPE_BOOL;

	m_lx->must_get_next();
	string varname = token_string();

	// Var name must not be a number
	if (varname.is_numeric())
		error ("variable name must not be a number");

	script_variable* var = declare_global_variable (type, varname);
	(void) var;
	m_lx->must_get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_goto()
{
	check_not_toplevel();

	// Get the name of the label
	m_lx->must_get_next();

	// Find the mark this goto statement points to
	string target = token_string();
	int m = m_writer->find_byte_mark (target);

	// If not set, define it
	if (m == MAX_MARKS)
	{
		m = m_writer->add_mark (target);
		g_undefined_labels[m] = new string (target);
	}

	// Add a reference to the mark.
	m_writer->write (dh_goto);
	m_writer->add_reference (m);
	m_lx->must_get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_if()
{
	check_not_toplevel();
	push_scope();

	// Condition
	m_lx->must_get_next (tk_paren_start);

	// Read the expression and write it.
	m_lx->must_get_next();
	data_buffer* c = parse_expression (TYPE_INT);
	m_writer->write_buffer (c);

	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	int marknum = m_writer->add_mark ("");

	// Use dh_if_not_goto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	m_writer->write (dh_if_not_goto);
	m_writer->add_reference (marknum);

	// Store it
	SCOPE (0).mark1 = marknum;
	SCOPE (0).type = e_if_scope;
}

// ============================================================================
//
void botscript_parser::parse_else()
{
	check_not_toplevel();
	m_lx->must_get_next (tk_brace_start);

	// Don't use PushScope as it resets the scope
	g_ScopeCursor++;

	if (g_ScopeCursor >= MAX_SCOPE)
		error ("too deep scope");

	if (SCOPE (0).type != e_if_scope)
		error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = m_writer->add_mark ("");

	// Instruction to jump to the end after if block is complete
	m_writer->write (dh_goto);
	m_writer->add_reference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	m_writer->move_mark (SCOPE (0).mark1);
	SCOPE (0).type = e_else_scope;
}

// ============================================================================
//
void botscript_parser::parse_while_block()
{
	check_not_toplevel();
	push_scope();

	// While loops need two marks - one at the start of the loop and one at the
	// end. The condition is checked at the very start of the loop, if it fails,
	// we use goto to skip to the end of the loop. At the end, we loop back to
	// the beginning with a go-to statement.
	int mark1 = m_writer->add_mark (""); // start
	int mark2 = m_writer->add_mark (""); // end

	// Condition
	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();
	data_buffer* expr = parse_expression (TYPE_INT);
	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);

	// write condition
	m_writer->write_buffer (expr);

	// Instruction to go to the end if it fails
	m_writer->write (dh_if_not_goto);
	m_writer->add_reference (mark2);

	// Store the needed stuff
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).type = e_while_scope;
}

// ============================================================================
//
void botscript_parser::parse_for_block()
{
	check_not_toplevel();
	push_scope();

	// Initializer
	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();
	data_buffer* init = parse_statement();

	if (!init)
		error ("bad statement for initializer of for");

	m_lx->must_get_next (tk_semicolon);

	// Condition
	m_lx->must_get_next();
	data_buffer* cond = parse_expression (TYPE_INT);

	if (!cond)
		error ("bad statement for condition of for");

	m_lx->must_get_next (tk_semicolon);

	// Incrementor
	m_lx->must_get_next();
	data_buffer* incr = parse_statement();

	if (!incr)
		error ("bad statement for incrementor of for");

	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);

	// First, write out the initializer
	m_writer->write_buffer (init);

	// Init two marks
	int mark1 = m_writer->add_mark ("");
	int mark2 = m_writer->add_mark ("");

	// Add the condition
	m_writer->write_buffer (cond);
	m_writer->write (dh_if_not_goto);
	m_writer->add_reference (mark2);

	// Store the marks and incrementor
	SCOPE (0).mark1 = mark1;
	SCOPE (0).mark2 = mark2;
	SCOPE (0).buffer1 = incr;
	SCOPE (0).type = e_for_scope;
}

// ============================================================================
//
void botscript_parser::parse_do_block()
{
	check_not_toplevel();
	push_scope();
	m_lx->must_get_next (tk_brace_start);
	SCOPE (0).mark1 = m_writer->add_mark ("");
	SCOPE (0).type = e_do_scope;
}

// ============================================================================
//
void botscript_parser::parse_switch_block()
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

	check_not_toplevel();
	push_scope();
	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();
	m_writer->write_buffer (parse_expression (TYPE_INT));
	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);
	SCOPE (0).type = e_switch_scope;
	SCOPE (0).mark1 = m_writer->add_mark (""); // end mark
	SCOPE (0).buffer1 = null; // default header
}

// ============================================================================
//
void botscript_parser::parse_switch_case()
{
	// case is only allowed inside switch
	if (SCOPE (0).type != e_switch_scope)
		error ("case label outside switch");

	// Get the literal (Zandronum does not support expressions here)
	m_lx->must_get_next (tk_number);
	int num = m_lx->get_token()->text.to_long();
	m_lx->must_get_next (tk_colon);

	for (int i = 0; i < MAX_CASE; i++)
		if (SCOPE (0).casenumbers[i] == num)
			error ("multiple case %d labels in one switch", num);

	// write down the expression and case-go-to. This builds
	// the case tree. The closing event will write the actual
	// blocks and move the marks appropriately.
	//	 AddSwitchCase will add the reference to the mark
	// for the case block that this heralds, and takes care
	// of buffering setup and stuff like that.
	//	 null the switch buffer for the case-go-to statement,
	// we want it all under the switch, not into the case-buffers.
	m_writer->SwitchBuffer = null;
	m_writer->write (dh_case_goto);
	m_writer->write (num);
	add_switch_case (null);
	SCOPE (0).casenumbers[SCOPE (0).casecursor] = num;
}

// ============================================================================
//
void botscript_parser::parse_switch_default()
{
	if (SCOPE (0).type != e_switch_scope)
		error ("default label outside switch");

	if (SCOPE (0).buffer1)
		error ("multiple default labels in one switch");

	m_lx->must_get_next (tk_colon);

	// The default header is buffered into buffer1, since
	// it has to be the last of the case headers
	//
	// Since the expression is pushed into the switch
	// and is only popped when case succeeds, we have
	// to pop it with dh_drop manually if we end up in
	// a default.
	data_buffer* b = new data_buffer;
	SCOPE (0).buffer1 = b;
	b->write (dh_drop);
	b->write (dh_goto);
	add_switch_case (b);
}

// ============================================================================
//
void botscript_parser::parse_break()
{
	if (!g_ScopeCursor)
		error ("unexpected `break`");

	m_writer->write (dh_goto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case e_if_scope:
		case e_switch_scope:
		{
			m_writer->add_reference (SCOPE (0).mark1);
		} break;

		case e_for_scope:
		case e_while_scope:
		{
			m_writer->add_reference (SCOPE (0).mark2);
		} break;

		default:
		{
			error ("unexpected `break`");
		} break;
	}

	m_lx->must_get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_continue()
{
	m_lx->must_get_next (tk_semicolon);

	int curs;
	bool found = false;

	// Fall through the scope until we find a loop block
	for (curs = g_ScopeCursor; curs > 0 && !found; curs--)
	{
		switch (scopestack[curs].type)
		{
			case e_for_scope:
			case e_while_scope:
			case e_do_scope:
			{
				m_writer->write (dh_goto);
				m_writer->add_reference (scopestack[curs].mark1);
				found = true;
			} break;

			default:
				break;
		}
	}

	// No loop blocks
	if (!found)
		error ("`continue`-statement not inside a loop");
}

// ============================================================================
//
void botscript_parser::parse_block_end()
{
	// Closing brace
	// If we're in the block stack, we're descending down from it now
	if (g_ScopeCursor > 0)
	{
		switch (SCOPE (0).type)
		{
			case e_if_scope:
				// Adjust the closing mark.
				m_writer->move_mark (SCOPE (0).mark1);

				// We're returning from if, thus else can be next
				g_CanElse = true;
				break;

			case e_else_scope:
				// else instead uses mark1 for itself (so if expression
				// fails, jump to else), mark2 means end of else
				m_writer->move_mark (SCOPE (0).mark2);
				break;

			case e_for_scope:
				// write the incrementor at the end of the loop block
				m_writer->write_buffer (SCOPE (0).buffer1);

				// fall-thru
			case e_while_scope:
				// write down the instruction to go back to the start of the loop
				m_writer->write (dh_goto);
				m_writer->add_reference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				m_writer->move_mark (SCOPE (0).mark2);
				break;

			case e_do_scope:
			{
				m_lx->must_get_next (tk_while);
				m_lx->must_get_next (tk_paren_start);
				m_lx->must_get_next();
				data_buffer* expr = parse_expression (TYPE_INT);
				m_lx->must_get_next (tk_paren_end);
				m_lx->must_get_next (tk_semicolon);

				// If the condition runs true, go back to the start.
				m_writer->write_buffer (expr);
				m_writer->write (dh_if_goto);
				m_writer->add_reference (SCOPE (0).mark1);
				break;
			}

			case e_switch_scope:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != -1)
					m_writer->SwitchBuffer = SCOPE (1).casebuffers[SCOPE (1).casecursor];
				else
					m_writer->SwitchBuffer = null;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					m_writer->write_buffer (SCOPE (0).buffer1);
							else
					{
						m_writer->write (dh_drop);
						m_writer->write (dh_goto);
						m_writer->add_reference (SCOPE (0).mark1);
					}

				// Go through all of the buffers we
				// recorded down and write them.
				for (int u = 0; u < MAX_CASE; u++)
				{
					if (!SCOPE (0).casebuffers[u])
						continue;

					m_writer->move_mark (SCOPE (0).casemarks[u]);
					m_writer->write_buffer (SCOPE (0).casebuffers[u]);
				}

				// Move the closing mark here
				m_writer->move_mark (SCOPE (0).mark1);
				break;
			}

			case e_unknown_scope:
				break;
		}

		// Descend down the stack
		g_ScopeCursor--;
		return;
	}

	int dataheader =	(g_current_mode == MODE_EVENT) ? dh_end_event :
						(g_current_mode == MODE_MAINLOOP) ? dh_end_main_loop :
						(g_current_mode == MODE_ONENTER) ? dh_end_on_enter :
						(g_current_mode == MODE_ONEXIT) ? dh_end_on_exit : -1;

	if (dataheader == -1)
		error ("unexpected `}`");

	// Data header must be written before mode is changed because
	// onenter and mainloop go into special buffers, and we want
	// the closing data headers into said buffers too.
	m_writer->write (dataheader);
	g_current_mode = MODE_TOPLEVEL;
	m_lx->get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_const()
{
	constant_info info;

	// Get the type
	m_lx->must_get_next();
	string typestring = token_string();
	info.type = GetTypeByName (typestring);

	if (info.type == TYPE_UNKNOWN || info.type == TYPE_VOID)
		error ("unknown type `%1` for constant", typestring);

	m_lx->must_get_next();
	info.name = token_string();

	m_lx->must_get_next (tk_assign);

	switch (info.type)
	{
		case TYPE_BOOL:
		case TYPE_INT:
		{
			m_lx->must_get_next (tk_number);
		} break;

		case TYPE_STRING:
		{
			m_lx->must_get_next (tk_string);
		} break;

		case TYPE_UNKNOWN:
		case TYPE_VOID:
			break;
	}

	info.val = m_lx->get_token()->text;
	g_ConstInfo << info;

	m_lx->must_get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_label()
{
	check_not_toplevel();
	string label_name = token_string();

	// want no conflicts..
	if (find_command_by_name (label_name))
		error ("label name `%1` conflicts with command name\n", label_name);

	if (find_global_variable (label_name))
		error ("label name `%1` conflicts with variable\n", label_name);

	// See if a mark already exists for this label
	int mark = -1;

	for (int i = 0; i < MAX_MARKS; i++)
	{
		if (g_undefined_labels[i] && *g_undefined_labels[i] == label_name)
		{
			mark = i;
			m_writer->move_mark (i);

			// No longer undefinde
			delete g_undefined_labels[i];
			g_undefined_labels[i] = null;
		}
	}

	// Not found in unmarked lists, define it now
	if (mark == -1)
		m_writer->add_mark (label_name);

	m_lx->must_get_next (tk_colon);
}

// =============================================================================
//
void botscript_parser::parse_eventdef()
{
	event_info* e = new event_info;

	m_lx->must_get_next (tk_number);
	e->number = token_string().to_long();
	m_lx->must_get_next (tk_colon);
	m_lx->must_get_next (tk_symbol);
	e->name = m_lx->get_token()->text;
	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_semicolon);
	add_event (e);
}

// =============================================================================
//
void botscript_parser::parse_funcdef()
{
	command_info* comm = new command_info;

	// Number
	m_lx->must_get_next (tk_number);
	comm->number = m_lx->get_token()->text.to_long();

	m_lx->must_get_next (tk_colon);

	// Name
	m_lx->must_get_next (tk_symbol);
	comm->name = m_lx->get_token()->text;

	if (IsKeyword (comm->name))
		error ("function name `%1` conflicts with keyword", comm->name);

	m_lx->must_get_next (tk_colon);

	// Return value
	m_lx->must_get_any_of ({tk_int, tk_void, tk_bool, tk_str});
	comm->returnvalue = GetTypeByName (m_lx->get_token()->text); // TODO
	assert (comm->returnvalue != -1);

	m_lx->must_get_next (tk_colon);

	// Num args
	m_lx->must_get_next (tk_number);
	comm->numargs = m_lx->get_token()->text.to_long();

	m_lx->must_get_next (tk_colon);

	// Max args
	m_lx->must_get_next (tk_number);
	comm->maxargs = m_lx->get_token()->text.to_long();

	// Argument types
	int curarg = 0;

	while (curarg < comm->maxargs)
	{
		command_argument arg;
		m_lx->must_get_next (tk_colon);
		m_lx->must_get_any_of ({tk_int, tk_bool, tk_str});
		type_e type = GetTypeByName (m_lx->get_token()->text);
		assert (type != -1 && type != TYPE_VOID);
		arg.type = type;

		m_lx->must_get_next (tk_paren_start);
		m_lx->must_get_next (tk_symbol);
		arg.name = m_lx->get_token()->text;

		// If this is an optional parameter, we need the default value.
		if (curarg >= comm->numargs)
		{
			m_lx->must_get_next (tk_assign);

			switch (type)
			{
				case TYPE_INT:
				case TYPE_BOOL:
					m_lx->must_get_next (tk_number);
					break;

				case TYPE_STRING:
					m_lx->must_get_next (tk_string);
					break;

				case TYPE_UNKNOWN:
				case TYPE_VOID:
					break;
			}

			arg.defvalue = m_lx->get_token()->text.to_long();
		}

		m_lx->must_get_next (tk_paren_end);
		comm->args << arg;
		curarg++;
	}

	m_lx->must_get_next (tk_semicolon);
	add_command_definition (comm);
}

// ============================================================================
// Parses a command call
data_buffer* botscript_parser::ParseCommand (command_info* comm)
{
	data_buffer* r = new data_buffer (64);

	if (g_current_mode == MODE_TOPLEVEL)
		error ("command call at top level");

	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();

	int curarg = 0;

	while (1)
	{
		if (token_is (tk_paren_end))
		{
			if (curarg < comm->numargs)
				error ("too few arguments passed to %1\n\tprototype: %2",
					comm->name, get_command_signature (comm));

			break;
			curarg++;
		}

		if (curarg >= comm->maxargs)
			error ("too many arguments passed to %1\n\tprototype: %2",
				comm->name, get_command_signature (comm));

		r->merge (parse_expression (comm->args[curarg].type));
		m_lx->must_get_next();

		if (curarg < comm->numargs - 1)
		{
			m_lx->must_be (tk_comma);
			m_lx->must_get_next();
		}
		else if (curarg < comm->maxargs - 1)
		{
			// Can continue, but can terminate as well.
			if (token_is (tk_paren_end))
			{
				curarg++;
				break;
			}
			else
			{
				m_lx->must_be (tk_comma);
				m_lx->must_get_next();
			}
		}

		curarg++;
	}

	// If the script skipped any optional arguments, fill in defaults.
	while (curarg < comm->maxargs)
	{
		r->write (dh_push_number);
		r->write (comm->args[curarg].defvalue);
		curarg++;
	}

	r->write (dh_command);
	r->write (comm->number);
	r->write (comm->maxargs);

	return r;
}

// ============================================================================
// Is the given operator an assignment operator?
//
static bool is_assignment_operator (int oper)
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
static word get_data_header_by_operator (script_variable* var, int oper)
{
	if (is_assignment_operator (oper))
	{
		if (!var)
			error ("operator %d requires left operand to be a variable\n", oper);

		// TODO: At the moment, vars only are global
		// OPER_ASSIGNLEFTSHIFT and OPER_ASSIGNRIGHTSHIFT do not
		// have data headers, instead they are expanded out in
		// the operator parser
		switch (oper)
		{
			case OPER_ASSIGNADD: return dh_add_global_var;
			case OPER_ASSIGNSUB: return dh_subtract_global_var;
			case OPER_ASSIGNMUL: return dh_multiply_global_var;
			case OPER_ASSIGNDIV: return dh_divide_global_var;
			case OPER_ASSIGNMOD: return dh_mod_global_var;
			case OPER_ASSIGN: return dh_assign_global_var;

			default: error ("bad assignment operator!!\n");
		}
	}

	switch (oper)
	{
		case OPER_ADD: return dh_add;
		case OPER_SUBTRACT: return dh_subtract;
		case OPER_MULTIPLY: return dh_multiply;
		case OPER_DIVIDE: return dh_divide;
		case OPER_MODULUS: return dh_modulus;
		case OPER_EQUALS: return dh_equals;
		case OPER_NOTEQUALS: return dh_not_equals;
		case OPER_LESSTHAN: return dh_less_than;
		case OPER_GREATERTHAN: return dh_greater_than;
		case OPER_LESSTHANEQUALS: return dh_at_most;
		case OPER_GREATERTHANEQUALS: return dh_at_least;
		case OPER_LEFTSHIFT: return dh_left_shift;
		case OPER_RIGHTSHIFT: return dh_right_shift;
		case OPER_OR: return dh_or_logical;
		case OPER_AND: return dh_and_logical;
		case OPER_BITWISEOR: return dh_or_bitwise;
		case OPER_BITWISEEOR: return dh_eor_bitwise;
		case OPER_BITWISEAND: return dh_and_bitwise;
	}

	error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
//
data_buffer* botscript_parser::parse_expression (type_e reqtype)
{
	data_buffer* retbuf = new data_buffer (64);

	// Parse first operand
	retbuf->merge (parse_expr_value (reqtype));

	// Parse any and all operators we get
	int oper;

	while ( (oper = parse_operator (true)) != -1)
	{
		// We peeked the operator, move forward now
		m_lx->skip();

		// Can't be an assignement operator, those belong in assignments.
		if (is_assignment_operator (oper))
			error ("assignment operator inside expression");

		// Parse the right operand.
		m_lx->must_get_next();
		data_buffer* rb = parse_expr_value (reqtype);

		if (oper == OPER_TERNARY)
		{
			// Ternary operator requires - naturally - a third operand.
			m_lx->must_get_next (tk_colon);
			m_lx->must_get_next();
			data_buffer* tb = parse_expr_value (reqtype);

			// It also is handled differently: there isn't a dataheader for ternary
			// operator. Instead, we abuse PUSHNUMBER and IFNOTGOTO for this.
			// Behold, big block of writing madness! :P
			int mark1 = retbuf->add_mark (""); // start of "else" case
			int mark2 = retbuf->add_mark (""); // end of expression
			retbuf->write (dh_if_not_goto); // if the first operand (condition)
			retbuf->add_reference (mark1); // didn't eval true, jump into mark1
			retbuf->merge (rb); // otherwise, perform second operand (true case)
			retbuf->write (dh_goto); // afterwards, jump to the end, which is
			retbuf->add_reference (mark2); // marked by mark2.
			retbuf->move_mark (mark1); // move mark1 at the end of the true case
			retbuf->merge (tb); // perform third operand (false case)
			retbuf->move_mark (mark2); // move the ending mark2 here
		}
		else
		{
			// write to buffer
			retbuf->merge (rb);
			retbuf->write (get_data_header_by_operator (null, oper));
		}
	}

	return retbuf;
}

// ============================================================================
// Parses an operator string. Returns the operator number code.
//
#define ISNEXT(C) (m_lx->peek_next_string (peek ? 1 : 0) == C)

int botscript_parser::parse_operator (bool peek)
{
	string oper;

	if (peek)
		oper += m_lx->peek_next_string();
	else
		oper += token_string();

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
	oper += m_lx->peek_next_string (peek ? 1 : 0);
	equalsnext = m_lx->peek_next_string (peek ? 2 : 1) == ("=");

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
		m_lx->must_get_next();
		return o;
	}

	// Three-char opers
	oper += m_lx->peek_next_string (peek ? 2 : 1);
	o =	oper == "<<=" ? OPER_ASSIGNLEFTSHIFT :
		oper == ">>=" ? OPER_ASSIGNRIGHTSHIFT :
		-1;

	if (o != -1)
	{
		m_lx->must_get_next();
		m_lx->must_get_next();
	}

	return o;
}

// ============================================================================
//
string botscript_parser::parse_float()
{
	m_lx->must_be (tk_number);
	string floatstring = token_string();
	lexer::token tok;

	// Go after the decimal point
	if (m_lx->peek_next (&tok) && tok.type == tk_dot)
	{
		m_lx->skip();
		m_lx->must_get_next (tk_number);
		floatstring += ".";
		floatstring += token_string();
	}

	return floatstring;
}

// ============================================================================
// Parses a value in the expression and returns the data needed to push
// it, contained in a data buffer. A value can be either a variable, a command,
// a literal or an expression.
//
data_buffer* botscript_parser::parse_expr_value (type_e reqtype)
{
	data_buffer* b = new data_buffer (16);
	script_variable* g;

	// Prefixing "!" means negation.
	bool negate = token_is (tk_exclamation_mark);

	if (negate) // Jump past the "!"
		m_lx->skip();

	// Handle strlen
	/* if (token_string() == "strlen")
	{
		m_lx->must_get_next (tk_paren_start);
		m_lx->must_get_next();

		// By this token we should get a string constant.
		constant_info* constant = find_constant (token_string());

		if (!constant || constant->type != TYPE_STRING)
			error ("strlen only works with const str");

		if (reqtype != TYPE_INT)
			error ("strlen returns int but %1 is expected\n", GetTypeName (reqtype));

		b->write (dh_push_number);
		b->write (constant->val.len());

		m_lx->must_get_next (tk_paren_end);
	}
	else */
	if (token_is (tk_paren_start))
	{
		// Expression
		m_lx->must_get_next();
		data_buffer* c = parse_expression (reqtype);
		b->merge (c);
		m_lx->must_get_next (tk_paren_end);
	}
	else if (command_info* comm = find_command_by_name (token_string()))
	{
		delete b;

		// Command
		if (reqtype && comm->returnvalue != reqtype)
			error ("%1 returns an incompatible data type", comm->name);

		b = ParseCommand (comm);
	}
	else if (constant_info* constant = find_constant (token_string()))
	{
		// Type check
		if (reqtype != constant->type)
			error ("constant `%1` is %2, expression requires %3\n",
				constant->name, GetTypeName (constant->type),
				GetTypeName (reqtype));

		switch (constant->type)
		{
			case TYPE_BOOL:
			case TYPE_INT:
				b->write (dh_push_number);
				b->write (atoi (constant->val));
				break;

			case TYPE_STRING:
				b->write_string (constant->val);
				break;

			case TYPE_VOID:
			case TYPE_UNKNOWN:
				break;
		}
	}
	else if ((g = find_global_variable (token_string())))
	{
		// Global variable
		b->write (dh_push_global_var);
		b->write (g->index);
	}
	else
	{
		// If nothing else, check for literal
		switch (reqtype)
		{
		case TYPE_VOID:
		case TYPE_UNKNOWN:
			error ("unknown identifier `%1` (expected keyword, function or variable)", token_string());
			break;

		case TYPE_BOOL:
		case TYPE_INT:
		{
			m_lx->must_be (tk_number);

			// All values are written unsigned - thus we need to write the value's
			// absolute value, followed by an unary minus for negatives.
			b->write (dh_push_number);

			long v = token_string().to_long();
			b->write (static_cast<word> (abs (v)));

			if (v < 0)
				b->write (dh_unary_minus);

			break;
		}

		case TYPE_STRING:
			// PushToStringTable either returns the string index of the
			// string if it finds it in the table, or writes it to the
			// table and returns it index if it doesn't find it there.
			m_lx->must_be (tk_string);
			b->write_string (token_string());
			break;
		}
	}

	// Negate it now if desired
	if (negate)
		b->write (dh_negate_logical);

	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
//
data_buffer* botscript_parser::parse_assignment (script_variable* var)
{
	bool global = !var->statename.len();

	// Get an operator
	m_lx->must_get_next();
	int oper = parse_operator();

	if (!is_assignment_operator (oper))
		error ("expected assignment operator");

	if (g_current_mode == MODE_TOPLEVEL)
		error ("can't alter variables at top level");

	// Parse the right operand
	m_lx->must_get_next();
	data_buffer* retbuf = new data_buffer;
	data_buffer* expr = parse_expression (var->type);

	// <<= and >>= do not have data headers. Solution: expand them.
	// a <<= b -> a = a << b
	// a >>= b -> a = a >> b
	if (oper == OPER_ASSIGNLEFTSHIFT || oper == OPER_ASSIGNRIGHTSHIFT)
	{
		retbuf->write (global ? dh_push_global_var : dh_push_local_var);
		retbuf->write (var->index);
		retbuf->merge (expr);
		retbuf->write ((oper == OPER_ASSIGNLEFTSHIFT) ? dh_left_shift : dh_right_shift);
		retbuf->write (global ? dh_assign_global_var : dh_assign_local_var);
		retbuf->write (var->index);
	}
	else
	{
		retbuf->merge (expr);
		long dh = get_data_header_by_operator (var, oper);
		retbuf->write (dh);
		retbuf->write (var->index);
	}

	return retbuf;
}

// ============================================================================
//
void botscript_parser::push_scope()
{
	g_ScopeCursor++;

	if (g_ScopeCursor >= MAX_SCOPE)
		error ("too deep scope");

	ScopeInfo* info = &SCOPE (0);
	info->type = e_unknown_scope;
	info->mark1 = 0;
	info->mark2 = 0;
	info->buffer1 = null;
	info->casecursor = -1;

	for (int i = 0; i < MAX_CASE; i++)
	{
		info->casemarks[i] = MAX_MARKS;
		info->casebuffers[i] = null;
		info->casenumbers[i] = -1;
	}
}

// ============================================================================
//
data_buffer* botscript_parser::parse_statement()
{
	if (find_constant (token_string())) // There should not be constants here.
		error ("invalid use for constant\n");

	// If it's a variable, expect assignment.
	if (script_variable* var = find_global_variable (token_string()))
		return parse_assignment (var);

	return null;
}

// ============================================================================
//
void botscript_parser::add_switch_case (data_buffer* b)
{
	ScopeInfo* info = &SCOPE (0);

	info->casecursor++;

	if (info->casecursor >= MAX_CASE)
		error ("too many cases in one switch");

	// Init a mark for the case buffer
	int m = m_writer->add_mark ("");
	info->casemarks[info->casecursor] = m;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (b)
		b->add_reference (m);
	else
		m_writer->add_reference (m);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	info->casebuffers[info->casecursor] = m_writer->SwitchBuffer = new data_buffer;
}

// ============================================================================
//
constant_info* find_constant (const string& tok)
{
	for (int i = 0; i < g_ConstInfo.size(); i++)
		if (g_ConstInfo[i].name == tok)
			return &g_ConstInfo[i];

	return null;
}

// ============================================================================
//
bool botscript_parser::token_is (e_token a)
{
	return (m_lx->get_token_type() == a);
}

// ============================================================================
//
string botscript_parser::token_string()
{
	return m_lx->get_token()->text;
}

// ============================================================================
//
string botscript_parser::describe_position() const
{
	lexer::token* tok = m_lx->get_token();
	return tok->file + ":" + string (tok->line) + ":" + string (tok->column);
}
