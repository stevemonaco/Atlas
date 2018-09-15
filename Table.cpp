//-----------------------------------------------------------------------------
// Table - A table library by Klarth
// email - stevemonaco@hotmail.com
// Open source and free to use
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <map>
#include <utility>
#include <list>
#include "Table.h"

using namespace std;

const char* HexAlphaNum = "ABCDEFabcdef0123456789";

Table::Table()
{
	TblEntries = 0;
	memset(LongestText, 0, 256 * 4);
	LongestText[(int)'<'] = 5; // Length of <$XX>
	LongestText[(int)'('] = 5; // Length of ($XX)
	DefEndLine = "<LINE>";
	DefEndString = "<END>";
	LongestHex = 1;
	StringCount = 0;
	bAddEndToken = true;
}

Table::~Table()
{
	// Clear Errors
	if (!Errors.empty())
		Errors.clear();

	// Clear the map
	if (!LookupHex.empty())
		LookupHex.clear();
}



//-----------------------------------------------------------------------------
// EncodeStream() - Encodes text in a vector to the string tables
//-----------------------------------------------------------------------------

unsigned int Table::EncodeStream(string& scriptbuf, unsigned int& BadCharOffset)
{
	TBL_STRING TblString;
	TXT_STRING TxtString;
	string hexstr;
	string subtextstr;
	unsigned char i;
	unsigned int EncodedSize = 0;
	bool bIsEndToken = false;
	std::map<string, string>::iterator mapit;
	unsigned int BufOffset = 0;

	hexstr.reserve(LongestHex * 2);

	if (scriptbuf.empty())
		return 0;

	if (!StringTable.empty())
	{
		TBL_STRING RestoreStr = StringTable.back();
		if (RestoreStr.EndToken.empty()) // No end string...restore and keep adding
		{
			StringTable.pop_back();
			TblString.Text = RestoreStr.Text;
			TxtString = TxtStringTable.back();
			TxtStringTable.pop_back();
		}
	}

	while (BufOffset < scriptbuf.size()) // Translate the whole buffer
	{
		bIsEndToken = false;
		i = LongestText[(unsigned char)scriptbuf[BufOffset]]; // Use LUT
		while (i)
		{
			subtextstr = scriptbuf.substr(BufOffset, i);
			mapit = LookupHex.find(subtextstr);
			if (mapit == LookupHex.end()) // if the entry isn't found
			{
				i--;
				continue;
			}

			hexstr = mapit->second;
			TxtString.Text += subtextstr;

			// Search to see if it's an end token, if it is, add to the string table
			for (unsigned int j = 0; j < EndTokens.size(); j++)
				if (EndTokens[j] == subtextstr)
				{
					bIsEndToken = true;
					if (bAddEndToken)
						AddToTable(hexstr, &TblString);

					TxtString.EndToken = subtextstr;
					TblString.EndToken = subtextstr;
					EncodedSize += (unsigned int)TblString.Text.size();
					TxtStringTable.push_back(TxtString);
					StringTable.push_back(TblString);
					TxtString.EndToken = "";
					TxtString.Text.clear();
					TblString.EndToken = "";
					TblString.Text.clear();
					break; // Only once
				}

			if (!bIsEndToken)
				AddToTable(hexstr, &TblString);

			BufOffset += i;
			break; // Entry is finished
		}
		if (i == 0) // no entries found
		{
			BadCharOffset = BufOffset;
			return -1;
		}
	}

	// Encode any extra data that doesn't have an EndToken
	if (!TblString.Text.empty())
		StringTable.push_back(TblString);
	if (!TxtString.Text.empty())
		TxtStringTable.push_back(TxtString);

	EncodedSize += (unsigned int)TblString.Text.size();

	scriptbuf.clear();

	return EncodedSize;
}

