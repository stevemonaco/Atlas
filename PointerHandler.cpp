#include "stdafx.h"
#include <fstream>
#include <string>
#include "PointerHandler.h"
#include "Pointer.h"
#include "AtlasLogger.h"
#include "GenericVariable.h"
#include "AtlasCore.h"
#include "AtlasTypes.h"

using namespace std;

PointerHandler::PointerHandler(VariableMap* Map)
{
	this->Map = Map;
}

bool PointerHandler::CreatePointer(std::string& PtrId, std::string& AddressType,
		__int64 Offsetting,	unsigned int Size, unsigned int HeaderSize)
{
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr != NULL) // Already initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has already been allocated", PtrId.c_str());
		return false;
	}
	Ptr = new CustomPointer;
	if(!Ptr->Init(Offsetting, Size, HeaderSize))
	{
		Logger.ReportError(CurrentLine, "Invalid size parameter for CREATEPTR");
		return false;
	}
	if(!Ptr->SetAddressType(AddressType))
	{
		Logger.ReportError(CurrentLine, "Invalid address type for CREATEPTR");
		return false;
	}

	Map->SetVarData(PtrId, Ptr, P_CUSTOMPOINTER);
	return true;
}

unsigned int PointerHandler::GetPtrSize(string& PtrId)
{
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr == NULL) // Uninitialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with CREATEPTR", PtrId.c_str());
		return -1;
	}

	return Ptr->GetSize();
}

unsigned int PointerHandler::GetPtrAddress(std::string& PtrId, unsigned int ScriptPos,
								  unsigned int& Size)
{
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr == NULL) // Uninitialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with CREATEPTR", PtrId.c_str());
		return -1;
	}
	unsigned int Address = Ptr->GetAddress(ScriptPos);
	Size = Ptr->GetSize();
	return Address;
}

bool PointerHandler::CreatePointerList(std::string& ListId, const char* Filename,
									   std::string& PtrId)
{
	PointerList* List = (PointerList*)Map->GetVar(ListId)->GetData();
	if(List != NULL) // Already initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has already been allocated", ListId.c_str());
		return false;
	}
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr == NULL)
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with CREATEPTR", PtrId.c_str());
		return false;
	}

	List = new PointerList;
	bool Success = List->Create(Filename, *Ptr);
	Map->SetVarData(ListId, List, P_POINTERLIST);
	return Success;
}

unsigned int PointerHandler::GetListAddress(std::string& ListId, unsigned int ScriptPos, unsigned int& Size, unsigned int& WritePos)
{
	PointerList* List = (PointerList*)Map->GetVar(ListId)->GetData();
	if(List == NULL) // Not initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with PTRLIST", ListId.c_str());
		return -1;
	}
	return List->GetAddress(ScriptPos, Size, WritePos);
}

bool PointerHandler::CreatePointerTable(std::string& TblId, unsigned int Start,
		unsigned int Increment,	std::string& PtrId)
{
	PointerTable* Tbl = (PointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl != NULL) // Already allocated
	{
		Logger.ReportError(CurrentLine, "Identifier %s has already been allocated", TblId.c_str());
		return false;
	}
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr == NULL)
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with CREATEPTR", PtrId.c_str());
		return false;
	}
	Tbl = new PointerTable;
	Tbl->Create(Increment, Start, *Ptr);
	Map->SetVarData(TblId, Tbl, P_POINTERTABLE);
	return true;
}

bool PointerHandler::CreateEmbPointerTable(string& TblId, unsigned int Start, unsigned int PtrCount, string& PtrId)
{
	EmbPointerTable* Tbl = (EmbPointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl != NULL) // Already allocated
	{
		Logger.ReportError(CurrentLine, "Identifier %s has already been allocated", TblId.c_str());
		return false;
	}
	CustomPointer* Ptr = (CustomPointer*)Map->GetVar(PtrId)->GetData();
	if(Ptr == NULL)
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with CREATEPTR", PtrId.c_str());
		return false;
	}

	Tbl = new EmbPointerTable;
	Tbl->Create(Start, PtrCount, *Ptr);
	Map->SetVarData(TblId, Tbl, P_EMBPOINTERTABLE);
	return true;
}

unsigned int PointerHandler::GetEmbTableAddress(string& TblId, unsigned int ScriptPos, 
												unsigned int& Size, unsigned int& WritePos)
{
	EmbPointerTable* Tbl = (EmbPointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl == NULL) // Not initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with EMBPTRTBL", TblId.c_str());
		return -1;
	}
	return Tbl->GetAddress(ScriptPos, Size, WritePos);
}

unsigned int PointerHandler::GetEmbTableAddress(string& TblId, unsigned int ScriptPos, unsigned int PtrNum,
		unsigned int& Size, unsigned int& WritePos)
{
	EmbPointerTable* Tbl = (EmbPointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl == NULL) // Not initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with EMBPTRTBL", TblId.c_str());
		return -1;
	}
	return Tbl->GetAddress(ScriptPos, PtrNum, Size, WritePos);
}

unsigned int PointerHandler::GetTableAddress(std::string& TblId, unsigned int ScriptPos,
		unsigned int& Size,	unsigned int& WritePos)
{
	PointerTable* Tbl = (PointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl == NULL) // Not initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with PTRTBL", TblId.c_str());
		return -1;
	}
	return Tbl->GetAddress(ScriptPos, Size, WritePos);
}

