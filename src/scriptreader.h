#ifndef BOTC_SCRIPTREADER_H
#define BOTC_SCRIPTREADER_H

#include <stdio.h>
#include "main.h"
#include "commands.h"
#include "objwriter.h"

#define MAX_FILESTACK 8
#define MAX_SCOPE 32
#define MAX_CASE 64

class ScriptVar;

// Operators
enum operator_e {
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

// Mark types
enum marktype_e {
	MARKTYPE_LABEL,
	MARKTYPE_IF,
	MARKTYPE_INTERNAL, // internal structures
};

// Block types
enum scopetype_e {
	SCOPETYPE_UNKNOWN,
	SCOPETYPE_IF,
	SCOPETYPE_WHILE,
	SCOPETYPE_FOR,
	SCOPETYPE_DO,
	SCOPETYPE_SWITCH,
	SCOPETYPE_ELSE,
};

// ============================================================================
// Meta-data about blocks
struct ScopeInfo {
	unsigned int mark1;
	unsigned int mark2;
	scopetype_e type;
	DataBuffer* buffer1;
	
	// switch-related stuff
	// Which case are we at?
	short casecursor;
	
	// Marks to case-blocks
	int casemarks[MAX_CASE];
	
	// Numbers of the case labels
	int casenumbers[MAX_CASE];
	
	// actual case blocks
	DataBuffer* casebuffers[MAX_CASE];
	
	// What is the current buffer of the block?
	DataBuffer* recordbuffer;
};

// ============================================================================
typedef struct {
	string name;
	type_e type;
	string val;
} constinfo_t;

// ============================================================================
// The script reader reads the script, parses it and tells the object writer
// the bytecode it needs to write to file.
class ScriptReader {
public:
	// ====================================================================
	// MEMBERS
	FILE* fp[MAX_FILESTACK];
	string filepath[MAX_FILESTACK];
	int fc;
	
	unsigned int pos[MAX_FILESTACK];
	unsigned int curline[MAX_FILESTACK];
	unsigned int curchar[MAX_FILESTACK];
	ScopeInfo scopestack[MAX_SCOPE];
	long savedpos[MAX_FILESTACK]; // filepointer cursor position
	string token;
	int commentmode;
	long prevpos;
	string prevtoken;
	
	// ====================================================================
	// METHODS
	// scriptreader.cxx:
	ScriptReader (string path);
	~ScriptReader ();
	void OpenFile (string path);
	void CloseFile (unsigned int u = MAX_FILESTACK);
	char ReadChar ();
	char PeekChar (int offset = 0);
	bool Next (bool peek = false);
	void Prev ();
	string PeekNext (int offset = 0);
	void Seek (unsigned int n, int origin);
	void MustNext (const char* c = "");
	void MustThis (const char* c);
	void MustString (bool gotquote = false);
	void MustNumber (bool fromthis = false);
	void MustBool ();
	bool BoolValue ();
	
	void ParserError (const char* message, ...);
	void ParserWarning (const char* message, ...);
	
	// parser.cxx:
	void ParseBotScript (ObjWriter* w);
	DataBuffer* ParseCommand (CommandDef* comm);
	DataBuffer* ParseExpression (type_e reqtype);
	DataBuffer* ParseAssignment (ScriptVar* var);
	int ParseOperator (bool peek = false);
	DataBuffer* ParseExprValue (type_e reqtype);
	string ParseFloat ();
	void PushScope ();
	
	// preprocessor.cxx:
	void PreprocessDirectives ();
	void PreprocessMacros ();
	DataBuffer* ParseStatement (ObjWriter* w);
	void AddSwitchCase (ObjWriter* w, DataBuffer* b);
	
private:
	bool atnewline;
	char c;
	void ParserMessage (const char* header, string message);
	
	bool DoDirectivePreprocessing ();
	char PPReadChar ();
	void PPMustChar (char c);
	string PPReadWord (char &term);
};

constinfo_t* FindConstant (string token);
extern bool g_Neurosphere;

#endif // BOTC_SCRIPTREADER_H