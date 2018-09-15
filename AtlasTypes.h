#pragma once

#include <map>
#include <string>

/* Misc Functions */
static const unsigned int CMD_JMP1 = 0;
static const unsigned int CMD_JMP2 = 1;
static const unsigned int CMD_JMP3 = 2;
static const unsigned int CMD_JMP4 = 3;
static const unsigned int CMD_SMA = 4;
static const unsigned int CMD_HDR = 5;
static const unsigned int CMD_STRTYPE = 6;
static const unsigned int CMD_ADDTBL = 7;
static const unsigned int CMD_ACTIVETBL = 8;
static const unsigned int CMD_VAR = 9;
/* Pointer Functions */
static const unsigned int CMD_WUB = 10;
static const unsigned int CMD_WBB = 11;
static const unsigned int CMD_WHB = 12;
static const unsigned int CMD_WLB = 13;
static const unsigned int CMD_W16 = 14;
static const unsigned int CMD_W24 = 15;
static const unsigned int CMD_W32 = 16;
static const unsigned int CMD_EMBSET = 17;
static const unsigned int CMD_EMBTYPE = 18;
static const unsigned int CMD_EMBWRITE = 19;
/* Debugging Functions */
static const unsigned int CMD_BREAK = 20;
/* Extended Pointer Functionality */
static const unsigned int CMD_PTRTBL = 21;
static const unsigned int CMD_WRITETBL = 22;
static const unsigned int CMD_PTRLIST = 23;
static const unsigned int CMD_WRITELIST = 24;
static const unsigned int CMD_AUTOWRITETBL = 25;
static const unsigned int CMD_AUTOWRITELIST = 26;
static const unsigned int CMD_CREATEPTR = 27;
static const unsigned int CMD_WRITEPTR = 28;

static const unsigned int CMD_LOADEXT = 29;
static const unsigned int CMD_EXECEXT = 30;
static const unsigned int CMD_DISABLETABLE = 31;
static const unsigned int CMD_DISABLELIST = 32;
static const unsigned int CMD_PASCALLEN = 33;
static const unsigned int CMD_AUTOEXEC = 34;
static const unsigned int CMD_DISABLEEXEC = 35;
static const unsigned int CMD_FIXEDLENGTH = 36;

static const unsigned int CMD_WUBCUST = 37;
static const unsigned int CMD_WBBCUST = 38;
static const unsigned int CMD_WHBCUST = 39;
static const unsigned int CMD_WLBCUST = 40;
static const unsigned int CMD_ENDIANSWAP = 41;
static const unsigned int CMD_STRINGALIGN = 42;

static const unsigned int CMD_EMBPTRTABLE = 43;
static const unsigned int CMD_WHW = 44;
static const unsigned int CMD_WHWCUST = 45;
static const unsigned int CMD_SETTARGETFILE = 46;
static const unsigned int CMD_SETPTRFILE = 47;
static const unsigned int CMD_WRITEEMBTBL1 = 48;
static const unsigned int CMD_WRITEEMBTBL2 = 49;
static const unsigned int CMD_WRITETBL2 = 50;

static const unsigned int CMD_FILL = 51;
static const unsigned int CMD_FILL2 = 52;
static const unsigned int CMD_INSERTBINARY = 53;

static const unsigned int CommandCount = 54;

static const char* CommandStrings[CommandCount] = { "JMP", "JMP", "JMP", "JMP", "SMA", "HDR", "STRTYPE",
	"ADDTBL", "ACTIVETBL", "VAR", "WUB", "WBB", "WHB", "WLB", "W16", "W24", "W32",
	"EMBSET", "EMBTYPE", "EMBWRITE", "BREAK", "PTRTBL",	"WRITE", "PTRLIST", "WRITE",
	"AUTOWRITE", "AUTOWRITE", "CREATEPTR", "WRITE",	"LOADEXT", "EXECEXT", "DISABLE", "DISABLE",
	"PASCALLEN", "AUTOEXEC", "DISABLE", "FIXEDLENGTH", "WUB", "WBB", "WHB", "WLB",
	"ENDIANSWAP", "STRINGALIGN", "EMBPTRTBL", "WHW", "WHW", "SETTARGETFILE", "SETPTRFILE", "WRITE",
	"WRITE", "WRITE", "FILL", "FILL", "INSERTBINARY" };

// Parameter types
static const unsigned int TypeCount = 12;

static const unsigned int P_INVALID = 0;
static const unsigned int P_VOID = 1;
static const unsigned int P_STRING = 2;
static const unsigned int P_VARIABLE = 3;
static const unsigned int P_NUMBER = 4;
static const unsigned int P_DOUBLE = 5;
static const unsigned int P_TABLE = 6;
static const unsigned int P_POINTERTABLE = 7;
static const unsigned int P_EMBPOINTERTABLE = 8;
static const unsigned int P_POINTERLIST = 9;
static const unsigned int P_CUSTOMPOINTER = 10;
static const unsigned int P_EXTENSION = 11;

static const char* TypeStrings[TypeCount] = { "INVALID", "VOID", "STRING", "VARIABLE",
	"NUMBER", "DOUBLE", "TABLE", "POINTERTABLE", "EMBPOINTERTABLE", "POINTERLIST", "CUSTOMPOINTER", "EXTENSION" };

