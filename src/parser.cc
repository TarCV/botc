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
#include "stringtable.h"
#include "variables.h"
#include "containers.h"
#include "lexer.h"
#include "data_buffer.h"

#define SCOPE(n) (m_scope_stack[m_scope_cursor - n])

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
	if (m_current_mode != MODE_TOPLEVEL)
		error ("%1-statements may only be defined at top level!", token_string());
}

// ============================================================================
//
void botscript_parser::check_not_toplevel()
{
	if (m_current_mode == MODE_TOPLEVEL)
		error ("%1-statements must not be defined at top level!", token_string());
}

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void botscript_parser::parse_botscript (string file_name)
{
	// Lex and preprocess the file
	m_lx->process_file (file_name);

	m_current_mode = MODE_TOPLEVEL;
	m_num_states = 0;
	m_num_events = 0;
	m_scope_cursor = 0;
	m_state_spawn_defined = false;
	m_got_main_loop = false;
	m_if_expression = null;
	m_can_else = false;

	// Zero the entire block stack first
	// TODO: this shouldn't be necessary
	for (int i = 0; i < MAX_SCOPE; i++)
		ZERO (m_scope_stack[i]);

	while (m_lx->get_next())
	{
		// Check if else is potentically valid
		if (token_is (tk_else) && !m_can_else)
			error ("else without preceding if");

		if (!token_is (tk_else))
			m_can_else = false;

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
					buffer()->merge_and_destroy (parse_command (comm));
					m_lx->must_get_next (tk_semicolon);
					continue;
				}

				// If nothing else, parse it as a statement
				data_buffer* b = parse_statement();

				if (!b)
					error ("unknown token `%1`", token_string());

				buffer()->merge_and_destroy (b);
				m_lx->must_get_next (tk_semicolon);
			}
			break;
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (m_current_mode != MODE_TOPLEVEL)
		error ("script did not end at top level; a `}` is missing somewhere");

	// stateSpawn must be defined!
	if (!m_state_spawn_defined)
		error ("script must have a state named `stateSpawn`!");

	// Ensure no goto target is left undefined
	if (m_undefined_labels.is_empty() == false)
	{
		string_list names;

		for (undefined_label& undf : m_undefined_labels)
			names << undf.name;

		error ("labels `%1` are referenced via `goto` but are not defined\n", names);
	}

	// Dump the last state's onenter and mainloop
	write_member_buffers();

	// String table
	write_string_table();
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
		m_state_spawn_defined = true;

	// Must end in a colon
	m_lx->must_get_next (tk_colon);

	// write the previous state's onenter and
	// mainloop buffers to file now
	if (m_current_state.is_empty() == false)
		write_member_buffers();

	buffer()->write_dword (dh_state_name);
	buffer()->write_string (statename);
	buffer()->write_dword (dh_state_index);
	buffer()->write_dword (m_num_states);

	m_num_states++;
	m_current_state = statename;
	m_got_main_loop = false;
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
	m_current_mode = MODE_EVENT;
	buffer()->write_dword (dh_event);
	buffer()->write_dword (e->number);
	m_num_events++;
}

// ============================================================================
//
void botscript_parser::parse_mainloop()
{
	check_toplevel();
	m_lx->must_get_next (tk_brace_start);

	m_current_mode = MODE_MAINLOOP;
	m_main_loop_buffer->write_dword (dh_main_loop);
}

// ============================================================================
//
void botscript_parser::parse_on_enter_exit()
{
	check_toplevel();
	bool onenter = (token_is (tk_onenter));
	m_lx->must_get_next (tk_brace_start);

	m_current_mode = onenter ? MODE_ONENTER : MODE_ONEXIT;
	buffer()->write_dword (onenter ? dh_on_enter : dh_on_exit);
}

