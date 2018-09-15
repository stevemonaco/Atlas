#include "stdafx.h"
#include <map>
#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <utility>
#include "AtlasTypes.h"
#include "AtlasParser.h"
#include "GenericVariable.h"
#include "AtlasLogger.h"
#include "AtlasCore.h"

using namespace std;

AtlasParser::AtlasParser(VariableMap* Map)
{
	VarMap = Map;

	// Initialize the function lookup map
	for(unsigned int i=0; i < CommandCount; i++)
		CmdMap.insert(multimap<string,unsigned int>::value_type(CommandStrings[i], i));

	for(unsigned int i=0; i < TypeCount; i++)
		TypeMap.insert(multimap<string,unsigned int>::value_type(TypeStrings[i], i));

	CurrentLine = 0;
	CurBlock.StartLine = -1;
}

AtlasParser::~AtlasParser()
{
}

bool AtlasParser::ParseFile(ifstream& infile)
{
	list<string> text;
	unsigned char utfheader[4];
	
	string line;

	// Detect UTF-8 header
	if(infile.peek() == 0xEF)
	{
		infile.read((char*)utfheader, 3);
		if(utfheader[0] != 0xEF || utfheader[1] != 0xBB || utfheader[2] != 0xBF)
			infile.seekg(ios::beg); // Seek beginning, not a UTF-8 header
	}

	// Read the file
	while(!infile.eof())
	{
		getline(infile, line);
		text.push_back(line);
	}

	infile.close();
	CurrentLine = 1;

	// Parse the file and build the series of AtlasBlocks
	for(ListStringIt it = text.begin(); it != text.end(); it++)
	{
		ParseLine(*it);
		CurrentLine++;
	}

	if(!CurBlock.Commands.empty() || !CurBlock.TextLines.empty())
		FlushBlock();

	for(ListErrorIt i = Logger.Errors.begin(); i != Logger.Errors.end(); i++)
	{
		if(i->Severity == FATALERROR)
			return false;
	}

	return true;
}

void AtlasParser::ParseLine(string& line)
{
	size_t firstchar = line.find_first_not_of(" \t", 0);

	if(firstchar == string::npos) // All whitespace
	{
		string s = "";
		AddText(s);
		return;
	}

	string editline = line.substr(firstchar, line.length() - firstchar);

	switch(line[firstchar])
	{
	case '#': // Atlas command
		if(CurBlock.TextLines.empty()) // No text, build more commands
		{
			ParseCommand(editline);
		}
		else // Clear, parse the command
		{
			FlushBlock();
			ParseCommand(editline);
		}
		break;
	case '/': // Possible comment
		if(line.length() > firstchar+1)
		{
			if(line[firstchar+1] != '/') // Not a comment "//", but text
				AddText(line);
			// else; Comment
		}
		else // Single text character of '/'
			AddText(line);
		break;
	default:  // Text
		AddText(line);
		break;
	}
}

inline void AtlasParser::FlushBlock()
{
	Blocks.push_back(CurBlock);
	CurBlock.TextLines.clear();
	CurBlock.Commands.clear();
	CurBlock.StartLine = -1;
}

inline void AtlasParser::AddText(string& text)
{
	if(CurBlock.StartLine == -1)
		CurBlock.StartLine = CurrentLine;

	CurBlock.TextLines.push_back(text);
}

inline void AtlasParser::ParseCommand(string& line)
{
	if(line[0] != '#')
		printf("Bug, %s %d.  Should start with a '#'\n'%s'", __FILE__, __LINE__, line);

	size_t curpos = 1;
	std::string CmdStr;
	
	Parameter Param;
	Command Command;
	Param.Type = P_INVALID;

	char ch;

	while(curpos < line.length() && (ch = line[curpos]) != '(')
	{
		if(isalpha(ch) || isdigit(ch))
			CmdStr += ch;
		else Logger.ReportError(CurrentLine, "Invalid syntax: Nonalphabetical character in command");

		curpos++;
	}

	curpos = line.find_first_not_of(" \t", curpos + 1); // Skip ')', get first non 
														// wspace character

	// Parse parameters
	unsigned int ParamNum = 1;
	while(curpos < line.length() && (ch = line[curpos]) != ')')
	{
		if(ch == ',')
		{
			// Trim trailing whitespace
			size_t Last;
			for(Last = Param.Value.length() - 1; Last > 0; Last--)
				if(Param.Value[Last] != ' ' && Param.Value[Last] != '\t')
					break;
			if(Last < Param.Value.length())
				Param.Value.erase(Last+1);

			Param.Type = IdentifyType(Param.Value);
			if(Param.Type == P_INVALID)
				Logger.ReportError(CurrentLine, "Invalid argument for %s for parameter %d", CmdStr.c_str(), ParamNum);
			Command.Parameters.push_back(Param);
			Param.Type = P_INVALID;
			Param.Value.clear();
			ParamNum++;
			curpos = line.find_first_not_of(" \t", curpos + 1);
		}
		else
		{
			Param.Value += ch;
			curpos++;
		}
	}

	// Trim trailing whitespace
	size_t Last;
	for(Last = Param.Value.length() - 1; Last > 0; Last--)
		if(Param.Value[Last] != ' ' && Param.Value[Last] != '\t')
			break;
	if(Last < Param.Value.length())
	Param.Value.erase(Last+1);

	Param.Type = IdentifyType(Param.Value);
	if(Param.Type == P_INVALID)
		Logger.ReportError(CurrentLine, "Invalid argument for %s for parameter %d", CmdStr.c_str(), ParamNum);

	for(ListErrorIt i = Logger.Errors.begin(); i != Logger.Errors.end(); i++)
		if(i->Severity = FATALERROR)
			return;

	Command.Parameters.push_back(Param);

	AddCommand(CmdStr, Command);
}

