//-----------------------------------------------------------------------------
// AtlasCore - A class to insert Atlas-type scripts
// By Steve Monaco (stevemonaco@hotmail.com)
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <string>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <map>
#include <string>
#include <sstream>
#include <ctime>
#include "Table.h"
#include "AtlasCore.h"
#include "AtlasTypes.h"
#include "GenericVariable.h"
#include "AtlasFile.h"
#include "Pointer.h"
#include "AtlasLogger.h"
#include "AtlasStats.h"
#include "PointerHandler.h"
#include "AtlasExtension.h"

using namespace std;

// Constructor

AtlasCore Atlas;
unsigned int CurrentLine = 1;
int bSwap = 0;
int StringAlign = 0;
int MaxEmbPtr = 0;

AtlasCore::AtlasCore() : PtrHandler(&VarMap), Parser(&VarMap), Extensions(&VarMap)
{
	CurrentLine = 1;
	HeaderSize = 0;
	IsInJmp = false;
}

// Destructor
AtlasCore::~AtlasCore()
{
}

bool AtlasCore::Insert(const char* RomFileName, const char* ScriptFileName)
{
	ifstream script;
	script.open(ScriptFileName, ios::in);
	if(!script.is_open())
	{
		printf("Unable to open script file '%s'\n", ScriptFileName);
		return false;
	}

	if(!File.OpenFileT(RomFileName))
	{
		printf("Unable to open target file '%s'\n", RomFileName);
		return false;
	}

	// Target and pointer files will initially be the same file
	if(!File.OpenFileP(RomFileName))
	{
		printf("Unable to open pointer file '%s'\n", RomFileName);
		return false;
	}

	// Parse file
	bool ParseSuccess = false;
	clock_t ParseStart = clock();
	ParseSuccess = Parser.ParseFile(script);
	clock_t ParseTime = clock() - ParseStart;

	PrintSummary("Parsing", ParseTime);

	if(!ParseSuccess)
		return false;

	EmbPtrs.SetListSize(MaxEmbPtr+1);

	// Insert file
	clock_t InsertionStart = clock();

	for(ListBlockIt Block = Parser.Blocks.begin(); Block != Parser.Blocks.end(); Block++)
	{
		File.FlushText();

		// Execute list of commands
		for(ListCmdIt com = Block->Commands.begin(); com != Block->Commands.end(); com++)
		{
			CurrentLine = com->Line;
			if(!ExecuteCommand(*com))
				goto InsertionSummary;
		}

		if(Block->StartLine != -1)
			CurrentLine = Block->StartLine;

		// Insert text strings
		for(ListStringIt text = Block->TextLines.begin(); text != Block->TextLines.end(); text++)
		{
			if(!text->empty())
			{
				if(!IsInJmp)
				{
					Logger.ReportError(CurrentLine, "\"You must specify an address using JMP before inserting text\"");
					goto InsertionSummary;
				}
				if(!File.InsertText(*text, CurrentLine))
					goto InsertionSummary;
			}
			CurrentLine++;
		}
	}

	File.FlushText();

InsertionSummary:
	clock_t InsertionTime = clock() - InsertionStart;

	PrintSummary("Insertion", InsertionTime);

	Stats.End(CurrentLine); // Hack for the last line
	PrintStatistics();
	if(MaxEmbPtr != 0)
		PrintUnwrittenPointers();

	return true;
}

unsigned int AtlasCore::GetHeaderSize()
{
	return HeaderSize;
}

void AtlasCore::SetDebugging(FILE* output)
{
	if(output == NULL)
		Logger.SetLogStatus(false);
	else
		Logger.SetLogStatus(true);
	Logger.SetLogSource(output);
}

void AtlasCore::PrintStatistics()
{
	const char* Frame = "+------------------------------------------------------------------------------";

	if(Stats.Stats.size() == 0); // Do nothing
	else if(Stats.Stats.size() == 1)
	{
		PrintStatisticsBlock("Total", Stats.Stats.front());
	}
	else // Print each block's content
	{
		unsigned int blocknum = 1;
		char buf[127];
		for(ListStatsIt i = Stats.Stats.begin(); i != Stats.Stats.end(); i++)
		{
			_snprintf(buf, 127, "Block %d", blocknum);
			PrintStatisticsBlock(buf, *i);
			blocknum++;
		}

		Stats.GenerateTotalStats(Total);

		// Print out the total statistics
		printf("%s\n", Frame);
		printf("| Total Statistics\n");
		printf("| Script Size %u\n", Total.ScriptSize);
		printf("| Script Inserted %u\n", Total.ScriptSize - Total.ScriptOverflowed);
		if(Total.ScriptOverflowed != 0)
			printf("| Script Overflowed %u\n", Total.ScriptOverflowed);
		if(Total.MaxBound != -1)
			printf("| Space Remaining %u\n", Total.SpaceRemaining);
		printf("|\n");

		if(Total.HasCommands())
		{
			printf("| Command Execution Listing\n");
			for(int j = 0; j < CommandCount; j++)
			{
				if(Total.ExecCount[j] != 0)
					printf("| %s: %u\n", CommandStrings[j], Total.ExecCount[j]);
			}
		}

		if(Total.EmbPointerWrites > 0 || Total.AutoPointerWrites > 0 || 
			Total.FailedListWrites > 0 || Total.ExtPointerWrites > 0)
			printf("| Pointer Listing\n");
		if(Total.PointerWrites != 0)
			printf("| General Pointers Written: %u\n", Total.PointerWrites);
		if(Total.EmbPointerWrites != 0)
			printf("| Embedded Pointers Written: %u\n", Total.EmbPointerWrites);
		if(Total.AutoPointerWrites != 0)
			printf("| Autowrite Pointers Written: %u\n", Total.AutoPointerWrites);
		if(Total.FailedListWrites != 0)
			printf("| Failed PointerList Writes: %u\n", Total.FailedListWrites);
		if(Total.ExtPointerWrites != 0)
			printf("| Extension Pointer Writes: %u\n", Total.ExtPointerWrites);
		printf("%s\n\n", Frame);
	}
}

