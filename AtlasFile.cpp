#include "stdafx.h"
#include <cstdio>
#include <string>
#include "AtlasFile.h"
#include "Table.h"
#include "AtlasLogger.h"
#include "AtlasStats.h"
#include "AtlasCore.h"
#include "AtlasExtension.h"

using namespace std;

AtlasFile::AtlasFile()
{
	tfile = NULL;
	pfile = NULL;

	MaxScriptPos = -1;
	ActiveTbl = NULL;
	BytesInserted = 0;
	TotalBytes = 0;
	TotalBytesSkipped = 0;

	StrType = STR_ENDTERM;
	PascalLength = 1;

	FixedPadValue = 0;
	StringLength = 0;
}

AtlasFile::~AtlasFile()
{
	if(tfile != NULL)
		fclose(tfile);
	if(pfile != NULL)
		fclose(pfile);
}

bool AtlasFile::OpenFileT(const char* FileName)
{
	// Reset vars for new file
	MaxScriptPos = -1;

	tfile = fopen(FileName, "r+b");
	return tfile != NULL;
}

bool AtlasFile::OpenFileP(const char* Filename)
{
	pfile = fopen(Filename, "r+b");
	return pfile != NULL;
}

void AtlasFile::CloseFileT()
{
	if(tfile != NULL)
		fclose(tfile);
}

void AtlasFile::CloseFileP()
{
	if(pfile != NULL)
		fclose(pfile);
}

void AtlasFile::WriteP(const void* Data, const unsigned int Size, 
					  const unsigned int DataCount, const unsigned int Pos)
{
	unsigned int OldPos = ftell(pfile);
	fseek(pfile, Pos, SEEK_SET);
	fwrite(Data, Size, DataCount, pfile);
	fseek(pfile, OldPos, SEEK_SET);
}

void AtlasFile::WriteT(const void* Data, const unsigned int Size, 
					  const unsigned int DataCount, const unsigned int Pos)
{
	unsigned int OldPos = ftell(tfile);
	fseek(tfile, Pos, SEEK_SET);
	fwrite(Data, Size, DataCount, tfile);
	fseek(tfile, OldPos, SEEK_SET);
}

// Does not revert file offset
void AtlasFile::WriteT(const void* Data, const unsigned int Size, const unsigned int DataCount)
{
	fwrite(Data, Size, DataCount, tfile);
}

unsigned int AtlasFile::GetPosT()
{
	return ftell(tfile);
}

FILE* AtlasFile::GetFileT()
{
	return tfile;
}

FILE* AtlasFile::GetFileP()
{
	return pfile;
}

void AtlasFile::GetScriptBuf(std::list<TBL_STRING>& Strings)
{
	Strings = ActiveTbl->StringTable;
}

void AtlasFile::SetScriptBuf(std::list<TBL_STRING>& Strings)
{
	ActiveTbl->StringTable = Strings;
}

unsigned int AtlasFile::GetStringType()
{
	return StrType;
}

void AtlasFile::MoveT(const unsigned int Pos, const unsigned int ScriptBound)
{
	if(tfile)
		fseek(tfile, Pos, SEEK_SET);
	MaxScriptPos = ScriptBound;
}

void AtlasFile::MoveT(const unsigned int Pos)
{
	if(tfile)
		fseek(tfile, Pos, SEEK_SET);
}

void AtlasFile::SetTable(Table* Tbl)
{
	FlushText();
	ActiveTbl = Tbl;
}

bool AtlasFile::SetStringType(std::string& Type)
{
	for(int i = 0; i < StringTypeCount; i++)
	{
		if(Type == StringTypes[i])
		{
			StrType = i;
			return true;
		}
	}

	return false;
}

bool AtlasFile::SetPascalLength(unsigned int Length)
{
	switch(Length)
	{
	case 1: case 2: case 3: case 4:
		PascalLength = Length;
		break;
	default:
		return false;
	}
	return true;
}

bool AtlasFile::SetFixedLength(unsigned int StrLength, unsigned int PadValue)
{
	if(PadValue > 65536)
		return false;

	StringLength = StrLength;
	FixedPadValue = (unsigned char)PadValue;

	return true;
}

bool AtlasFile::DisableWrite(string& EndTag, bool isPointerTable)
{
	if(isPointerTable)
	{
		std::map<string, PointerTable*>::iterator it;
		it = TblAutoWrite.find(EndTag);
		if(it == TblAutoWrite.end())
			return false;
		TblAutoWrite.erase(it);
	}
	else
	{
		std::map<string, PointerList*>::iterator it;
		it = ListAutoWrite.find(EndTag);
		if(it == ListAutoWrite.end())
			return false;
		ListAutoWrite.erase(it);
	}
	return true;
}