inline void Table::InitHexTable()
{
	char textbuf[16];
	char hexbuf[16];

	for (unsigned int i = 0; i < 0x100; i++)
	{
		sprintf(textbuf, "<$%02X>", i);
		sprintf(hexbuf, "%02X", i);
		LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
		// WindHex style hex codes
		sprintf(textbuf, "($%02X)", i);
		LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
	}
	for (unsigned int i = 0x0A; i < 0x100; i += 0x10)
	{
		for (unsigned int j = 0; j < 6; j++)
		{
			sprintf(textbuf, "<$%02x>", i + j);
			sprintf(hexbuf, "%02X", i + j);
			LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
			// WindHex style hex codes (shouldn't be necessary for lowercase, though)
			sprintf(textbuf, "($%02x)", i);
			LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
		}
	}
}

//-----------------------------------------------------------------------------
// GetHexValue() - Returns a Hex value from a Text string from the table
//-----------------------------------------------------------------------------

inline string& Table::GetHexValue(string& Textstring)
{
	return (LookupHex.find(Textstring))->second;
}

//-----------------------------------------------------------------------------
// OpenTable() - Opens, Parses, and Loads a file to memory
//-----------------------------------------------------------------------------

int Table::OpenTable(const char* TableFilename)
{
	string HexVal;
	char testchar;
	string TextString;
	list<string> EntryList;
	string Entry;

	LineNumber = 0;
	LookupHex.clear();
	InitHexTable();

	ifstream TblFile(TableFilename);
	if (!TblFile.is_open()) // File can't be opened
		return TBL_OPEN_ERROR;

	unsigned char utfheader[4];
	// Detect UTF-8 header
	if (TblFile.peek() == 0xEF)
	{
		TblFile.read((char*)utfheader, 3);
		if (utfheader[0] != 0xEF || utfheader[1] != 0xBB || utfheader[2] != 0xBF)
			TblFile.seekg(ios::beg); // Seek beginning, not a UTF-8 header
	}

	// Read the file
	while (!TblFile.eof())
	{
		getline(TblFile, Entry);
		EntryList.push_back(Entry);
	}

	TblFile.close();

	// Parse each line
	for (list<string>::iterator i = EntryList.begin(); i != EntryList.end(); i++)
	{
		LineNumber++;

		if (i->length() == 0) // Blank line
			continue;

		testchar = (*i)[0];

		switch (testchar)
		{
			// Various bookmark and handakuten/dakuten/linked entries are skipped
		case '(': case '[': case '{': case '$': case '!': case '@':
			break;

		case '*': // End line
			if (!parseendline(*i))
				return TBL_PARSE_ERROR;
			break;

		case '/': // End string
			if (!parseendstring(*i))
				return TBL_PARSE_ERROR;
			break;

		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': // Normal entry value
			if (!parseentry(*i))
				return TBL_PARSE_ERROR;
			break;

		default:
			return TBL_PARSE_ERROR;
		}
	}

	return TBL_OK;
}

//-----------------------------------------------------------------------------
// parseendline() - parses a break line table value: ex, *FE
// You can also define messages like *FE=<End Text>
//-----------------------------------------------------------------------------

inline bool Table::parseendline(string line)
{
	line.erase(0, 1);
	size_t pos = line.find_first_not_of(HexAlphaNum, 0);
	string hexstr;
	string textstr;

	if (pos == string::npos) // Non-alphanum characters not found, *FE type entry?
	{
		textstr = DefEndLine;
		hexstr = line;
		LookupHex.insert(std::map<string, string>::value_type(textstr, hexstr));
		return true;
	}
	else
		hexstr = line.substr(0, pos);

	if ((hexstr.length() % 2) != 0) // Hex token length is not a multiple of 2
	{
		return false;
	}

	pos = line.find_first_of("=", 0);
	if (pos == string::npos) // No equal sign means it's a default entry
		textstr = DefEndLine;
	else
	{
		line.erase(0, pos + 1);
		textstr = line;
	}

	if (textstr.length() > LongestText[(unsigned char)textstr[0]])
		LongestText[(unsigned char)textstr[0]] = (int)textstr.length();

	LookupHex.insert(std::map<string, string>::value_type(textstr, hexstr));

	return true;
}



