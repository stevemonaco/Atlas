#pragma once

#include <string>
#include <list>

enum ErrorSeverity { FATALERROR = 0, WARNING };

typedef struct AtlasError
{
	std::string Error;
	ErrorSeverity Severity;
	unsigned int LineNumber;
} AtlasError;

typedef std::list<AtlasError>::iterator ListErrorIt;

class AtlasLogger
{
public:
	AtlasLogger();
	~AtlasLogger();

	void ReportError(unsigned int ScriptLine, const char* FormatStr ...);
	void ReportWarning(unsigned int ScriptLine, const char* FormatStr ...);
	void Log(const char* FormatStr ...);
	void SetLogSource(FILE* OutputSource);
	void SetLogStatus(bool LoggingOn);
	void BugReportLine(unsigned int Line, const char* Filename, const char* Msg);
	void BugReport(unsigned int Line, const char* Filename, const char* FormatStr ...);

	std::list<AtlasError> Errors;

private:
	FILE* output;
	static const unsigned int BufSize = 512;
	char buf[BufSize];
	bool isLogging;
};

extern AtlasLogger Logger;

#define ReportBug(msg) Logger.BugReport(__LINE__, __FILE__, msg)