bool AtlasFile::DisableAutoExtension(string& FuncName, string& EndTag)
{
	std::map<string, ExtensionFunction>::iterator it;
	it = ExtAutoWrite.find(EndTag);
	if(it == ExtAutoWrite.end())
	{
		Logger.ReportError(CurrentLine, "'%s' has not been defined as an autoexec end token", EndTag);
		return false;
	}
	ExtAutoWrite.erase(it);
	return true;
}

bool AtlasFile::AutoWrite(PointerList* List, string& EndTag)
{
	bool EndTokenFound = false;
	for(size_t i = 0; i < ActiveTbl->EndTokens.size(); i++)
	{
		if(EndTag == ActiveTbl->EndTokens[i])
			EndTokenFound = true;
	}
	if(EndTokenFound)
		ListAutoWrite.insert(map<string,PointerList*>::value_type(EndTag, List));
	return EndTokenFound;
}

bool AtlasFile::AutoWrite(PointerTable* Tbl, string& EndTag)
{
	bool EndTokenFound = false;
	for(size_t i = 0; i < ActiveTbl->EndTokens.size(); i++)
	{
		if(EndTag == ActiveTbl->EndTokens[i])
			EndTokenFound = true;
	}
	if(EndTokenFound)
		TblAutoWrite.insert(map<string,PointerTable*>::value_type(EndTag, Tbl));
	return EndTokenFound;
}

bool AtlasFile::AutoWrite(AtlasExtension* Ext, string& FuncName, string& EndTag)
{
	bool EndTokenFound = false;
	ExtensionFunction Func;

	for(size_t i = 0; i < ActiveTbl->EndTokens.size(); i++)
	{
		if(EndTag == ActiveTbl->EndTokens[i])
			EndTokenFound = true;
	}
	Func = Ext->GetFunction(FuncName);
	if(!EndTokenFound)
	{
		Logger.ReportError(CurrentLine, "'%s' has not been defined as an end token in the active table", EndTag);
		return false;
	}
	if(Func == NULL)
	{
		Logger.ReportError(CurrentLine, "Function 's' could not be found in the extension", FuncName);
		return false;
	}

	ExtAutoWrite.insert(map<string,ExtensionFunction>::value_type(EndTag, Func));
	return true;
}

bool AtlasFile::InsertText(string& Text, unsigned int Line)
{
	if(ActiveTbl == NULL)
	{
		// Add error
		printf("No active table loaded\n");
		return false;
	}
	unsigned int BadCharPos = 0;
	if(ActiveTbl->EncodeStream(Text, BadCharPos) == -1) // Failed insertion, char missing from tbl
	{
		// Add error
		Logger.ReportError(Line, "Character '%c' missing from table.  String = '%s'", Text[BadCharPos], Text.c_str());
		return false;
	}
	return true;
}

bool AtlasFile::FlushText()
{
	static unsigned int Size;
	static unsigned int Address;
	static unsigned int WritePos;
	AtlasContext* Context = NULL;

	if(ActiveTbl == NULL)
		return false;

	if(ActiveTbl->StringTable.empty())
		return true;

	// For every string, check autowrite/autoexec (list, table, and extension)
	// Automatically write pointer if appropriate end string is found, then write the text string to ROM
	ListTxtStringIt j = ActiveTbl->TxtStringTable.begin();
	for(ListTblStringIt i = ActiveTbl->StringTable.begin();
		i != ActiveTbl->StringTable.end(); i++, j++)
	{
		// #ALIGNSTRING, must do before autowrite writes a pointer
		AlignString();
		
		if(!i->EndToken.empty()) // If there's an end token, check for autowrite
		{
			ListIt = ListAutoWrite.find(i->EndToken);
			if(ListIt != ListAutoWrite.end())
			{
				Address = ListIt->second->GetAddress(GetPosT(), Size, WritePos);
				if(Address != -1)
				{
					if(bSwap)
						Address = EndianSwap(Address, Size/8);
					WriteP(&Address, Size/8, 1, WritePos);
					Logger.Log("%6u AUTOWRITE Invoked ScriptPos $%X PointerPos $%X PointerValue $%08X\n", CurrentLine,
						GetPosT(), WritePos, Address);
					Stats.IncAutoPointerWrites();
				}
				else
					Stats.IncFailedListWrites();
			}
			TblIt = TblAutoWrite.find(i->EndToken);
			if(TblIt != TblAutoWrite.end())
			{
				Address = TblIt->second->GetAddress(GetPosT(), Size, WritePos);
				if(bSwap)
					Address = EndianSwap(Address, Size/8);
				WriteP(&Address, Size/8, 1, WritePos);
				Logger.Log("%6u AUTOWRITE Invoked ScriptPos $%X PointerPos $%X PointerValue $%08X\n", CurrentLine,
					GetPosT(), WritePos, Address);
				Stats.IncAutoPointerWrites();
			}
			ExtIt = ExtAutoWrite.find(i->EndToken);
			if(ExtIt != ExtAutoWrite.end())
			{
				Atlas.CreateContext(&Context);
				bool Success = Atlas.ExecuteExtensionFunction(ExtIt->second, &Context);
				delete Context;
				Context = NULL;
				if(!Success)
				{
					Logger.ReportError(CurrentLine, "Autoexecuting extension with end token '%s' failed", i->EndToken);
					return false;
				}
				else
					Logger.Log("%6u AUTOEXEC  Invoked ScriptPos $%X PointerPos $%X PointerValue $%08X\n", CurrentLine,
						GetPosT(), WritePos, Address);
			}
		}

		CurTextString = j->Text;
		WriteString(i->Text);
		Logger.Log("%s\n", CurTextString.c_str());
		CurTextString.clear();
	}

	ActiveTbl->StringTable.clear();

	return true;
}