//-----------------------------------------------------------------------------
// parseendstring() - parses a string break table value
// Only ones like /FF=<end>
// and /<end> (gives a blank string value)
//-----------------------------------------------------------------------------

inline bool Table::parseendstring(string line)
{
	line.erase(0, 1);
	size_t pos = line.find_first_of(HexAlphaNum, 0);

	string hexstr = "";
	string textstr = "";

	if (pos != 0) // /<end> type entry
	{
		EndTokens.push_back(line);
		LookupHex.insert(std::map<string, string>::value_type(line, hexstr));
		return true;
	}

	pos = line.find_first_not_of(HexAlphaNum, 0);
	hexstr = line.substr(0, pos);

	if ((hexstr.length() % 2) != 0) // Hex token length is not a multiple of 2
	{
		return false;
	}

	pos = line.find_first_of("=", 0);
	if (pos == string::npos) // No "=" found, a /FF type entry
		textstr = DefEndString;
	else
	{
		line.erase(0, pos + 1);
		textstr = line;
		if (textstr.length() == 0) // Blank RHS
		{
			return false;
		}
	}

	if (textstr.length() > LongestText[(unsigned char)textstr[0]])
		LongestText[(unsigned char)textstr[0]] = (int)textstr.length();

	EndTokens.push_back(textstr);
	LookupHex.insert(std::map<string, string>::value_type(textstr, hexstr));

	return true;
}




//-----------------------------------------------------------------------------
// parseentry() - parses a hex = text line
//-----------------------------------------------------------------------------

inline bool Table::parseentry(string line)
{
	// Get location after hex string
	size_t pos = line.find_first_not_of(HexAlphaNum, 0);

	if (pos == string::npos) // String is all hex characters
	{
		return false;
	}

	string hexstr = line.substr(0, pos);

	if ((hexstr.length() % 2) != 0) // Hex string not a multiple of 2 length
	{
		return false;
	}

	string textstr;
	pos = line.find_first_of("=", 0);
	if (pos == line.length() - 1) // End of the line, blank entry means it's an error
	{
		return false;
	}
	else
	{
		line.erase(0, pos + 1);
		textstr = line;
	}

	if (textstr.length() > LongestText[(unsigned char)textstr[0]])
		LongestText[(unsigned char)textstr[0]] = (int)textstr.length();

	LookupHex.insert(std::map<string, string>::value_type(textstr, hexstr));

	return true;
}



//-----------------------------------------------------------------------------
// parsews() - Eats all blanks and eoln's until a valid character or eof
//-----------------------------------------------------------------------------

inline void Table::parsews(ifstream& file)
{
	char testch;
	do {
		file.get(testch);
		if (testch == '\n')
			LineNumber++;
	} while (((testch == SPACE) || (testch == '\n')) && (!file.eof()));

	if ((!file.eof()) || (testch != '\n'))
		file.seekg(-1, ios::cur);
}



//-----------------------------------------------------------------------------
// HexToDec(...) - Converts a hex character to its dec equiv
//-----------------------------------------------------------------------------

inline unsigned short HexToDec(char HexChar)
{
	switch (HexChar)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'a': case 'A': return 10;
	case 'b': case 'B': return 11;
	case 'c': case 'C': return 12;
	case 'd': case 'D': return 13;
	case 'e': case 'E': return 14;
	case 'f': case 'F': return 15;
	}

	// Never gets here
	printf("badstr");
	return 15;
}

inline void Table::AddToTable(string& Hexstring, TBL_STRING* TblStr)
{
	for (unsigned int k = 0; k < Hexstring.length(); k += 2)
		TblStr->Text += (HexToDec(Hexstring[k + 1]) | (HexToDec(Hexstring[k]) << 4));
}