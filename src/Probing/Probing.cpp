#include "../Platforms/Platforms.h"

#include <math.h>
#include <typeinfo>
#include <any>		//std::any
#include <memory>	//std::unique_ptr

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

//For image loading
#include "stb_image.h"

bool bProbingPageInitialized = false;

//Probe Op types
const std::vector<const char*> szProbeOpNames = { "Boss Center", "Bore Center", "Pocket Center", "Web Center", "Single Axis" };

std::shared_ptr<ProbeOperation> Probing_ManualOp;	//For manual operations (from the utilies menu), this is the one we're currently viewing/doing

//Start a new manual probing operation from the utility menu.  This deletes any previous op and creates a new one
template <typename ProbeClass>	//This will carry the datatype of the new class we're going to create
void Probing_StartManualOp()
{
	Probing_ManualOp.reset();	//Delete the previous one
	Probing_ManualOp = std::shared_ptr<ProbeClass>(new ProbeClass);	//Create a new one
}
void ProbingEndManualOp()
{
	Probing_ManualOp.reset();
}

//Create a new probe operation, deleting the old one if it exists
void Probing_InstantiateNewOp(std::shared_ptr<ProbeOperation> &Op, int iOpType)
{
	Op.reset();	//Delete what was there

	if (iOpType == PROBE_OP_TYPE_BOSS)
		Op = std::shared_ptr<ProbeOperation_BossCenter>(new ProbeOperation_BossCenter);
	else if (iOpType == PROBE_OP_TYPE_BORE)
		Op = std::shared_ptr<ProbeOperation_BoreCenter>(new ProbeOperation_BoreCenter);
	else if (iOpType == PROBE_OP_TYPE_WEB)
		Op = std::shared_ptr<ProbeOperation_WebCenter>(new ProbeOperation_WebCenter);
	else if (iOpType == PROBE_OP_TYPE_POCKET)
		Op = std::shared_ptr<ProbeOperation_PocketCenter>(new ProbeOperation_PocketCenter);
	else if (iOpType == PROBE_OP_TYPE_SINGLEAXIS)
		Op = std::shared_ptr<ProbeOperation_SingleAxis>(new ProbeOperation_SingleAxis);
}


void ProbeOperation::ZeroWCS(bool x, bool y, bool z, float x_val, float y_val, float z_val)
{
	char szCmd[50];
	char szTemp[10];

	sprintf(szCmd, "G10 L20 P%d", iWCSIndex - 1);

	if (x)
	{
		szTemp[0] = 0x0;

		if (x_val < -500.0f) //Random, but it's clearly still the default value
			strcpy(szTemp, " X0");
		else
			sprintf(szTemp, " X%0.1f", x_val);
		strcat(szCmd, szTemp);
	}
	if (y)
	{
		szTemp[0] = 0x0;

		if (y_val < -500.0f) //Random, but it's clearly still the default value
			strcpy(szTemp, " Y0");
		else
			sprintf(szTemp, " Y%0.1f", y_val);
		strcat(szCmd, szTemp);
	}
	if (z)
	{
		szTemp[0] = 0x0;

		if (z_val < -500.0f) //Random, but it's clearly still the default value
			strcpy(szTemp, " Z0");
		else
			sprintf(szTemp, " Z%0.1f", z_val);
		strcat(szCmd, szTemp);
	}

	Comms_SendString(szCmd);
}

