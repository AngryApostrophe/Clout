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
		Data.iLineCount = 0;

		Op.Data = Data;
	}
	else if (Op.iType == CLOUT_OP_RAPID_TO)
	{
		CloutProgram_Op_RapidTo Data;
		Data.Coords.x = j.value("X", 0.0f);
		Data.Coords.y = j.value("Y", 0.0f);
		Data.Coords.z = j.value("Z", 0.0f);

		Data.bUseAxis[0] = j.value("Use_X", false);
		Data.bUseAxis[1] = j.value("Use_Y", false);
		Data.bUseAxis[2] = j.value("Use_Z", false);

		Data.bUseFeedrate = j.value("Supply_Feedrate", false);
		Data.fFeedrate = j.value("Feedrate", 300.0f);

		Data.bUseWCS = j.value("Supply_WCS", false);
		
		std::string WCS = j.value("WCS", "G54");

		if (WCS == "G53")
			Data.WCS = Carvera::CoordSystem::G53;
		else if (WCS == "G54")
			Data.WCS = Carvera::CoordSystem::G54;
		else if (WCS == "G55")
			Data.WCS = Carvera::CoordSystem::G55;
		else if (WCS == "G56")
			Data.WCS = Carvera::CoordSystem::G56;
		else if (WCS == "G57")
			Data.WCS = Carvera::CoordSystem::G57;
		else if (WCS == "G58")
			Data.WCS = Carvera::CoordSystem::G58;
		else if (WCS == "G59")
			Data.WCS = Carvera::CoordSystem::G59;

		Op.Data = Data;
	}
	else if (Op.iType == CLOUT_OP_ATC_TOOL_CHANGE)
	{
		CloutProgram_Op_ATC_Tool_Change Data;
		Data.iNewTool = j.value("Tool", -99);

		Op.Data = Data;
	}
	else if (Op.iType == CLOUT_OP_INSTALL_PROBE)
	{
		CloutProgram_Op_InstallTouchProbe Data;
		Data.bConfirmFunction = j.value("Confirm_Function", true);

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
		{ {"Type", CLOUT_OP_RAPID_TO}, {"X", 40.0f}, {"Y", 140.0f}, {"Z", 10.0f}, {"Use_X", true}, {"Use_Y", true}, {"Use_Z", true} },
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