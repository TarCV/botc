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

#ifndef TOKENS_H
#define TOKENS_H

// =======================================================
enum e_token
{
	// Non-word tokens
	tk_equals,				// ----- 0
	tk_brackets,			// - 1
	tk_add_assign,			// - 2
	tk_sub_assign,			// - 3
	tk_multiply_assign,		// - 4
	tk_divide_assign,		// ----- 5
	tk_modulus_assign,		// - 6
	tk_single_quote,		// - 7
	tk_dollar_sign,			// - 8
	tk_paren_start,			// - 9
	tk_paren_end,			// ----- 10
	tk_bracket_start,		// - 11
	tk_bracket_end,			// - 12
	tk_brace_start,			// - 13
	tk_brace_end,			// - 14
	tk_assign,				// ----- 15
	tk_plus,				// - 16
	tk_minus,				// - 17
	tk_multiply,			// - 18
	tk_divide,				// - 19
	tk_modulus,				// ----- 20
	tk_comma,				// - 21
	tk_lesser,				// - 22
	tk_greater,				// - 23
	tk_dot,					// - 24
	tk_colon,				// ----- 25
	tk_semicolon,			// - 26
	tk_hash,				// - 27
	tk_exclamation_mark,	// - 28
	tk_arrow,				// - 29

	// --------------
	// Named tokens
	tk_bool,				// ----- 30
	tk_break,				// - 31
	tk_case,				// - 32
	tk_continue,			// - 33
	tk_const,				// - 34
	tk_default,				// ----- 35
	tk_do,					// - 36
	tk_else,				// - 37
	tk_event,				// - 38
	tk_for,					// - 39
	tk_goto,				// ----- 40
	tk_if,					// - 41
	tk_int,					// - 42
	tk_mainloop,			// - 43
	tk_onenter,				// - 44
	tk_onexit,				// ----- 45
	tk_state,				// - 46
	tk_switch,				// - 47
	tk_str,					// - 48
	tk_void,				// - 49
	tk_while,				// ----- 50

	// These ones aren't implemented yet but I plan to do so, thus they are
	// reserved. Also serves as a to-do list of sorts for me. >:F
	tk_enum,				// - 51
	tk_func,				// - 52
	tk_return,				// - 53

	// --------------
	// Generic tokens
	tk_symbol,				// - 54
	tk_number,				// ----- 55
	tk_string,				// - 56

	last_named_token = (int) tk_symbol - 1,
	tk_any = INT_MAX
};

#endif