static const unsigned int Types[CommandCount][5] = {
{ P_NUMBER }, { P_NUMBER, P_NUMBER }, { P_NUMBER, P_STRING }, { P_NUMBER, P_NUMBER, P_STRING }, // JMP1 JMP2 JMP3 JMP4
{ P_STRING }, { P_NUMBER }, { P_STRING }, { P_STRING, P_TABLE }, { P_TABLE }, // SMA HDR STRTYPE ADDTBL ACTIVETBL
{ P_VARIABLE, P_VARIABLE }, { P_NUMBER }, {P_NUMBER }, // VAR WUB WBB
{ P_NUMBER }, { P_NUMBER }, { P_NUMBER }, { P_NUMBER }, { P_NUMBER }, // WHB WLB W16 W24 W32
{ P_NUMBER }, { P_STRING, P_NUMBER, P_NUMBER }, { P_NUMBER }, // EMBSET EMBTYPE EMBWRITE
{ P_VOID }, // BREAK
{ P_POINTERTABLE, P_NUMBER, P_NUMBER, P_CUSTOMPOINTER }, { P_POINTERTABLE }, // PTRTBL WRITE
{ P_POINTERLIST, P_STRING, P_CUSTOMPOINTER }, { P_POINTERLIST }, // PTRLIST WRITE
{ P_POINTERTABLE, P_STRING }, { P_POINTERLIST, P_STRING }, // AUTOWRITE AUTOWRITE
{ P_CUSTOMPOINTER, P_STRING, P_NUMBER, P_NUMBER }, { P_CUSTOMPOINTER, P_NUMBER }, // CREATEPTR WRITE
{ P_EXTENSION, P_STRING }, { P_EXTENSION, P_STRING }, // LOADEXT EXECEXT
{ P_POINTERTABLE, P_STRING }, { P_POINTERLIST, P_STRING }, // DISABLE DISABLE
{ P_NUMBER }, { P_EXTENSION, P_STRING, P_STRING }, // PASCALLEN AUTOEXEC
{ P_STRING, P_STRING }, { P_NUMBER, P_NUMBER }, // DISABLE FIXEDLENGTH
{ P_CUSTOMPOINTER, P_NUMBER }, { P_CUSTOMPOINTER, P_NUMBER }, // WUB WBB
{ P_CUSTOMPOINTER, P_NUMBER }, { P_CUSTOMPOINTER, P_NUMBER }, // WHB WLB
{ P_STRING }, { P_NUMBER }, // ENDIANSWAP STRINGALIGN
{ P_EMBPOINTERTABLE, P_NUMBER, P_CUSTOMPOINTER }, // EMBPTRTBL
{ P_NUMBER }, { P_CUSTOMPOINTER, P_NUMBER }, { P_STRING }, {P_STRING }, // WHW WHW SETTARGETFILE SETPTRFILE
{ P_EMBPOINTERTABLE }, { P_EMBPOINTERTABLE, P_NUMBER }, { P_POINTERTABLE, P_NUMBER }, // WRITE WRITE WRITE
{ P_NUMBER } , { P_NUMBER, P_NUMBER }, { P_STRING, P_NUMBER, P_NUMBER } // FILL FILL2 INSERTBINARY
};

static const unsigned int ParamCount[CommandCount] = { 1, 2, 2, 3, 1, // JMP1 JMP2 JMP3 JMP4 SMA
1, 1, 2, 1, 2, 1, 1, // HDR STRTYPE ADDTBL ACTIVETBL VAR WUB WBB
1, 1, 1, 1, 1, 1, 3, 1, // WHB WLB W16 W24 W32 EMBSET EMBTYPE EMBWRITE
1, // BREAK
4, 1, 3, 1, 2, 2, // PTRTBL WRITE PTRLIST WRITE AUTOWRITE AUTOWRITE
4, 2, 2, 2, 2, 2, 1, 3, 2, 2, // CREATEPTR WRITE LOADEXT EXECEXT DISABLE DISABLE PASCALLEN AUTOEXEC DISABLE FIXEDLENGTH
2, 2, 2, 2, 1, 1, 3, 1, 2, 1, 1, // WUB WBB WHB WLB ENDIANSWAP STRINGALIGN EMBPTRTBL WHW WHW SETTARGETFILE SETPTRFILE
1, 2, 2, 1, 2, 3 }; // WRITE WRITE WRITE FILL FILL INSERTBINARY

typedef struct Parameter
{
	std::string Value;
	unsigned int Type;
} Parameter;

typedef struct Command
{
	unsigned int Function;
	std::vector<Parameter> Parameters;
	unsigned int Line;
} Command;

typedef struct AtlasBlock
{
	std::list<Command> Commands;
	std::list<std::string> TextLines;
	unsigned int StartLine;
} AtlasBlock;

typedef std::multimap<std::string, unsigned int> StrCmdMap;
typedef std::multimap<std::string, unsigned int>::const_iterator StrCmdMapIt;

typedef std::map<std::string, unsigned int> StrTypeMap;
typedef std::map<std::string, unsigned int>::const_iterator StrTypeMapIt;

typedef std::vector<Parameter>::iterator ListParamIt;
typedef std::list<std::string>::iterator ListStringIt;
typedef std::list<Command>::iterator ListCmdIt;
typedef std::list<AtlasBlock>::iterator ListBlockIt;