// ============================================================================
//
void botscript_parser::parse_variable_declaration()
{
	// For now, only globals are supported
	if (m_current_mode != MODE_TOPLEVEL || m_current_state.is_empty() == false)
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
	byte_mark* mark = buffer()->find_mark_by_name (target);

	// If not set, define it
	if (!mark)
	{
		undefined_label undf;
		undf.name = target;
		undf.target = buffer()->add_mark (target);
		m_undefined_labels << undf;
	}

	// Add a reference to the mark.
	buffer()->write_dword (dh_goto);
	buffer()->add_reference (mark);
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
	buffer()->merge_and_destroy (c);

	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);

	// Add a mark - to here temporarily - and add a reference to it.
	// Upon a closing brace, the mark will be adjusted.
	byte_mark* mark = buffer()->add_mark ("");

	// Use dh_if_not_goto - if the expression is not true, we goto the mark
	// we just defined - and this mark will be at the end of the scope block.
	buffer()->write_dword (dh_if_not_goto);
	buffer()->add_reference (mark);

	// Store it
	SCOPE (0).mark1 = mark;
	SCOPE (0).type = e_if_scope;
}

// ============================================================================
//
void botscript_parser::parse_else()
{
	check_not_toplevel();
	m_lx->must_get_next (tk_brace_start);

	// Don't use PushScope as it resets the scope
	m_scope_cursor++;

	if (m_scope_cursor >= MAX_SCOPE)
		error ("too deep scope");

	if (SCOPE (0).type != e_if_scope)
		error ("else without preceding if");

	// write down to jump to the end of the else statement
	// Otherwise we have fall-throughs
	SCOPE (0).mark2 = buffer()->add_mark ("");

	// Instruction to jump to the end after if block is complete
	buffer()->write_dword (dh_goto);
	buffer()->add_reference (SCOPE (0).mark2);

	// Move the ifnot mark here and set type to else
	buffer()->adjust_mark (SCOPE (0).mark1);
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
	byte_mark* mark1 = buffer()->add_mark (""); // start
	byte_mark* mark2 = buffer()->add_mark (""); // end

	// Condition
	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();
	data_buffer* expr = parse_expression (TYPE_INT);
	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);

	// write condition
	buffer()->merge_and_destroy (expr);

	// Instruction to go to the end if it fails
	buffer()->write_dword (dh_if_not_goto);
	buffer()->add_reference (mark2);

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
	buffer()->merge_and_destroy (init);

	// Init two marks
	byte_mark* mark1 = buffer()->add_mark ("");
	byte_mark* mark2 = buffer()->add_mark ("");

	// Add the condition
	buffer()->merge_and_destroy (cond);
	buffer()->write_dword (dh_if_not_goto);
	buffer()->add_reference (mark2);

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
	SCOPE (0).mark1 = buffer()->add_mark ("");
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
	buffer()->merge_and_destroy (parse_expression (TYPE_INT));
	m_lx->must_get_next (tk_paren_end);
	m_lx->must_get_next (tk_brace_start);
	SCOPE (0).type = e_switch_scope;
	SCOPE (0).mark1 = buffer()->add_mark (""); // end mark
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
	m_switch_buffer = null;
	buffer()->write_dword (dh_case_goto);
	buffer()->write_dword (num);
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
	b->write_dword (dh_drop);
	b->write_dword (dh_goto);
	add_switch_case (b);
}

