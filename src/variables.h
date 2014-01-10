#ifndef BOTC_VARIABLES_H
#define BOTC_VARIABLES_H

#include "str.h"
#include "scriptreader.h"

struct ScriptVar {
	string name;
	string statename;
	type_e type;
	int value;
	unsigned int index;
};

extern list<ScriptVar> g_GlobalVariables;
extern list<ScriptVar> g_LocalVariables;

#define ITERATE_GLOBAL_VARS(u) \
	for (u = 0; u < MAX_SCRIPT_VARIABLES; u++)
#define ITERATE_SCRIPT_VARS(g) \
	for (g = g_ScriptVariable; g != null; g = g->next)

ScriptVar* DeclareGlobalVariable (ScriptReader* r, type_e type, string name);
deprecated unsigned int CountGlobalVars ();
ScriptVar* FindGlobalVariable (string name);

#endif // BOTC_VARIABLES_H