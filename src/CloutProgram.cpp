#include <Windows.h>
#include <imgui.h>

#include "Clout.h"
#include "Helpers.h"
#include "Console.h"
#include "CloutProgram.h"


const char* szOperationName[] = {
"",					//CLOUT_OP_NULL			0
"ATC Tool Change",		//CLOUT_OP_ATC_TOOL_CHANGE	1
"Rapid To",			//CLOUT_OP_RAPID_TO			2
"Home X/Y",			//CLOUT_OP_HOME_XY			3
"Install Touch Probe",	//CLOUT_OP_INSTALL_PROBE		4
"Run Probe Operation",	//CLOUT_OP_PROBE_OP			5
"Run G Code File",		//CLOUT_OP_RUN_GCODE_FILE	6
"Run Custom G Code",	//CLOUT_OP_CUSTOM_GCODE		7
"Alert User"			//CLOUT_OP_ALERT_USER		8
};


//Handlers to convert to/from JSON

void to_json(json& j, const CloutProgram_Op Op)
{
}

void from_json(const json& j, CloutProgram_Op& Op)
{
	if (Op.iType == CLOUT_OP_RUN_GCODE_FILE)
	{
		CloutProgram_Op_Run_GCode_File Data;
		Data.iStartLineNum = j.value("Start", -1);
		Data.iLastLineNum = j.value("End", -1);
		Data.sFilename = j.value("Filename", "");

		Op.Data = Data;
	}
	else if (Op.iType == CLOUT_OP_RAPID_TO)
	{
		CloutProgram_Op_Rapid_To Data;
		Data.Coords.x = j.value("X", -99999999999);
		Data.Coords.y = j.value("Y", -99999999999);
		Data.Coords.z = j.value("Z", -99999999999);
		Data.fFeedrate = j.value("Feedrate", -9999999999);

		Op.Data = Data;
	}
	else if (Op.iType == CLOUT_OP_ATC_TOOL_CHANGE)
	{
		CloutProgram_Op_ATC_Tool_Change Data;
		Data.iNewTool = j.value("Tool", -99);

		Op.Data = Data;
	}
}


void to_json(json& j, const CloutProgram& C) {
}
void from_json(const json& j, CloutProgram& Prog)
{	
	Prog.iOpsCount = 0;

	for (auto& jOp : Prog.jData["Operations"]) //Loop through all the operations in the JSON
	{
		CloutProgram_Op &Op = Prog.Ops[Prog.iOpsCount]; //Reference to the current Op in our storage

		//Read the operation type
			jOp.at("Type").get_to(Op.iType);

		//Read the data
			jOp.get_to(Op);

		Prog.iOpsCount++;
	}
}




CloutProgram::CloutProgram()
{
	iOpsCount = 0;

	//Sample data
	jData["Name"] = "My Test Program";
	jData["File Version"] = {1, 0}; //1.0

	jData["Operations"] = 
	{
		{ {"Type", CLOUT_OP_RUN_GCODE_FILE},  {"End", 69}, {"Start", 3}, {"Filename", ".\\test.nc"} },
		{ {"Type", CLOUT_OP_ATC_TOOL_CHANGE}, {"Tool", -1} },
		{ {"Type", CLOUT_OP_INSTALL_PROBE} },
		{ {"Type", CLOUT_OP_RAPID_TO}, {"X", 40.0f}, {"Y", 140.0f}, {"Z", 10.0f} },
		{ {"Type", CLOUT_OP_PROBE_OP} , {"Probe Type", "Bore Center"}, {"Bore Diameter", 23.0f} }
	};
};

void CloutProgram::AddOperation(CloutProgram_Op* NewOp)
{
	//Operations[iOperationsCount] = NewOp;
	//iOperationsCount++;
}

void CloutProgram::MoveOperationUp(int iIdx)
{
	if (iIdx == 0)
		return;

	CloutProgram_Op op = Ops[iIdx-1];
	Ops[iIdx-1] = Ops[iIdx];
	Ops[iIdx] = op;
}

void CloutProgram::LoadFromFile(const char *szFilename)
{
}