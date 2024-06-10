#include "../Platforms/Platforms.h"

#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

ProbeOperation_PocketCenter::ProbeOperation_PocketCenter()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bProbingType = PROBE_OP_TYPE_POCKET;

	szWindowIdent = "PocketCenter";

	imgPreview[0] = 0;
	imgPreview[1] = 0;
	LoadPreviewImage(&imgPreview[0], "./res/ProbePocketX.png");
	LoadPreviewImage(&imgPreview[1], "./res/ProbePocketY.png");;


	iAxisIndex = 0;
	fPocketWidth = 15.0f;
	fOvertravel = 5.0f;
}

void ProbeOperation_PocketCenter::StateMachine()
{
	char sCmd[50];

	switch (iState)
	{
	case PROBE_STATE_IDLE:
		bOperationRunning = false;
		break;

	case PROBE_STATE_START:
		//Save the feedrates for later
		StartFeedrate.x = MachineStatus.FeedRates.x;
		StartFeedrate.y = MachineStatus.FeedRates.y;
		StartFeedrate.z = MachineStatus.FeedRates.z;

		//Save the starting position
		StartPos.x = MachineStatus.Coord.Working.x;
		StartPos.y = MachineStatus.Coord.Working.y;
		StartPos.z = MachineStatus.Coord.Working.z;

		bOperationRunning = true; //Limit what we show on the console (from comms module)

		bStepIsRunning = false;

		if (iAxisIndex == 0) //X axis
			iState = PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST;
		else if (iAxisIndex == 1) //Y axis
			iState = PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST;
		else
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected axis selected.  Aborted.");
			iState = PROBE_STATE_IDLE;
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_X_FAST:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.3 X-%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_X_SLOW:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.5 X%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos1))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_START_X:
		if (!bStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf(sCmd, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.x - StartPos.x) < 0.1)
			{
				bStepIsRunning = 0;
				iState++;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_X_FAST:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.3 X%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_X_SLOW:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.5 X-%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos2))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_CENTER_X:
		if (!bStepIsRunning)
		{
			//Calculate center in Machine coords
			EndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;

			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
			sprintf(sCmd, "G53 G0 X%0.2f F%d", EndPos.x, iProbingSpeedFast); //Move back to the real center, in MCS
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.x - EndPos.x) < 0.1) //Wait until we get to the center X position
			{
				bStepIsRunning = 0;
				iState = PROBE_STATE_POCKETCENTER_FINISH;
			}
		}
		break;
		break;


	case PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_FAST:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MIN_Y_SLOW:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.5 Y%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos3))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_START_Y:
		if (!bStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf(sCmd, "G0 Y%0.2f F%d", StartPos.y, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Working.y - StartPos.y) < 0.1)
			{
				bStepIsRunning = 0;
				iState++;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_FAST:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.3 Y%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_PROBE_MAX_Y_SLOW:
		if (!bStepIsRunning)
		{
			sprintf(sCmd, "G38.5 Y-%0.2f F%d", (fPocketWidth / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			int iRet = IsEventSet(&hProbeResponseEvent);

			if (iRet == EVENT_RESULT_SUCCESS) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos4))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet != EVENT_RESULT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_CENTER_Y:
		if (!bStepIsRunning)
		{
			//Calculate center in Machine coords
			EndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;

			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
			sprintf(sCmd, "G53 G0 Y%0.2f F%d", EndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.y - EndPos.y) < 0.1) //Wait to get to the target position
			{
				bStepIsRunning = 0;
				iState = PROBE_STATE_POCKETCENTER_FINISH;
			}
		}
		break;

	case PROBE_STATE_POCKETCENTER_FINISH:
		sprintf(sCmd, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		if (iAxisIndex == 0) //X axis
		{
			//Zero the WCS if requested
			if (bZeroWCS)
				ZeroWCS(true, false, false);	//Set this location to X0

			EndPos.x = MachineStatus.Coord.Working.x;
			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Pocket center X: %0.03f", EndPos.x);
		}
		else if (iAxisIndex == 1) //Y axis
		{
			//Zero the WCS if requested
			if (bZeroWCS)
				ZeroWCS(false, true, false);	//Set this location to X0

			EndPos.y = MachineStatus.Coord.Working.y;
			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Pocket center Y: %0.03f", EndPos.y);
		}

		iState = PROBE_STATE_COMPLETE;
		break;
	}
}


void ProbeOperation_PocketCenter::DrawSubwindow()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat(sUnits, "in"); //Inches

	ImGui::Text("Probe inside a pocket to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of pocket at the desired Z height");


	ImGui::Image((void*)(intptr_t)imgPreview[iAxisIndex], ScaledByWindowScale(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ScaledByWindowScale(450.0f)) * 0.5f);	//Center the image in the window
	ImGui::PushItemWidth(ScaledByWindowScale(200));	//Set the width of the textboxes

	const char szAxisChoices[][2] = { "X", "Y" };
	if (ImGui::BeginCombo("Probe Axis##PocketCenter", szAxisChoices[iAxisIndex]))
	{
		for (int x = 0; x < 2; x++)
		{
			//Add the item
			const bool is_selected = (iAxisIndex == x);
			if (ImGui::Selectable(szAxisChoices[x], is_selected))
				iAxisIndex = x;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	ImGui::SameLine(); HelpMarker("Axis in which to probe.");

	//Pocket width
	sprintf(sString, "%%0.3f%s", sUnits);
	ImGui::InputFloat("Pocket width", &fPocketWidth, 0.01f, 0.1f, sString);
	ImGui::SameLine(); HelpMarker("Nominal total width of the pocket, in current machine units.");

	//Overtravel Distance
	sprintf(sString, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Overtravel distance", &fOvertravel, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("Distance beyond the nominal pocket width to continue probing before failing.");

	//Feed rate
	ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
	ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");

	ImGui::PopItemWidth();


	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::Checkbox("Zero WCS", &bZeroWCS);
	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
				//iWCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
	if (iWCSIndex < Carvera::CoordSystem::G54)
		iWCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

	if (ImGui::BeginCombo("##ProbingPocketCenter_WCS", szWCSChoices[iWCSIndex]))
	{
		x = Carvera::CoordSystem::G54;
		while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
		{
			//Add the item
			const bool is_selected = (iWCSIndex == (x));
			if (ImGui::Selectable(szWCSChoices[x], is_selected))
				iWCSIndex = (x);

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
			x++;
		}

		ImGui::EndCombo();
	}

	if (!bZeroWCS)
		ImGui::EndDisabled();

	ImGui::SameLine();
	HelpMarker("If selected, after completion of the probing operation the desired WCS coordinates will be reset to (0,0)");
}
/*
bool ProbeOperation_PocketCenter::DrawPopup()
{
	bool bRetVal = true;	//The operation continues to run

	BeginPopup();

	DrawSubwindow();

	bRetVal = EndPopup();

	StateMachine(); //Run the state machine if an operation is going

	return bRetVal;
}*/