#include <Windows.h>
#include <math.h>

#include <imgui.h>

#include "Clout.h"
#include "Helpers.h"
#include "Comms.h"
#include "Console.h"
#include "Probing.h"

//For image loading
#include "stb_image.h"

bool bProbingPageInitialized = false;

//Common settings
	int iProbingSpeedFast = 300;
	int iProbingSpeedSlow = 10;
	
	int iProbingState = 0; //Idle
	bool bProbingStepIsRunning = false; //True if we have sent this step to the machine, and are just waiting for it to complete.

	DOUBLE_XYZ ProbingStartFeedrate; //What was the feed rate before we started?  We'll restore this when we're done

	DOUBLE_XYZ ProbeStartPos; //Where we were when we started this operation
	DOUBLE_XYZ ProbePos1, ProbePos2, ProbePos3, ProbePos4; //General purpose use, depending on which operation we're running. 
	DOUBLE_XYZ ProbeEndPos; //Final position we'll tell the user.

	bool bProbing_ZeroWCS = false;
	int iProbing_WCSIndex = 0;


	#define PROBE_STATE_IDLE			0	//Not currently running
	#define PROBE_STATE_COMPLETE		255	//All done, close the window
	
//Bore Center
	#define PROBE_STATE_BORECENTER_START			1	//User just clicked Run
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

	GLuint imgProbingBoreCenter = 0;
	float  fProbingBoreCenter_BoreDiameter = 25.0f;
	float  fProbingBoreCenter_Overtravel = 5.0f;

//Single Axis
	#define PROBE_STATE_SINGLEAXIS_START			100	//User just clicked Run
	#define PROBE_STATE_SINGLEAXIS_PROBE_FAST		101	//Probing in desired direction
	#define PROBE_STATE_SINGLEAXIS_PROBE_SLOW		102	//Probing away from the detected edge
	#define PROBE_STATE_SINGLEAXIS_GETCOORDS		103	//Ask for updated coordinates.  Otherwise we may be too quick and could use coords from 1/3 sec ago.
	#define PROBE_STATE_SINGLEAXIS_FINISH			104	//Update any WCS offsets, inform user, etc.

	GLuint imgProbingSingleAxis[5] = {0,0,0,0,0};		//One for each axis and direction
	int    iProbingSingleAxis_AxisDirectionIndex = 0;		//0 = x-,  1 = x+,  2 = y+,  3 = y-,  4 = z-
	float  fProbingSingleAxis_Travel = 5.0f;			//How far to travel while looking for the edge

//Pocket center
	#define PROBE_STATE_POCKETCENTER_START			200	//User just clicked Run.  May jump to either PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST or PROBE_STATE_POCKETCENTER_START_Y depending on what we've selected
	#define PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST	201	//Probing left until we find the minimum x
	#define PROBE_STATE_POCKETCENTER_PROBE_MIN_X_SLOW	202	//Moving slowly to the right until we de-trigger
	#define PROBE_STATE_POCKETCENTER_START_X		203	//Moving to start x
	#define PROBE_STATE_POCKETCENTER_PROBE_MAX_X_FAST	204	//Probing right until we find the maximum x
	#define PROBE_STATE_POCKETCENTER_PROBE_MAX_X_SLOW	205	//Moving slowly to the left until we de-trigger
	#define PROBE_STATE_POCKETCENTER_CENTER_X		206	//Move to center x, done proving
	#define PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST	207	//Probing back until we find the minimum y
	#define PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_SLOW	208	//Moving forward slowly until we de-trigger
	#define PROBE_STATE_POCKETCENTER_START_Y		209	//Moving to start y
	#define PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_FAST	210	//Probing forward until we find the maximum y
	#define PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_SLOW	211	//Moving slowly back until we de-trigger
	#define PROBE_STATE_POCKETCENTER_CENTER_Y		212	//Move to center y, done probing
	#define PROBE_STATE_POCKETCENTER_FINISH			213	//Update any WCS offsets, inform user, etc.

	GLuint imgProbingPocketCenter[2] = { 0,0 };			//One for X and Y
	int	  iProbingPocketCenter_AxisIndex = 0;			//Which axis to probe in.  0=X,  1=Y
	float  fProbingPocketCenter_PocketWidth = 15.0f;		//Nominal total width of the Pocket
	float  fProbingPocketCenter_Overtravel = 5.0f;		//How far beyond the nominal width we attempt to probe before failing

//Boss center
	#define PROBE_STATE_BOSSCENTER_START			300	//User just clicked Run
	#define PROBE_STATE_BOSSCENTER_TO_MIN_X			301	//Travel to the min X (still at clearance height)
	#define PROBE_STATE_BOSSCENTER_LOWER_MIN_X		302	//Lower to probing height
	#define PROBE_STATE_BOSSCENTER_PROBE_MIN_X_FAST	303	//Probing right until we find the minimum x
	#define PROBE_STATE_BOSSCENTER_PROBE_MIN_X_SLOW	304	//Moving slowly to the left until we de-trigger
	#define PROBE_STATE_BOSSCENTER_CLEAR_MIN_X		305	//Moving further away before we raise up
	#define PROBE_STATE_BOSSCENTER_RAISE_MIN_X		306	//Raise back up to clearance height
	#define PROBE_STATE_BOSSCENTER_START_X			307	//Move back to starting point
	#define PROBE_STATE_BOSSCENTER_TO_MAX_X			308	//Travel to the max X (still at clearance height)
	#define PROBE_STATE_BOSSCENTER_LOWER_MAX_X		309	//Lower to probing height
	#define PROBE_STATE_BOSSCENTER_PROBE_MAX_X_FAST	310	//Probing left until we find the maximum x
	#define PROBE_STATE_BOSSCENTER_PROBE_MAX_X_SLOW	311	//Moving slowly to the right until we de-trigger
	#define PROBE_STATE_BOSSCENTER_CLEAR_MAX_X		312	//Moving further away before we raise up
	#define PROBE_STATE_BOSSCENTER_RAISE_MAX_X		313	//Raise back up to clearance height
	#define PROBE_STATE_BOSSCENTER_CENTER_X			314	//Move to center x
	#define PROBE_STATE_BOSSCENTER_TO_MIN_Y			315	//Travel to the min Y (still at clearance height)
	#define PROBE_STATE_BOSSCENTER_LOWER_MIN_Y		316	//Lower to probing height
	#define PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_FAST	317	//Probing forward until we find the minimum y
	#define PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_SLOW	318	//Moving back slowly until we de-trigger
	#define PROBE_STATE_BOSSCENTER_CLEAR_MIN_Y		319	//Moving further away before we raise up
	#define PROBE_STATE_BOSSCENTER_RAISE_MIN_Y		320	//Raise back up to clearance height
	#define PROBE_STATE_BOSSCENTER_START_Y			321	//Moving to start y
	#define PROBE_STATE_BOSSCENTER_TO_MAX_Y			322	//Travel to the max Y (still at clearance height)
	#define PROBE_STATE_BOSSCENTER_LOWER_MAX_Y		323	//Lower to probing height
	#define PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_FAST	324	//Probing back until we find the maximum y
	#define PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_SLOW	325	//Moving slowly foward until we de-trigger
	#define PROBE_STATE_BOSSCENTER_CLEAR_MAX_Y		326	//Moving further away before we raise up
	#define PROBE_STATE_BOSSCENTER_RAISE_MAX_Y		327	//Raise back up to clearance height
	#define PROBE_STATE_BOSSCENTER_CENTER			328	//Moving to the center, done probing
	#define PROBE_STATE_BOSSCENTER_FINISH			329	//Update any WCS offsets, inform user, etc.

	GLuint imgProbingBossCenter = 0;
	float  fProbingBossCenter_BossDiameter = 25.0f;		//Nominal diameter of the boss feater
	float  fProbingBossCenter_Clearance = 5.0f;			//How far outside of the nominal diameter should we go before we turn back in
	float  fProbingBossCenter_Overtravel = 5.0f;			//How far inside the nominal diameter we attempt to probe before failing
	float  fProbingBossCenter_ZDepth = -10.0f;			//How deep we should lower the probe when we're ready to measure on the way back in.  Negative numbers are lower.

