#pragma once
#include <list>
#include <string>
#include "AtlasTypes.h"

using namespace std;

class InsertionStatistics
{
public:
	InsertionStatistics();
	void AddStats(InsertionStatistics& Stats);
	void ClearStats();
	void Init(unsigned int StartPos, unsigned int UpperBound, unsigned int LineStart, const string& BlockName);
	void AddCmd(unsigned int CmdNum);
	bool HasCommands();

	InsertionStatistics& operator=(const InsertionStatistics& rhs);

	string Name;
	unsigned int StartPos;
	unsigned int ScriptSize;
	unsigned int ScriptOverflowed;
	unsigned int SpaceRemaining;
	unsigned int MaxBound;

	unsigned int LineStart;
	unsigned int LineEnd;

	unsigned int PointerWrites;
	unsigned int EmbPointerWrites;
	unsigned int AutoPointerWrites;
	unsigned int FailedListWrites;
	unsigned int ExtPointerWrites;

	unsigned int ExecCount[CommandCount];
};

class StatisticsHandler
{
public:
	StatisticsHandler();
	~StatisticsHandler();

	std::list<InsertionStatistics> Stats;

	void NewStatsBlock(unsigned int StartPos, unsigned int UpperBound, unsigned int LineStart, const string& BlockName);
	void AddCmd(unsigned int CmdNum);
	void AddScriptBytes(unsigned int Count);
	void End(unsigned int EndLine);
	void GenerateTotalStats(InsertionStatistics& Total);
	void IncGenPointerWrites();
	void IncEmbPointerWrites();
	void IncAutoPointerWrites();
	void IncFailedListWrites();
	void IncExtPointerWrites();

private:
	InsertionStatistics CurBlock;
};

typedef std::list<InsertionStatistics>::iterator ListStatsIt;
typedef std::list<InsertionStatistics>::reverse_iterator ListStatsRevIt;

extern StatisticsHandler Stats;