//Run a single probing step from the state machine
void ProbeOperation::RunProbeStep(const char* szCmd, DOUBLE_XYZ* ResultXYZ, bool bFailOnTouch)
{
	if (!bStepIsRunning)
	{
		Comms_SendString(szCmd);

		bStepIsRunning = true;
	}
	else //Step is running.  Monitor for completion
	{
		CarveraMessage msg;
		int iRet = Comms_PopMessageOfType(CARVERA_MSG_PROBE , &msg);

		if (iRet > 0) //We've received a probing event from Carvera
		{
			iRet = ProbingSuccessOrFail(msg.cData, ResultXYZ, !bFailOnTouch);

			if (!bFailOnTouch) //This is the normal mode
			{
				if (iRet)
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else //In this mode, we're hoping it doesn't touch anything
			{	
				if (iRet == 0) //Didn't touch anyting, that's good
				{
					bStepIsRunning = 0;
					iState++;
				}
				else //We hit something when we shouldn't have, abort
				{
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Probe unexpectedly triggered.  Aborted.");
					iState = PROBE_STATE_IDLE;
				}
			}
		}
		else if (iRet < 0) //Error while waiting for response
		{
			//Abort anything going on, just in case it's running away
				char szOut[3];
				szOut[0] = 0x18; //Abort command
				szOut[1] = 0x0;
				Comms_SendString(szOut);

			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown error waiting for probe response.  Aborted.");
			iState = PROBE_STATE_IDLE;
		}
	}
}

bool ProbeOperation::RunMoveStep(const char* szCmd, DOUBLE_XYZ LocationWCS, bool bUseMCS)
{
	DOUBLE_XYZ *CurPos = &MachineStatus.Coord.Working;
	if (bUseMCS)
		CurPos = &MachineStatus.Coord.Machine;
	
	if (!bStepIsRunning)
	{
		Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
		Comms_SendString(szCmd);
		bStepIsRunning = true;
	}
	else //Step is running.  Monitor for completion
	{
		if (MachineStatus.Status == Carvera::Status::Idle && fabs(CurPos->y - LocationWCS.y) < 0.1 && fabs(CurPos->x - LocationWCS.x) < 0.1 && fabs(CurPos->z - LocationWCS.z) < 0.1)
		{
			bStepIsRunning = 0;
			iState++;
			return true;
		}
	}

	return false;
}

int ProbeOperation::ProbingSuccessOrFail(char* s, DOUBLE_XYZ* xyz, bool bAbortOnFail)
{
	int iResult = 0;
	//Make sure we got something
	if (s == 0)
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error reading response.  Aborted.");
		iState = PROBE_STATE_IDLE;

		return -1; //Abort
	}

	//Get the result
	char* c = strstr(s, "]");
	if (c == 0)
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected response from Carvera.  Aborted.");
		iState = PROBE_STATE_IDLE;

		return -1;
	}
	c--; //Move to the digit before

	//Success
	if (*c == '1')
	{
		//Save these values if requested
		if (xyz != 0)
			CommaStringTo3Doubles(s + 5, &xyz->x, &xyz->y, &xyz->z);

		iResult = 1;
	}
	//Failed to find the edge
	else
	{
		//Sometimes we're hoping to NOT find an edge.  This is a safety feature

		if (bAbortOnFail)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Failed to find the edge.  Aborted.");
			iState = PROBE_STATE_IDLE;
		}
	}

	return iResult;
}

bool ProbeOperation::DrawPopup()
{
	bool bRetVal = true;	//The operation continues to run

	char szString[50]; //General purpose string

	// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	//Create the modal popup
		sprintf(szString, "Probe %s", szProbeOpNames[bProbingType]);
		ImGui::BeginPopupModal(szString, NULL, ImGuiWindowFlags_AlwaysAutoResize);

	//Draw the derived class's subwindow page
		DrawSubwindow();

	ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space before the buttons

	//Run button
		if (iState != PROBE_STATE_IDLE)
			ImGui::BeginDisabled();

		sprintf(szString, "Run##%s", szWindowIdent);
		if (ImGui::Button(szString, ScaledByWindowScale(120, 0)))
		{
			//ImGui::CloseCurrentPopup();
			iState = PROBE_STATE_START;
		}
		else if (iState != PROBE_STATE_IDLE)
			ImGui::EndDisabled();

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();

	//Cancel button
		sprintf(szString, "Cancel##%s", szWindowIdent);
		if (ImGui::Button(szString, ScaledByWindowScale(120, 0)))
		{
			if (iState != PROBE_STATE_IDLE)
			{

				szString[0] = 0x18; //Abort command
				szString[1] = 0x0;
				Comms_SendString(szString);
			}

			ImGui::CloseCurrentPopup();
			bRetVal = false;	//The operation is done

			//TODO: If running, ask if they want to cancel the op before closing
		}


	//Close the window once we're done
		if (iState == PROBE_STATE_COMPLETE)
		{
			iState = PROBE_STATE_IDLE;
			ImGui::CloseCurrentPopup();

			bRetVal = false;	//The operation is done
		}

		ImGui::EndPopup();


	StateMachine(); //Run the state machine

	return bRetVal;
}

