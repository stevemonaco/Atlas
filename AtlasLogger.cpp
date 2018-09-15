#include "stdafx.h"
#include <cstdio>
#include <string>
#include <list>
#include <cstdarg>
#include "AtlasLogger.h"

using namespace std;

AtlasLogger Logger;

AtlasLogger::AtlasLogger()
{
	output = NULL;
	isLogging = true;
	Errors.clear();
}

AtlasLogger::~AtlasLogger()
{
	if(output != NULL && output != stdout)
		fclose(output);
}

void AtlasLogger::ReportError(unsigned int ScriptLine, const char* FormatStr ...)
{
	AtlasError Error;
	Error.Severity = FATALERROR;
	Error.LineNumber = ScriptLine;

	va_list arglist;
	va_start(arglist, FormatStr);
	int length = _vsnprintf(buf, BufSize, FormatStr, arglist);
	va_end(arglist);

	Error.Error.assign(buf, length);

	Errors.push_back(Error);
}

void AtlasLogger::ReportWarning(unsigned int ScriptLine, const char* FormatStr ...)
{
	AtlasError Error;
	Error.Severity = WARNING;
	Error.LineNumber = ScriptLine;

	va_list arglist;
	va_start(arglist, FormatStr);
	int length = _vsnprintf(buf, BufSize, FormatStr, arglist);
	va_end(arglist);

	Error.Error.assign(buf, length);

	Errors.push_back(Error);
}

void AtlasLogger::Log(const char* FormatStr ...)
{
	if(isLogging && output)
	{
		va_list arglist;
		va_start(arglist, FormatStr);
		vfprintf(output, FormatStr, arglist);
		va_end(arglist);
	}
}

void AtlasLogger::SetLogStatus(bool LoggingOn)
{
	isLogging = LoggingOn;
}

void AtlasLogger::SetLogSource(FILE* OutputSource)
{
	output = OutputSource;
}

void AtlasLogger::BugReportLine(unsigned int Line, const char* Filename, const char* Msg)
{
	fprintf(stderr, "Bug: %s Line %u in source file %s\n", Msg, Line, Filename);
}

void AtlasLogger::BugReport(unsigned int Line, const char* Filename, const char* FormatStr, ...)
{
	fprintf(stderr, "Bug: ");
	va_list arglist;
	va_start(arglist, FormatStr);
	vfprintf(stderr, FormatStr, arglist);
	va_end(arglist);
	fprintf(stderr, " Line %u in source file %s\n", Line, Filename);
}