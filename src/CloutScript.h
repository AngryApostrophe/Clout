#include <any>
#include <variant>

#include <json.hpp>
using json = nlohmann::json;

#include "Probing/Probing.h"

#define OpBaseClass(var) std::visit([](CloutScript_Op& outputvar) -> CloutScript_Op& { return outputvar; }, var)


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
class CloutScript_Op;
class CloutScript_Op_ATC_Tool_Change;
class CloutScript_Op_InstallTouchProbe;
class CloutScript_Op_RapidTo;
class CloutScript_Op_Custom_GCode;
class CloutScript_Op_ProbeOp;
class CloutScript_Op_Run_GCode_File;
class CloutScript_Op_OpenCollet;
class CloutScript_Op_CloseCollet;
class CloutScript_Op_HomeXY;

using CloutScript_Op_Datatypes = std::variant<CloutScript_Op, CloutScript_Op_ATC_Tool_Change, CloutScript_Op_InstallTouchProbe, CloutScript_Op_RapidTo, CloutScript_Op_Custom_GCode, CloutScript_Op_ProbeOp, CloutScript_Op_Run_GCode_File, CloutScript_Op_OpenCollet, CloutScript_Op_CloseCollet, CloutScript_Op_HomeXY>;



#define STATE_OP_COMPLETE	9999	//Each op will set to this state when it's complete and ready to move on

class CloutScript_Op
{
public:
	CloutScript_Op() { iType = CLOUT_OP_NULL; iState=0; bEditorExpanded = false; FullText.clear(); bKeepItem = true; };

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
class CloutScript_Op_ATC_Tool_Change : public CloutScript_Op
{
public:
	CloutScript_Op_ATC_Tool_Change();

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
class CloutScript_Op_InstallTouchProbe : public CloutScript_Op
{
public:
	CloutScript_Op_InstallTouchProbe();

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
class CloutScript_Op_RapidTo : public CloutScript_Op
{
public:
	CloutScript_Op_RapidTo();

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

//Custom G Code
class CloutScript_Op_Custom_GCode : public CloutScript_Op
{
public:
	CloutScript_Op_Custom_GCode();

	std::string strGCode;
	std::string strTransmitted; //This includes the sync stuff at the end

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
		virtual void ParseToJSON(json& j);
};

//Probe Operation
class CloutScript_Op_ProbeOp : public CloutScript_Op
{
public:
	CloutScript_Op_ProbeOp();

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
class CloutScript_Op_Run_GCode_File : public CloutScript_Op
{
public:
	CloutScript_Op_Run_GCode_File();
	~CloutScript_Op_Run_GCode_File() {};

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
class CloutScript_Op_OpenCollet : public CloutScript_Op
{
public:
	CloutScript_Op_OpenCollet();
	~CloutScript_Op_OpenCollet() {};

	bool bConfirmWithOperator;	//True if we want to ask the operator if he's ready, so we don't drop something important

	//Inherited
	virtual void StateMachine();
	virtual void DrawDetailTab();
	virtual void DrawEditorSummaryInfo();
	virtual void ParseFromJSON(const json& j);
	virtual void ParseToJSON(json& j);
};

//Close Collet
class CloutScript_Op_CloseCollet : public CloutScript_Op
{
public:
	CloutScript_Op_CloseCollet();
	~CloutScript_Op_CloseCollet() {};

	bool bConfirmWithOperator;	//True if we want to ask the operator if he's ready

	//Inherited
	virtual void StateMachine();
	virtual void DrawDetailTab();
	virtual void DrawEditorSummaryInfo();
	virtual void ParseFromJSON(const json& j);
	virtual void ParseToJSON(json& j);
};

//Home XY
class CloutScript_Op_HomeXY : public CloutScript_Op
{
public:
	CloutScript_Op_HomeXY();
	~CloutScript_Op_HomeXY() {};

	bool bRemoveOffsets;	//True if we want to remove all WCS offsets and set 0,0 back to bottom left corner

	//Inherited
	virtual void StateMachine();
	virtual void DrawDetailTab();
	virtual void DrawEditorSummaryInfo();
	virtual void ParseFromJSON(const json& j);
	virtual void ParseToJSON(json& j);
};











class CloutScript
{
public:
	CloutScript();
	CloutScript(const char* szFilename) { CloutScript(); LoadFromFile(szFilename); };
	~CloutScript(){};

	void LoadFromFile(const char *szFilename);
	void SaveToFile(const char* szFilename);
	
	void AddOperation(std::shared_ptr<CloutScript_Op> NewOp); //Add a new operation to the program
	void AddOperation(CloutScript_Op_Datatypes NewOp); //Add a new operation to the program
	void AddNewOperation(int iType); //Create a new operation of the given type and add it to the program

	void DeleteOperation(int iIndex); //Delete an operation from the program

	void MoveOperationUp(int iIdx);

	void Erase();	//Erase the current program

	std::vector <CloutScript_Op_Datatypes> Ops;
	CloutScript_Op& GetOp(int idx){  return OpBaseClass(Ops[idx]); };	//Get a reference to the base class of Op[idx]


	json jData; //JSON Data
};



//JSON import/export handlers
//void to_json(json& j, const CloutScript_Op Op);
//void from_json(const json& j, CloutScript_Op& Op);
void to_json(json& j, CloutScript_Op_Datatypes& refOp);
void from_json(const json& j, CloutScript_Op_Datatypes& refOp);
void to_json(json& j, const CloutScript& C);
void from_json(const json& j, CloutScript& Prog);
