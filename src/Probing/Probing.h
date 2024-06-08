#pragma once

#include <vector>
#include <memory> //shared_ptr

//Foward declare
class ProbeOperation;
class ProbeOperation_BoreCenter;
class ProbeOperation_SingleAxis;
class ProbeOperation_PocketCenter;
class ProbeOperation_BossCenter;
class ProbeOperation_WebCenter;

//Exported functions
void Probing_InitPage();
void Probing_Draw();
void Probing_InstantiateNewOp(std::shared_ptr<ProbeOperation>& Op, int iOpType);	//Create a new Probing operation of type iOpType;

//Define probe operation types
#define PROBE_OP_TYPE_BOSS		0
#define PROBE_OP_TYPE_BORE		1
#define PROBE_OP_TYPE_POCKET		2
#define PROBE_OP_TYPE_WEB		3
#define PROBE_OP_TYPE_SINGLEAXIS	4
extern const std::vector<const char*> szProbeOpNames;	//GUI names of each of the above

//Common settings
extern int iProbingSpeedFast;
extern int iProbingSpeedSlow;

class ProbeOperation
{
public:
	ProbeOperation() { iState=0; bStepIsRunning=false; szWindowIdent=0; bZeroWCS=true; iWCSIndex=0;};
	virtual ~ProbeOperation(){};

	void LoadPreviewImage(GLuint* img, const char* szFilename);
	void ZeroWCS(bool x, bool y, bool z, float x_val = -10000.0f, float y_val = -10000.0f, float z_val = -10000.0f);
	int ProbingSuccessOrFail(char* s, DOUBLE_XYZ* xyz = 0, bool bAbortOnFail = true);

	//void Init();
	virtual void StateMachine(){};
	virtual void DrawSubwindow(){};
	bool DrawPopup();								//Returns FALSE if this operation has completed and can be deleted

	BYTE bProbingType;								//What type of probing operation is this?

protected:
	char *szWindowIdent;							//Identifier of the window for ImGui

	GLuint imgPreview;								//Image to preview the operation

	int iState;									//For the state machine
	bool bStepIsRunning;							//True if we have sent this step to Carvera, and are just waiting for it to complete.

	DOUBLE_XYZ StartFeedrate;						//What was the feed rate before we started?  We'll restore this when we're done
	DOUBLE_XYZ StartPos;							//Where we were when we started this operation
	DOUBLE_XYZ ProbePos1, ProbePos2, ProbePos3, ProbePos4;	//General purpose use, depending on which operation we're running. 
	DOUBLE_XYZ EndPos;								//Final position we'll tell the user.

	bool bZeroWCS;									//Do we want to zero out a WCS when we're done?
	int iWCSIndex;									//Which WCS to zero on completion
};



#define PROBE_STATE_IDLE			0	//Not currently running
#define PROBE_STATE_START		1	//User just clicked Run
#define PROBE_STATE_COMPLETE		255	//All done, close the window

//Bore center
#define PROBE_STATE_BORECENTER_PROBE_MIN_X_FAST	2	//Probing left until we find the minimum x
#define PROBE_STATE_BORECENTER_PROBE_MIN_X_SLOW	3	//Moving slowly to the right until we de-trigger
#define PROBE_STATE_BORECENTER_START_X			4	//Moving to start x
#define PROBE_STATE_BORECENTER_PROBE_MAX_X_FAST	5	//Probing right until we find the maximum x
#define PROBE_STATE_BORECENTER_PROBE_MAX_X_SLOW	6	//Moving slowly to the left until we de-trigger
#define PROBE_STATE_BORECENTER_CENTER_X			7	//Move to center x
#define PROBE_STATE_BORECENTER_PROBE_MIN_Y_FAST	8	//Probing back until we find the minimum y
#define PROBE_STATE_BORECENTER_PROBE_MIN_Y_SLOW	9	//Moving forward slowly until we de-trigger
#define PROBE_STATE_BORECENTER_START_Y			10	//Moving to start y
#define PROBE_STATE_BORECENTER_PROBE_MAX_Y_FAST	11	//Probing forward until we find the maximum y
#define PROBE_STATE_BORECENTER_PROBE_MAX_Y_SLOW	12	//Moving slowly back until we de-trigger
#define PROBE_STATE_BORECENTER_CENTER			13	//Moving to the center, done probing
#define PROBE_STATE_BORECENTER_FINISH			14	//Update any WCS offsets, inform user, etc.

