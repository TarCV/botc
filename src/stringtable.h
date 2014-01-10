#ifndef BOTC_STRINGTABLE_H
#define BOTC_STRINGTABLE_H

#include "main.h"
#include "bots.h"

int get_string_table_index(const string& a);
const string_list& get_string_table();
int num_strings_in_table();

#endif // BOTC_STRINGTABLE_H
