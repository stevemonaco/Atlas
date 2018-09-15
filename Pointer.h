#pragma once
#include <list>

// MachineAddresses- The type of addressing the machine uses
static const unsigned int MA_INVALID = 0;
static const unsigned int LINEAR = 1;
static const unsigned int LOROM00 = 2;
static const unsigned int LOROM80 = 3;
static const unsigned int HIROM = 4;
static const unsigned int GB = 5;

static const unsigned int AddressTypeCount = 6;
static const char* AddressTypes[AddressTypeCount] = { "INVALID", "LINEAR", "LOROM00", "LOROM80",
	"HIROM", "GB" };

class Pointer
{
public:
	Pointer();
	~Pointer();
	bool SetAddressType(std::string& AddressString);
	bool SetAddressType(unsigned int Type);
	void SetHeaderSize(const unsigned int Size);

	// Pointer writing functions
	unsigned short Pointer::Get16BitPointer(const unsigned int ScriptPos) const;
	unsigned int Pointer::Get24BitPointer(const unsigned int ScriptPos) const;
	unsigned int Pointer::Get32BitPointer(const unsigned int ScriptPos) const;

	unsigned char Pointer::GetLowByte(const unsigned int ScriptPos) const;
	unsigned char Pointer::GetHighByte(const unsigned int ScriptPos) const;
	unsigned char Pointer::GetBankByte(const unsigned int ScriptPos) const;
	unsigned char Pointer::GetUpperByte(const unsigned int ScriptPos) const;
	unsigned int Pointer::GetHighWord(const unsigned int ScriptPos) const;

protected:
	unsigned int AddressType;
	unsigned int HeaderSize;
	virtual unsigned int GetAddress(const unsigned int Address) const;
	unsigned int GetMachineAddress(unsigned int Address) const;

private:
	// Machine Address translation functions
	unsigned int GetLoROMAddress(unsigned int Offset) const;
	unsigned int GetHiROMAddress(unsigned int Offset) const;
	unsigned int GetGBAddress(unsigned int Offset) const;
};

class CustomPointer : public virtual Pointer
{
public:
	bool Init(__int64 Offsetting, unsigned int Size, unsigned int HeaderSize);
	unsigned int GetSize();
	unsigned int GetAddress(const unsigned int Address) const;
private:
	__int64 Offsetting;
	unsigned int Size;
};

class EmbeddedPointer : public virtual Pointer
{
public:
	EmbeddedPointer();
	~EmbeddedPointer();

	bool SetTextPosition(const unsigned int Address);
	bool SetPointerPosition(const unsigned int Address);
	void SetSize(const unsigned int size);
	void SetOffsetting(const __int64 Offsetting);

	unsigned int GetTextPosition() const;
	unsigned int GetPointer() const;
	unsigned int GetPointerPosition() const;
	unsigned int GetSize() const;
private:
	__int64 Offsetting;
	unsigned int TextPos;
	unsigned int PointerPos;
	unsigned int Size;
};

typedef std::list<EmbeddedPointer>::iterator ListEmbPtrIt;

class EmbeddedPointerHandler
{
public:
	EmbeddedPointerHandler();
	~EmbeddedPointerHandler();

	void SetListSize(int Size);
	int GetListSize();
	bool GetPointerState(const unsigned int PointerNum, unsigned int& TextPos, unsigned int& PointerPos);
	bool SetType(std::string& AddressString, const __int64 Offsetting, const unsigned int PointerSize);
	unsigned int GetDefaultSize();
	bool SetTextPosition(const unsigned int PointerNum, const unsigned int TextPos);
	bool SetPointerPosition(const unsigned int PointerNum, const unsigned int PointerPos);
	void SetHeaderSize(const unsigned int HeaderSize);
	bool SetAddressType(std::string& Type);

	unsigned int GetTextPosition(const unsigned int PointerNum);
	unsigned int GetPointerPosition(const unsigned int PointerNum);
	unsigned int GetPointerValue(const unsigned int PointerNum);
	unsigned int GetSize(const unsigned int PointerNum);

private:
	std::vector<EmbeddedPointer> PtrList;
	unsigned int AddressType;
	__int64 Offsetting;
	unsigned int PtrSize;
	unsigned int HdrSize;
};