inline unsigned int AtlasParser::IdentifyType(string& str)
{
	if(str.empty())
		return P_VOID;

	size_t charpos = 0;

	// Check for number (int/uint)
	if(str[0] == '$')
	{
		charpos = str.find_first_of('-', 1);
		if(charpos == 1 || charpos == string::npos)
		{
			charpos = str.find_first_not_of("-0123456789ABCDEF", 1);
			if(charpos == string::npos)
				return P_NUMBER;
		}
	}
	else
	{
		charpos = str.find_first_of('-');
		if(charpos == 0 || charpos == string::npos)
		{
			charpos = str.find_first_not_of("0123456789", 1);
			if(charpos == string::npos)
				return P_NUMBER;
		}
	}
	
	// Check for double
	charpos = str.find_first_not_of("0123456789.", 0);
	if(charpos == string::npos)
		return P_DOUBLE;

	// Check for variable
	charpos = str.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJLMNOPQRSTUVWXYZ", 0);
	if(charpos == 0)
	{
		charpos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJLMNOPQRSTUVWXYZ0123456789", 0);
		if(charpos == string::npos)
			return P_VARIABLE;
	}

	// Check for string
	if((str[0] == '"') && (str[str.length()-1] == '"'))
	{
		str = str.substr(1, str.length()-2);
		return P_STRING;
	}

	return P_INVALID;
}

inline bool AtlasParser::AddCommand(string& CmdStr, Command& Cmd)
{
	bool bFound = false;
	unsigned int CmdNum = 0;

	Cmd.Line = CurrentLine;
	pair<StrCmdMapIt, StrCmdMapIt> val = CmdMap.equal_range(CmdStr);
	if(val.first == val.second) // not found
	{
		Logger.ReportError(CurrentLine, "Invalid command %s", CmdStr.c_str());
		return false;
	}
    
	// Found one or more matches
	for(StrCmdMapIt i = val.first; i != val.second; i++)
	{
		CmdNum = i->second;
		if(ParamCount[CmdNum] != Cmd.Parameters.size())
			continue;
		// Found a matching arg count function, check types
		ListParamIt it = Cmd.Parameters.begin();
		for(unsigned int j = 0; j < Cmd.Parameters.size(); j++, it++)
		{
			if((it->Type == P_VARIABLE) && (CmdNum != CMD_VAR)) // Type-checking for vars
			{
				GenericVariable* var = VarMap->GetVar(it->Value);
				if(var) // Found, check the type
				{
					if(var->GetType() != Types[CmdNum][j]) // Type mismatch
						break;
					else
						it->Type = Types[CmdNum][j];
				}
				else // NULL
				{
					Logger.ReportError(CurrentLine, "Undefined variable %s", (it->Value).c_str());
					return false;
				}
			}
			if(it->Type != Types[CmdNum][j]) // Type checking for everything
				break;
			if(j == Cmd.Parameters.size() - 1) // Verified final parameter
				bFound = true;
		}
		if(bFound)
			break;
	}

	if(bFound) // Successful lookup
	{
		Cmd.Function = CmdNum;
		if(CmdNum == CMD_VAR) // Variable declaration, handled here explicitly
		{
			return AddUnitializedVariable(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value);
		}
		else
		{
			// Hack for preallocating just enough embedded pointers
			if(CmdNum == CMD_EMBSET || CmdNum == CMD_EMBWRITE)
			{
				int ptrcount = StringToUInt(Cmd.Parameters[0].Value);
				if(ptrcount > MaxEmbPtr)
					MaxEmbPtr = ptrcount;
			}
			CurBlock.Commands.push_back(Cmd);
		}
		return true;
	}
	else
	{
		ostringstream ErrorStr;
		ErrorStr << "Invalid parameters " << "(";
		if(Cmd.Parameters.size() > 0)
			ErrorStr << TypeStrings[Cmd.Parameters.front().Type];
		for(unsigned int i = 1; i < Cmd.Parameters.size(); i++)
			ErrorStr  << "," << TypeStrings[Cmd.Parameters[i].Type];
		ErrorStr << ") for " << CmdStr;
		Logger.ReportError(CurrentLine, "%s", ErrorStr.str().c_str());
		return false;
	}
}

inline bool AtlasParser::AddUnitializedVariable(string& VarName, string& Type)
{
	StrTypeMapIt it = TypeMap.find(Type);
	if(it == TypeMap.end()) // not found
	{
		Logger.ReportError(CurrentLine, "Invalid VAR declaration of type %s", Type.c_str());
		return false;
	}
	else // Add to the variable map
	{
		VarMap->AddVar(VarName, 0, it->second);
		return true;
	}
}