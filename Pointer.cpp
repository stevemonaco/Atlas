#include "stdafx.h"
#include <string>
#include "Pointer.h"
#include "AtlasLogger.h"

using namespace std;

Pointer::Pointer()
{
	AddressType = LINEAR;
	HeaderSize = 0;
}

Pointer::~Pointer()
{
}

bool Pointer::SetAddressType(string& Type)
{
	for(int i = 0; i < AddressTypeCount; i++)
	{
		if(Type == AddressTypes[i])
		{
			AddressType = i;
			return true;
		}
	}

	return false;
}

bool Pointer::SetAddressType(unsigned int Type)
{
	if(Type < AddressTypeCount)
	{
		AddressType = Type;
		return true;
	}
	else
		return false;
}

void Pointer::SetHeaderSize(const unsigned int Size)
{
	HeaderSize = Size;
}

unsigned int Pointer::GetAddress(const unsigned int Address) const
{
	return GetMachineAddress(Address);
}

unsigned int Pointer::GetMachineAddress(unsigned int Address) const
{
	Address -= HeaderSize;

	switch(AddressType)
	{
	case LINEAR:
		return Address;
	case LOROM00:
		return GetLoROMAddress(Address);
	case LOROM80:
		return GetLoROMAddress(Address) + 0x800000;
	case HIROM:
		return GetHiROMAddress(Address);
	case GB:
		return GetGBAddress(Address);
	default:
		return Address; // Error handling
	}
}

unsigned int Pointer::GetLoROMAddress(unsigned int Offset) const
{
	char bankbyte = (char) ((Offset & 0xFF0000) >> 16);
	unsigned short int Word = (unsigned short int) (Offset & 0xFFFF);
	unsigned int Address = 0;
	
	if(Word >= 0x8000)
		Address = bankbyte * 0x20000 + 0x10000 + Word;
	else
		Address = bankbyte * 0x20000 + Word + 0x8000;

	return Address;
}

unsigned int Pointer::GetHiROMAddress(unsigned int Offset) const
{
	unsigned int Address = 0;

	Address = Offset + 0xC00000;

	return Address;
}

unsigned int Pointer::GetGBAddress(unsigned int Offset) const
{
	unsigned int Address = 0;
	unsigned short int Bank = 0;
	unsigned short int Word = 0;

	Bank = Offset / 0x4000;
	Word = Offset % ((Bank+1) * 0x4000);

	Address = Bank * 0x10000 + Word;

	return Address;
}

unsigned char Pointer::GetUpperByte(const unsigned int ScriptPos) const
{
	return (GetAddress(ScriptPos) & 0xFF000000) >> 24;
}



// #WBB(param) - Working

unsigned char Pointer::GetBankByte(const unsigned int ScriptPos) const
{
	return (GetAddress(ScriptPos) & 0xFF0000) >> 16;
}



// #WHB(param) - Working

unsigned char Pointer::GetHighByte(const unsigned int ScriptPos) const
{
	return (GetAddress(ScriptPos) & 0xFF00) >> 8;
}



// #WLB(param) - Working

unsigned char Pointer::GetLowByte(const unsigned int ScriptPos) const
{
	return GetAddress(ScriptPos) & 0xFF;
}



// #W16(param) - Working

unsigned short Pointer::Get16BitPointer(const unsigned int ScriptPos) const
{
	return GetAddress(ScriptPos) & 0xFFFF;
}



// #W24(param) - Working

unsigned int Pointer::Get24BitPointer(const unsigned int ScriptPos) const
{
	return GetAddress(ScriptPos) & 0xFFFFFF;
}



// #W32 - Working

unsigned int Pointer::Get32BitPointer(const unsigned int ScriptPos) const
{
	return GetAddress(ScriptPos);
}

// #WHW (Write High Word) - Working

unsigned int Pointer::GetHighWord(const unsigned int ScriptPos) const
{
	return ((GetAddress(ScriptPos) & 0xFFFF0000) >> 16);
}

//--------------------------- Custom Pointer ----------------------------------
//                                                                           \\
//                                                                           \\

bool CustomPointer::Init(__int64 Offsetting, unsigned int Size, unsigned int HeaderSize)
{
	this->Offsetting = Offsetting;
	SetHeaderSize(HeaderSize);
	switch(Size)
	{
	case 8: case 16: case 24: case 32:
		this->Size = Size;
		break;
	default:
		return false;
	}
	return true;
}

unsigned int CustomPointer::GetSize()
{
	return Size;
}

unsigned int CustomPointer::GetAddress(const unsigned int Address) const
{
	unsigned int Val;
	Val = (unsigned int) ((__int64)GetMachineAddress(Address) - Offsetting);
	switch(Size)
	{
	case 8:
		return Val & 0xFF;
	case 16:
		return Val & 0xFFFF;
	case 24:
		return Val & 0xFFFFFF;
	case 32:
		return Val;
	default:
		Logger.BugReport(__LINE__, __FILE__, "Bad size in CustomPointer::GetAddress");
		return -1;
	}
}

//--------------------------- Embedded Pointer --------------------------------
//                                                                           \\
//                                                                           \\

EmbeddedPointer::EmbeddedPointer()
{
	TextPos = -1;
	PointerPos = -1;
	Size = 0;
}

EmbeddedPointer::~EmbeddedPointer()
{
}

bool EmbeddedPointer::SetPointerPosition(const unsigned int Address)
{
	PointerPos = Address;
	if(TextPos != -1)
		return true; // Return true if pointer is ready to write
	else
		return false;
}

bool EmbeddedPointer::SetTextPosition(const unsigned int Address)
{
	TextPos = Address;
	if(PointerPos != -1)
		return true;
	else
		return false;
}

