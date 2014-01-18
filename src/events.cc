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

#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "scriptreader.h"
#include "str.h"
#include "events.h"
#include "lexer.h"

static void unlink_events();
static list<event_info*> g_events;

// ============================================================================
// Read event definitions from file
void init_events()
{
	lexer lx;
	lx.process_file ("events.def");
	int num_events = 0;

	while (lx.get_next())
	{
		lx.must_be (tk_symbol);
		event_info* e = new event_info;
		e->name = lx.get_token()->text;
		e->number = num_events++;
		g_events << e;
	}

	printf ("%d event definitions read.\n", num_events);
	atexit (&unlink_events);
}

// ============================================================================
// Delete event definitions recursively
static void unlink_events()
{
	print ("Freeing event information.\n");

	for (event_info* e : g_events)
		delete e;

	g_events.clear();
}

// ============================================================================
// Finds an event definition by index
event_info* find_event_by_index (int idx)
{
	return g_events[idx];
}

// ============================================================================
// Finds an event definition by name
event_info* find_event_by_name (string a)
{
	for (event_info* e : g_events)
		if (a.to_uppercase() == e->name.to_uppercase())
			return e;

	return null;
}
