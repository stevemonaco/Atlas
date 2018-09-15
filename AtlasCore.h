#pragma once

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include "Table.h"
#include "GenericVariable.h"
#include "AtlasParser.h"
#include "AtlasFile.h"
#include "Pointer.h"
#include "AtlasLogger.h"
#include "AtlasStats.h"
#include "PointerHandler.h"
#include "AtlasExtension.h"
using namespace std;

//-----------------------------------------------------------------------------
// AtlasCore Functionality
//-----------------------------------------------------------------------------

class AtlasCore
{
public:
	AtlasCore();
	~AtlasCore();

	bool Insert(const char* RomFileName, const char* ScriptFileName);
	void SetDebugging(FILE* output);
	void CreateContext(AtlasContext** Context);
	bool ExecuteExtension(string& ExtId, string& FunctionName, AtlasContext** Context);
	bool ExecuteExtensionFunction(ExtensionFunction Func, AtlasContext** Context);
	unsigned int GetHeaderSize();

private:
	AtlasParser Parser;
	AtlasFile File;
	//AtlasFile GameFile;
	//AtlasFile PtrFile;
	VariableMap VarMap; // Variable Map for identifiers
	PointerHandler PtrHandler;
	Pointer DefaultPointer;
	EmbeddedPointerHandler EmbPtrs;
	InsertionStatistics Total;
	ExtensionManager Extensions;
	
	bool IsInJmp;
	unsigned int HeaderSize;
	
	bool ExecuteCommand(Command& Cmd);

	void PrintSummary(const char* Title, unsigned int TimeCompleted);
	void PrintStatistics();
	void PrintStatisticsBlock(const char* Title, InsertionStatistics& Stats);
	void PrintUnwrittenPointers();

	bool AddTable(Command& Cmd);
	bool ActivateTable(string& TableName);
	bool LoadTable(string& FileName, Table** Tbl);
	bool SetEndianSwap(string& Swap);
};

// Global Core variables
extern unsigned int CurrentLine;
extern int bSwap;
extern int StringAlign;
extern int MaxEmbPtr;
extern AtlasCore Atlas;

// Misc functions
inline unsigned int StringToUInt(std::string& NumberString);
__int64 StringToInt64(std::string& NumberString);
unsigned int GetHexDigit(char digit);
unsigned int EndianSwap(unsigned int Num, int Size);