//Web center
	GLuint imgProbingWebCenter[2] = { 0,0 };			//One for X and Y
	int	  iProbingWebCenter_AxisIndex = 0;				// Which axis to probe in.  0=X,  1=Y
	float  fProbingWebCenter_WebWidth = 15.0f;			//Nominal total width of the feature
	float  fProbingWebCenter_Clearance = 5.0f;			//How far outside of the nominal width should we go before we turn back in
	float  fProbingWebCenter_Overtravel = 5.0f;			//How far inside the nominal width we attempt to probe before failing
	float  fProbingWebCenter_ZDepth = -10.0f;			//How deep we should lower the probe when we're ready to measure on the way back in.  Negative numbers are lower.





void ProbingLoadImage(GLuint *img, const char *szFilename)
{
	if (img != 0 && *img == 0) //Make sure there's nothing already there
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Loading image %s", szFilename);
		if (LoadTextureFromFile(szFilename, img) == 0)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "^Error loading image!");
		}
	}
}

//Load the required images
void Probing_InitPage()
{
	ProbingLoadImage(&imgProbingBoreCenter, "./res/ProbeBore.png");
	ProbingLoadImage(&imgProbingBossCenter, "./res/ProbeBoss.png");
	
	ProbingLoadImage(&imgProbingSingleAxis[0], "./res/ProbeSingleAxisX0.png");
	ProbingLoadImage(&imgProbingSingleAxis[1], "./res/ProbeSingleAxisX1.png");
	ProbingLoadImage(&imgProbingSingleAxis[2], "./res/ProbeSingleAxisY2.png");
	ProbingLoadImage(&imgProbingSingleAxis[3], "./res/ProbeSingleAxisY3.png");
	ProbingLoadImage(&imgProbingSingleAxis[4], "./res/ProbeSingleAxisZ4.png");

	ProbingLoadImage(&imgProbingPocketCenter[0], "./res/ProbePocketX.png");
	ProbingLoadImage(&imgProbingPocketCenter[1], "./res/ProbePocketY.png");
	ProbingLoadImage(&imgProbingWebCenter[0], "./res/ProbeWebX.png");
	ProbingLoadImage(&imgProbingWebCenter[1], "./res/ProbeWebY.png");
	
	bProbingPageInitialized = TRUE;
}


void Probing_ZeroWCS(bool x, bool y, bool z, float x_val = -10000.0f, float y_val = -10000.0f, float z_val = -10000.0f)
{
	char szCmd[50];
	char szTemp[10];

	sprintf_s(szCmd, 50, "G10 L20 P%d", iProbing_WCSIndex-1);

	if (x)
	{
		szTemp[0] = 0x0;

		if (x_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " X0");
		else
			sprintf_s(szTemp, 10, " X%0.1f", x_val);
		strcat_s(szCmd, 50, szTemp);
	}
	if (y)
	{
		szTemp[0] = 0x0;

		if (y_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " Y0");
		else
			sprintf_s(szTemp, 10, " Y%0.1f", y_val);
		strcat_s(szCmd, 50, szTemp);
	}
	if (z)
	{
		szTemp[0] = 0x0;

		if (z_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " Z0");
		else
			sprintf_s(szTemp, 10, " Z%0.1f", z_val);
		strcat_s(szCmd, 50, szTemp);
	}

	Comms_SendString(szCmd);
}


int Probing_SuccessOrFail(char* s, DOUBLE_XYZ* xyz, bool bAbortOnFail)
{
	int iResult = 0;
	//Make sure we got something
		if (s == 0)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error reading response.  Aborted.");
			iProbingState = PROBE_STATE_IDLE;
			
			return -1; //Abort
		}

	//Get the result
		char* c = strstr(s, "]");
		if (c == 0)
		{
			free(sProbeReplyMessage);
			sProbeReplyMessage = 0;
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected response from Carvera.  Aborted.");
			iProbingState = PROBE_STATE_IDLE;
			
			return -1;
		}
		c--; //Move to the digit before

	//Success
		if (*c == '1')
		{
			//Save these values if requested
				if (xyz != 0)
					CommaStringTo3Doubles(sProbeReplyMessage + 5, &xyz->x, &xyz->y, &xyz->z);

			iResult = 1;
		}
	//Failed to find the edge
		else
		{
			//Sometimes we're hoping to NOT find an edge.  This is a safety feature

			if (bAbortOnFail)
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Failed to find the edge.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}

	free(sProbeReplyMessage);
	sProbeReplyMessage = 0;

	return iResult;
}

