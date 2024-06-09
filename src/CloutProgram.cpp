#include <imgui.h>

#include "Clout.h"
#include "Console.h"
#include "Probing/Probing.h"
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


  void CloutProgram_InstantiateNewOp(CloutProgram_Op_Datatypes &Op, int iOpType)
 {
	 if (iOpType == CLOUT_OP_ATC_TOOL_CHANGE)
		 Op = CloutProgram_Op_ATC_Tool_Change();
	 else if (iOpType == CLOUT_OP_RAPID_TO)
		 Op = CloutProgram_Op_RapidTo();
	 else if (iOpType == CLOUT_OP_INSTALL_PROBE)
		 Op = CloutProgram_Op_InstallTouchProbe();
	 else if (iOpType == CLOUT_OP_PROBE_OP)
		 Op = CloutProgram_Op_ProbeOp();
	 else if (iOpType == CLOUT_OP_RUN_GCODE_FILE)
		 Op = CloutProgram_Op_Run_GCode_File();
	 else if (iOpType == CLOUT_OP_CUSTOM_GCODE)
		 Op = CloutProgram_Op_Custom_GCode();
 }
 

//Handlers to convert to/from JSON

void to_json(json& j, const CloutProgram_Op Op)
{
}

void from_json(const json& j, CloutProgram_Op_Datatypes &refOp)
{
	OpBaseClass(refOp).ParseFromJSON(j);
}

void to_json(json& j, const CloutProgram& C) {
}
void from_json(const json& j, CloutProgram& Prog)
{	
	Prog.Ops.clear();

	for (auto& jOp : Prog.jData["Operations"]) //Loop through all the operations in the JSON
	{
		int iOpType;
		jOp.at("Type").get_to(iOpType);

		//Create the operation
			CloutProgram_Op_Datatypes NewOp;
			CloutProgram_InstantiateNewOp(NewOp, iOpType);	//This creates the new object with the appropriate datatype
			
		//Read the operation type
			jOp.at("Type").get_to(OpBaseClass(NewOp).iType);

		//Setup the title
			OpBaseClass(NewOp).FullText = szOperationName[OpBaseClass(NewOp).iType];

		//Read the data
			jOp.get_to(NewOp);

		//Add it to the program;
			Prog.AddOperation(NewOp);
	}
}


CloutProgram::CloutProgram()
{
	Ops.clear();

	//Sample data
	jData["Name"] = "My Test Program";
	jData["File Version"] = {1, 0}; //1.0

	jData["Operations"] = 
	{
		{ {"Type", CLOUT_OP_RUN_GCODE_FILE},  {"End", 69}, {"Start", 3}, {"Filename", ".\\test.nc"} },
		{ {"Type", CLOUT_OP_ATC_TOOL_CHANGE}, {"Tool", -1} },
		{ {"Type", CLOUT_OP_INSTALL_PROBE} },
		{ {"Type", CLOUT_OP_RAPID_TO}, {"X", 40.0f}, {"Y", 140.0f}, {"Z", 10.0f}, {"Use_X", true}, {"Use_Y", true}, {"Use_Z", true} },
		{ {"Type", CLOUT_OP_PROBE_OP} , {"Probe Op Type", "Bore Center"}, {"Bore Diameter", 23.0f} }
	};
};

void CloutProgram::AddOperation(CloutProgram_Op_Datatypes NewOp)
{
	Ops.push_back(NewOp);
}

void CloutProgram::MoveOperationUp(int iIdx)
{
	if (iIdx == 0)
		return;

//	CloutProgram_Op_Datatypes op = Ops[iIdx - 1];
//	Ops[iIdx-1] = Ops[iIdx];
//	Ops[iIdx] = op;
	std::swap(Ops[iIdx-1], Ops[iIdx]);
}

void CloutProgram::LoadFromFile(const char *szFilename)
{
}

void CloutProgram::Erase()
{
	Ops.clear();
}