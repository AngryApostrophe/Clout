#include <any>
#include <variant>

#include <json.hpp>
using json = nlohmann::json;

#include "Probing/Probing.h"

#define OpBaseClass(var) std::visit([](CloutProgram_Op& outputvar) -> CloutProgram_Op& { return outputvar; }, var)


#define CLOUT_JSON_VER_MAJ	1	//JSON file major version
#define CLOUT_JSON_VER_MIN	0	//JSON file minor version



#define CLOUT_OP_NULL				0
#define CLOUT_OP_ATC_TOOL_CHANGE	1	//Use the ATC to change to a new tool (or drop current tool and not replace it)
#define CLOUT_OP_RAPID_TO			2	//Rapid to a new location
#define CLOUT_OP_HOME_XY			3	//Home X and Y axes
#define CLOUT_OP_INSTALL_PROBE		4	//Work with the user to install the touch probe
#define CLOUT_OP_PROBE_OP			5	//Run a probe operation
#define CLOUT_OP_RUN_GCODE_FILE		6	//Run G Code from a file
#define CLOUT_OP_CUSTOM_GCODE		7	//Run custom G code
#define CLOUT_OP_ALERT_USER			8	//Alert the user to something
#define CLOUT_OP_OPEN_COLLET		9	//Open the collet to release the tool
#define CLOUT_OP_CLOSE_COLLET		10	//Open the collet to grab the tool

extern std::vector<const char*> strOperationName;	//Array of strings describing the above

extern std::vector<std::string> strOpsSections;
extern std::vector<std::vector<int>> OrganizedOps;


//Forward declare types
class CloutProgram_Op;
class CloutProgram_Op_ATC_Tool_Change;
class CloutProgram_Op_InstallTouchProbe;
class CloutProgram_Op_RapidTo;
class CloutProgram_Op_Custom_GCode;
class CloutProgram_Op_ProbeOp;
class CloutProgram_Op_Run_GCode_File;
class CloutProgram_Op_OpenCollet;
class CloutProgram_Op_CloseCollet;

using CloutProgram_Op_Datatypes = std::variant<CloutProgram_Op, CloutProgram_Op_ATC_Tool_Change, CloutProgram_Op_InstallTouchProbe, CloutProgram_Op_RapidTo, CloutProgram_Op_Custom_GCode, CloutProgram_Op_ProbeOp, CloutProgram_Op_Run_GCode_File, CloutProgram_Op_OpenCollet, CloutProgram_Op_CloseCollet>;



#define STATE_OP_COMPLETE	9999	//Each op will set to this state when it's complete and ready to move on

class CloutProgram_Op
{
public:
	CloutProgram_Op() { iType = CLOUT_OP_NULL; iState=0; bEditorExpanded = false; FullText.clear(); bKeepItem = true; };

	int iType; //What type of operation is this?

	std::string FullText;	//The full text of this operation.  Eg:  Type is "Rapid To" and full text would be "Rapid To (400,134) @ 300"

	//For running the operation:
		int iState;
		virtual void StateMachine(){};

	//Drawing stuff
		virtual void DrawDetailTab(){};
		virtual void DrawEditorSummaryInfo() {};

		bool bEditorExpanded; //Used for the Program Editor.  True if this operation is expanded.
		bool bKeepItem; //Set to false if the user has clicked X to delete this from the editor.

	//JSON stuff
		virtual void ParseFromJSON(const json& j) {};
		virtual void ParseToJSON(json& j) 
		{
			j["Type"] = strOperationName[iType];
		};
};


//ATC Tool Change
#define STATE_ATCCHANGE_START		0
#define STATE_ATCCHANGE_RUNNING		1
class CloutProgram_Op_ATC_Tool_Change : public CloutProgram_Op
{
public:
	CloutProgram_Op_ATC_Tool_Change();

	void GenerateFullTitle();

	int iNewTool;			//Index of the tool to change to. -1=None,  0=Wireless Probe, 1-6,    -99=Not defined, error.

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};


//Install touch probe
#define STATE_INSTALLPROBE_START		0
#define STATE_INSTALLPROBE_RETURNTOOL	1	//Return a tool currently in the spindle
#define STATE_INSTALLPROBE_CLEARANCE	2	//Go to clearance position
#define STATE_INSTALLPROBE_OPENCOLLET	3	//Open the collet
#define STATE_INSTALLPROBE_CONFIRM		4	//Confirm ready
#define STATE_INSTALLPROBE_CLOSECOLLET	5	//Close the collet
#define STATE_INSTALLPROBE_TEST			6	//Test functionality
class CloutProgram_Op_InstallTouchProbe : public CloutProgram_Op
{
public:
	CloutProgram_Op_InstallTouchProbe();