// ============================================================================
//
void botscript_parser::parse_break()
{
	if (!m_scope_cursor)
		error ("unexpected `break`");

	buffer()->write_dword (dh_goto);

	// switch and if use mark1 for the closing point,
	// for and while use mark2.
	switch (SCOPE (0).type)
	{
		case e_if_scope:
		case e_switch_scope:
		{
			buffer()->add_reference (SCOPE (0).mark1);
		} break;

		case e_for_scope:
		case e_while_scope:
		{
			buffer()->add_reference (SCOPE (0).mark2);
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
	for (curs = m_scope_cursor; curs > 0 && !found; curs--)
	{
		switch (m_scope_stack[curs].type)
		{
			case e_for_scope:
			case e_while_scope:
			case e_do_scope:
			{
				buffer()->write_dword (dh_goto);
				buffer()->add_reference (m_scope_stack[curs].mark1);
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
	if (m_scope_cursor > 0)
	{
		switch (SCOPE (0).type)
		{
			case e_if_scope:
				// Adjust the closing mark.
				buffer()->adjust_mark (SCOPE (0).mark1);

				// We're returning from if, thus else can be next
				m_can_else = true;
				break;

			case e_else_scope:
				// else instead uses mark1 for itself (so if expression
				// fails, jump to else), mark2 means end of else
				buffer()->adjust_mark (SCOPE (0).mark2);
				break;

			case e_for_scope:
				// write the incrementor at the end of the loop block
				buffer()->merge_and_destroy (SCOPE (0).buffer1);

				// fall-thru
			case e_while_scope:
				// write down the instruction to go back to the start of the loop
				buffer()->write_dword (dh_goto);
				buffer()->add_reference (SCOPE (0).mark1);

				// Move the closing mark here since we're at the end of the while loop
				buffer()->adjust_mark (SCOPE (0).mark2);
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
				buffer()->merge_and_destroy (expr);
				buffer()->write_dword (dh_if_goto);
				buffer()->add_reference (SCOPE (0).mark1);
				break;
			}

			case e_switch_scope:
			{
				// Switch closes. Move down to the record buffer of
				// the lower block.
				if (SCOPE (1).casecursor != -1)
					m_switch_buffer = SCOPE (1).casebuffers[SCOPE (1).casecursor];
				else
					m_switch_buffer = null;

				// If there was a default in the switch, write its header down now.
				// If not, write instruction to jump to the end of switch after
				// the headers (thus won't fall-through if no case matched)
				if (SCOPE (0).buffer1)
					buffer()->merge_and_destroy (SCOPE (0).buffer1);
				else
				{
					buffer()->write_dword (dh_drop);
					buffer()->write_dword (dh_goto);
					buffer()->add_reference (SCOPE (0).mark1);
				}

				// Go through all of the buffers we
				// recorded down and write them.
				for (int u = 0; u < MAX_CASE; u++)
				{
					if (!SCOPE (0).casebuffers[u])
						continue;

					buffer()->adjust_mark (SCOPE (0).casemarks[u]);
					buffer()->merge_and_destroy (SCOPE (0).casebuffers[u]);
				}

				// Move the closing mark here
				buffer()->adjust_mark (SCOPE (0).mark1);
				break;
			}

			case e_unknown_scope:
				break;
		}

		// Descend down the stack
		m_scope_cursor--;
		return;
	}

	int dataheader =	(m_current_mode == MODE_EVENT) ? dh_end_event :
						(m_current_mode == MODE_MAINLOOP) ? dh_end_main_loop :
						(m_current_mode == MODE_ONENTER) ? dh_end_on_enter :
						(m_current_mode == MODE_ONEXIT) ? dh_end_on_exit : -1;

	if (dataheader == -1)
		error ("unexpected `}`");

	// Data header must be written before mode is changed because
	// onenter and mainloop go into special buffers, and we want
	// the closing data headers into said buffers too.
	buffer()->write_dword (dataheader);
	m_current_mode = MODE_TOPLEVEL;
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
	m_constants << info;

	m_lx->must_get_next (tk_semicolon);
}

// ============================================================================
//
void botscript_parser::parse_label()
{
	check_not_toplevel();
	string label_name = token_string();
	byte_mark* mark = null;

	// want no conflicts..
	if (find_command_by_name (label_name))
		error ("label name `%1` conflicts with command name\n", label_name);

	if (find_global_variable (label_name))
		error ("label name `%1` conflicts with variable\n", label_name);

	// See if a mark already exists for this label
	for (undefined_label& undf : m_undefined_labels)
	{
		if (undf.name != label_name)
			continue;

		mark = undf.target;
		buffer()->adjust_mark (mark);

		// No longer undefined
		m_undefined_labels.remove (undf);
		break;
	}

	// Not found in unmarked lists, define it now
	if (mark == null)
		buffer()->add_mark (label_name);

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
data_buffer* botscript_parser::parse_command (command_info* comm)
{
	data_buffer* r = new data_buffer (64);

	if (m_current_mode == MODE_TOPLEVEL)
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

		r->merge_and_destroy (parse_expression (comm->args[curarg].type));
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
		r->write_dword (dh_push_number);
		r->write_dword (comm->args[curarg].defvalue);
		curarg++;
	}

	r->write_dword (dh_command);
	r->write_dword (comm->number);
	r->write_dword (comm->maxargs);

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
	retbuf->merge_and_destroy (parse_expr_value (reqtype));

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
			byte_mark* mark1 = retbuf->add_mark (""); // start of "else" case
			byte_mark* mark2 = retbuf->add_mark (""); // end of expression
			retbuf->write_dword (dh_if_not_goto); // if the first operand (condition)
			retbuf->add_reference (mark1); // didn't eval true, jump into mark1
			retbuf->merge_and_destroy (rb); // otherwise, perform second operand (true case)
			retbuf->write_dword (dh_goto); // afterwards, jump to the end, which is
			retbuf->add_reference (mark2); // marked by mark2.
			retbuf->adjust_mark (mark1); // move mark1 at the end of the true case
			retbuf->merge_and_destroy (tb); // perform third operand (false case)
			retbuf->adjust_mark (mark2); // move the ending mark2 here
		}
		else
		{
			// write to buffer
			retbuf->merge_and_destroy (rb);
			retbuf->write_dword (get_data_header_by_operator (null, oper));
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

		b->write_dword (dh_push_number);
		b->write_dword (constant->val.len());

		m_lx->must_get_next (tk_paren_end);
	}
	else */
	if (token_is (tk_paren_start))
	{
		// Expression
		m_lx->must_get_next();
		data_buffer* c = parse_expression (reqtype);
		b->merge_and_destroy (c);
		m_lx->must_get_next (tk_paren_end);
	}
	else if (command_info* comm = find_command_by_name (token_string()))
	{
		delete b;

		// Command
		if (reqtype && comm->returnvalue != reqtype)
			error ("%1 returns an incompatible data type", comm->name);

		b = parse_command (comm);
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
				b->write_dword (dh_push_number);
				b->write_dword (atoi (constant->val));
				break;

			case TYPE_STRING:
				b->write_string_index (constant->val);
				break;

			case TYPE_VOID:
			case TYPE_UNKNOWN:
				break;
		}
	}
	else if ((g = find_global_variable (token_string())))
	{
		// Global variable
		b->write_dword (dh_push_global_var);
		b->write_dword (g->index);
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
			b->write_dword (dh_push_number);

			long v = token_string().to_long();
			b->write_dword (static_cast<word> (abs (v)));

			if (v < 0)
				b->write_dword (dh_unary_minus);

			break;
		}

		case TYPE_STRING:
			// PushToStringTable either returns the string index of the
			// string if it finds it in the table, or writes it to the
			// table and returns it index if it doesn't find it there.
			m_lx->must_be (tk_string);
			b->write_string_index (token_string());
			break;
		}
	}

	// Negate it now if desired
	if (negate)
		b->write_dword (dh_negate_logical);

	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// token to be the name of the variable, and expects the variable to be given.
//
data_buffer* botscript_parser::parse_assignment (script_variable* var)
{
	// Get an operator
	m_lx->must_get_next();
	int oper = parse_operator();

	if (!is_assignment_operator (oper))
		error ("expected assignment operator");

	if (m_current_mode == MODE_TOPLEVEL)
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
		retbuf->write_dword (var->is_global() ? dh_push_global_var : dh_push_local_var);
		retbuf->write_dword (var->index);
		retbuf->merge_and_destroy (expr);
		retbuf->write_dword ((oper == OPER_ASSIGNLEFTSHIFT) ? dh_left_shift : dh_right_shift);
		retbuf->write_dword (var->is_global() ? dh_assign_global_var : dh_assign_local_var);
		retbuf->write_dword (var->index);
	}
	else
	{
		retbuf->merge_and_destroy (expr);
		long dh = get_data_header_by_operator (var, oper);
		retbuf->write_dword (dh);
		retbuf->write_dword (var->index);
	}

	return retbuf;
}

