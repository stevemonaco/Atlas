#include "stdafx.h"
#include <list>
#include <cstdlib>
#include "AtlasTypes.h"
#include "AtlasLogger.h"
#include "AtlasStats.h"

StatisticsHandler Stats;

InsertionStatistics::InsertionStatistics()
{
	ClearStats();
}

void InsertionStatistics::Init(unsigned int StartPos, unsigned int UpperBound, unsigned int LineStart)
{
	ClearStats();
	this->StartPos = StartPos;
	this->LineStart = LineStart;
	if(UpperBound != -1)
		MaxBound = UpperBound;
}

void InsertionStatistics::AddStats(InsertionStatistics& Stats)
{
	ScriptSize += Stats.ScriptSize;
	ScriptOverflowed += Stats.ScriptOverflowed;
	SpaceRemaining += Stats.SpaceRemaining;
	PointerWrites += Stats.PointerWrites;
	EmbPointerWrites += Stats.EmbPointerWrites;
	AutoPointerWrites += Stats.AutoPointerWrites;
	FailedListWrites += Stats.FailedListWrites;
	ExtPointerWrites += Stats.ExtPointerWrites;

	for(int j = 0; j < CommandCount; j++)
		ExecCount[j] += Stats.ExecCount[j];
}

void InsertionStatistics::ClearStats()
{
	ScriptSize = 0;
	ScriptOverflowed = 0;
	SpaceRemaining = 0;
	PointerWrites = 0;
	EmbPointerWrites = 0;
	AutoPointerWrites = 0;
	FailedListWrites = 0;
	ExtPointerWrites = 0;

	StartPos = 0;
	MaxBound = -1;

	LineStart = 0;
	LineEnd = 0;
	memset(ExecCount, 0, 4 * CommandCount);
}

void InsertionStatistics::AddCmd(unsigned int CmdNum)
{
	ExecCount[CmdNum]++;
	switch(CmdNum)
	{
	case CMD_WUB: case CMD_WBB: case CMD_WHB: case CMD_WLB: case CMD_WHW:
	case CMD_W16: case CMD_W24:	case CMD_W32: case CMD_WRITEPTR:
	case CMD_WUBCUST: case CMD_WBBCUST: case CMD_WHBCUST: case CMD_WLBCUST:
	case CMD_WHWCUST:
		PointerWrites++;
		break;
	default:
		break;
	}
}

bool InsertionStatistics::HasCommands()
{
	for(unsigned int i = 0; i < CommandCount; i++)
		if(ExecCount[i] != 0)
			return true;

	return false;
}

InsertionStatistics& InsertionStatistics::operator=(const InsertionStatistics& rhs)
{
	if(this == &rhs)
		return *this;

	ScriptSize = rhs.ScriptSize;
	ScriptOverflowed = rhs.ScriptOverflowed;
	SpaceRemaining = rhs.SpaceRemaining;
	PointerWrites = rhs.PointerWrites;
	EmbPointerWrites = rhs.EmbPointerWrites;

	StartPos = rhs.StartPos;
	MaxBound = rhs.MaxBound;

	LineStart = rhs.LineStart;
	LineEnd = rhs.LineEnd;

	for(unsigned int i = 0; i < CommandCount; i++)
		ExecCount[i] = rhs.ExecCount[i];

	return *this;
}

StatisticsHandler::StatisticsHandler()
{
}

StatisticsHandler::~StatisticsHandler()
{
}

void StatisticsHandler::NewStatsBlock(unsigned int StartPos, unsigned int UpperBound, unsigned int LineStart)
{
	if(CurBlock.LineStart != 0) // If not first block
	{
		CurBlock.ScriptOverflowed = 0;
		CurBlock.SpaceRemaining = 0;

		if(CurBlock.MaxBound != -1) // if there is a MaxBound, calc overflow and remaining space
		{
			if(CurBlock.StartPos + CurBlock.ScriptSize > CurBlock.MaxBound+1)
				CurBlock.ScriptOverflowed = CurBlock.StartPos + CurBlock.ScriptSize - CurBlock.MaxBound;
			else
				CurBlock.ScriptOverflowed = 0;

			if(CurBlock.MaxBound+1 > (CurBlock.StartPos + CurBlock.ScriptSize))
				CurBlock.SpaceRemaining = CurBlock.MaxBound+1 - (CurBlock.StartPos + CurBlock.ScriptSize);
			else
				CurBlock.SpaceRemaining = 0;
		}
		CurBlock.LineEnd = LineStart;
		Stats.push_back(CurBlock);
	}

	CurBlock.Init(StartPos, UpperBound, LineStart);
}

void StatisticsHandler::AddCmd(unsigned int CmdNum)
{
	if(CmdNum < CommandCount)
		CurBlock.AddCmd(CmdNum);
}

void StatisticsHandler::IncGenPointerWrites()
{
	CurBlock.PointerWrites++;
}

void StatisticsHandler::IncEmbPointerWrites()
{
	CurBlock.EmbPointerWrites++;
}

void StatisticsHandler::IncAutoPointerWrites()
{
	CurBlock.AutoPointerWrites++;
}

void StatisticsHandler::IncFailedListWrites()
{
	CurBlock.FailedListWrites++;
}

void StatisticsHandler::IncExtPointerWrites()
{
	CurBlock.ExtPointerWrites++;
}

void StatisticsHandler::AddScriptBytes(unsigned int Count)
{
	CurBlock.ScriptSize += Count;
}

void StatisticsHandler::End(unsigned int EndLine)
{
	if(CurBlock.StartPos + CurBlock.ScriptSize > CurBlock.MaxBound)
		CurBlock.ScriptOverflowed = CurBlock.StartPos + CurBlock.ScriptSize - CurBlock.MaxBound;
	else
		CurBlock.ScriptOverflowed = 0;

	if(CurBlock.MaxBound != -1 && CurBlock.MaxBound > (CurBlock.StartPos + CurBlock.ScriptSize))
		CurBlock.SpaceRemaining = CurBlock.MaxBound - (CurBlock.StartPos + CurBlock.ScriptSize);
	else
		CurBlock.SpaceRemaining = 0;

	CurBlock.LineEnd = EndLine;
	Stats.push_back(CurBlock);
	CurBlock.ClearStats();
}

void StatisticsHandler::GenerateTotalStats(InsertionStatistics& Total)
{
	if(Stats.empty())
	{
		ReportBug("Invalid size for statistics list in StatisticsHandler::GenerateTotalStats");
		return;
	}
	else if(Stats.size() == 1)
	{
		Total = Stats.front();
		return;
	}

	for(ListStatsIt i = Stats.begin(); i != Stats.end(); i++)
		Total.AddStats(*i);
}