void AtlasCore::PrintStatisticsBlock(const char* Title, InsertionStatistics& Stats)
{
	const char* Frame = "+------------------------------------------------------------------------------";

	printf("%s\n", Frame);
	printf("| %s\n|   Start: Line %u  File Position $%X", Title, Stats.LineStart, Stats.StartPos);
	if(Stats.MaxBound != -1)
		printf("  Bound $%X", Stats.MaxBound);
	printf("\n");
	printf("|   End: Line %u  File Position $%X\n", Stats.LineEnd, Stats.StartPos + Stats.ScriptSize - Stats.ScriptOverflowed);
	printf("%s\n", Frame);
	printf("| Script size %u\n", Stats.ScriptSize);
	printf("| Bytes Inserted %u", Stats.ScriptSize - Stats.ScriptOverflowed);
	if(Stats.ScriptOverflowed != 0)
		printf("\n| Script Overflowed %u", Stats.ScriptOverflowed);
	if(Stats.MaxBound != -1)
		printf("\n| Space Remaining %u", Stats.SpaceRemaining);
	printf("\n|\n");

	if(Stats.HasCommands())
	{
		printf("| Command Execution Listing\n");
		for(int j = 0; j < CommandCount; j++)
		{
			if(Stats.ExecCount[j] != 0)
				printf("|   %s: %u\n", CommandStrings[j], Stats.ExecCount[j]);
		}
		printf("|\n");
	}

	printf("| Pointer Listing\n");
	printf("|   General Pointers Written: %u\n", Stats.PointerWrites);
	if(Stats.EmbPointerWrites != 0)
		printf("|   Embedded Pointers Written: %u\n", Stats.EmbPointerWrites);
	if(Stats.AutoPointerWrites != 0)
		printf("|   Autowrite Pointers Written: %u\n", Stats.AutoPointerWrites);
	if(Stats.FailedListWrites != 0)
		printf("|   Failed PointerList Writes: %u\n", Stats.FailedListWrites);
	if(Stats.ExtPointerWrites != 0)
		printf("|   Extension Pointer Writes: %u\n", Stats.ExtPointerWrites);
	printf("%s\n\n", Frame);
}

void AtlasCore::PrintSummary(const char* Title, unsigned int TimeCompleted)
{
	unsigned int SumErrors = 0;
	unsigned int SumWarnings = 0;
	printf("%s summary: %u msecs\n", Title, TimeCompleted);

	// Print errors and warnings
	for(ListErrorIt i = Logger.Errors.begin(); i != Logger.Errors.end(); i++)
	{
		if(i->Severity == FATALERROR)
		{
			printf("Error: ");
			SumErrors++;
		}
		else if(i->Severity == WARNING)
		{
			printf("Warning: ");
			SumWarnings++;
		}
		printf("%s on line %d\n", i->Error.c_str(), i->LineNumber);
	}
	Logger.Errors.clear();
	printf("%s - %u error(s), %u warning(s)\n\n", Title, SumErrors, SumWarnings);
}

void AtlasCore::PrintUnwrittenPointers()
{
	const char* Frame = "+------------------------------------------------------------------------------";

	printf("%s\n", Frame);
	printf("Printing Initialized But Unwritten Embedded Pointers Summary\n");
	unsigned int TextPos, PointerPos, count = 0;

	for(int i = 0; i < EmbPtrs.GetListSize(); i++)
	{
		if(!EmbPtrs.GetPointerState(i, TextPos, PointerPos)) // Pointer is uninit'd or not fully written
		{
			if(TextPos == -1 && PointerPos == -1) // Uninitialized
				continue;
			printf("| EmbPtr $%X EMBWRITE $%08X EMBSET $%08X\n", i, TextPos, PointerPos);
			count++;
		}
	}
	printf("|\n| %d Pointer(s) Detected\n", count);
	printf("%s\n\n", Frame);
}

