#pragma once
#include <map>
#include <string>
#include <vector>
#include "AtlasTypes.h"

using namespace std;

class GenericVariable
{
public:
	GenericVariable();
	GenericVariable(void* Data, unsigned int Type);
	~GenericVariable();

	unsigned int GetType();
	void* GetData();
	void SetData(void* Data, unsigned int Type);
	void SetData(void* Data);

private:
	bool Free();

	void* DataPointer;
	unsigned int DataType;
};

typedef std::map<std::string,GenericVariable*> VarMapType;
typedef std::map<std::string,GenericVariable*>::iterator VariableMapIt;
typedef std::map<std::string,GenericVariable*>::value_type VariableMapValue;

class VariableMap
{
public:
	VariableMap();
	~VariableMap();

	bool AddVar(std::string& Identifier, void* Data, unsigned int Type);
	bool Exists(std::string& Identifier, unsigned int Type);
	bool Exists(std::string& Identifier);
	GenericVariable* GetVar(std::string& Identifier);
	void SetVarData(std::string& Identifier, void* Data, unsigned int Type);
	void SetVar(std::string& Identifier, GenericVariable* Var);
	void* GetData(std::string& Identifier);
	unsigned int GetVarType(std::string& Identifier);

private:
	VarMapType VarMap;	// Maps strings to variables
	VariableMapIt VarMapIt; // Iterator for the map
};