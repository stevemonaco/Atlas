//-----------------------------------------------------------------------------
// Return Messages
//-----------------------------------------------------------------------------
#pragma once

#define TBL_OK				0x00 // Success
#define TBL_OPEN_ERROR		0x01 // Cannot open the Table properly
#define TBL_PARSE_ERROR		0x02 // Cannot parse how the Table is typed
#define NO_MATCHING_ENTRY	0x10 // There was an entry that cannot be matched in the table

// Other
#define SPACE				0x20

#include "stdafx.h"
#include <string>
#include <vector>
#include <fstream>
#include <map>

// Structure for errors
typedef struct TBL_ERROR
{
	unsigned int LineNo; // The line number which the error occurred
	std::string ErrorDesc; // A description of what the error was
} TBL_ERROR;

// Data Structure for a table bookmark
typedef struct TBL_BOOKMARK
{
	unsigned int address;
	std::string description;
} TBL_BOOKMARK;

// Data Structure for a script dump bookmark
typedef struct TBL_DUMPMARK
{
	unsigned int StartAddress;
	unsigned int EndAddress;
	std::string description;
} TBL_DUMPMARK;

// Data Structure for a script insertion bookmark
typedef struct TBL_INSMARK
{
	unsigned int address;
	std::string filename;
	std::string description;
} TBL_INSMARK;

// Data Structure for a script string
typedef struct TBL_STRING
{
	std::string Text;
	std::string EndToken;
} TBL_STRING;

// Data Structure for an unencoded (text) string
typedef struct TXT_STRING
{
	std::string Text;
	std::string EndToken;
} TXT_STRING;

typedef std::map<std::string,std::string> StrStrMap;
typedef std::list<TBL_STRING>::iterator ListTblStringIt;
typedef std::list<TXT_STRING>::iterator ListTxtStringIt;

//-----------------------------------------------------------------------------
// Table Interfaces
//-----------------------------------------------------------------------------

class Table
{
public:
	Table();
	~Table();

	int OpenTable(const char* TableFilename);
	unsigned int EncodeStream(std::string& scriptbuf, unsigned int& BadCharOffset);

	std::vector<TBL_ERROR>		Errors;			// Errors
	std::vector<TBL_BOOKMARK>	Bookmarks;		// Normal bookmarks
	std::vector<TBL_DUMPMARK>	Dumpmarks;		// Script dump bookmarks
	std::vector<TBL_INSMARK>	Insertmarks;	// Insertion bookmarks
	std::list<TBL_STRING>		StringTable;	// (Encoded) String table
	std::list<TXT_STRING>		TxtStringTable; // Text String Table
	std::vector<std::string>	EndTokens;		// String end tokens

	std::map<std::string, std::string> LookupHex; // for looking up hex values.  (insertion)

	unsigned int StringCount;
	bool bAddEndToken;

private:
	inline void InitHexTable();

	inline bool parsebookmark(std::ifstream& file);
	inline bool parseendline(std::ifstream& file);
	inline bool parseendstring(std::ifstream& file);
	inline bool parseentry(std::ifstream& file);
	inline bool parsescriptinsert(std::ifstream& file);
	inline bool parsescriptdump(std::ifstream& file);
	inline void parsews(std::ifstream& file);

	inline std::string& GetHexValue(std::string& Textstring);

	inline void AddToTable(std::string& Hexstring, TBL_STRING* TblStr);

	std::string DefEndLine;
	std::string DefEndString;
	unsigned int LineNumber;	// The line number that the library is reading
	unsigned int TblEntries;	// The number of table entries
	unsigned int LongestHex;	// The longest hex entry, in bytes

	unsigned int LongestText[256];
};

inline unsigned short HexToDec(char HexChar);