bool AtlasCore::ExecuteCommand(Command& Cmd)
{
	static unsigned int PtrValue;
	static unsigned int PtrNum;
	static unsigned int PtrPos;
	static unsigned int Size;
	static bool Success;
	static unsigned char PtrByte;
	static unsigned int StartPos;
	static PointerList* List = NULL;
	static PointerTable* Tbl = NULL;
	static AtlasContext* Context = NULL;
	static AtlasExtension* Ext = NULL;
	string FuncName;

	if(IsInJmp && Cmd.Function != CMD_JMP1 && Cmd.Function != CMD_JMP2)
		Stats.AddCmd(Cmd.Function);
	else
		Total.AddCmd(Cmd.Function);

	switch(Cmd.Function)
	{
	case CMD_JMP1:
		File.MoveT(StringToUInt(Cmd.Parameters[0].Value), -1);
		Stats.NewStatsBlock(File.GetPosT(), -1, Cmd.Line);
		Logger.Log("%6u JMP       ROM Position is now $%X\n", Cmd.Line, StringToUInt(Cmd.Parameters[0].Value));
		IsInJmp = true;
		return true;
	case CMD_JMP2:
		File.MoveT(StringToUInt(Cmd.Parameters[0].Value), StringToUInt(Cmd.Parameters[1].Value));
		Stats.NewStatsBlock(File.GetPosT(), StringToUInt(Cmd.Parameters[1].Value), Cmd.Line);
		Logger.Log("%6u JMP       ROM Position is now $%X with max bound of $%X\n",
			Cmd.Line, StringToUInt(Cmd.Parameters[0].Value), StringToUInt(Cmd.Parameters[1].Value));
		IsInJmp = true;
		return true;
	case CMD_SMA:
		Success = DefaultPointer.SetAddressType(Cmd.Parameters[0].Value);
		if(Success)
			Logger.Log("%6u SMA       Addressing type is now '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_HDR:
		unsigned int Size;
		Size = StringToUInt(Cmd.Parameters[0].Value);
		EmbPtrs.SetHeaderSize(Size);
		DefaultPointer.SetHeaderSize(Size);
		HeaderSize = Size;
		Logger.Log("%6u HDR       Header size is now $%X\n", Cmd.Line, StringToUInt(Cmd.Parameters[0].Value));
		return true;
	case CMD_STRTYPE:
		Success = File.SetStringType(Cmd.Parameters[0].Value);
		if(Success)
			Logger.Log("%6u STRTYPE   String type is now '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_ADDTBL:
		Success = AddTable(Cmd);
		if(Success)
			Logger.Log("%6u ADDTBL    Added table '%s' as '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(), Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_ACTIVETBL:
		Success = ActivateTable(Cmd.Parameters[0].Value);
		if(Success)
			Logger.Log("%6u ACTIVETBL Active table is now '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_VAR: // Already handled by AtlasParser to validate types, should never get here
		return true;
	case CMD_WUB:
		PtrValue = DefaultPointer.GetUpperByte(File.GetPosT());
		File.WriteP(&PtrValue, 1, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u WUB       ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_WBB:
		PtrValue = DefaultPointer.GetBankByte(File.GetPosT());
		File.WriteP(&PtrValue, 1, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u WBB       ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_WHB:
		PtrValue = DefaultPointer.GetHighByte(File.GetPosT());
		File.WriteP(&PtrValue, 1, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u WHB       ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_WLB:
		PtrValue = DefaultPointer.GetLowByte(File.GetPosT());
		File.WriteP(&PtrValue, 1, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u WLB       ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_W16:
		PtrValue = DefaultPointer.Get16BitPointer(File.GetPosT());
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, 2);
		File.WriteP(&PtrValue, 2, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u W16       ScriptPos $%X PointerPos $%X PointerValue $%04X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_W24:
		PtrValue = DefaultPointer.Get24BitPointer(File.GetPosT());
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, 3);
		File.WriteP(&PtrValue, 3, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u W24       ScriptPos $%X PointerPos $%X PointerValue $%06X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_W32:
		PtrValue = DefaultPointer.Get32BitPointer(File.GetPosT());
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, 4);
		File.WriteP(&PtrValue, 4, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u W32       ScriptPos $%X PointerPos $%X PointerValue $%08\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_EMBSET:
		PtrNum = StringToUInt(Cmd.Parameters[0].Value);
		Success = EmbPtrs.SetPointerPosition(PtrNum, File.GetPosT());
		Size = EmbPtrs.GetSize(PtrNum);
		if(Size == -1)
			return false;
		Logger.Log("%6u EMBSET    Pointer Position %u set to $%X\n", Cmd.Line, PtrNum, File.GetPosT());
		if(Success) // Write out embedded pointer
		{
			PtrValue = EmbPtrs.GetPointerValue(PtrNum);
			if(File.GetMaxWritableBytes() > Size / 8)
			{
				if(bSwap)
					PtrValue = EndianSwap(PtrValue, Size/8);
				int tpos = File.GetPosT();
				File.WriteT(&PtrValue, Size / 8, 1); // Emb pointers are within script files
				Logger.Log("%6u EMBSET    Triggered Write: ScriptPos $%X PointerPos $%X PointerValue $%X Size %dd", Cmd.Line,
					EmbPtrs.GetTextPosition(PtrNum), tpos, PtrValue, Size);
				Stats.IncEmbPointerWrites();
			}
			else
				Logger.Log("%6u EMBSET    Failed to write due to insufficient space\n");
		}
		else // Reserve space so the embedded pointer and script don't compete
		{    // for the same part of the file
			if(File.GetMaxWritableBytes() > Size / 8)
			{
				unsigned int Zero = 0;
				File.WriteT(&Zero, Size/8, 1);
			}
			else
				Logger.Log("%6u EMBSET    Failed to write due to insufficient space\n");
		}
		return true;
	case CMD_EMBTYPE:
		Success = EmbPtrs.SetType(Cmd.Parameters[0].Value, StringToInt64(Cmd.Parameters[2].Value), StringToUInt(Cmd.Parameters[1].Value));
		if(!Success)
			Logger.ReportError(Cmd.Line, "Bad size %d for EMBTYPE",	StringToUInt(Cmd.Parameters[0].Value));
		else
			Logger.Log("%6u EMBTYPE   Embedded Pointer size %u Offsetting %I64d\n", Cmd.Line, StringToUInt(Cmd.Parameters[1].Value), StringToInt64(Cmd.Parameters[0].Value));
		return Success;
	case CMD_EMBWRITE:
		PtrNum = StringToUInt(Cmd.Parameters[0].Value);
		Success = EmbPtrs.SetTextPosition(PtrNum, File.GetPosT());
		Size = EmbPtrs.GetSize(PtrNum);
		if(Size == -1)
			return false;
		Logger.Log("%6u EMBWRITE  Pointed Position %u set to $%X\n", Cmd.Line, PtrNum, File.GetPosT());
		if(Success) // Write out embedded pointer
		{
			PtrPos = EmbPtrs.GetPointerPosition(PtrNum);
			PtrValue = EmbPtrs.GetPointerValue(PtrNum);
			if(File.GetMaxWritableBytes() > Size / 8)
			{
				if(bSwap)
					PtrValue = EndianSwap(PtrValue, Size/8);
				File.WriteT(&PtrValue, Size/8, 1, PtrPos);
				Logger.Log("%6u EMBWRITE  Triggered Write: ScriptPos $%X PointerPos $%X PointerValue $%X Size %dd\n", Cmd.Line,
					File.GetPosT(), PtrPos, PtrValue, Size);
				Stats.IncEmbPointerWrites();
			}
			else
				Logger.Log("%6u EMBWRITE  Failed to write due to insufficient space\n");
		}
		return true;
	case CMD_BREAK:
		return false;
	case CMD_PTRTBL:
		Success = PtrHandler.CreatePointerTable(Cmd.Parameters[0].Value, StringToUInt(Cmd.Parameters[1].Value), StringToUInt(Cmd.Parameters[2].Value), Cmd.Parameters[3].Value);
		if(Success)
			Logger.Log("%6u PTRTBL    Pointer Table '%s' created StartPos $%X Increment %dd CustomPointer '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				StringToUInt(Cmd.Parameters[1].Value), StringToUInt(Cmd.Parameters[2].Value), Cmd.Parameters[3].Value.c_str());
		return Success;
	case CMD_WRITETBL:
		PtrValue = PtrHandler.GetTableAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size, PtrPos);
		if(PtrValue == -1)
			return false;
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteP(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     PointerTable '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_PTRLIST:
		Success = PtrHandler.CreatePointerList(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value.c_str(), Cmd.Parameters[2].Value);
		if(Success)
			Logger.Log("%6u PTRTBL    Pointer List '%s' created from file '%s' CustomPointer '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				Cmd.Parameters[1].Value.c_str(), Cmd.Parameters[2].Value.c_str());
		return Success;
	case CMD_WRITELIST:
		PtrValue = PtrHandler.GetListAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size, PtrPos);
		if(PtrValue == -1)
			return false;
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteP(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     PointerList '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_AUTOWRITETBL:
		Tbl = (PointerTable*)VarMap.GetVar(Cmd.Parameters[0].Value)->GetData();
		if(Tbl == NULL)
		{
			Logger.ReportError(CurrentLine, "Identifier '%s' has not been initialized with PTRTBL", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		Success = File.AutoWrite(Tbl, Cmd.Parameters[1].Value);
		if(!Success)
			Logger.ReportError(CurrentLine, "'%s' has not been defined as an end token in the active table", Cmd.Parameters[1].Value.c_str());
		else
			Logger.Log("%6u AUTOWRITE EndTag '%s' PointerTable '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_AUTOWRITELIST:
		List = (PointerList*)VarMap.GetVar(Cmd.Parameters[0].Value)->GetData();
		if(List == NULL)
		{
			Logger.ReportError(CurrentLine, "Identifier '%s' has not been initialized with PTRLIST", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		Success = File.AutoWrite(List, Cmd.Parameters[1].Value);
		if(!Success)
			Logger.ReportError(CurrentLine, "'%s' has not been defined as an end token in the active table", Cmd.Parameters[1].Value.c_str());
		else
			Logger.Log("%6u AUTOWRITE EndTag '%s' PointerList '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_CREATEPTR:
		Success = PtrHandler.CreatePointer(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value,
			StringToInt64(Cmd.Parameters[2].Value), StringToUInt(Cmd.Parameters[3].Value), HeaderSize);
		if(Success)
			Logger.Log("%6u CREATEPTR CustomPointer '%s' Addressing '%s' Offsetting %I64d Size %dd HeaderSize $%X\n", Cmd.Line,
				Cmd.Parameters[0].Value.c_str(), Cmd.Parameters[1].Value.c_str(), StringToInt64(Cmd.Parameters[2].Value), StringToUInt(Cmd.Parameters[3].Value), HeaderSize);
		return Success;
	case CMD_WRITEPTR:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteP(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_LOADEXT:
		Success = Extensions.LoadExtension(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value);
		if(Success)
			Logger.Log("%6u LOADEXT   Loaded extension %s successfully\n", Cmd.Line, Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_EXECEXT:
		return ExecuteExtension(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value, &Context);
	case CMD_DISABLETABLE:
		Tbl = (PointerTable*)VarMap.GetVar(Cmd.Parameters[0].Value)->GetData();
		if(Tbl == NULL)
		{
			Logger.ReportError(CurrentLine, "Identifier '%s' has not been initialized with PTRTBL", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		Success = File.DisableWrite(Cmd.Parameters[1].Value, true);
		if(!Success)
			Logger.ReportError(CurrentLine, "'%s' has not been defined as an autowrite end token", Cmd.Parameters[1].Value.c_str());
		else
			Logger.Log("%6u DISABLE   EndTag '%s' PointerTable '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_DISABLELIST:
		List = (PointerList*)VarMap.GetVar(Cmd.Parameters[0].Value)->GetData();
		if(List == NULL)
		{
			Logger.ReportError(CurrentLine, "Identifier '%s' has not been initialized with PTRLIST", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		Success = File.DisableWrite(Cmd.Parameters[1].Value, false);
		if(!Success)
			Logger.ReportError(CurrentLine, "'%s' has not been defined as an autowrite end token", Cmd.Parameters[1].Value.c_str());
		else
			Logger.Log("%6u DISABLE   EndTag '%s' PointerList '%s'\n", Cmd.Line, Cmd.Parameters[1].Value.c_str(),
				Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_PASCALLEN:
		Success = File.SetPascalLength(StringToUInt(Cmd.Parameters[0].Value));
		if(Success)
			Logger.Log("%6u PASCALLEN Length for pascal strings set to %u\n", Cmd.Line, StringToUInt(Cmd.Parameters[0].Value));
		else
			Logger.ReportError(CurrentLine, "Invalid length %u for PASCALLEN", StringToUInt(Cmd.Parameters[0].Value));
		return Success;
	case CMD_AUTOEXEC:
		Ext = (AtlasExtension*)VarMap.GetVar(Cmd.Parameters[0].Value)->GetData();
		if(Ext == NULL)
		{
			Logger.ReportError(CurrentLine, "Identifier '%s' has not been initialized with LOADEXT", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		Success = File.AutoWrite(Ext, Cmd.Parameters[1].Value, Cmd.Parameters[2].Value);
		if(Success)
			Logger.Log("%6u AUTOEXEC  EndTag '%s' Extension '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
				Cmd.Parameters[1].Value.c_str());
		return Success;
	case CMD_DISABLEEXEC:
		Success = File.DisableAutoExtension(Cmd.Parameters[0].Value, Cmd.Parameters[1].Value);
		if(Success)
			Logger.Log("%6u DISABLE   EndTag '%s' Extension Function '%s'\n", Cmd.Line, Cmd.Parameters[1].Value.c_str(),
				Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_FIXEDLENGTH:
		Success = File.SetFixedLength(StringToUInt(Cmd.Parameters[0].Value), StringToUInt(Cmd.Parameters[1].Value));
		if(Success)
			Logger.Log("%6u FIXEDLENGTH Length %d PaddingValue %d\n", Cmd.Line, StringToUInt(Cmd.Parameters[0].Value),
				StringToUInt(Cmd.Parameters[0].Value));
		else
			Logger.ReportError(CurrentLine, "FixedLength used a padding value not in the range of 0-255");
		return Success;
	case CMD_WUBCUST:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		PtrByte = (PtrValue & 0xFF000000) >> 24;
		File.WriteP(&PtrByte, 1, 1, PtrPos);
		Logger.Log("%6u WUB       CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrByte);
		return true;
	case CMD_WBBCUST:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		PtrByte = (PtrValue & 0xFF0000) >> 16;
		File.WriteP(&PtrByte, 1, 1, PtrPos);
		Logger.Log("%6u WBB       CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrByte);
		return true;
	case CMD_WHBCUST:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		PtrByte = (PtrValue & 0xFF00) >> 8;
		File.WriteP(&PtrByte, 1, 1, PtrPos);
		Logger.Log("%6u WHB       CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrByte);
		return true;
	case CMD_WLBCUST:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		PtrByte = PtrValue & 0xFF;
		File.WriteP(&PtrByte, 1, 1, PtrPos);
		Logger.Log("%6u WLB       CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%02X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrByte);
		return true;
	case CMD_ENDIANSWAP:
		Success = SetEndianSwap(Cmd.Parameters[0].Value);
		if(Success)
			Logger.Log("%6u ENDIANSWAP '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_STRINGALIGN:
		StringAlign = StringToUInt(Cmd.Parameters[0].Value);
		Logger.Log("%6u STRINGALIGN '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return true;
	case CMD_EMBPTRTABLE:
		Success = PtrHandler.CreateEmbPointerTable(Cmd.Parameters[0].Value, File.GetPosT(), StringToUInt(Cmd.Parameters[1].Value), Cmd.Parameters[2].Value);
		StartPos = File.GetPosT();
		Size = PtrHandler.GetPtrSize(Cmd.Parameters[2].Value);
		if(Success)
		{
			int TableSize = (Size / 8) * StringToUInt(Cmd.Parameters[1].Value);
			if(File.GetMaxWritableBytes() == -1 || (int)File.GetMaxWritableBytes() > TableSize)
			{
				// Reserve space inside the script for the embedded pointer table
				unsigned char Zero = 0;
				for(int i = 0; i < TableSize; i++)
					File.WriteT(&Zero, 1, 1);

				Logger.Log("%6u EMBPTRTBL Pointer Table '%s' created StartPos $%X PtrCount %dd CustomPointer '%s'\n", Cmd.Line, Cmd.Parameters[0].Value.c_str(),
					StartPos, StringToUInt(Cmd.Parameters[1].Value), Cmd.Parameters[2].Value.c_str());
			}
			else
				Logger.Log("%6u EMBPTRTABLE Failed to allocate due to insufficient space within script\n", Cmd.Line);
		}
		return Success;
	case CMD_WHW:
		PtrValue = DefaultPointer.GetHighWord(File.GetPosT());
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, 2);
		File.WriteP(&PtrValue, 2, 1, StringToUInt(Cmd.Parameters[0].Value));
		Logger.Log("%6u WHW       ScriptPos $%X PointerPos $%X PointerValue $%04X\n", Cmd.Line,
			File.GetPosT(), StringToUInt(Cmd.Parameters[0].Value), PtrValue);
		return true;
	case CMD_WHWCUST:
		PtrValue = PtrHandler.GetPtrAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size);
		PtrPos = StringToUInt(Cmd.Parameters[1].Value);
		if(PtrValue == -1)
			return false;
		PtrValue = (PtrValue & 0xFFFF0000) >> 16;
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, 2);
		File.WriteP(&PtrByte, 2, 1, PtrPos);
		Logger.Log("%6u WHW       CustomPointer '%s' ScriptPos $%X PointerPos $%X PointerValue $%04X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_SETTARGETFILE:
		File.CloseFileT();
		Success = File.OpenFileT(Cmd.Parameters[0].Value.c_str());
		IsInJmp = false;
		if(!Success)
			Logger.ReportError(CurrentLine, "SETTARGETFILE Could not open file '%s'", Cmd.Parameters[0].Value.c_str());
		else
			Logger.Log("%6u SETTARGETFILE '%s'", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_SETPTRFILE:
		File.CloseFileP();
		Success = File.OpenFileP(Cmd.Parameters[0].Value.c_str());
		if(!Success)
			Logger.ReportError(CurrentLine, "SETPTRFILE Could not open file '%s'", Cmd.Parameters[0].Value.c_str());
		else
			Logger.Log("%6u SETPTRFILE '%s'", Cmd.Line, Cmd.Parameters[0].Value.c_str());
		return Success;
	case CMD_WRITEEMBTBL1:
		PtrValue = PtrHandler.GetEmbTableAddress(Cmd.Parameters[0].Value, File.GetPosT(), Size, PtrPos);
		if(PtrValue == -1)
		{
			Logger.ReportError(CurrentLine, "WRITE EMBPTRTBL '%s' could not write due to insufficient space'", Cmd.Parameters[0].Value.c_str());
			return false;
		}
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteT(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     PointerTable '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_WRITEEMBTBL2:
		PtrValue = PtrHandler.GetEmbTableAddress(Cmd.Parameters[0].Value, File.GetPosT(), StringToUInt(Cmd.Parameters[1].Value), Size, PtrPos);
		if(PtrValue == -1)
		{
			Logger.ReportError(CurrentLine, "WRITE EMBPTRTBL '%s' could not write PtrNum '%d' due to out of table range'", Cmd.Parameters[0].Value.c_str(), StringToUInt(Cmd.Parameters[1].Value));
			return false;
		}
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteT(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     PointerTable '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	case CMD_WRITETBL2:
		PtrValue = PtrHandler.GetTableAddress(Cmd.Parameters[0].Value, File.GetPosT(), StringToUInt(Cmd.Parameters[1].Value), Size, PtrPos);
		if(PtrValue == -1)
			return false;
		if(bSwap)
			PtrValue = EndianSwap(PtrValue, Size/8);
		File.WriteP(&PtrValue, Size/8, 1, PtrPos);
		Logger.Log("%6u WRITE     PointerTable '%s' ScriptPos $%X PointerPos $%X PointerValue $%08X\n", Cmd.Line,
			Cmd.Parameters[0].Value.c_str(), File.GetPosT(), PtrPos, PtrValue);
		return true;
	default:
		Logger.BugReport(__LINE__, __FILE__, "Bad Cmd #%u", Cmd.Function);
		return false;
	}
}

bool AtlasCore::ActivateTable(std::string& TableName)
{
	Table* Tbl = (Table*)(VarMap.GetVar(TableName))->GetData();
	if(Tbl == NULL)
	{
		ostringstream ErrorStr;
		ErrorStr << "Uninitialized variable " << TableName << " used";
		Logger.ReportError(CurrentLine, "Uninitialized variable '%s' used", TableName);
		return false;
	}
	else
	{
		File.SetTable(Tbl);
		return true;
	}
}

bool AtlasCore::AddTable(Command& Cmd)
{
	Table* Tbl;
	GenericVariable* Var;
	Tbl = (Table*)(VarMap.GetVar(Cmd.Parameters[1].Value))->GetData();
	if(!LoadTable(Cmd.Parameters[0].Value, &Tbl))
		return false;

	Var = new GenericVariable(Tbl, P_TABLE);
	VarMap.SetVar(Cmd.Parameters[1].Value, Var);
	return true;
}

bool AtlasCore::LoadTable(std::string& FileName, Table** Tbl)
{
	if(*Tbl != NULL) // Initialized already, overwrite
	{
		delete Tbl;
		Tbl = NULL;
	}
	*Tbl = new Table;
    int Result = (*Tbl)->OpenTable(FileName.c_str());

	ostringstream ErrorStr;
	switch(Result)
	{
	case TBL_OK:
		break;
	case TBL_PARSE_ERROR:
		Logger.ReportError(CurrentLine, "The table file '%s' is incorrectly formatted", FileName.c_str());
		return false;
	case TBL_OPEN_ERROR:
		Logger.ReportError(CurrentLine, "The table file '%s' could not be opened", FileName.c_str());
		return false;
	}

	return true;
}


void AtlasCore::CreateContext(AtlasContext** Context)
{
	if(*Context == NULL)
		*Context = new AtlasContext;
	(*Context)->CurrentLine = CurrentLine;
	(*Context)->ScriptPos = File.GetPosT();
	(*Context)->ScriptRemaining = File.GetMaxWritableBytes();
	(*Context)->Target = File.GetFileT();
	File.GetScriptBuf((*Context)->StringTable);
	(*Context)->PointerPosition = 0;
	(*Context)->PointerSize = 0;
	(*Context)->PointerValue = 0;
}


bool AtlasCore::ExecuteExtension(std::string& ExtId, std::string& FunctionName,
										 AtlasContext** Context)
{
	bool Success = false;
	CreateContext(Context);
	unsigned int DllRet = Extensions.ExecuteExtension(ExtId, FunctionName, Context);
	if(DllRet == -1)
	{
		DllRet = NO_ACTION;
		Success = false;
	}
	if(DllRet > MAX_RETURN_VAL)
	{
		Logger.ReportWarning(CurrentLine, "Extension returned invalid value %u", DllRet);
		Success = false;;
	}
	if(DllRet & REPLACE_TEXT)
	{
		File.SetScriptBuf((*Context)->StringTable);
		Logger.Log("%6u EXECEXT   REPLACE_TEXT\n", CurrentLine);
		Success = true;
	}
	if(DllRet & WRITE_POINTER)
	{
		unsigned int Size = (*Context)->PointerSize;
		if(Size == 8 || Size == 16 || Size == 24 || Size == 32)
		{
			Size /= 8;
			File.WriteP(&(*Context)->PointerValue, (*Context)->PointerSize, 1, (*Context)->PointerPosition);
			Logger.Log("%6u EXECEXT   WRITE_POINTER ScriptPos $%X PointerPos $%X PointerValue $%06X\n", CurrentLine,
				File.GetPosT(), (*Context)->PointerPosition, (*Context)->PointerValue);
			Success = true;
			Stats.IncExtPointerWrites();
		}
		else
		{
			Logger.ReportError(CurrentLine, "EXTEXEC   Extension function '%s' returning WRITE_POINTER has an unsupported PointerSize field", FunctionName);
			Success = false;
		}
	}

	delete (*Context);
	*Context = NULL;
	Logger.Log("%6u EXTEXEC   Executed function '%s' from '%s' successfully\n", CurrentLine, FunctionName.c_str(), ExtId.c_str());
	return true;
}

bool AtlasCore::ExecuteExtensionFunction(ExtensionFunction Func, AtlasContext** Context)
{
	unsigned int DllRet = Func(Context);
	bool Success = false;
	if(DllRet > MAX_RETURN_VAL)
	{
		Logger.ReportWarning(CurrentLine, "Extension returned invalid value %u", DllRet);
		Success = false;
	}

	if(DllRet & REPLACE_TEXT)
	{
		File.SetScriptBuf((*Context)->StringTable);
		Logger.Log("%6u EXECEXT   REPLACE_TEXT\n", CurrentLine);
		Success = true;
	}
	if(DllRet & WRITE_POINTER)
	{
		unsigned int Size = (*Context)->PointerSize;
		if(Size == 8 || Size == 16 || Size == 24 || Size == 32)
		{
			Size /= 8;
			File.WriteP(&(*Context)->PointerValue, (*Context)->PointerSize, 1, (*Context)->PointerPosition);
			Logger.Log("%6u EXTEXEC   WRITE_POINTER ScriptPos $%X PointerPos $%X PointerValue $%06X\n", CurrentLine,
				File.GetPosT(), (*Context)->PointerPosition, (*Context)->PointerValue);
			Success = true;
			Stats.IncExtPointerWrites();
		}
		else
		{
			Logger.ReportError(CurrentLine, "EXTEXEC   Extension function returning WRITE_POINTER has an unsupported PointerSize field");
			Success = false;
		}
	}

	delete (*Context);
	*Context = NULL;
	return true;
}

bool AtlasCore::SetEndianSwap(string& Swap)
{
	if(Swap == "TRUE")
		bSwap = 1;
	else if(Swap == "FALSE")
		bSwap = 0;
	else
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// StringToUInt() - Converts a $ABCD string from hexadecimal radix else decimal
// Status - Working
//-----------------------------------------------------------------------------

unsigned int StringToUInt(std::string& NumberString)
{
	unsigned int offset = 0;

	if(NumberString[0] == '$')
	{
		offset = strtoul(NumberString.substr(1, NumberString.length()).c_str(), NULL, 16);
	}
	else
		offset = strtoul(NumberString.c_str(), NULL, 10);

	return offset;
}

//-----------------------------------------------------------------------------
// StringToInt64() - Converts a string to int64
// Status - Works+Fixed
//-----------------------------------------------------------------------------

__int64 StringToInt64(string& NumberString)
{
	__int64 Num = 0;
	bool bNeg = false;
	size_t Pos = 0;
	unsigned __int64 Mult;

	if(NumberString[Pos] == '$') // hex
	{
		Pos++;
		if(NumberString[Pos] == '-')
		{
			bNeg = true;
			Pos++;
		}
		size_t i = NumberString.length() - 1;
		Num += GetHexDigit(NumberString[i]);
		i--;
		Mult = 16;
		for(i; i >= Pos; i--, Mult*=16)
			Num += Mult * GetHexDigit(NumberString[i]);
	}
	else // dec
	{
		if(NumberString[Pos] == '-')
		{
			bNeg = true;
			Pos++;
		}
		size_t i = NumberString.length() - 1;
		Num += GetHexDigit(NumberString[i]);
		if(i != 0)
		{
			i--;
			Mult = 10;
			for(i; i > Pos; i--, Mult*=10)
				Num += Mult * (NumberString[i] - '0');
			Num += Mult * (NumberString[i] - '0'); // prevent underflow of i
		}
	}

	if(bNeg)
		Num = -Num;
	return Num;
}

unsigned int GetHexDigit(char digit)
{
	switch(digit)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'A': case 'a':
		return 10;
	case 'B': case 'b':
		return 11;
	case 'C': case 'c':
		return 12;
	case 'D': case 'd':
		return 13;
	case 'E': case 'e':
		return 14;
	case 'F': case 'f':
		return 15;
	default:
		return 0;
	}
}

unsigned int EndianSwap(unsigned int Num, int Size)
{
	unsigned int a = 0;
	switch(Size)
	{
	case 1:
		return Num;
	case 2:
		a = (Num & 0xFF00) >> 8;
		a |= (Num & 0x00FF) << 8;
		return a;
	case 3:
		a = (Num & 0xFF) << 16;
		a |= (Num & 0xFF00);
		a |= (Num & 0xFF0000) >> 16;
		return a;
	case 4:
		a = (Num & 0xFF) << 24;
		a |= (Num & 0xFF00) << 8;
		a |= (Num & 0xFF0000) >> 8;
		a |= (Num & 0xFF000000) >> 24;
		return a;
	}

	return -1;
}