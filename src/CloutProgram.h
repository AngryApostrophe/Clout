#include <any>
#include <variant>

#include <json.hpp>
using json = nlohmann::json;

#include "Probing/Probing.h"

#define OpBaseClass(var) std::visit([](CloutProgram_Op& outputvar) -> CloutProgram_Op& { return outputvar; }, var)


#define CLOUT_OP_NULL			0
#define CLOUT_OP_ATC_TOOL_CHANGE	1	//Use the ATC to change to a new tool (or drop current tool and not replace it)
#define CLOUT_OP_RAPID_TO		2	//Rapid to a new location
#define CLOUT_OP_HOME_XY			3	//Home X and Y axes
#define CLOUT_OP_INSTALL_PROBE	4	//Work with the user to install the touch probe
#define CLOUT_OP_PROBE_OP		5	//Run a probe operation
#define CLOUT_OP_RUN_GCODE_FILE	6	//Run G Code from a file
#define CLOUT_OP_CUSTOM_GCODE		7	//Run custom G code
#define CLOUT_OP_ALERT_USER		8	//Alert the user to something

extern const char* szOperationName[];	//Array of strings describing the above


//Forward declare types
class CloutProgram_Op;
class CloutProgram_Op_ATC_Tool_Change;
class CloutProgram_Op_InstallTouchProbe;
class CloutProgram_Op_RapidTo;
class CloutProgram_Op_Custom_GCode;
class CloutProgram_Op_ProbeOp;
class CloutProgram_Op_Run_GCode_File;

using CloutProgram_Op_Datatypes = std::variant<CloutProgram_Op, CloutProgram_Op_ATC_Tool_Change, CloutProgram_Op_InstallTouchProbe, CloutProgram_Op_RapidTo, CloutProgram_Op_Custom_GCode, CloutProgram_Op_ProbeOp, CloutProgram_Op_Run_GCode_File>;



class CloutProgram_Op
{
public:
	CloutProgram_Op() { iType = CLOUT_OP_NULL; bEditorExpanded = false; FullText.clear(); };

	int iType; //What type of operation is this?

	std::string FullText;	//The full text of this operation.  Eg:  Type is "Rapid To" and full text would be "Rapid To (400,134) @ 300"

	bool bEditorExpanded; //Used for the Program Editor.  True if this operation is expanded.

	//For running the operation:
		int iState;
		virtual void StateMachine(){};

	//Drawing stuff
		virtual void DrawDetailTab(){};
		virtual void DrawEditorSummaryInfo() {};

	//JSON stuff
		virtual void ParseFromJSON(const json& j) {};
};


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
};


class CloutProgram_Op_InstallTouchProbe : public CloutProgram_Op
{
public:
	CloutProgram_Op_InstallTouchProbe();

	bool bConfirmFunction;	//True if we wish to confirm that the touch probe is working before moving on

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
};

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
};

class CloutProgram_Op_Run_GCode_File : public CloutProgram_Op
{
public:
	CloutProgram_Op_Run_GCode_File();
	~CloutProgram_Op_Run_GCode_File() {};

	std::string sFilename;
	
	std::vector<std::string> sGCode_Line;
	
	int iStartLineNum;	//Line number that we will begin executing at
	int iLastLineNum;	//Final line number we will execute

	//Inherited
		virtual void StateMachine();
		virtual void DrawDetailTab();
		virtual void DrawEditorSummaryInfo();
		virtual void ParseFromJSON(const json& j);
};





class CloutProgram
{
public:
	CloutProgram();
	~CloutProgram(){};

	void LoadFromFile(const char *szFilename);
	void AddOperation(std::shared_ptr<CloutProgram_Op> NewOp); //Add a new operation to the program
	void AddOperation(CloutProgram_Op_Datatypes NewOp); //Add a new operation to the program
	void DeleteOperation(int iIndex); //Delete an operation from the program

	void MoveOperationUp(int iIdx);

	void Erase();	//Erase the current program

	std::vector <CloutProgram_Op_Datatypes> Ops;
	CloutProgram_Op& GetOp(int idx){  return OpBaseClass(Ops[idx]); };	//Get a reference to the base class of Op[idx]


	json jData; //JSON Data
};



//JSON import/export handlers
void to_json(json& j, const CloutProgram_Op Op);
void from_json(const json& j, CloutProgram_Op& Op);
void to_json(json& j, const CloutProgram& C);
void from_json(const json& j, CloutProgram& Prog);