//Load the required images
void Probing_InitPage()
{	
	bProbingPageInitialized = true;
}

void Probing_Draw() //This is called from inside the main draw code
{
	ImVec2 BtnSize = ScaledByWindowScale(110, 25);

	if (!bProbingPageInitialized)
		Probing_InitPage();

	if (ImGui::BeginTable("table_probing_ops", 2))// , ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH))
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Pocket center", BtnSize))
		{
			ImGui::OpenPopup("Probe Pocket Center");
			Probing_StartManualOp<ProbeOperation_PocketCenter>();
		}

		ImVec2 sizeLeftButtons = ImGui::GetItemRectSize();

		ImGui::TableSetColumnIndex(1);

		if (ImGui::Button("Single Axis", BtnSize))
		{
			ImGui::OpenPopup("Probe Single Axis");
			Probing_StartManualOp<ProbeOperation_SingleAxis>();
		}

		ImVec2 sizeRightButtons = ImGui::GetItemRectSize();

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Boss center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Boss Center");
			Probing_StartManualOp<ProbeOperation_BossCenter>();
		}

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Int Corner", sizeRightButtons);
		ImGui::EndDisabled();


		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Bore Center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Bore Center");
			Probing_StartManualOp<ProbeOperation_BoreCenter>();
		}

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Ext Corner", sizeRightButtons);
		ImGui::EndDisabled();
		
		
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Web center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Web Center");
			Probing_StartManualOp<ProbeOperation_WebCenter>();
		}

		//These have to go inside the Table.  If it's called after EndTable() they won't appear
		if (Probing_ManualOp != nullptr)	//Do we have an popup operation running?
		{
			if (Probing_ManualOp->DrawPopup() == 0)	//Draw the popup.  If it's completed, delete the op
				ProbingEndManualOp();
		}

		ImGui::EndTable();
	}
}

void ProbeOperation::ParseToJSON(json& j)
{
	j["Zero WCS"] = bZeroWCS;
	j["WCS"] = szWCSNames[iWCSIndex];

	j["Fast Speed"] = iProbingSpeedFast;
	j["Slow Speed"] = iProbingSpeedSlow;
}

void ProbeOperation::ParseFromJSON(const json& j)
{
	bZeroWCS = j.value("Zero WCS", false);

	iProbingSpeedFast = j.value("Fast Speed", 300);
	iProbingSpeedSlow = j.value("Slow Speed", 10);

	std::string strWCS = j.value("WCS", "G54");

	if (strWCS == "G53")
		iWCSIndex = Carvera::CoordSystem::G53;
	else if (strWCS == "G54")
		iWCSIndex = Carvera::CoordSystem::G54;
	else if (strWCS == "G55")
		iWCSIndex = Carvera::CoordSystem::G55;
	else if (strWCS == "G56")
		iWCSIndex = Carvera::CoordSystem::G56;
	else if (strWCS == "G57")
		iWCSIndex = Carvera::CoordSystem::G57;
	else if (strWCS == "G58")
		iWCSIndex = Carvera::CoordSystem::G58;
	else if (strWCS == "G59")
		iWCSIndex = Carvera::CoordSystem::G59;
}