void EmbeddedPointer::SetSize(const unsigned int size)
{
	Size = size;
}

unsigned int EmbeddedPointer::GetSize() const
{
	return Size;
}

void EmbeddedPointer::SetOffsetting(const __int64 Offsetting)
{
	this->Offsetting = Offsetting;
}

unsigned int EmbeddedPointer::GetPointer() const
{
	unsigned int Val = (unsigned int)(GetAddress(TextPos) - Offsetting);
	switch(Size)
	{
	case 8:
		return Val & 0xFF;
	case 16:
		return Val & 0xFFFF;
	case 24:
		return Val & 0xFFFFFF;
	case 32:
		return Val & 0xFFFFFFFF;
	default:
		Logger.BugReport(__LINE__, __FILE__, 
			"Bad embedded pointer size %d in EmbeddedPointer::GetTextPosition", Size);
		return 0;
	}
}

unsigned int EmbeddedPointer::GetTextPosition() const
{
	return TextPos;
}

unsigned int EmbeddedPointer::GetPointerPosition() const
{
	return PointerPos;
}

//------------------------ Embedded Pointer Handler ---------------------------
//                                                                           \\
//                                                                           \\

EmbeddedPointerHandler::EmbeddedPointerHandler()
{
	PtrSize = 0;
	HdrSize = 0;
	Offsetting = 0;
}

EmbeddedPointerHandler::~EmbeddedPointerHandler()
{
}

void EmbeddedPointerHandler::SetListSize(int Size)
{
	PtrList.reserve(Size);
	if((int)PtrList.size() < Size)
	{
		int j = Size - (int)PtrList.size();

		EmbeddedPointer elem;
		elem.SetAddressType(AddressType);
		elem.SetSize(PtrSize);
		elem.SetHeaderSize (HdrSize);
		elem.SetOffsetting(Offsetting);
		elem.SetPointerPosition(-1);
		elem.SetTextPosition(-1);

		for(int i = 0; i < j; i++)
			PtrList.push_back(elem);
	}
}

int EmbeddedPointerHandler::GetListSize()
{
	return (int)PtrList.size();
}

bool EmbeddedPointerHandler::GetPointerState(const unsigned int PointerNum, unsigned int& TextPos,
											 unsigned int& PointerPos)
{
	if(PtrList.size() < PointerNum)
	{
		TextPos = -1;
		PointerPos = -1;
		return false;
	}

	TextPos = GetTextPosition(PointerNum);
	PointerPos = GetPointerPosition(PointerNum);

	if(TextPos == -1 || PointerPos == -1)
		return false;
	else
		return true;
}

bool EmbeddedPointerHandler::SetType(std::string& AddressString, const __int64 Offsetting, const unsigned int PointerSize)
{
	this->Offsetting = Offsetting;
	switch(PointerSize)
	{
	case 8: case 16: case 24: case 32:
		PtrSize = PointerSize;
		break;
	default: // Bad size
		return false;
	}
	return SetAddressType(AddressString);
}

void EmbeddedPointerHandler::SetHeaderSize(const unsigned int HeaderSize)
{
	HdrSize = HeaderSize;
}

unsigned int EmbeddedPointerHandler::GetDefaultSize()
{
	return PtrSize;
}

unsigned int EmbeddedPointerHandler::GetSize(const unsigned int PointerNum)
{
	unsigned int i = PointerNum;
	if(PtrList.size() < i)
		return -1;

	return PtrList[i].GetSize();
}

bool EmbeddedPointerHandler::SetTextPosition(const unsigned int PointerNum, const unsigned int TextPos)
{
	unsigned int i = PointerNum;
	if(PtrList.size() < i)
		return false;

	// If still with default allocation
	if(PtrList[i].GetTextPosition() == -1 && PtrList[i].GetPointerPosition() == -1)
	{
		PtrList[i].SetAddressType(AddressType);
		PtrList[i].SetSize(PtrSize);
		PtrList[i].SetHeaderSize(HdrSize);
		PtrList[i].SetOffsetting(Offsetting);
	}

	return PtrList[i].SetTextPosition(TextPos);
}

bool EmbeddedPointerHandler::SetPointerPosition(const unsigned int PointerNum, const unsigned int PointerPos)
{
	unsigned int i = PointerNum;
	if(PtrList.size() < i)
		return false;

	// If still with default allocation
	if(PtrList[i].GetTextPosition() == -1 && PtrList[i].GetPointerPosition() == -1)
	{
		PtrList[i].SetAddressType(AddressType);
		PtrList[i].SetSize(PtrSize);
		PtrList[i].SetHeaderSize(HdrSize);
		PtrList[i].SetOffsetting(Offsetting);
	}

	return PtrList[i].SetPointerPosition(PointerPos);
}

unsigned int EmbeddedPointerHandler::GetPointerValue(const unsigned int PointerNum)
{
	if(PtrList.size() < PointerNum)
		return -1;

	return PtrList[PointerNum].GetPointer();
}

unsigned int EmbeddedPointerHandler::GetTextPosition(const unsigned int PointerNum)
{
	if(PtrList.size() < PointerNum)
		return -1;

	return PtrList[PointerNum].GetTextPosition();
}

unsigned int EmbeddedPointerHandler::GetPointerPosition(const unsigned int PointerNum)
{
	if(PtrList.size() < PointerNum)
		return -1;

	return PtrList[PointerNum].GetPointerPosition();
}

bool EmbeddedPointerHandler::SetAddressType(std::string& Type)
{
	for(int i = 0; i < AddressTypeCount; i++)
	{
		if(Type == AddressTypes[i])
		{
			AddressType = i;
			return true;
		}
	}

	return false;
}