class ProbeOperation_BoreCenter : public ProbeOperation
{
public:
	ProbeOperation_BoreCenter();

	virtual void StateMachine();
	virtual void DrawSubwindow();

	float  fBoreDiameter;
	float  fOvertravel;
};

//Single Axis
#define PROBE_STATE_SINGLEAXIS_PROBE_FAST		2	//Probing in desired direction
#define PROBE_STATE_SINGLEAXIS_PROBE_SLOW		3	//Probing away from the detected edge
#define PROBE_STATE_SINGLEAXIS_GETCOORDS		4	//Ask for updated coordinates.  Otherwise we may be too quick and could use coords from 1/3 sec ago.
#define PROBE_STATE_SINGLEAXIS_FINISH			5	//Update any WCS offsets, inform user, etc.

class ProbeOperation_SingleAxis : public ProbeOperation
{
public:
	ProbeOperation_SingleAxis();

	virtual void StateMachine();
	virtual void DrawSubwindow();

	GLuint imgPreview[5] = { 0,0,0,0,0 };		//One for each axis and direction

	int    iAxisDirectionIndex;	//0 = x-,  1 = x+,  2 = y+,  3 = y-,  4 = z-
	float  fTravel;			//How far to travel while looking for the edge
};


//Pocket center
#define PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST	2	//Probing left until we find the minimum x
#define PROBE_STATE_POCKETCENTER_PROBE_MIN_X_SLOW	3	//Moving slowly to the right until we de-trigger
#define PROBE_STATE_POCKETCENTER_START_X		4	//Moving to start x
#define PROBE_STATE_POCKETCENTER_PROBE_MAX_X_FAST	5	//Probing right until we find the maximum x
#define PROBE_STATE_POCKETCENTER_PROBE_MAX_X_SLOW	6	//Moving slowly to the left until we de-trigger
#define PROBE_STATE_POCKETCENTER_CENTER_X		7	//Move to center x, done proving
#define PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST	8	//Probing back until we find the minimum y
#define PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_SLOW	9	//Moving forward slowly until we de-trigger
#define PROBE_STATE_POCKETCENTER_START_Y		10	//Moving to start y
#define PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_FAST	11	//Probing forward until we find the maximum y
#define PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_SLOW	12	//Moving slowly back until we de-trigger
#define PROBE_STATE_POCKETCENTER_CENTER_Y		13	//Move to center y, done probing
#define PROBE_STATE_POCKETCENTER_FINISH			14	//Update any WCS offsets, inform user, etc.

class ProbeOperation_PocketCenter : public ProbeOperation
{
public:
	ProbeOperation_PocketCenter();

	virtual void StateMachine();
	virtual void DrawSubwindow();

	GLuint imgPreview[2] = { 0,0 };	//One for X and Y

	int iAxisIndex = 0;				//Which axis to probe in.  0=X,  1=Y
	float fPocketWidth = 15.0f;		//Nominal total width of the Pocket
	float fOvertravel = 5.0f;		//How far beyond the nominal width we attempt to probe before failing
};