	bool bConfirmFunction;	//True if we wish to confirm that the touch probe is working before moving on

	bool bProbeStatus; //Current status of touch probe trigger.

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};


//Rapid To
#define STATE_RAPIDTO_START		0
#define STATE_RAPIDTO_MOVING	1
class CloutProgram_Op_RapidTo : public CloutProgram_Op
{
public:
	CloutProgram_Op_RapidTo();

	void GenerateFullTitle();

	bool bUseAxis[3];					//True if we are changing each of the axes
	DOUBLE_XYZ Coords;					//New coords to rapid to.  <-9999999 if the field is not used
	
	bool bUseFeedrate;					//True if we're supplying a new feedrate
	float fFeedrate;					//Feedrate to use.
	
	bool bUseWCS;						//True if we're changing WCS
	Carvera::CoordSystem::eCoordSystem WCS;	//If using WCS, which one?

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};

class CloutProgram_Op_Custom_GCode : public CloutProgram_Op
{
public:
	CloutProgram_Op_Custom_GCode();

	std::string szGCode;

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};

class CloutProgram_Op_ProbeOp : public CloutProgram_Op
{
public:
	CloutProgram_Op_ProbeOp();

	std::shared_ptr<ProbeOperation> ProbeOp;	//Pointer to this probing op object

	void Change_ProbeOp_Type(int iNewType);

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};

//Run a gcode file
#define STATE_RUNFILE_START			0
#define STATE_RUNFILE_UPLOAD		1
#define STATE_RUNFILE_RUNNING		2
class CloutProgram_Op_Run_GCode_File : public CloutProgram_Op
{
public:
	CloutProgram_Op_Run_GCode_File();
	~CloutProgram_Op_Run_GCode_File() {};

	std::string sFilename;
	
	std::vector<std::string> sGCode_Line;
	
	int iStartLineNum;	//Line number that we will begin executing at
	int iLastLineNum;	//Final line number we will execute

	void ReadFromFile();

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};

//Open Collet
#define STATE_OPENCOLLET_START		0
#define STATE_OPENCOLLET_RUNNING	1
class CloutProgram_Op_OpenCollet : public CloutProgram_Op
{
public:
	CloutProgram_Op_OpenCollet();
	~CloutProgram_Op_OpenCollet() {};

	bool bConfirmWithOperator;	//True if we want to ask the operator if he's ready, so we don't drop something important

	//Inherited
	virtual void StateMachine();
	virtual void DrawDetailTab();
	virtual void DrawEditorSummaryInfo();
	virtual void ParseFromJSON(const json& j);
	virtual void ParseToJSON(json& j);
};

//Close Collet
#define STATE_CLOSECOLLET_START		0
#define STATE_CLOSECOLLET_RUNNING	1
class CloutProgram_Op_CloseCollet : public CloutProgram_Op
{
public:
	CloutProgram_Op_CloseCollet();
	~CloutProgram_Op_CloseCollet() {};

	bool bConfirmWithOperator;	//True if we want to ask the operator if he's ready

	//Inherited
	virtual void StateMachine();
	virtual void DrawDetailTab();
	virtual void DrawEditorSummaryInfo();
	virtual void ParseFromJSON(const json& j);
	virtual void ParseToJSON(json& j);
};









class CloutProgram
{
public:
	CloutProgram();
	CloutProgram(const char* szFilename) { CloutProgram(); LoadFromFile(szFilename); };
	~CloutProgram(){};

	void LoadFromFile(const char *szFilename);
	void SaveToFile(const char* szFilename);
	
	void AddOperation(std::shared_ptr<CloutProgram_Op> NewOp); //Add a new operation to the program
	void AddOperation(CloutProgram_Op_Datatypes NewOp); //Add a new operation to the program
	void AddNewOperation(int iType); //Create a new operation of the given type and add it to the program

	void DeleteOperation(int iIndex); //Delete an operation from the program

	void MoveOperationUp(int iIdx);

	void Erase();	//Erase the current program

	std::vector <CloutProgram_Op_Datatypes> Ops;
	CloutProgram_Op& GetOp(int idx){  return OpBaseClass(Ops[idx]); };	//Get a reference to the base class of Op[idx]


	json jData; //JSON Data
};



//JSON import/export handlers
//void to_json(json& j, const CloutProgram_Op Op);
//void from_json(const json& j, CloutProgram_Op& Op);
void to_json(json& j, CloutProgram_Op_Datatypes& refOp);
void from_json(const json& j, CloutProgram_Op_Datatypes& refOp);
void to_json(json& j, const CloutProgram& C);
void from_json(const json& j, CloutProgram& Prog);
