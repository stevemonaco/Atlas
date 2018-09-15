#pragma once
#include <list>
#include <string>
#include "Pointer.h"
#include "GenericVariable.h"

using namespace std;

class PointerHandler
{
public:
	PointerHandler(VariableMap* Map);
	bool CreatePointer(string& PtrId, string& AddressType,
		__int64 Offsetting,	unsigned int Size, unsigned int HeaderSize);
	unsigned int GetPtrAddress(string& PtrId, unsigned int ScriptPos, unsigned int& Size);
	bool CreatePointerList(string& ListId, const char* Filename, string& PtrId);
	bool CreatePointerTable(string& TblId, unsigned int Start, unsigned int Increment,
		string& PtrId);
	bool CreateEmbPointerTable(string& TblId, unsigned int Start, unsigned int PtrCount, string& PtrId);
	unsigned int GetListAddress(string& ListId, unsigned int ScriptPos,
		unsigned int& Size, unsigned int& WritePos);
	unsigned int GetTableAddress(string& TblId, unsigned int ScriptPos,
		unsigned int& Size,	unsigned int& WritePos);
	unsigned int GetTableAddress(string& TblId, unsigned int ScriptPos,
		unsigned int PtrNum, unsigned int& Size, unsigned int& WritePos);

	unsigned int GetEmbTableAddress(string& TblId, unsigned int ScriptPos, unsigned int& Size, unsigned int& WritePos);
	unsigned int GetEmbTableAddress(string& TblId, unsigned int ScriptPos, unsigned int PtrNum,
		unsigned int& Size, unsigned int& WritePos);
	unsigned int GetPtrSize(string& PtrId);
private:
	VariableMap* Map;
};

class PointerList
{
public:
	PointerList();
	~PointerList();

	bool Create(const char* Filename, CustomPointer& CustPointer);
	unsigned int GetAddress(unsigned int TextPosition, unsigned int& Size, unsigned int& WritePos);
private:
	std::list<unsigned int> LocationList;
	std::list<unsigned int>::iterator LocationIt;
	unsigned int Location;
	CustomPointer Pointer;
};

class PointerTable
{
public:
	PointerTable();
	~PointerTable();
	bool Create(unsigned int Inc, unsigned int StartOffset, CustomPointer& CustPointer);
	unsigned int GetAddress(unsigned int TextPosition, unsigned int& Size, unsigned int& WritePos);
	unsigned int GetAddress(unsigned int TextPosition, unsigned int PtrNum, unsigned int& Size, unsigned int& WritePos);

private:
	unsigned int Increment;
	unsigned int CurOffset;
	unsigned int TableStart;
	CustomPointer Pointer;
};

class EmbPointerTable
{
public:
	EmbPointerTable();
	~EmbPointerTable();

	bool Create(unsigned int StartOffset, unsigned int PointerCount, CustomPointer& CustPointer);
	unsigned int GetAddress(unsigned int TextPosition, unsigned int& Size, unsigned int& WritePos);
	unsigned int GetAddress(unsigned int TextPosition, unsigned int PtrNum, 
		unsigned int &Size, unsigned int &WritePos);

private:
	unsigned int TableStart;
	unsigned int CurPointer;
	unsigned int PtrCount;
	CustomPointer Pointer;
};