inline bool AtlasFile::WriteString(string& text)
{
	unsigned int StringSize = 0;
	int PadBytes;

	// Write string type
	if(StrType == STR_ENDTERM)
		StringSize = WriteNullString(text);
	else if(StrType == STR_PASCAL)
		StringSize = WritePascalString(text);
	else
		return false;

	// #FIXEDLENGTH padding
	if(StringLength != 0)
	{
		PadBytes = StringLength - StringSize;
		if(PadBytes > 0)
		{
			for(int i = 0; i < PadBytes; i++)
				fputc((int)FixedPadValue, tfile);
			BytesInserted += PadBytes;
		}
	}

	return true;
}

inline unsigned int AtlasFile::WriteNullString(string& text)
{
	unsigned int size = (unsigned int)text.length();
	unsigned int maxwrite = GetMaxWritableBytes();

	Stats.AddScriptBytes(size);

	// Truncate string if it overflows ROM bounds
	if(maxwrite < size)
	{
		int overflowbytes = size - maxwrite;
		TotalBytesSkipped += overflowbytes;
		size = maxwrite;
	}

	// Truncate string if it's too long for a fixed length string
	if(size > StringLength && StringLength != 0)
	{
		TotalBytesSkipped += (size - StringLength);
		size = StringLength;
		printf("Changed string length for %s to %d at %X\n", CurTextString.c_str(), StringLength, GetPosT());
	}

	fwrite(text.data(), 1, size, tfile);
	BytesInserted += size;

	return size;
}

inline unsigned int AtlasFile::WritePascalString(string& text)
{
	unsigned int size = (unsigned int)text.length();
	unsigned int maxwrite = GetMaxWritableBytes();

	Stats.AddScriptBytes(size+PascalLength);

	// Truncate string if it overflows ROM bounds
	if(PascalLength > maxwrite) // PascalLength doesn't even fit
		goto nowrite;
	if(maxwrite < size + PascalLength) // PascalLength and maybe partial string fits
	{
		int overflowbytes = (size+PascalLength) - maxwrite;
		TotalBytesSkipped += overflowbytes;
		size = maxwrite - PascalLength;
	}

	// Truncate string if it's too long for a fixed length string
	if(size > StringLength && StringLength != 0)
	{
		TotalBytesSkipped += (size - StringLength);
		size = StringLength - PascalLength;
		printf("Changed string length for %s to %d at %X\n", text.c_str(), StringLength, GetPosT());
	}

	int swaplen = size;
	if(bSwap)
		swaplen = EndianSwap(size, PascalLength);

	fwrite(&swaplen, PascalLength, 1, tfile);
	fwrite(text.c_str(), 1, size, tfile);
	BytesInserted += size+PascalLength;

nowrite:
	return size+PascalLength;
}

inline void AtlasFile::AlignString()
{
	if(StringAlign != 0) // String align turned on
	{
		int curoffset = GetPosT() - Atlas.GetHeaderSize();
		int PadBytes = StringAlign - (curoffset % StringAlign);
		if(PadBytes == StringAlign)
			PadBytes = 0;
		if(PadBytes > 0)
		{
			for(int i = 0; i < PadBytes; i++)
				fputc(0, tfile);
			BytesInserted += PadBytes;
		}
	}
}

inline unsigned int AtlasFile::GetMaxWritableBytes()
{
	if(MaxScriptPos == -1)
		return -1;
	unsigned int CurPos = ftell(tfile);
	if(CurPos > MaxScriptPos)
		return 0;
	return MaxScriptPos - CurPos + 1;
}