#include "Platforms/Platforms.h"

#include <imgui.h>

#include <iostream>
#include <fstream>

#include "Clout.h"
#include "Console.h"
#include "Probing/Probing.h"
#include "CloutProgram.h"


std::vector<const char*> strOperationName = {
"",						//CLOUT_OP_NULL				0
"ATC Tool Change",		//CLOUT_OP_ATC_TOOL_CHANGE	1
"Rapid To",				//CLOUT_OP_RAPID_TO			2
"Home X/Y",				//CLOUT_OP_HOME_XY			3
"Install Touch Probe",	//CLOUT_OP_INSTALL_PROBE	4
"Run Probe Operation",	//CLOUT_OP_PROBE_OP			5
"Run G Code File",		//CLOUT_OP_RUN_GCODE_FILE	6
"Run Custom G Code",	//CLOUT_OP_CUSTOM_GCODE		7
"Alert User",			//CLOUT_OP_ALERT_USER		8
"Open Collet",			//CLOUT_OP_OPEN_COLLET		9
"Close Collet"			//CLOUT_OP_CLOSE_COLLET		10
};


//These give us better organization of the available operations in the GUI
std::vector<std::string> strOpsSections = { "Tool", "Movement", "Probe", "G Code", "Misc" };
std::vector<std::vector<int>> OrganizedOps = {
//Tool
	{CLOUT_OP_ATC_TOOL_CHANGE, CLOUT_OP_OPEN_COLLET, CLOUT_OP_CLOSE_COLLET},
//Movement
	{CLOUT_OP_RAPID_TO, CLOUT_OP_HOME_XY},
//Probe
	{CLOUT_OP_INSTALL_PROBE, CLOUT_OP_PROBE_OP},
//G Code
	{CLOUT_OP_RUN_GCODE_FILE, CLOUT_OP_CUSTOM_GCODE},
//Misc
	{CLOUT_OP_ALERT_USER}
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
	 else if (iOpType == CLOUT_OP_OPEN_COLLET)
		 Op = CloutProgram_Op_OpenCollet();
	 else if (iOpType == CLOUT_OP_CLOSE_COLLET)
		 Op = CloutProgram_Op_CloseCollet();

	 //Setup the base stuff
		 OpBaseClass(Op).FullText = strOperationName[iOpType];
		 OpBaseClass(Op).iType = iOpType;
 }
 

//Handlers to convert to/from JSON

void to_json(json& j, CloutProgram_Op_Datatypes& refOp)
{
	OpBaseClass(refOp).ParseToJSON(j);
}

void from_json(const json& j, CloutProgram_Op_Datatypes &refOp)
{
	OpBaseClass(refOp).ParseFromJSON(j);
}

void to_json(json& j, CloutProgram& Prog)
{
}
void from_json(const json& j, CloutProgram& Prog)
{	
	Prog.Ops.clear();

	for (auto& jOp : Prog.jData["Operations"]) //Loop through all the operations in the JSON
	{
		//Read the type of operation
			int iOpType = CLOUT_OP_NULL;

			//Read in the string value
				std::string strType;
				jOp.at("Type").get_to(strType);
		
			//Look for that in our list
				for (int x = 0; x < strOperationName.size(); x++)
				{
					if (strType == strOperationName[x])
					{
						iOpType = x;
						break;
					}
				}

		//Create the operation
			CloutProgram_Op_Datatypes NewOp;
			CloutProgram_InstantiateNewOp(NewOp, iOpType);	//This creates the new object with the appropriate datatype

		//Read the data
			jOp.get_to(NewOp);

		//Add it to the program;
			Prog.AddOperation(NewOp);
	}
}


CloutProgram::CloutProgram()
{
	Ops.clear();
	jData.clear();
};

void CloutProgram::AddOperation(CloutProgram_Op_Datatypes NewOp)
{
	Ops.push_back(NewOp);
}

void CloutProgram::AddNewOperation(int iType)
{
	CloutProgram_Op_Datatypes NewOp;
	CloutProgram_InstantiateNewOp(NewOp, iType);	//This creates the new object with the appropriate datatype

	//If it's a probe op, give it a default sub-op
		if (iType == CLOUT_OP_PROBE_OP)
			std::get<CloutProgram_Op_ProbeOp>(NewOp).Change_ProbeOp_Type(PROBE_OP_TYPE_BORE);

			

	AddOperation(NewOp);
}

void CloutProgram::MoveOperationUp(int iIdx)
{
	if (iIdx == 0)
		return;

	std::swap(Ops[iIdx-1], Ops[iIdx]);
}

void CloutProgram::LoadFromFile(const char *szFilename)
{
	Ops.clear();
	jData.clear();

	//Read the JSON data from the file
		std::ifstream f(szFilename);
		jData = json::parse(f);
		f.close();

	//Convert that to our data format
		jData.get_to(*this);
}

void CloutProgram::SaveToFile(const char* szFilename)
{
	//Rebuild the JSON data
		jData.clear();
		jData["Name"] = "Clout Program";
		jData["File Version"] = { CLOUT_JSON_VER_MAJ, CLOUT_JSON_VER_MIN };

		//Build the array of operations
			json jOps;
			for (int x = 0; x < Ops.size(); x++)
			{
				json jThisOp;
				jThisOp.clear();
				GetOp(x).ParseToJSON(jThisOp);
				jOps.push_back(jThisOp);
			}

			jData["Operations"] = jOps;
	
	//Write to the file
		std::filebuf fb;
		fb.open(szFilename, std::ios::out);

		std::ostream file(&fb);
		file << std::setw(4) << jData << std::endl;

		fb.close();
}

void CloutProgram::Erase()
{
	Ops.clear();
}