void Probing_BoreCenter_StateMachine()
{
	char sCmd[50];

	switch (iProbingState)
	{
		case PROBE_STATE_IDLE:
			bOperationRunning = false;
		break;

		case PROBE_STATE_BORECENTER_START:
			//Save the feedrates for later
				ProbingStartFeedrate.x = MachineStatus.FeedRates.x;
				ProbingStartFeedrate.y = MachineStatus.FeedRates.y;
				ProbingStartFeedrate.z = MachineStatus.FeedRates.z;

			//Save the starting position
				ProbeStartPos.x = MachineStatus.Coord.Working.x;
				ProbeStartPos.y = MachineStatus.Coord.Working.y;
				ProbeStartPos.z = MachineStatus.Coord.Working.z;

			bOperationRunning = true; //Limit what we show on the console (from comms module)

			bProbingStepIsRunning = false;
			iProbingState++;			
		break;

		case PROBE_STATE_BORECENTER_PROBE_MIN_X_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", (fProbingBoreCenter_BoreDiameter/2.0f)+fProbingBoreCenter_Overtravel, iProbingSpeedFast);
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_PROBE_MIN_X_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 X%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos1))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_START_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 X%0.2f F%d", ProbeStartPos.x, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - ProbeStartPos.x) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_PROBE_MAX_X_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedFast);
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_PROBE_MAX_X_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 X-%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos2))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_CENTER_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  
				sprintf_s(sCmd, 50, "G0 X%0.2f F%d", ProbeStartPos.x, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - ProbeStartPos.x) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;


		case PROBE_STATE_BORECENTER_PROBE_MIN_Y_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedFast);
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BORECENTER_PROBE_MIN_Y_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 Y%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos3))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BORECENTER_START_Y:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Y%0.2f F%d", ProbeStartPos.y, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.y - ProbeStartPos.y) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BORECENTER_PROBE_MAX_Y_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedFast);
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_PROBE_MAX_Y_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 Y-%0.2f F%d", (fProbingBoreCenter_BoreDiameter / 2.0f) + fProbingBoreCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos4))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_CENTER:
			if (!bProbingStepIsRunning)
			{
				//Calculate center in Machine coords
					ProbeEndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;				
					ProbeEndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;

				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
				sprintf_s(sCmd, 50, "G53 G0 X%0.2f Y%0.2f F%d", ProbeEndPos.x, ProbeEndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.y - ProbeEndPos.y) < 0.1) //We only need to monitor Y here, since that's what we last moved
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
		break;

		case PROBE_STATE_BORECENTER_FINISH:
			ProbeEndPos.x = MachineStatus.Coord.Working.x;
			ProbeEndPos.y = MachineStatus.Coord.Working.y;

			//Zero the WCS if requested
				if (bProbing_ZeroWCS)
					Probing_ZeroWCS(true, true, false);	//Set this location to 0,0

			sprintf_s(sCmd, 50, "G0 F%f", ProbingStartFeedrate.x); //Restore the feedrate to what it was
			Comms_SendString(sCmd);

			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Bore center: (%0.03f, %0.03f)", ProbeEndPos.x, ProbeEndPos.y);
			iProbingState = PROBE_STATE_COMPLETE;
		break;
	}
}

