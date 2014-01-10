#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "bots.h"
#include "stringtable.h"

static string_list g_string_table;

// ============================================================================
//
const string_list& get_string_table()
{
	return g_string_table;
}

// ============================================================================
// Potentially adds a string to the table and returns the index of it.
//
int get_string_table_index (const string& a)
{
	// Must not be too long.
	if (strlen (a) >= MAX_STRING_LENGTH)
		error ("string `%s` too long (%d characters, max is %d)\n",
			   a.c_str(), a.length(), MAX_STRING_LENGTH);

	// Find a free slot in the table.
	int idx;

	for (idx = 0; idx < g_string_table.size(); idx++)
	{
		// String is already in the table, thus return it.
		if (g_string_table[idx] == a)
			return idx;
	}

	// Check if the table is already full
	if (g_string_table.size() == MAX_LIST_STRINGS - 1)
		error ("too many strings!\n");

	// Now, dump the string into the slot
	g_string_table[idx] = a;

	return idx;
}

// ============================================================================
// Counts the amount of strings in the table.
//
int num_strings_in_table()
{
	return g_string_table.size();
}