// ============================================================================
//
void botscript_parser::push_scope()
{
	m_scope_cursor++;

	if (m_scope_cursor >= MAX_SCOPE)
		error ("too deep scope");

	ScopeInfo* info = &SCOPE (0);
	info->type = e_unknown_scope;
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
	byte_mark* casemark = buffer()->add_mark ("");
	info->casemarks[info->casecursor] = casemark;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (b)
		b->add_reference (casemark);
	else
		buffer()->add_reference (casemark);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	info->casebuffers[info->casecursor] = m_switch_buffer = new data_buffer;
}

// ============================================================================
//
constant_info* botscript_parser::find_constant (const string& tok)
{
	for (int i = 0; i < m_constants.size(); i++)
		if (m_constants[i].name == tok)
			return &m_constants[i];

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

// ============================================================================
//
data_buffer* botscript_parser::buffer()
{
	if (m_switch_buffer != null)
		return m_switch_buffer;

	if (m_current_mode == MODE_MAINLOOP)
		return m_main_loop_buffer;

	if (m_current_mode == MODE_ONENTER)
		return m_on_enter_buffer;

	return m_main_buffer;
}

// ============================================================================
//
void botscript_parser::write_member_buffers()
{
	// If there was no mainloop defined, write a dummy one now.
	if (!m_got_main_loop)
	{
		m_main_loop_buffer->write_dword (dh_main_loop);
		m_main_loop_buffer->write_dword (dh_end_main_loop);
	}

	// Write the onenter and mainloop buffers, in that order in particular.
	for (data_buffer** bufp : list<data_buffer**> ({&m_on_enter_buffer, &m_main_loop_buffer}))
	{
		buffer()->merge_and_destroy (*bufp);

		// Clear the buffer afterwards for potential next state
		*bufp = new data_buffer;
	}

	// Next state definitely has no mainloop yet
	m_got_main_loop = false;
}

// ============================================================================
//
// Write string table
//
void botscript_parser::write_string_table()
{
	int stringcount = num_strings_in_table();

	if (!stringcount)
		return;

	// Write header
	m_main_buffer->write_dword (dh_string_list);
	m_main_buffer->write_dword (stringcount);

	// Write all strings
	for (int i = 0; i < stringcount; i++)
		m_main_buffer->write_string (get_string_table()[i]);
}

// ============================================================================
//
// Write the compiled bytecode to a file
//
void botscript_parser::write_to_file (string outfile)
{
	FILE* fp = fopen (outfile, "w");
	CHECK_FILE (fp, outfile, "writing");

	// First, resolve references
	for (int u = 0; u < MAX_MARKS; u++)
	{
		mark_reference* ref = m_main_buffer->get_refs()[u];

		if (!ref)
			continue;

		// Substitute the placeholder with the mark position
		generic_union<word> uni;
		uni.as_word = static_cast<word> (ref->target->pos);

		for (int v = 0; v < (int) sizeof (word); v++)
			memset (m_main_buffer->get_buffer() + ref->pos + v, uni.as_bytes[v], 1);

		/*
		printf ("reference %u at %d resolved to %u at %d\n",
			u, ref->pos, ref->num, MainBuffer->marks[ref->num]->pos);
		*/
	}

	// Then, dump the main buffer to the file
	fwrite (m_main_buffer->get_buffer(), 1, m_main_buffer->get_write_size(), fp);
	print ("-- %1 byte%s1 written to %2\n", m_main_buffer->get_write_size(), outfile);
	fclose (fp);
}