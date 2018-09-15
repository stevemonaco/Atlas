#pragma once
#include <cstdio>
#include <string>
#include <map>
#include <iterator>
#include "Table.h"
#include "PointerHandler.h"
#include "AtlasExtension.h"
using namespace std;

static const unsigned int STR_ENDTERM = 0;
static const unsigned int STR_PASCAL = 1;
static const unsigned int StringTypeCount = 2;
static const char* StringTypes[StringTypeCount] = { "ENDTERM", "PASCAL" };

class AtlasFile
{
public:
	AtlasFile();
	~AtlasFile();

	bool AutoWrite(PointerList* List, string& EndTag);
	bool AutoWrite(PointerTable* Tbl, string& EndTag);
	bool AutoWrite(AtlasExtension* Ext, string& FuncName, string& EndTag);
	bool DisableAutoExtension(string& FuncName, string& EndTag);
	bool DisableWrite(string& EndTag, bool isPointerTable);

	// File functions.  T for text file, P for pointer file
	bool OpenFileT(const char* FileName);
	bool OpenFileP(const char* FileName);
	void CloseFileT();
	void CloseFileP();
	void MoveT(const unsigned int Pos, const unsigned int ScriptBound);
	void MoveT(const unsigned int Pos);
	void WriteP(const void* Data, const unsigned int Size, const unsigned int DataCount, const unsigned int Pos);
	void WriteT(const void* Data, const unsigned int Size, const unsigned int DataCount, const unsigned int Pos);
	void WriteT(const void* Data, const unsigned int Size, const unsigned int DataCount);
	unsigned int GetPosT();

	unsigned int GetMaxBound() { return MaxScriptPos; }
	unsigned int GetBytesInserted() { return BytesInserted; }
	unsigned int GetBytesOverflowed() { return TotalBytesSkipped; }

	void SetTable(Table* Tbl);
	bool SetStringType(string& Type);
	bool SetPascalLength(unsigned int Length);
	bool SetFixedLength(unsigned int StrLength, unsigned int PadValue);

	bool InsertText(string& Text, unsigned int Line);
	bool FlushText();

	inline unsigned int GetMaxWritableBytes();
	FILE* GetFileT();
	FILE* GetFileP();
	void GetScriptBuf(list<TBL_STRING>& Strings);
	void SetScriptBuf(list<TBL_STRING>& Strings);
	unsigned int GetStringType();

private:
	FILE* tfile; // Target file for script
	FILE* pfile; // Pointer write file
	Table* ActiveTbl;
	PointerHandler* PtrHandler;
	map<string, PointerList*> ListAutoWrite;
	map<string, PointerTable*> TblAutoWrite;
	map<string, ExtensionFunction> ExtAutoWrite;
	map<string, PointerList*>::iterator ListIt;
	map<string, PointerTable*>::iterator TblIt;
	map<string, ExtensionFunction>::iterator ExtIt;

	inline bool WriteString(string& text);
	inline unsigned int WriteNullString(string& text);
	inline unsigned int WritePascalString(string& text);
	inline void AlignString();

	unsigned int MaxScriptPos;
	unsigned int BytesInserted;
	unsigned int TotalBytesSkipped;
	unsigned int TotalBytes;

	unsigned int StrType;
	unsigned int PascalLength;

	unsigned int StringLength;
	unsigned char FixedPadValue;
	string CurTextString; // Used to keep track and report text that overflows the FIXEDLENGTH value
};