void Probing_BoreCenterPopup()
{	
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches
			
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Probe Bore Center", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		return;


	ImGui::Text("Probe inside a bore to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of bore at the desired Z height");
	

	ImGui::Image((void*)(intptr_t)imgProbingBoreCenter, ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

		//Bore Diameter
			sprintf_s(sString, 10, "%%0.3f%s", sUnits);
			ImGui::InputFloat("Bore diameter", &fProbingBoreCenter_BoreDiameter, 0.01f, 0.1f, sString);
				ImGui::SameLine(); HelpMarker("Nominal diameter of the bore, in current machine units.");

		//Overtravel Distance
			sprintf_s(sString, 10, "%%0.2f%s", sUnits);
			ImGui::InputFloat("Overtravel distance", &fProbingBoreCenter_Overtravel, 0.1f, 1.0f, sString);
			ImGui::SameLine(); HelpMarker("Distance beyond the nominal diameter to continue probing before failing.");
			//if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
			//ImGui::SetItemTooltip("Distance beyond the nominal diameter to continue probing before failing.");

		//Feed rate
			ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
				ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
			ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
				ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");
	
				
	ImGui::SeparatorText("Completion");
	
		//Zero WCS?
			ImGui::Checkbox("Zero WCS", &bProbing_ZeroWCS);
			ImGui::SameLine();

			//Disable the combo if the option isn't selected
			if (!bProbing_ZeroWCS)
				ImGui::BeginDisabled();
			
			//The combo box
				//iProbing_WCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
				if (iProbing_WCSIndex < Carvera::CoordSystem::G54)
					iProbing_WCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

				if (ImGui::BeginCombo("##ProbingBoreCenter_WCS", szWCSChoices[iProbing_WCSIndex]))
				{
					x = Carvera::CoordSystem::G54;
					while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
					{
						//Add the item
							const bool is_selected = (iProbing_WCSIndex == (x));
							if (ImGui::Selectable(szWCSChoices[x], is_selected))
								iProbing_WCSIndex = (x);

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						x++;
					}

					ImGui::EndCombo();
				}

				if (!bProbing_ZeroWCS)
					ImGui::EndDisabled();

				ImGui::SameLine();
				HelpMarker("If selected, after completion of the probing operation the desired WCS coordinates will be reset to (0,0)");


	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##BoreCenter", ImVec2(120, 0)))
	{
		//ImGui::CloseCurrentPopup();
		iProbingState = PROBE_STATE_BORECENTER_START;
	}
	
	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##BoreCenter", ImVec2(120, 0))) 
	{ 
		if (iProbingState != PROBE_STATE_IDLE)
		{

			sString[0] = 0x18; //Abort command
			sString[1] = 0x0;
			Comms_SendString(sString);
		}
		
		ImGui::CloseCurrentPopup(); 

		//TODO: If running, ask if they want to cancel the op before closing
	}
	

	//Close the window once we're done
		if (iProbingState == PROBE_STATE_COMPLETE)
		{
			iProbingState = PROBE_STATE_IDLE;
			ImGui::CloseCurrentPopup();
		}

	ImGui::EndPopup();

	Probing_BoreCenter_StateMachine(); //Run the state machine if an operation is going
}

void Probing_BossCenter_StateMachine()
{
	char sCmd[50];

	switch (iProbingState)
	{
	case PROBE_STATE_IDLE:
		bOperationRunning = false;
		break;

	case PROBE_STATE_BOSSCENTER_START:
		//Save the feedrates for later
			ProbingStartFeedrate.x = MachineStatus.FeedRates.x;
			ProbingStartFeedrate.y = MachineStatus.FeedRates.y;
			ProbingStartFeedrate.z = MachineStatus.FeedRates.z;

		//Save the starting position
			ProbeStartPos.x = MachineStatus.Coord.Working.x;
			ProbeStartPos.y = MachineStatus.Coord.Working.y;
			ProbeStartPos.z = MachineStatus.Coord.Working.z;

		bOperationRunning = true; //Limit what we show on the console (from comms module)

		bProbingStepIsRunning = false;
		iProbingState = PROBE_STATE_BOSSCENTER_TO_MIN_X;
		break;

	case PROBE_STATE_BOSSCENTER_TO_MIN_X:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", (fProbingBossCenter_BossDiameter / 2.0f) + fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to the min x.  Treat it as a probe just in case we hit something we shouldn't
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
				{
					bProbingStepIsRunning = 0;
					iProbingState = PROBE_STATE_BOSSCENTER_LOWER_MIN_X;
				}
				else
				{
					//We hit something when we shouldn't have, abort
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_BOSSCENTER_LOWER_MIN_X:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 Z%0.2f F%d", fProbingBossCenter_ZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
				else
				{
					//We hit something when we shouldn't have, abort
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_BOSSCENTER_PROBE_MIN_X_FAST:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", fProbingBossCenter_Clearance + fProbingBossCenter_Overtravel, iProbingSpeedFast); //Probe in towards the center
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_BOSSCENTER_PROBE_MIN_X_SLOW:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 X-%0.2f F%d", fProbingBossCenter_Overtravel, iProbingSpeedSlow); //Back off to the left, at slow speed
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos1))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MIN_X:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to clearance x pos
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_BOSSCENTER_RAISE_MIN_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Z%0.2f F%d", ProbeStartPos.z, iProbingSpeedFast); //Raise back up to the start Z
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.z - ProbeStartPos.z) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_START_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 X%0.2f F%d", ProbeStartPos.x, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - ProbeStartPos.x) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_TO_MAX_X:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", (fProbingBossCenter_BossDiameter / 2.0f) + fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to the max x.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_LOWER_MAX_X:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Z%0.2f F%d", fProbingBossCenter_ZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;


		case PROBE_STATE_BOSSCENTER_PROBE_MAX_X_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", fProbingBossCenter_Clearance + fProbingBossCenter_Overtravel, iProbingSpeedFast); //Probe in towards the center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_X_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 X%0.2f F%d", fProbingBossCenter_Overtravel, iProbingSpeedSlow); //Back off to the right, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos2))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MAX_X:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to clearance x pos
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_RAISE_MAX_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Z%0.2f F%d", ProbeStartPos.z, iProbingSpeedFast); //Raise back up to the start Z
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.z - ProbeStartPos.z) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_CENTER_X:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  
				sprintf_s(sCmd, 50, "G0 X%0.2f F%d", ProbeStartPos.x, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - ProbeStartPos.x) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_TO_MIN_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", (fProbingBossCenter_BossDiameter / 2.0f) + fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to the min y.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_LOWER_MIN_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Z%0.2f F%d", fProbingBossCenter_ZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", fProbingBossCenter_Clearance + fProbingBossCenter_Overtravel, iProbingSpeedFast); //Probe forward towards the center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 Y-%0.2f F%d", fProbingBossCenter_Overtravel, iProbingSpeedSlow); //Back off to the rear, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos3))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MIN_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to clearance y pos
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_RAISE_MIN_Y:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Z%0.2f F%d", ProbeStartPos.z, iProbingSpeedFast); //Raise back up to the start Z
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.z - ProbeStartPos.z) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_START_Y:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Y%0.2f F%d", ProbeStartPos.y, iProbingSpeedFast); //All the way back to center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.y - ProbeStartPos.y) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_TO_MAX_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", (fProbingBossCenter_BossDiameter / 2.0f) + fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to the max y.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_LOWER_MAX_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Z%0.2f F%d", fProbingBossCenter_ZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;


		case PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_FAST:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", fProbingBossCenter_Clearance + fProbingBossCenter_Overtravel, iProbingSpeedFast); //Probe rearwards towards the center
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_SLOW:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.5 Y%0.2f F%d", fProbingBossCenter_Overtravel, iProbingSpeedSlow); //Back off forward, at slow speed
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos4))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MAX_Y:
			if (!bProbingStepIsRunning)
			{
				sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", fProbingBossCenter_Clearance, iProbingSpeedFast); //Move to clearance y pos
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, 0, false) == 0) //Make sure we didn't hit something
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
					else
					{
						//We hit something when we shouldn't have, abort
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_RAISE_MAX_Y:
			if (!bProbingStepIsRunning)
			{
				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

				sprintf_s(sCmd, 50, "G0 Z%0.2f F%d", ProbeStartPos.z, iProbingSpeedFast); //Raise back up to the start Z
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.z - ProbeStartPos.z) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_CENTER:
			if (!bProbingStepIsRunning)
			{
				//Calculate center in Machine coords
					ProbeEndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;
					ProbeEndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;

				Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
				sprintf_s(sCmd, 50, "G53 G0 X%0.2f Y%0.2f F%d", ProbeEndPos.x, ProbeEndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
				Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.y - ProbeEndPos.y) < 0.1 && fabs(MachineStatus.Coord.Machine.x - ProbeEndPos.x) < 0.1)
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			break;

		case PROBE_STATE_BOSSCENTER_FINISH:
			ProbeEndPos.x = MachineStatus.Coord.Working.x;
			ProbeEndPos.y = MachineStatus.Coord.Working.y;

			//Zero the WCS if requested
				if (bProbing_ZeroWCS)
					Probing_ZeroWCS(true, true, false);	//Set this location to 0,0

			sprintf_s(sCmd, 50, "G0 F%f", ProbingStartFeedrate.x); //Restore the feedrate to what it was
			Comms_SendString(sCmd);

			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Boss center: (%0.03f, %0.03f)", ProbeEndPos.x, ProbeEndPos.y);
			iProbingState = PROBE_STATE_COMPLETE;
			break;
	}
}


