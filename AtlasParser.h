#pragma once

#include <string>
#include <map>
#include <fstream>
#include "AtlasTypes.h"
#include "GenericVariable.h"

class AtlasParser
{
public:
	AtlasParser(VariableMap* Map);
	~AtlasParser();
	bool ParseFile(std::ifstream& infile);

	std::list<AtlasBlock> Blocks;

private:
	void ParseLine(std::string& line);
	inline void ParseCommand(std::string& line);
	inline void FlushBlock();
	inline void AddText(std::string& text);
	inline unsigned int IdentifyType(std::string& str);
	inline bool AddCommand(std::string& CmdStr, Command& Cmd);
	inline bool AddUnitializedVariable(std::string& VarName, std::string& Type);

	unsigned int CurrentLine;
	AtlasBlock CurBlock;
	StrCmdMap CmdMap;
	StrTypeMap TypeMap;
	VariableMap* VarMap;
};