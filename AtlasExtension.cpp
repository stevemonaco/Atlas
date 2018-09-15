#include "stdafx.h"
#include <string>
#include <windows.h>
#include "AtlasExtension.h"
#include "AtlasLogger.h"
#include "AtlasCore.h"

using namespace std;

ExtensionManager::ExtensionManager(VariableMap* Map)
{
	VarMap = Map;
}

bool ExtensionManager::LoadExtension(string& ExtId, string& ExtensionFile)
{
	// Use file extension to pick which derived AtlasExtension to use
	AtlasExtension* Ext;
	size_t Pos = ExtensionFile.find_last_of('.');
	if(Pos == string::npos || Pos >= ExtensionFile.length()-1)
		return false;
	else if(ExtensionFile.substr(Pos+1, ExtensionFile.length()-1) == "dll") // dll
	{
	}
	else
	{
		Logger.ReportError(CurrentLine, "Unsupported file format used in LOADEXT");
		return false;
	}

	Ext = (AtlasExtension*)VarMap->GetVar(ExtId)->GetData();
	if(Ext != NULL)
	{
		Logger.ReportError(CurrentLine, "%s has alrady been initialized with LOADEXT", ExtId.c_str());
		delete Ext;
		return false;
	}
	Ext = NULL;
	Ext = new AtlasExtension;
	if(!Ext->LoadExtension(ExtensionFile))
	{
		Logger.ReportError(CurrentLine, "%s could not be loaded", ExtensionFile.c_str());
		delete Ext;
		return false;
	}

	VarMap->SetVarData(ExtId, Ext, P_EXTENSION);
	return true;
}

unsigned int ExtensionManager::ExecuteExtension(string& ExtId, string& FunctionName, AtlasContext** Context)
{
	AtlasExtension* Ext;
	Ext = (AtlasExtension*)VarMap->GetVar(ExtId)->GetData();
	if(Ext == NULL)
	{
		Logger.ReportError(CurrentLine, "%s has not been initialized by LOADEXT", ExtId.c_str());
		return -1;
	}
	if(!Ext->IsLoaded())
	{
		ReportBug("Extension not loaded but initialized in ExtensionManager::ExecuteExtension");
		return -1;
	}

	ExtensionFunction Func = Ext->GetFunction(FunctionName);
	if(Func == NULL)
	{
		Logger.ReportError(CurrentLine, "Function %s was not found in the extension file", FunctionName.c_str());
		return -1;
	}

	unsigned int Res = Func(Context);
	if(Res > MAX_RETURN_VAL)
	{
		Logger.ReportWarning(CurrentLine, "Extension returned invalid value %u", Res);
		Res = NO_ACTION;
	}

	return Res;
}

AtlasExtension::AtlasExtension() : Extension(NULL)
{
}

AtlasExtension::~AtlasExtension()
{
	if(Extension)
		FreeLibrary(Extension);
}

bool AtlasExtension::LoadExtension(string& ExtensionName)
{
	Extension = LoadLibraryA(ExtensionName.c_str());
	
	if(Extension)
		return true;
	else
		return false;
}

bool AtlasExtension::IsLoaded()
{
	if(Extension)
		return true;
	else
		return false;
}

ExtensionFunction AtlasExtension::GetFunction(string& FunctionName)
{
	if(NULL == Extension)
		return NULL;

	ExtensionFunction func = (ExtensionFunction)GetProcAddress(Extension, FunctionName.c_str());

	return func;
}