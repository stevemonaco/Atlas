#include "stdafx.h"
#include <string>
#include <map>
#include <vector>
#include "GenericVariable.h"
#include "Table.h"
#include "AtlasCore.h"
#include "AtlasExtension.h"
#include "Pointer.h"
#include "AtlasTypes.h"

using namespace std;

GenericVariable::GenericVariable(void* Data, unsigned int Type)
{
	DataType = Type;
	DataPointer = Data;
}

GenericVariable::GenericVariable()
{
	DataType = P_INVALID;
	DataPointer = NULL;
}

GenericVariable::~GenericVariable()
{
	Free();
}

void GenericVariable::SetData(void* Data, unsigned int Type)
{
	Free();
	DataType = Type;
	DataPointer = Data;
}

void* GenericVariable::GetData()
{
	return DataPointer;
}

unsigned int GenericVariable::GetType()
{
	return DataType;
}

bool GenericVariable::Free()
{
	if(DataPointer == NULL)
		return true;
	switch(DataType)
	{
	case P_INVALID:
		break;
	case P_STRING:
		delete (string*)DataPointer;
		break;
	case P_NUMBER:
		delete (__int64*)DataPointer;
		break;
	case P_DOUBLE:
		delete (double*)DataPointer;
		break;
	case P_TABLE:
		delete (Table*)DataPointer;
		break;
	case P_POINTERTABLE:
		delete (PointerTable*)DataPointer;
		break;
	case P_POINTERLIST:
		delete (PointerList*)DataPointer;
		break;
	case P_CUSTOMPOINTER:
		delete (CustomPointer*)DataPointer;
		break;
	case P_EXTENSION:
		delete (AtlasExtension*)DataPointer;
		break;
	default:
		return false;
	}

	return true;
}

VariableMap::VariableMap()
{
}

VariableMap::~VariableMap()
{
	for(VarMapIt = VarMap.begin(); VarMapIt != VarMap.end(); VarMapIt++)
		delete VarMapIt->second;
}

bool VariableMap::AddVar(string& Identifier, void* Data, unsigned int Type)
{
	VarMapIt = VarMap.find(string(Identifier));
	if(VarMapIt != VarMap.end()) // Already a variable under that Identifier
		return false;

	GenericVariable* CVar = new GenericVariable;
	CVar->SetData(Data, Type);
	VarMap[Identifier] = CVar;
	return true;
}

bool VariableMap::Exists(string& Identifier)
{
	VarMapIt = VarMap.find(Identifier);
	if(VarMapIt == VarMap.end()) // Not found
		return false;
	return true;
}

bool VariableMap::Exists(string& Identifier, unsigned int Type)
{
	VarMapIt = VarMap.find(Identifier);
	if(VarMapIt == VarMap.end()) // Identifier not found
		return false;
	if(VarMap[string(Identifier)]->GetType() != Type)
		return false;
	return true;
}

GenericVariable* VariableMap::GetVar(string& Identifier)
{
	VarMapIt = VarMap.find(Identifier);
	if(VarMapIt == VarMap.end())
		return NULL;
	else
		return VarMapIt->second;
}

void VariableMap::SetVar(string& Identifier, GenericVariable* Var)
{
	VarMapIt = VarMap.lower_bound(Identifier);
	if(VarMapIt != VarMap.end() && !(VarMap.key_comp()(Identifier, VarMapIt->first)))
	{
		delete VarMapIt->second;
		VarMapIt->second = Var;
	}
	else // Add variable
		VarMap.insert(VarMapIt, VariableMapValue(Identifier, Var));
}

void VariableMap::SetVarData(string& Identifier, void* Data, unsigned int Type)
{
	VarMapIt = VarMap.lower_bound(Identifier);
	if(VarMapIt != VarMap.end() && !(VarMap.key_comp()(Identifier, VarMapIt->first)))
		(VarMapIt->second)->SetData(Data, Type);
	else // Add variable
		VarMap.insert(VarMapIt, VariableMapValue(Identifier, new GenericVariable(Data, Type)));
}

unsigned int VariableMap::GetVarType(string& Identifier)
{
	if(!Exists(Identifier))
		return P_INVALID;
	return VarMap[string(Identifier)]->GetType();
}

void* VariableMap::GetData(string& Identifier)
{
	if(!Exists(Identifier))
		return NULL;
	return VarMap[Identifier]->GetData();
}