unsigned int PointerHandler::GetTableAddress(std::string& TblId, unsigned int ScriptPos, unsigned int PtrNum,
		unsigned int& Size,	unsigned int& WritePos)
{
	PointerTable* Tbl = (PointerTable*)Map->GetVar(TblId)->GetData();
	if(Tbl == NULL) // Not initialized
	{
		Logger.ReportError(CurrentLine, "Identifier %s has not been initialized with PTRTBL", TblId.c_str());
		return -1;
	}
	return Tbl->GetAddress(ScriptPos, PtrNum, Size, WritePos);
}

PointerList::PointerList()
{
	Location = 0;
}

PointerList::~PointerList()
{
}

bool PointerList::Create(const char* Filename, CustomPointer& CustPointer)
{
	ifstream Input(Filename);
	if(!Input.is_open())
	{
		// File Error
		return false;
	}

	string Line;
	size_t FirstPos = 0;
	bool bRet = true;
	unsigned int Res = 0;

	unsigned int CurLine = 1;
	while(!Input.eof())
	{
		getline(Input, Line);
		FirstPos = Line.find_first_not_of(" \t", 0);

		if(FirstPos == string::npos) // Whitespace line
			continue;

		if(Line.length() > FirstPos+1)
			if(Line[FirstPos] == '/' && Line[FirstPos] == '/') // Comment
				continue;

		// Trim trailing whitespace
		size_t Last;
		for(Last = Line.length() - 1; Last > 0; Last--)
			if(Line[Last] != ' ' && Line[Last] != '\t')
				break;
		if(Last < Line.length())
			Line.erase(Last+1);

		if(Line[FirstPos] == '$')
		{
			FirstPos++;
			if(string::npos == Line.find_first_not_of("0123456789ABCDEF", FirstPos))
				Res = strtoul(Line.substr(FirstPos, Line.length() - FirstPos).c_str(), NULL, 16);
			else
			{
				Logger.ReportError(CurLine, "Error parsing %s in %s", Line.c_str(), Filename);
				bRet = false;
			}
		}
		else
		{
			if(string::npos == Line.find_first_not_of("0123456789", FirstPos))
				Res = strtoul(Line.substr(FirstPos, Line.length() - FirstPos).c_str(), NULL, 10);
			else
			{
				Logger.ReportError(CurLine, "Error parsing %s in %s", Line.c_str(), Filename);
				bRet = false;
			}
		}

		LocationList.push_back(Res);
		CurLine++;
	}

	Input.close();
	LocationIt = LocationList.begin();
	Location = 0;
	Pointer = CustPointer;
	return bRet;
}

unsigned int PointerList::GetAddress(unsigned int TextPosition, unsigned int& Size, unsigned int& WritePos)
{
	if(Location < LocationList.size())
	{
		Size = Pointer.GetSize();
		WritePos = *LocationIt;
		LocationIt++;
		Location++;
		return Pointer.GetAddress(TextPosition);
	}
	else
		return -1;
}



PointerTable::PointerTable()
{
	Increment = 0;
	CurOffset = 0;
}

PointerTable::~PointerTable()
{
}

bool PointerTable::Create(unsigned int Inc, unsigned int StartOffset, CustomPointer& CustPointer)
{
	Increment = Inc;
	CurOffset = StartOffset;
	Pointer = CustPointer;
	TableStart = StartOffset;
	return true;
}

unsigned int PointerTable::GetAddress(unsigned int TextPosition, unsigned int& Size, unsigned int& WritePos)
{
	Size = Pointer.GetSize();
	WritePos = CurOffset;
	CurOffset += Increment;
	return Pointer.GetAddress(TextPosition);
}

unsigned int PointerTable::GetAddress(unsigned int TextPosition, unsigned int PtrNum, 
									  unsigned int& Size, unsigned int& WritePos)
{
	Size = Pointer.GetSize();

	WritePos = TableStart + (PtrNum * Increment);
	CurOffset = WritePos + Increment;
	return Pointer.GetAddress(TextPosition);
}

EmbPointerTable::EmbPointerTable()
{
	TableStart = 0;
	CurPointer = 0;
	PtrCount = 0;
}

EmbPointerTable::~EmbPointerTable()
{
}

bool EmbPointerTable::Create(unsigned int StartOffset, unsigned int PointerCount, CustomPointer& CustPointer)
{
	TableStart = StartOffset;
	CurPointer = 0;
	PtrCount = PointerCount;
	Pointer = CustPointer;
	return true;
}

unsigned int EmbPointerTable::GetAddress(unsigned int TextPosition, unsigned int &Size, unsigned int &WritePos)
{
	if(CurPointer >= PtrCount)
		return -1;  // Out of bounds
	Size = Pointer.GetSize();

	WritePos = TableStart + CurPointer*(Size/8);
	CurPointer++;
	return Pointer.GetAddress(TextPosition);
}

unsigned int EmbPointerTable::GetAddress(unsigned int TextPosition, unsigned int PtrNum,
										 unsigned int &Size, unsigned int &WritePos)
{
	if(PtrNum >= PtrCount)
		return -1;  // Out of bounds

	Size = Pointer.GetSize();

	WritePos = TableStart + PtrNum * (Size/8);
	CurPointer = PtrNum+1;
	return Pointer.GetAddress(TextPosition);
}