//Boss center
#define PROBE_STATE_BOSSCENTER_TO_MIN_X			2	//Travel to the min X (still at clearance height)
#define PROBE_STATE_BOSSCENTER_LOWER_MIN_X		3	//Lower to probing height
#define PROBE_STATE_BOSSCENTER_PROBE_MIN_X_FAST	4	//Probing right until we find the minimum x
#define PROBE_STATE_BOSSCENTER_PROBE_MIN_X_SLOW	5	//Moving slowly to the left until we de-trigger
#define PROBE_STATE_BOSSCENTER_CLEAR_MIN_X		6	//Moving further away before we raise up
#define PROBE_STATE_BOSSCENTER_RAISE_MIN_X		7	//Raise back up to clearance height
#define PROBE_STATE_BOSSCENTER_START_X			8	//Move back to starting point
#define PROBE_STATE_BOSSCENTER_TO_MAX_X			9	//Travel to the max X (still at clearance height)
#define PROBE_STATE_BOSSCENTER_LOWER_MAX_X		10	//Lower to probing height
#define PROBE_STATE_BOSSCENTER_PROBE_MAX_X_FAST	11	//Probing left until we find the maximum x
#define PROBE_STATE_BOSSCENTER_PROBE_MAX_X_SLOW	12	//Moving slowly to the right until we de-trigger
#define PROBE_STATE_BOSSCENTER_CLEAR_MAX_X		13	//Moving further away before we raise up
#define PROBE_STATE_BOSSCENTER_RAISE_MAX_X		14	//Raise back up to clearance height
#define PROBE_STATE_BOSSCENTER_CENTER_X			15	//Move to center x
#define PROBE_STATE_BOSSCENTER_TO_MIN_Y			16	//Travel to the min Y (still at clearance height)
#define PROBE_STATE_BOSSCENTER_LOWER_MIN_Y		17	//Lower to probing height
#define PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_FAST	18	//Probing forward until we find the minimum y
#define PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_SLOW	19	//Moving back slowly until we de-trigger
#define PROBE_STATE_BOSSCENTER_CLEAR_MIN_Y		20	//Moving further away before we raise up
#define PROBE_STATE_BOSSCENTER_RAISE_MIN_Y		21	//Raise back up to clearance height
#define PROBE_STATE_BOSSCENTER_START_Y			22	//Moving to start y
#define PROBE_STATE_BOSSCENTER_TO_MAX_Y			23	//Travel to the max Y (still at clearance height)
#define PROBE_STATE_BOSSCENTER_LOWER_MAX_Y		24	//Lower to probing height
#define PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_FAST	25	//Probing back until we find the maximum y
#define PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_SLOW	26	//Moving slowly foward until we de-trigger
#define PROBE_STATE_BOSSCENTER_CLEAR_MAX_Y		27	//Moving further away before we raise up
#define PROBE_STATE_BOSSCENTER_RAISE_MAX_Y		28	//Raise back up to clearance height
#define PROBE_STATE_BOSSCENTER_CENTER			29	//Moving to the center, done probing
#define PROBE_STATE_BOSSCENTER_FINISH			30	//Update any WCS offsets, inform user, etc.

class ProbeOperation_BossCenter : public ProbeOperation
{
public:
	ProbeOperation_BossCenter();

	virtual void StateMachine();
	virtual void DrawSubwindow();

	float  fBossDiameter = 25.0f;		//Nominal diameter of the boss feater
	float  fClearance = 5.0f;		//How far outside of the nominal diameter should we go before we turn back in
	float  fOvertravel = 5.0f;		//How far inside the nominal diameter we attempt to probe before failing
	float  fZDepth = -10.0f;			//How deep we should lower the probe when we're ready to measure on the way back in.  Negative numbers are lower.
};

//Web center
class ProbeOperation_WebCenter : public ProbeOperation
{
public:
	ProbeOperation_WebCenter();

	virtual void StateMachine();
	virtual void DrawSubwindow();

	GLuint imgPreview[2] = { 0,0 };	//One for X and Y

	int	  iAxisIndex;	//Which axis to probe in.  0=X,  1=Y
	float  fWebWidth;	//Nominal total width of the feature
	float  fClearance;	//How far outside of the nominal width should we go before we turn back in
	float  fOvertravel;	//How far inside the nominal width we attempt to probe before failing
	float  fZDepth;	//How deep we should lower the probe when we're ready to measure on the way back in.  Negative numbers are lower.
};