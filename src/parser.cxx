/*
	Copyright (c) 2013-2014, Santeri Piippo
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

#define TOKEN (string (m_lx->get_token()->string))
#define SCOPE(n) scopestack[g_ScopeCursor - n]

// TODO: make these static
int g_NumStates = 0;
int g_NumEvents = 0;
parsermode_e g_CurMode = MODE_TOPLEVEL;
string g_CurState = "";
bool g_stateSpawnDefined = false;
bool g_GotMainLoop = false;
int g_ScopeCursor = 0;
data_buffer* g_IfExpression = null;
bool g_CanElse = false;
string* g_UndefinedLabels[MAX_MARKS];
list<constant_info> g_ConstInfo;

static botscript_parser* g_current_parser = null;

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
	if (g_CurMode != MODE_TOPLEVEL)
		error ("%1-statements may only be defined at top level!", TOKEN.chars());
}

// ============================================================================
//
void botscript_parser::check_not_toplevel()
{
	if (g_CurMode == MODE_TOPLEVEL)
		error ("%1-statements must not be defined at top level!", TOKEN.chars());
}

// ============================================================================
// Main parser code. Begins read of the script file, checks the syntax of it
// and writes the data to the object file via Objwriter - which also takes care
// of necessary buffering so stuff is written in the correct order.
void botscript_parser::parse_botscript (string file_name, object_writer* w)
{
	// Lex and preprocess the file
	m_lx->process_file (file_name);

	// Zero the entire block stack first
	for (int i = 0; i < MAX_SCOPE; i++)
		ZERO (scopestack[i]);

	for (int i = 0; i < MAX_MARKS; i++)
		g_UndefinedLabels[i] = null;

	while (m_lx->get_next())
	{
		// Check if else is potentically valid
		if (TOKEN == "else" && !g_CanElse)
			error ("else without preceding if");

		if (TOKEN != "else")
			g_CanElse = false;

		switch (m_lx->get_token())
		{
			case tk_state:
			{
				check_toplevel();
				m_lx->must_get_next (tk_string);

				// State name must be a word.
				if (TOKEN.first (" ") != -1)
					error ("state name must be a single word, got `%1`", TOKEN);

				string statename = TOKEN;

				// stateSpawn is special - it *must* be defined. If we
				// encountered it, then mark down that we have it.
				if (-TOKEN == "statespawn")
					g_stateSpawnDefined = true;

				// Must end in a colon
				m_lx->must_get_next (tk_colon);

				// write the previous state's onenter and
				// mainloop buffers to file now
				if (g_CurState.len())
					w->write (Buffers();

				w->write (DH_STATENAME);
				w->write_string (statename);
				w->write (DH_STATEIDX);
				w->write (g_NumStates);

				g_NumStates++;
				g_CurState = TOKEN;
				g_GotMainLoop = false;
			}
			break;

			// ============================================================
			//
			case tk_event:
			{
				check_toplevel();

				// Event definition
				m_lx->must_get_next (tk_string);

				event_info* e = find_event_by_name (token_string());

				if (!e)
					error ("bad event, got `%1`\n", token_string());

				m_lx->must_get_next (tk_brace_start);
				g_CurMode = MODE_EVENT;
				w->write (DH_EVENT);
				w->write (e->number);
				g_NumEvents++;
				continue;
			} break;

			// ============================================================
			//
			case tk_mainloop:
			{
				check_toplevel();
				m_lx->must_get_next (tk_brace_start);

				// Mode must be set before dataheader is written here!
				g_CurMode = MODE_MAINLOOP;
				w->write (DH_MAINLOOP);
			}
			break;

			// ============================================================
			//
			case tk_onenter:
			case tk_onexit:
			{
				check_toplevel();
				bool onenter = (m_lx->get_token() == "onenter");
				m_lx->must_get_next (tk_brace_start);

				// Mode must be set before dataheader is written here,
				// because onenter goes to a separate buffer.
				g_CurMode = onenter ? MODE_ONENTER : MODE_ONEXIT;
				w->write (onenter ? DH_ONENTER : DH_ONEXIT);
			}
			break;

			// ============================================================
			//
			case tk_int:
			case tk_str:
			case tk_void:
			{
				// For now, only globals are supported
				if (g_CurMode != MODE_TOPLEVEL || g_CurState.len())
					error ("variables must only be global for now");

				type_e type =	(TOKEN == "int") ? TYPE_INT :
								(TOKEN == "str") ? TYPE_STRING :
								TYPE_BOOL;

				m_lx->must_get_next();

				// Var name must not be a number
				if (TOKEN.is_numeric())
					error ("variable name must not be a number");

				string varname = TOKEN;
				script_variable* var = DeclareGlobalVariable (this, type, varname);
				m_lx->must_get_next (tk_semicolon);
			}
			break;

			// ============================================================
			//
			case tk_goto:
			{
				check_not_toplevel();

				// Get the name of the label
				m_lx->must_get_next();

				// Find the mark this goto statement points to
				int m = w->find_byte_mark (TOKEN);

				// If not set, define it
				if (m == MAX_MARKS)
				{
					m = w->add_mark (TOKEN);
					g_UndefinedLabels[m] = new string (TOKEN);
				}

				// Add a reference to the mark.
				w->write (DH_GOTO);
				w->add_reference (m);
				m_lx->must_get_next (tk_semicolon);
				continue;
			}

			// ============================================================
			//
			case tk_if:
			{
				check_not_toplevel();
				push_scope();

				// Condition
				m_lx->must_get_next (tk_paren_start);

				// Read the expression and write it.
				m_lx->must_get_next();
				data_buffer* c = parse_expression (TYPE_INT);
				w->write_buffer (c);

				m_lx->must_get_next (tk_paren_end);
				m_lx->must_get_next (tk_brace_start);

				// Add a mark - to here temporarily - and add a reference to it.
				// Upon a closing brace, the mark will be adjusted.
				int marknum = w->add_mark ("");

				// Use DH_IFNOTGOTO - if the expression is not true, we goto the mark
				// we just defined - and this mark will be at the end of the scope block.
				w->write (DH_IFNOTGOTO);
				w->add_reference (marknum);

				// Store it
				SCOPE (0).mark1 = marknum;
				SCOPE (0).type = SCOPETYPE_IF;
			} break;

			// ============================================================
			//
			case tk_else:
			{
				check_not_toplevel();
				m_lx->must_get_next (tk_brace_start);

				// Don't use PushScope as it resets the scope
				g_ScopeCursor++;

				if (g_ScopeCursor >= MAX_SCOPE)
					error ("too deep scope");

				if (SCOPE (0).type != SCOPETYPE_IF)
					error ("else without preceding if");

				// write down to jump to the end of the else statement
				// Otherwise we have fall-throughs
				SCOPE (0).mark2 = w->add_mark ("");

				// Instruction to jump to the end after if block is complete
				w->write (DH_GOTO);
				w->add_reference (SCOPE (0).mark2);

				// Move the ifnot mark here and set type to else
				w->move_mark (SCOPE (0).mark1);
				SCOPE (0).type = SCOPETYPE_ELSE;
			}
			break;

			// ============================================================
			//
			case tk_while:
			{
				check_not_toplevel();
				push_scope();

				// While loops need two marks - one at the start of the loop and one at the
				// end. The condition is checked at the very start of the loop, if it fails,
				// we use goto to skip to the end of the loop. At the end, we loop back to
				// the beginning with a go-to statement.
				int mark1 = w->add_mark (""); // start
				int mark2 = w->add_mark (""); // end

				// Condition
				m_lx->must_get_next (tk_paren_start);
				m_lx->must_get_next();
				data_buffer* expr = parse_expression (TYPE_INT);
				m_lx->must_get_next (tk_paren_end);
				m_lx->must_get_next (tk_brace_start);

				// write condition
				w->write_buffer (expr);

				// Instruction to go to the end if it fails
				w->write (DH_IFNOTGOTO);
				w->add_reference (mark2);

				// Store the needed stuff
				SCOPE (0).mark1 = mark1;
				SCOPE (0).mark2 = mark2;
				SCOPE (0).type = SCOPETYPE_WHILE;
			}
			break;

			// ============================================================
			//
			case tk_for:
			{
				check_not_toplevel();
				push_scope();

				// Initializer
				m_lx->must_get_next (tk_paren_start);
				m_lx->must_get_next();
				data_buffer* init = parse_statement (w);

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
				data_buffer* incr = parse_statement (w);

				if (!incr)
					error ("bad statement for incrementor of for");

				m_lx->must_get_next (tk_paren_end);
				m_lx->must_get_next (tk_brace_start);

				// First, write out the initializer
				w->write_buffer (init);

				// Init two marks
				int mark1 = w->add_mark ("");
				int mark2 = w->add_mark ("");

				// Add the condition
				w->write_buffer (cond);
				w->write (DH_IFNOTGOTO);
				w->add_reference (mark2);

				// Store the marks and incrementor
				SCOPE (0).mark1 = mark1;
				SCOPE (0).mark2 = mark2;
				SCOPE (0).buffer1 = incr;
				SCOPE (0).type = SCOPETYPE_FOR;
			}
			break;

			// ============================================================
			//
			case tk_do:
			{
				check_not_toplevel();
				push_scope();
				m_lx->must_get_next (tk_brace_start);
				SCOPE (0).mark1 = w->add_mark ("");
				SCOPE (0).type = SCOPETYPE_DO;
			}
			break;

			// ============================================================
			//
			case tk_switch:
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
				w->write_buffer (parse_expression (TYPE_INT));
				m_lx->must_get_next (tk_paren_end);
				m_lx->must_get_next (tk_brace_start);
				SCOPE (0).type = SCOPETYPE_SWITCH;
				SCOPE (0).mark1 = w->add_mark (""); // end mark
				SCOPE (0).buffer1 = null; // default header
			}
			break;

			// ============================================================
			//
			case tk_case:
			{
				// case is only allowed inside switch
				if (SCOPE (0).type != SCOPETYPE_SWITCH)
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
				w->SwitchBuffer = null;
				w->write (DH_CASEGOTO);
				w->write (num);
				add_switch_case (w, null);
				SCOPE (0).casenumbers[SCOPE (0).casecursor] = num;
			}
			break;

			// ============================================================
			//
			case tk_default:
			{
				if (SCOPE (0).type != SCOPETYPE_SWITCH)
					error ("default label outside switch");

				if (SCOPE (0).buffer1)
					error ("multiple default labels in one switch");

				m_lx->must_get_next (tk_colon);

				// The default header is buffered into buffer1, since
				// it has to be the last of the case headers
				//
				// Since the expression is pushed into the switch
				// and is only popped when case succeeds, we have
				// to pop it with DH_DROP manually if we end up in
				// a default.
				data_buffer* b = new data_buffer;
				SCOPE (0).buffer1 = b;
				b->write (DH_DROP);
				b->write (DH_GOTO);
				add_switch_case (w, b);
			}
			break;

			// ============================================================
			//
			case tk_break:
			{
				if (!g_ScopeCursor)
					error ("unexpected `break`");

				w->write (DH_GOTO);

				// switch and if use mark1 for the closing point,
				// for and while use mark2.
				switch (SCOPE (0).type)
				{
					case SCOPETYPE_IF:
					case SCOPETYPE_SWITCH:
					{
						w->add_reference (SCOPE (0).mark1);
					} break;

					case SCOPETYPE_FOR:
					case SCOPETYPE_WHILE:
					{
						w->add_reference (SCOPE (0).mark2);
					} break;

					default:
					{
						error ("unexpected `break`");
					} break;
				}

				m_lx->must_get_next (tk_semicolon);
			}
			break;

			// ============================================================
			//
			case tk_continue:
			{
				m_lx->must_get_next (tk_semicolon);

				int curs;
				bool found = false;

				// Fall through the scope until we find a loop block
				for (curs = g_ScopeCursor; curs > 0 && !found; curs--)
				{
					switch (scopestack[curs].type)
					{
						case SCOPETYPE_FOR:
						case SCOPETYPE_WHILE:
						case SCOPETYPE_DO:
						{
							w->write (DH_GOTO);
							w->add_reference (scopestack[curs].mark1);
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
			break;

			case tk_brace_end:
			{
				// Closing brace
				// If we're in the block stack, we're descending down from it now
				if (g_ScopeCursor > 0)
				{
					switch (SCOPE (0).type)
					{
						case SCOPETYPE_IF:
							// Adjust the closing mark.
							w->move_mark (SCOPE (0).mark1);

							// We're returning from if, thus else can be next
							g_CanElse = true;
							break;

						case SCOPETYPE_ELSE:
							// else instead uses mark1 for itself (so if expression
							// fails, jump to else), mark2 means end of else
							w->move_mark (SCOPE (0).mark2);
							break;

						case SCOPETYPE_FOR:
							// write the incrementor at the end of the loop block
							w->write_buffer (SCOPE (0).buffer1);

							// fall-thru
						case SCOPETYPE_WHILE:
							// write down the instruction to go back to the start of the loop
							w->write (DH_GOTO);
							w->add_reference (SCOPE (0).mark1);

							// Move the closing mark here since we're at the end of the while loop
							w->move_mark (SCOPE (0).mark2);
							break;

						case SCOPETYPE_DO:
						{
							must_get_next (tk_while);
							m_lx->must_get_next (tk_paren_start);
							m_lx->must_get_next();
							data_buffer* expr = parse_expression (TYPE_INT);
							m_lx->must_get_next (tk_paren_end);
							m_lx->must_get_next (tk_semicolon);

							// If the condition runs true, go back to the start.
							w->write_buffer (expr);
							w->write (DH_IFGOTO);
							w->add_reference (SCOPE (0).mark1);
							break;
						}

						case SCOPETYPE_SWITCH:
						{
							// Switch closes. Move down to the record buffer of
							// the lower block.
							if (SCOPE (1).casecursor != -1)
								w->SwitchBuffer = SCOPE (1).casebuffers[SCOPE (1).casecursor];
							else
								w->SwitchBuffer = null;

							// If there was a default in the switch, write its header down now.
							// If not, write instruction to jump to the end of switch after
							// the headers (thus won't fall-through if no case matched)
							if (SCOPE (0).buffer1)
								w->write_buffer (SCOPE (0).buffer1);
										else
								{
									w->write (DH_DROP);
									w->write (DH_GOTO);
									w->add_reference (SCOPE (0).mark1);
								}

							// Go through all of the buffers we
							// recorded down and write them.
							for (int u = 0; u < MAX_CASE; u++)
							{
								if (!SCOPE (0).casebuffers[u])
									continue;

								w->move_mark (SCOPE (0).casemarks[u]);
								w->write_buffer (SCOPE (0).casebuffers[u]);
							}

							// Move the closing mark here
							w->move_mark (SCOPE (0).mark1);
							break;
						}

						case SCOPETYPE_UNKNOWN:
							break;
					}

					// Descend down the stack
					g_ScopeCursor--;
					continue;
				}

				int dataheader =	(g_CurMode == MODE_EVENT) ? DH_ENDEVENT :
									(g_CurMode == MODE_MAINLOOP) ? DH_ENDMAINLOOP :
									(g_CurMode == MODE_ONENTER) ? DH_ENDONENTER :
									(g_CurMode == MODE_ONEXIT) ? DH_ENDONEXIT : -1;

				if (dataheader == -1)
					error ("unexpected `}`");

				// Data header must be written before mode is changed because
				// onenter and mainloop go into special buffers, and we want
				// the closing data headers into said buffers too.
				w->write (dataheader);
				g_CurMode = MODE_TOPLEVEL;

				if (PeekNext() == ";")
					m_lx->must_get_next (tk_semicolon);
			}
			break;

			// ============================================================
			case tk_const:
			{
				constant_info info;

				// Get the type
				m_lx->must_get_next();
				info.type = GetTypeByName (TOKEN);

				if (info.type == TYPE_UNKNOWN || info.type == TYPE_VOID)
					error ("unknown type `%s` for constant", TOKEN.c_str());

				m_lx->must_get_next();
				info.name = TOKEN;

				m_lx->must_get_next (tk_assign);

				switch (info.type)
				{
					case TYPE_BOOL:
					case TYPE_INT:
					{
						m_lx->must_get_next (tk_number);
						info.val = m_lx->get_token()->text.to_long();
					} break;

					case TYPE_STRING:
					{
						m_lx->must_get_next (tk_string);
						info.val = m_lx->get_token()->text;
					} break;

					case TYPE_UNKNOWN:
					case TYPE_VOID:
						break;
				}

				g_ConstInfo << info;

				m_lx->must_get_next (tk_semicolon);
				continue;
			}

			default:
			{
				// ============================================================
				// Label
				lexer::token* next;
				if (m_lx->get_token() == tk_symbol &&
					m_lx->peek_next (next) &&
					next->type == tk_colon)
				{
					check_not_toplevel();
					string label_name = token_string();

					// want no conflicts..
					if (FindCommand (label_name))
						error ("label name `%s` conflicts with command name\n", label_name);

					if (FindGlobalVariable (label_name))
						error ("label name `%s` conflicts with variable\n", label_name);

					// See if a mark already exists for this label
					int mark = -1;

					for (int i = 0; i < MAX_MARKS; i++)
					{
						if (g_UndefinedLabels[i] && *g_UndefinedLabels[i] == label_name)
						{
							mark = i;
							w->move_mark (i);

							// No longer undefinde
							delete g_UndefinedLabels[i];
							g_UndefinedLabels[i] = null;
						}
					}

					// Not found in unmarked lists, define it now
					if (mark == -1)
						w->add_mark (label_name);

					m_lx->must_get_next (tk_colon);
					continue;
				}

				// Check if it's a command
				CommandDef* comm = FindCommand (TOKEN);

				if (comm)
				{
					w->get_current_buffer()->merge (ParseCommand (comm));
					m_lx->must_get_next (tk_semicolon);
					continue;
				}

				// ============================================================
				// If nothing else, parse it as a statement
				data_buffer* b = parse_statement (w);

				if (!b)
					error ("unknown TOKEN `%s`", TOKEN.chars());

				w->write_buffer (b);
				m_lx->must_get_next (tk_semicolon);
			}
			break;
		}
	}

	// ===============================================================================
	// Script file ended. Do some last checks and write the last things to main buffer
	if (g_CurMode != MODE_TOPLEVEL)
		error ("script did not end at top level; a `}` is missing somewhere");

	// stateSpawn must be defined!
	if (!g_stateSpawnDefined)
		error ("script must have a state named `stateSpawn`!");

	for (int i = 0; i < MAX_MARKS; i++)
		if (g_UndefinedLabels[i])
			error ("label `%s` is referenced via `goto` but isn't defined\n", g_UndefinedLabels[i]->chars());

	// Dump the last state's onenter and mainloop
	w->write_member_buffers();

	// String table
	w->write_string_table();
}

// ============================================================================
// Parses a command call
data_buffer* botscript_parser::ParseCommand (CommandDef* comm)
{
	data_buffer* r = new data_buffer (64);

	if (g_CurMode == MODE_TOPLEVEL)
		error ("command call at top level");

	m_lx->must_get_next (tk_paren_start);
	m_lx->must_get_next();

	int curarg = 0;

	while (1)
	{
		if (m_lx->get_token() == tk_paren_end)
		{
			if (curarg < comm->numargs)
				error ("too few arguments passed to %s\n\tprototype: %s",
					comm->name.chars(), GetCommandPrototype (comm).chars());

			break;
			curarg++;
		}

		if (curarg >= comm->maxargs)
			error ("too many arguments passed to %s\n\tprototype: %s",
				comm->name.chars(), GetCommandPrototype (comm).chars());

		r->merge (parse_expression (comm->argtypes[curarg]));
		m_lx->must_get_next();

		if (curarg < comm->numargs - 1)
		{
			m_lx->must_be (tk_comma);
			m_lx->must_get_next();
		}
		else if (curarg < comm->maxargs - 1)
		{
			// Can continue, but can terminate as well.
			if (m_lx->get_token() == tk_paren_end)
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
		r->write (DH_PUSHNUMBER);
		r->write (comm->defvals[curarg]);
		curarg++;
	}

	r->write (DH_COMMAND);
	r->write (comm->number);
	r->write (comm->maxargs);

	return r;
}

// ============================================================================
// Is the given operator an assignment operator?
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
			case OPER_ASSIGNADD: return DH_ADDGLOBALVAR;
			case OPER_ASSIGNSUB: return DH_SUBGLOBALVAR;
			case OPER_ASSIGNMUL: return DH_MULGLOBALVAR;
			case OPER_ASSIGNDIV: return DH_DIVGLOBALVAR;
			case OPER_ASSIGNMOD: return DH_MODGLOBALVAR;
			case OPER_ASSIGN: return DH_ASSIGNGLOBALVAR;

			default: error ("bad assignment operator!!\n");
		}
	}

	switch (oper)
	{
	case OPER_ADD: return DH_ADD;
	case OPER_SUBTRACT: return DH_SUBTRACT;
	case OPER_MULTIPLY: return DH_MULTIPLY;
	case OPER_DIVIDE: return DH_DIVIDE;
	case OPER_MODULUS: return DH_MODULUS;
	case OPER_EQUALS: return DH_EQUALS;
	case OPER_NOTEQUALS: return DH_NOTEQUALS;
	case OPER_LESSTHAN: return DH_LESSTHAN;
	case OPER_GREATERTHAN: return DH_GREATERTHAN;
	case OPER_LESSTHANEQUALS: return DH_LESSTHANEQUALS;
	case OPER_GREATERTHANEQUALS: return DH_GREATERTHANEQUALS;
	case OPER_LEFTSHIFT: return DH_LSHIFT;
	case OPER_RIGHTSHIFT: return DH_RSHIFT;
	case OPER_OR: return DH_ORLOGICAL;
	case OPER_AND: return DH_ANDLOGICAL;
	case OPER_BITWISEOR: return DH_ORBITWISE;
	case OPER_BITWISEEOR: return DH_EORBITWISE;
	case OPER_BITWISEAND: return DH_ANDBITWISE;
	}

	error ("DataHeaderByOperator: couldn't find dataheader for operator %d!\n", oper);
	return 0;
}

// ============================================================================
// Parses an expression, potentially recursively
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
		Next();

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
			retbuf->write (DH_IFNOTGOTO); // if the first operand (condition)
			retbuf->add_reference (mark1); // didn't eval true, jump into mark1
			retbuf->merge (rb); // otherwise, perform second operand (true case)
			retbuf->write (DH_GOTO); // afterwards, jump to the end, which is
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
#define ISNEXT(C) (PeekNext (peek ? 1 : 0) == C)
int botscript_parser::parse_operator (bool peek)
{
	string oper;

	if (peek)
		oper += PeekNext();
	else
		oper += TOKEN;

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
	oper += PeekNext (peek ? 1 : 0);
	equalsnext = PeekNext (peek ? 2 : 1) == ("=");

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
	oper += PeekNext (peek ? 2 : 1);
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
string botscript_parser::parse_float()
{
	m_lx->must_be (tk_number);
	string floatstring = TOKEN;
	lexer::token* tok;

	// Go after the decimal point
	if (m_lx->peek_next (tok) && tok->type == tk_dot)
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
data_buffer* botscript_parser::parse_expr_value (type_e reqtype)
{
	data_buffer* b = new data_buffer (16);

	script_variable* g;

	// Prefixing "!" means negation.
	bool negate = (m_lx->get_token() == tk_exclamation_mark);

	if (negate) // Jump past the "!"
		m_lx->skip();

	// Handle strlen
	if (TOKEN == "strlen")
	{
		m_lx->must_get_next (tk_paren_start);
		m_lx->must_get_next();

		// By this TOKEN we should get a string constant.
		constant_info* constant = find_constant (TOKEN);

		if (!constant || constant->type != TYPE_STRING)
			error ("strlen only works with const str");

		if (reqtype != TYPE_INT)
			error ("strlen returns int but %s is expected\n", GetTypeName (reqtype).c_str());

		b->write (DH_PUSHNUMBER);
		b->write (constant->val.len());

		m_lx->must_get_next (tk_paren_end);
	}
	else if (TOKEN == "(")
	{
		// Expression
		m_lx->must_get_next();
		data_buffer* c = parse_expression (reqtype);
		b->merge (c);
		m_lx->must_get_next (tk_paren_end);
	}
	else if (CommandDef* comm = FindCommand (TOKEN))
	{
		delete b;

		// Command
		if (reqtype && comm->returnvalue != reqtype)
			error ("%s returns an incompatible data type", comm->name.chars());

		b = ParseCommand (comm);
	}
	else if (constant_info* constant = find_constant (TOKEN))
	{
		// Type check
		if (reqtype != constant->type)
			error ("constant `%s` is %s, expression requires %s\n",
				constant->name.c_str(), GetTypeName (constant->type).c_str(),
				GetTypeName (reqtype).c_str());

		switch (constant->type)
		{
			case TYPE_BOOL:
			case TYPE_INT:
				b->write (DH_PUSHNUMBER);
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
	else if ((g = FindGlobalVariable (TOKEN)))
	{
		// Global variable
		b->write (DH_PUSHGLOBALVAR);
		b->write (g->index);
	}
	else
	{
		// If nothing else, check for literal
		switch (reqtype)
		{
		case TYPE_VOID:
		case TYPE_UNKNOWN:
			error ("unknown identifier `%s` (expected keyword, function or variable)", TOKEN.chars());
			break;

		case TYPE_BOOL:
		case TYPE_INT:
		{
			m_lx->must_be (tk_number);

			// All values are written unsigned - thus we need to write the value's
			// absolute value, followed by an unary minus for negatives.
			b->write (DH_PUSHNUMBER);

			long v = atol (TOKEN);
			b->write (static_cast<word> (abs (v)));

			if (v < 0)
				b->write (DH_UNARYMINUS);

			break;
		}

		case TYPE_STRING:
			// PushToStringTable either returns the string index of the
			// string if it finds it in the table, or writes it to the
			// table and returns it index if it doesn't find it there.
			MustString (true);
			b->write_string (TOKEN);
			break;
		}
	}

	// Negate it now if desired
	if (negate)
		b->write (DH_NEGATELOGICAL);

	return b;
}

// ============================================================================
// Parses an assignment. An assignment starts with a variable name, followed
// by an assignment operator, followed by an expression value. Expects current
// TOKEN to be the name of the variable, and expects the variable to be given.
data_buffer* botscript_parser::ParseAssignment (script_variable* var)
{
	bool global = !var->statename.len();

	// Get an operator
	m_lx->must_get_next();
	int oper = parse_operator();

	if (!is_assignment_operator (oper))
		error ("expected assignment operator");

	if (g_CurMode == MODE_TOPLEVEL) // TODO: lift this restriction
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
		retbuf->write (global ? DH_PUSHGLOBALVAR : DH_PUSHLOCALVAR);
		retbuf->write (var->index);
		retbuf->merge (expr);
		retbuf->write ( (oper == OPER_ASSIGNLEFTSHIFT) ? DH_LSHIFT : DH_RSHIFT);
		retbuf->write (global ? DH_ASSIGNGLOBALVAR : DH_ASSIGNLOCALVAR);
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

void botscript_parser::push_scope()
{
	g_ScopeCursor++;

	if (g_ScopeCursor >= MAX_SCOPE)
		error ("too deep scope");

	ScopeInfo* info = &SCOPE (0);
	info->type = SCOPETYPE_UNKNOWN;
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

data_buffer* botscript_parser::parse_statement (object_writer* w)
{
	if (find_constant (TOKEN)) // There should not be constants here.
		error ("invalid use for constant\n");

	// If it's a variable, expect assignment.
	if (script_variable* var = FindGlobalVariable (TOKEN))
		return ParseAssignment (var);

	return null;
}

void botscript_parser::add_switch_case (object_writer* w, data_buffer* b)
{
	ScopeInfo* info = &SCOPE (0);

	info->casecursor++;

	if (info->casecursor >= MAX_CASE)
		error ("too many cases in one switch");

	// Init a mark for the case buffer
	int m = w->add_mark ("");
	info->casemarks[info->casecursor] = m;

	// Add a reference to the mark. "case" and "default" both
	// add the necessary bytecode before the reference.
	if (b)
		b->add_reference (m);
	else
		w->add_reference (m);

	// Init a buffer for the case block and tell the object
	// writer to record all written data to it.
	info->casebuffers[info->casecursor] = w->SwitchBuffer = new data_buffer;
}

constant_info* find_constant (string TOKEN)
{
	for (int i = 0; i < g_ConstInfo.size(); i++)
		if (g_ConstInfo[i].name == TOKEN)
			return &g_ConstInfo[i];

	return null;
}

// ============================================================================
//
bool botscript_parser::token_is (e_token a)
{
	return (m_lx->get_token() == a);
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