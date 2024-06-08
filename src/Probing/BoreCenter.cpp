#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"


ProbeOperation_BoreCenter::ProbeOperation_BoreCenter()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bType = PROBE_OP_TYPE_BORE;

	szWindowIdent = "BoreCenter";

	imgPreview = 0;
	LoadPreviewImage(&imgPreview, "./res/ProbeBore.png");

	fBoreDiameter = 25.0f;
	fOvertravel = 5.0f;
}

void ProbeOperation_BoreCenter::StateMachine()
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
		iState++;
		break;

	case PROBE_STATE_BORECENTER_PROBE_MIN_X_FAST:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X-%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_PROBE_MIN_X_SLOW:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 X%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos1))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_START_X:
		if (!bStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf_s(sCmd, 50, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
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

	case PROBE_STATE_BORECENTER_PROBE_MAX_X_FAST:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 X%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_PROBE_MAX_X_SLOW:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 X-%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos2))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_CENTER_X:
		if (!bStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  
			sprintf_s(sCmd, 50, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
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


	case PROBE_STATE_BORECENTER_PROBE_MIN_Y_FAST:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 Y-%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_PROBE_MIN_Y_SLOW:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 Y%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos3))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_START_Y:
		if (!bStepIsRunning)
		{
			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning.  

			sprintf_s(sCmd, 50, "G0 Y%0.2f F%d", StartPos.y, iProbingSpeedFast); //All the way back to center
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
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

	case PROBE_STATE_BORECENTER_PROBE_MAX_Y_FAST:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.3 Y%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedFast);
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_PROBE_MAX_Y_SLOW:
		if (!bStepIsRunning)
		{
			sprintf_s(sCmd, 50, "G38.5 Y-%0.2f F%d", (fBoreDiameter / 2.0f) + fOvertravel, iProbingSpeedSlow); //Back towards center, at slow speed
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &ProbePos4))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
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

	case PROBE_STATE_BORECENTER_CENTER:
		if (!bStepIsRunning)
		{
			//Calculate center in Machine coords
			EndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;
			EndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;

			Comms_SendString("G90"); //G38 puts us in relative positioning.  Switch back to absolute positioning. 
			sprintf_s(sCmd, 50, "G53 G0 X%0.2f Y%0.2f F%d", EndPos.x, EndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			if (MachineStatus.Status == Carvera::Status::Idle && fabs(MachineStatus.Coord.Machine.y - EndPos.y) < 0.1) //We only need to monitor Y here, since that's what we last moved
			{
				bStepIsRunning = 0;
				iState++;
			}
		}
		break;

	case PROBE_STATE_BORECENTER_FINISH:
		EndPos.x = MachineStatus.Coord.Working.x;
		EndPos.y = MachineStatus.Coord.Working.y;

		//Zero the WCS if requested
		if (bZeroWCS)
			ZeroWCS(true, true, false);	//Set this location to 0,0

		sprintf_s(sCmd, 50, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Bore center: (%0.03f, %0.03f)", EndPos.x, EndPos.y);
		iState = PROBE_STATE_COMPLETE;
		break;
	}
}

void ProbeOperation_BoreCenter::DrawSubwindow()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	ImGui::Text("Probe inside a bore to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of bore at the desired Z height");

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 450.0f) * 0.5f);	//Center the image in the window
	ImGui::Image((void*)(intptr_t)imgPreview, ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(200);	//Set the width of the textboxes

	//Bore Diameter
		sprintf_s(sString, 10, "%%0.3f%s", sUnits);
		ImGui::InputFloat("Bore diameter", &fBoreDiameter, 0.01f, 0.1f, sString);
		ImGui::SameLine(); HelpMarker("Nominal diameter of the bore, in current machine units.");

	//Overtravel Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Overtravel distance", &fOvertravel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance beyond the nominal diameter to continue probing before failing.");

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

			if (ImGui::BeginCombo("##ProbingBoreCenter_WCS", szWCSChoices[iWCSIndex]))
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

bool ProbeOperation_BoreCenter::DrawPopup()
{
	bool bRetVal = TRUE;	//The operation continues to run

	BeginPopup();

	DrawSubwindow();

	bRetVal = EndPopup();

	StateMachine(); //Run the state machine if an operation is going

	return bRetVal;
}