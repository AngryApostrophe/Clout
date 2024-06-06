#include <any>
#include <variant>

#include <json.hpp>
using json = nlohmann::json;

#define MAX_OPERATIONS			255
#define MAX_GCODE_LENGTH			10000

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


struct CloutProgram_Op_ATC_Tool_Change
{
	int iNewTool;	//Index of the tool to change to. -1=None,  0=Wireless Probe, 1-6,    -99=Not defined, error.
};

struct CloutProgram_Op_Rapid_To
{
	DOUBLE_XYZ Coords;	//New coords to rapid to.  <-9999999 if the field is not used
	float fFeedrate;	//Feedrate to use. <0 if not used
};

struct CloutProgram_Op_Custom_GCode
{
	char szGCode[MAX_GCODE_LENGTH];
};

struct CloutProgram_Op_Run_GCode_File
{
	std::string sFilename;
	int iStartLineNum;	//Line number that we will begin executing at
	int iLastLineNum;	//Final line number we will execute
};


class CloutProgram_Op
{
public:
	CloutProgram_Op(){ iType = CLOUT_OP_NULL; bEditorExpanded = false;};

	int iType; //What type of operation is this?
	std::any Data; //Content is one of the above structs, depending on type

	bool bEditorExpanded; //Used for the Program Editor.  True if this operation is expanded.
};



class CloutProgram
{
public:
	CloutProgram();
	~CloutProgram(){};

	void LoadFromFile(const char *szFilename);
	void AddOperation(CloutProgram_Op *NewOp); //Add a new operation to the program
	void DeleteOperation(int iIndex); //Delete an operation from the program

	void MoveOperationUp(int iIdx);

	int iOpsCount;	//Number of operations in the program
	CloutProgram_Op Ops[MAX_OPERATIONS];	//The array of operations

	json jData; //JSON Data
};



//JSON import/export handlers
void to_json(json& j, const CloutProgram_Op Op);
void from_json(const json& j, CloutProgram_Op& Op);
void to_json(json& j, const CloutProgram& C);
void from_json(const json& j, CloutProgram& Prog);