void Probing_BossCenterPopup()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Probe Boss Center", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		return;


	ImGui::Text("Probe outside a boss to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of boss at the clearance Z height");


	ImGui::Image((void*)(intptr_t)imgProbingBossCenter, ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	//Boss Diameter
		sprintf_s(sString, 10, "%%0.3f%s", sUnits);
		ImGui::InputFloat("Boss diameter", &fProbingBossCenter_BossDiameter, 0.01f, 0.1f, sString);
		ImGui::SameLine(); HelpMarker("Nominal diameter of the boss, in current machine units.");

	//Clearance Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Clearance distance", &fProbingBossCenter_Clearance, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance traveled outside the nominal diameter before lowering and probing back in.");

	//Overtravel Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Overtravel distance", &fProbingBossCenter_Overtravel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance inside the nominal diameter to continue probing before failing.");

	//Z Depth
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Z probing depth", &fProbingBossCenter_ZDepth, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("How far to lower the probe when we start measuring.\nNote: Negative numbers are lower.");

	//Feed rate
		ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
		ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
		ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
		ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");

	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::Checkbox("Zero WCS", &bProbing_ZeroWCS);
	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bProbing_ZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
	x = MachineStatus.WCS;
	if (x < Carvera::CoordSystem::G54)
		x = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

	//The combo box
				//iProbing_WCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
		if (iProbing_WCSIndex < Carvera::CoordSystem::G54)
			iProbing_WCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

		if (ImGui::BeginCombo("##ProbingBossCenter_WCS", szWCSChoices[iProbing_WCSIndex]))
		{
			x = Carvera::CoordSystem::G54;
			while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
			{
				//Add the item
				const bool is_selected = (iProbing_WCSIndex == (x));
				if (ImGui::Selectable(szWCSChoices[x], is_selected))
					iProbing_WCSIndex = (x);

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				x++;
			}

			ImGui::EndCombo();
		}

		if (!bProbing_ZeroWCS)
			ImGui::EndDisabled();

		ImGui::SameLine();
		HelpMarker("If selected, after completion of the probing operation the desired WCS coordinates will be reset to (0,0)");

	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##BossCenter", ImVec2(120, 0)))
	{
		iProbingState = PROBE_STATE_BOSSCENTER_START;
	}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##BossCenter", ImVec2(120, 0)))
	{
		if (iProbingState != PROBE_STATE_IDLE)
		{

			sString[0] = 0x18; //Abort command
			sString[1] = 0x0;
			Comms_SendString(sString);
		}

		ImGui::CloseCurrentPopup();

		//TODO: If running, ask if they want to cancel the op before closing
	}


	//Close the window once we're done
	if (iProbingState == PROBE_STATE_COMPLETE)
	{
		iProbingState = PROBE_STATE_IDLE;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();

	Probing_BossCenter_StateMachine(); //Run the state machine if an operation is going
}



void Probing_PocketCenter_StateMachine()
{
	char sCmd[50];

	switch (iProbingState)
	{
	case PROBE_STATE_IDLE:
		bOperationRunning = false;
		break;

	case PROBE_STATE_POCKETCENTER_START:
		//Save the feedrates for later
			ProbingStartFeedrate.x = MachineStatus.FeedRates.x;
			ProbingStartFeedrate.y = MachineStatus.FeedRates.y;
			ProbingStartFeedrate.z = MachineStatus.FeedRates.z;

		//Save the starting position
			ProbeStartPos.x = MachineStatus.Coord.Working.x;
			ProbeStartPos.y = MachineStatus.Coord.Working.y;
			ProbeStartPos.z = MachineStatus.Coord.Working.z;

		bOperationRunning = true; //Limit what we show on the console (from comms module)

		bProbingStepIsRunning = false;

		if (iProbingPocketCenter_AxisIndex == 0) //X axis
			iProbingState = PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST;
		else if (iProbingPocketCenter_AxisIndex == 1) //Y axis
			iProbingState = PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST;
		else
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected axis selected.  Aborted.");
			iProbingState = PROBE_STATE_IDLE;
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_X_SLOW:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 X%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos1))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_START_X:
		if (!bProbingStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf_s(sCmd, 50, "G0 X%0.2f F%d", ProbeStartPos.x, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - ProbeStartPos.x) < 0.1)
			{
				bProbingStepIsRunning = 0;
				iProbingState++;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_X_FAST:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_X_SLOW:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 X-%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos2))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_CENTER_X:
		if (!bProbingStepIsRunning)
		{
			//Calculate center in Machine coords
				ProbeEndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;

			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
			sprintf_s(sCmd, 50, "G53 G0 X%0.2f F%d", ProbeEndPos.x, iProbingSpeedFast); //Move back to the real center, in MCS
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.x - ProbeEndPos.x) < 0.1) //Wait until we get to the center X position
			{
				bProbingStepIsRunning = 0;
				iProbingState = PROBE_STATE_POCKETCENTER_FINISH;
			}
		}
		break;
		break;


	case PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_SLOW:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 Y%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos3))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_START_Y:
		if (!bProbingStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf_s(sCmd, 50, "G0 Y%0.2f F%d", ProbeStartPos.y, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.y - ProbeStartPos.y) < 0.1)
			{
				bProbingStepIsRunning = 0;
				iProbingState++;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_FAST:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_SLOW:
		if (!bProbingStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 Y-%0.2f F%d", (fProbingPocketCenter_PocketWidth / 2.0f) + fProbingPocketCenter_Overtravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbePos4))
				{
					bProbingStepIsRunning = 0;
					iProbingState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iProbingState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_CENTER_Y:
		if (!bProbingStepIsRunning)
		{
			//Calculate center in Machine coords
				ProbeEndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;

			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
			sprintf_s(sCmd, 50, "G53 G0 Y%0.2f F%d", ProbeEndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
			Comms_SendString(sCmd);

			bProbingStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.y - ProbeEndPos.y) < 0.1) //Wait to get to the target position
			{
				bProbingStepIsRunning = 0;
				iProbingState = PROBE_STATE_POCKETCENTER_FINISH;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_FINISH:
		sprintf_s(sCmd, 50, "G0 F%f", ProbingStartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		if (iProbingPocketCenter_AxisIndex == 0) //X axis
		{
			//Zero the WCS if requested
				if (bProbing_ZeroWCS)
					Probing_ZeroWCS(true, false, false);	//Set this location to X0

			ProbeEndPos.x = MachineStatus.Coord.Working.x;
			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Pocket center X: %0.03f", ProbeEndPos.x);
		}
		else if (iProbingPocketCenter_AxisIndex == 1) //Y axis
		{
			//Zero the WCS if requested
				if (bProbing_ZeroWCS)
					Probing_ZeroWCS(false, true, false);	//Set this location to X0

			ProbeEndPos.y = MachineStatus.Coord.Working.y;
			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Pocket center Y: %0.03f", ProbeEndPos.y);
		}

		iProbingState = PROBE_STATE_COMPLETE;
		break;
	}
}

void Probing_PocketCenterPopup()
{
	int x;
	char sString[50]; //General purpose string
	
	char sUnits[5] = "mm"; //Currently select machine units
	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Probe Pocket Center", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		return;


	ImGui::Text("Probe inside a pocket to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of pocket at the desired Z height");


	ImGui::Image((void*)(intptr_t)imgProbingPocketCenter[iProbingPocketCenter_AxisIndex], ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	const char szAxisChoices[][2] = { "X", "Y" };
	if (ImGui::BeginCombo("Probe Axis##PocketCenter", szAxisChoices[iProbingPocketCenter_AxisIndex]))
	{
		for (int x = 0; x < 2; x++)
		{
			//Add the item
			const bool is_selected = (iProbingPocketCenter_AxisIndex == x);
			if (ImGui::Selectable(szAxisChoices[x], is_selected))
				iProbingPocketCenter_AxisIndex = x;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	ImGui::SameLine(); HelpMarker("Axis in which to probe.");

	//Pocket width
	sprintf_s(sString, 10, "%%0.3f%s", sUnits);
	ImGui::InputFloat("Pocket width", &fProbingPocketCenter_PocketWidth, 0.01f, 0.1f, sString);
	ImGui::SameLine(); HelpMarker("Nominal total width of the pocket, in current machine units.");

	//Overtravel Distance
	sprintf_s(sString, 10, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Overtravel distance", &fProbingPocketCenter_Overtravel, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("Distance beyond the nominal pocket width to continue probing before failing.");

	//Feed rate
	ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
	ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");


	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::Checkbox("Zero WCS", &bProbing_ZeroWCS);
	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bProbing_ZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
				//iProbing_WCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
		if (iProbing_WCSIndex < Carvera::CoordSystem::G54)
			iProbing_WCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

		if (ImGui::BeginCombo("##ProbingPocketCenter_WCS", szWCSChoices[iProbing_WCSIndex]))
		{
			x = Carvera::CoordSystem::G54;
			while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
			{
				//Add the item
				const bool is_selected = (iProbing_WCSIndex == (x));
				if (ImGui::Selectable(szWCSChoices[x], is_selected))
					iProbing_WCSIndex = (x);

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				x++;
			}

			ImGui::EndCombo();
		}

		if (!bProbing_ZeroWCS)
			ImGui::EndDisabled();

		ImGui::SameLine();
		HelpMarker("If selected, after completion of the probing operation the desired WCS coordinates will be reset to (0,0)");


	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##PocketCenter", ImVec2(120, 0)))
	{
		//ImGui::CloseCurrentPopup();
		iProbingState = PROBE_STATE_POCKETCENTER_START;
	}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##PocketCenter", ImVec2(120, 0)))
	{
		if (iProbingState != PROBE_STATE_IDLE)
		{

			sString[0] = 0x18; //Abort command
			sString[1] = 0x0;
			Comms_SendString(sString);
		}

		ImGui::CloseCurrentPopup();

		//TODO: If running, ask if they want to cancel the op before closing
	}


	//Close the window once we're done
	if (iProbingState == PROBE_STATE_COMPLETE)
	{
		iProbingState = PROBE_STATE_IDLE;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();

	Probing_PocketCenter_StateMachine(); //Run the state machine if an operation is going
}



void Probing_SingleAxis_StateMachine()
{
	char sCmd[50];

	switch (iProbingState)
	{
		case PROBE_STATE_IDLE:
			bOperationRunning = false;
		break;

		case PROBE_STATE_SINGLEAXIS_START:
			//Save the feedrates for later
				ProbingStartFeedrate.x = MachineStatus.FeedRates.x;
				ProbingStartFeedrate.y = MachineStatus.FeedRates.y;
				ProbingStartFeedrate.z = MachineStatus.FeedRates.z;

			//Save the starting position
				ProbeStartPos.x = MachineStatus.Coord.Working.x;
				ProbeStartPos.y = MachineStatus.Coord.Working.y;
				ProbeStartPos.z = MachineStatus.Coord.Working.z;

			bOperationRunning = true; //Limit what we show on the console (from comms module)

			bProbingStepIsRunning = false;
			iProbingState++;
		break;

		case PROBE_STATE_SINGLEAXIS_PROBE_FAST:
			if (!bProbingStepIsRunning)
			{
				strcpy_s(sCmd, 50, "G38.3 ");

				//Add the axis direction
					if (iProbingSingleAxis_AxisDirectionIndex == 0)
						strcat_s(sCmd, 50, "X-");
					else if (iProbingSingleAxis_AxisDirectionIndex == 1)
						strcat_s(sCmd, 50, "X");
					else if (iProbingSingleAxis_AxisDirectionIndex == 2)
						strcat_s(sCmd, 50, "Y");
					else if (iProbingSingleAxis_AxisDirectionIndex == 3)
						strcat_s(sCmd, 50, "Y-");
					else if (iProbingSingleAxis_AxisDirectionIndex == 4)
						strcat_s(sCmd, 50, "Z-");
					else
					{
						Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
						iProbingState = PROBE_STATE_IDLE;
						break;
					}

				//Add the travel distance
					char szTemp[10];
					sprintf_s(szTemp, 10, "%0.2f ", fProbingSingleAxis_Travel);
					strcat_s(sCmd, 50, szTemp);

				//Add the feed rate
					sprintf_s(szTemp, 10, "F%d", iProbingSpeedFast);
					strcat_s(sCmd, 50, szTemp);

				//Send it out
					Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

		case PROBE_STATE_SINGLEAXIS_PROBE_SLOW:
			if (!bProbingStepIsRunning)
			{
				strcpy_s(sCmd, 50, "G38.5 ");

				//Add the axis direction (opposite this time)
				if (iProbingSingleAxis_AxisDirectionIndex == 0)
					strcat_s(sCmd, 50, "X");
				else if (iProbingSingleAxis_AxisDirectionIndex == 1)
					strcat_s(sCmd, 50, "X-");
				else if (iProbingSingleAxis_AxisDirectionIndex == 2)
					strcat_s(sCmd, 50, "Y-");
				else if (iProbingSingleAxis_AxisDirectionIndex == 3)
					strcat_s(sCmd, 50, "Y");
				else if (iProbingSingleAxis_AxisDirectionIndex == 4)
					strcat_s(sCmd, 50, "Z");
				else
				{
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
					break;
				}

				//Add the travel distance
					char szTemp[10];
					sprintf_s(szTemp, 10, "%0.2f ", fProbingSingleAxis_Travel);
					strcat_s(sCmd, 50, szTemp);

				//Add the feed rate
					sprintf_s(szTemp, 10, "F%d", iProbingSpeedSlow);
					strcat_s(sCmd, 50, szTemp);

				//Do it
					Comms_SendString(sCmd);

				bProbingStepIsRunning = TRUE;
			}
			else //Step is running.  Monitor for completion
			{
				DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

				if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
				{
					if (Probing_SuccessOrFail(sProbeReplyMessage, &ProbeEndPos))
					{
						bProbingStepIsRunning = 0;
						iProbingState++;
					}
				}
				else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
				{
					//Abort anything going on, just in case it's running away
					sCmd[0] = 0x18; //Abort command
					sCmd[1] = 0x0;
					Comms_SendString(sCmd);

					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
					iProbingState = PROBE_STATE_IDLE;
				}
			}
		break;

	case PROBE_STATE_SINGLEAXIS_GETCOORDS:
		if (MachineStatus.Status == Carvera::Status::Idle && (fabs(MachineStatus.Coord.Machine.y - ProbeEndPos.y) < 0.1) && (fabs(MachineStatus.Coord.Machine.x - ProbeEndPos.x) < 0.1) && (fabs(MachineStatus.Coord.Machine.z - ProbeEndPos.z) < 0.1)) //Wait for the MachineStatus coords to catch up to the probe results
		{
			iProbingState = PROBE_STATE_SINGLEAXIS_FINISH;
		}
		break;

	case PROBE_STATE_SINGLEAXIS_FINISH:
		sprintf_s(sCmd, 50, "G0 F%f", ProbingStartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		//Get the end position in WCS
			ProbeEndPos.x = MachineStatus.Coord.Working.x;
			ProbeEndPos.y = MachineStatus.Coord.Working.y;
			ProbeEndPos.z = MachineStatus.Coord.Working.z;

		sCmd[0] = 0x0;

		if (iProbingSingleAxis_AxisDirectionIndex == 0 || iProbingSingleAxis_AxisDirectionIndex == 1)
			sprintf_s(sCmd, 50, "Edge at X: %0.03f", ProbeEndPos.x);
		if (iProbingSingleAxis_AxisDirectionIndex == 2 || iProbingSingleAxis_AxisDirectionIndex == 3)
			sprintf_s(sCmd, 50, "Edge at Y: %0.03f", ProbeEndPos.y);
		if (iProbingSingleAxis_AxisDirectionIndex == 4)
			sprintf_s(sCmd, 50, "Bottom at Z: %0.03f", ProbeEndPos.z);
		
		Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  %s", sCmd);
		iProbingState = PROBE_STATE_COMPLETE;
		break;
	}
}



void Probing_SingleAxisPopup()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	//Open the window
		if (!ImGui::BeginPopupModal("Probe Single Axis", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			return;

	//Description

	ImGui::Text("Probe along a single axis to find an edge");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be at start position to move in one axis only");


	ImGui::Image((void*)(intptr_t)imgProbingSingleAxis[iProbingSingleAxis_AxisDirectionIndex], ImVec2(450, 342));

	//ImGui::Separator();

//	ImGui::SeparatorText("Setup");
	ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

	//Which Axis
		//ImGui::Text("Probe direction:");
		ImGui::SeparatorText("Probe direction");

		if (ImGui::BeginTable("table_singleaxis_axis", 4 /*, ImGuiTableFlags_Borders*/))
		{
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 140);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 65);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.0f);

			//Y+
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);
				ImGui::PushID(0);
				
				//Make button red if it's selected
					if (iProbingSingleAxis_AxisDirectionIndex == 2)
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
					else
					{
						ImGuiStyle &style = ImGui::GetStyle();
						ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
					}
				
				if (ImGui::Button("Y+##ProbeSingleAxis", ImVec2(50, 50)))
					iProbingSingleAxis_AxisDirectionIndex = 2;
				
				ImGui::PopStyleColor(1);
				ImGui::PopID();

			ImGui::TableSetColumnIndex(3);
			ImGui::BeginDisabled(); //Can't probe Z+
			ImGui::Button("Z+##ProbeSingleAxis", ImVec2(50, 50));
			ImGui::EndDisabled();

			
			// X-
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(0);
			
				//Calculations for right alignment
					auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - 50 - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
					if (posX > ImGui::GetCursorPosX())
						ImGui::SetCursorPosX(posX);

				//Make button red if it's selected
					if (iProbingSingleAxis_AxisDirectionIndex == 0)
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
					else
					{
						ImGuiStyle& style = ImGui::GetStyle();
						ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
					}

				if (ImGui::Button("X-##ProbeSingleAxis", ImVec2(50, 50)))
					iProbingSingleAxis_AxisDirectionIndex = 0;

				ImGui::PopStyleColor(1);
				ImGui::PopID();


			// X+
				ImGui::TableSetColumnIndex(2);

				ImGui::PushID(0);

				//Make button red if it's selected
					if (iProbingSingleAxis_AxisDirectionIndex == 1)
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
					else
					{
						ImGuiStyle& style = ImGui::GetStyle();
						ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
					}

				if (ImGui::Button("X+##ProbeSingleAxis", ImVec2(50, 50)))
					iProbingSingleAxis_AxisDirectionIndex = 1;

				ImGui::PopStyleColor(1);
				ImGui::PopID();

			// Y-
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);
				ImGui::PushID(0);

				//Make button red if it's selected
					if (iProbingSingleAxis_AxisDirectionIndex == 3)
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
					else
					{
						ImGuiStyle& style = ImGui::GetStyle();
						ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
					}

				if (ImGui::Button("Y-##ProbeSingleAxis", ImVec2(50, 50)))
					iProbingSingleAxis_AxisDirectionIndex = 3;

				ImGui::PopStyleColor(1);
				ImGui::PopID();

			// Z-
				ImGui::TableSetColumnIndex(3);
				ImGui::PushID(0);

				//Make button red if it's selected
					if (iProbingSingleAxis_AxisDirectionIndex == 4)
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
					else
					{
						ImGuiStyle& style = ImGui::GetStyle();
						ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
					}

				if (ImGui::Button("Z-##ProbeSingleAxis", ImVec2(50, 50)))
					iProbingSingleAxis_AxisDirectionIndex = 4;

				ImGui::PopStyleColor(1);
				ImGui::PopID();

			ImGui::EndTable();
		}

		//ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

		ImGui::SeparatorText("Setup");

	//Travel Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Travel distance", &fProbingSingleAxis_Travel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("How far to probe in the desired direction before failing.");

//Feed rate
	ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
	ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");


	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::BeginDisabled(); //TODO: Need to know probe size so we can accurately set 0
	ImGui::Checkbox("Zero WCS", &bProbing_ZeroWCS);
	ImGui::EndDisabled();
	
	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bProbing_ZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
				//iProbing_WCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
		if (iProbing_WCSIndex < Carvera::CoordSystem::G54)
			iProbing_WCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

		if (ImGui::BeginCombo("##ProbingSingleAxis_WCS", szWCSChoices[iProbing_WCSIndex]))
		{
			x = Carvera::CoordSystem::G54;
			while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
			{
				//Add the item
				const bool is_selected = (iProbing_WCSIndex == (x));
				if (ImGui::Selectable(szWCSChoices[x], is_selected))
					iProbing_WCSIndex = (x);

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				x++;
			}

			ImGui::EndCombo();
		}

		if (!bProbing_ZeroWCS)
			ImGui::EndDisabled();

		ImGui::SameLine();
		HelpMarker("If selected, after completion of the probing operation the desired WCS axis will be zero'd");

	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##SingleAxis", ImVec2(120, 0)))
	{
		//ImGui::CloseCurrentPopup();
		iProbingState = PROBE_STATE_SINGLEAXIS_START;
	}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##SingleAxis", ImVec2(120, 0)))
	{
		if (iProbingState != PROBE_STATE_IDLE)
		{

			sString[0] = 0x18; //Abort command
			sString[1] = 0x0;
			Comms_SendString(sString);
		}

		ImGui::CloseCurrentPopup();

		//TODO: If running, ask if they want to cancel the op before closing
	}


	//Close the window once we're done
		if (iProbingState == PROBE_STATE_COMPLETE)
		{
			iProbingState = PROBE_STATE_IDLE;
			ImGui::CloseCurrentPopup();
		}

	ImGui::EndPopup();

	Probing_SingleAxis_StateMachine(); //Run the state machine if an operation is going.
}



void Probing_WebCenterPopup()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Probe Web Center", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		return;


	ImGui::Text("Probe outside a web feature to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of feature at the clearance Z height");


	ImGui::Image((void*)(intptr_t)imgProbingWebCenter[iProbingWebCenter_AxisIndex], ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	//Axis
	const char szAxisChoices[][2] = { "X", "Y" };
	if (ImGui::BeginCombo("Probe Axis##WebCenter", szAxisChoices[iProbingWebCenter_AxisIndex]))
	{
		for (int x = 0; x < 2; x++)
		{
			//Add the item
			const bool is_selected = (iProbingWebCenter_AxisIndex == x);
			if (ImGui::Selectable(szAxisChoices[x], is_selected))
				iProbingWebCenter_AxisIndex = x;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	ImGui::SameLine(); HelpMarker("Axis in which to probe.");

	//Web Diameter
	sprintf_s(sString, 10, "%%0.3f%s", sUnits);
	ImGui::InputFloat("Feature width##WebCenter", &fProbingWebCenter_WebWidth, 0.01f, 0.1f, sString);
	ImGui::SameLine(); HelpMarker("Nominal width of the feature, in current machine units.");

	//Clearance Distance
	sprintf_s(sString, 10, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Clearance distance##WebCenter", &fProbingWebCenter_Clearance, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("Distance traveled outside the nominal width before lowering and lowering and probing back in.");

	//Overtravel Distance
	sprintf_s(sString, 10, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Overtravel distance##WebCenter", &fProbingWebCenter_Overtravel, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("Distance inside the nominal width to continue probing before failing.");

	//Z Depth
	sprintf_s(sString, 10, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Z probing depth##WebCenter", &fProbingWebCenter_ZDepth, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("How far to lower the probe when we start measuring.\nNote: Negative numbers are lower.");

	//Feed rate
	ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
	ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");

	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::Checkbox("Zero WCS", &bProbing_ZeroWCS);
	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bProbing_ZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
				//iProbing_WCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
		if (iProbing_WCSIndex < Carvera::CoordSystem::G54)
			iProbing_WCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

		if (ImGui::BeginCombo("##ProbingWebCenter_WCS", szWCSChoices[iProbing_WCSIndex]))
		{
			x = Carvera::CoordSystem::G54;
			while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
			{
				//Add the item
				const bool is_selected = (iProbing_WCSIndex == (x));
				if (ImGui::Selectable(szWCSChoices[x], is_selected))
					iProbing_WCSIndex = (x);

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				x++;
			}

			ImGui::EndCombo();
		}

		if (!bProbing_ZeroWCS)
			ImGui::EndDisabled();

		ImGui::SameLine();
		HelpMarker("If selected, after completion of the probing operation the desired WCS axis will be zero'd");

	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	ImGui::BeginDisabled();
	if (ImGui::Button("Run", ImVec2(120, 0)))
	{
		//iProbingState = PROBE_STATE_BOSSCENTER_START;
	}
	ImGui::EndDisabled();

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(120, 0)))
	{
		if (iProbingState != PROBE_STATE_IDLE)
		{

			sString[0] = 0x18; //Abort command
			sString[1] = 0x0;
			Comms_SendString(sString);
		}

		ImGui::CloseCurrentPopup();

		//TODO: If running, ask if they want to cancel the op before closing
	}


	//Close the window once we're done
	if (iProbingState == PROBE_STATE_COMPLETE)
	{
		iProbingState = PROBE_STATE_IDLE;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();

	//Probing_WebCenter_StateMachine(); //Run the state machine if an operation is going
}


void Probing_Draw() //This is called from inside the main draw code
{
	if (!bProbingPageInitialized)
		Probing_InitPage();

	if (ImGui::BeginTable("table_probing_ops", 2))// , ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH))
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Pocket center"))
			ImGui::OpenPopup("Probe Pocket Center");

		ImVec2 sizeLeftButtons = ImGui::GetItemRectSize();

		ImGui::TableSetColumnIndex(1);

		if (ImGui::Button("Single Axis"))
			ImGui::OpenPopup("Probe Single Axis");

		ImVec2 sizeRightButtons = ImGui::GetItemRectSize();

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Boss center", sizeLeftButtons))
			ImGui::OpenPopup("Probe Boss Center");

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Int Corner", sizeRightButtons);
		ImGui::EndDisabled();


		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Bore Center", sizeLeftButtons))
			ImGui::OpenPopup("Probe Bore Center");

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Ext Corner", sizeRightButtons);
		ImGui::EndDisabled();
		
		
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Web center", sizeLeftButtons))
			ImGui::OpenPopup("Probe Web Center");

		//These have to go inside the Table.  If it's called after EndTable() they won't appear
			Probing_BoreCenterPopup();
			Probing_BossCenterPopup();
			Probing_PocketCenterPopup();
			Probing_SingleAxisPopup();
			Probing_WebCenterPopup();

		ImGui::EndTable();
	}
}