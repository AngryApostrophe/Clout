#include "../Platforms/Platforms.h"

#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

#include "../Resources/ProbeWebX.h"
#include "../Resources/ProbeWebY.h"
static GLuint imgProbeWebCenter[2] = {0,0};	//Only load it once, rather than every time we open a window


ProbeOperation_WebCenter::ProbeOperation_WebCenter()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bProbingType = PROBE_OP_TYPE_WEB;

	szWindowIdent = "WebCenter";

	if (imgProbeWebCenter[0] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeWebX_compressed_data, ProbeWebX_compressed_size, &imgProbeWebCenter[0]);
	imgPreview[0] = imgProbeWebCenter[0];

	if (imgProbeWebCenter[1] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeWebY_compressed_data, ProbeWebY_compressed_size, &imgProbeWebCenter[1]);
	imgPreview[1] = imgProbeWebCenter[1];
	
	iAxisIndex = 0;
	fWebWidth = 15.0f;
	fClearance = 5.0f;
	fOvertravel = 5.0f;
	fZDepth = -10.0f;
}

void ProbeOperation_WebCenter::StateMachine()
{
	char sCmd[50];
	DOUBLE_XYZ xyz;

	switch (iState)
	{
		case PROBE_STATE_IDLE:
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

			bStepIsRunning = false;
		
			if (iAxisIndex == 0) //X axis
				iState = PROBE_STATE_WEBCENTER_TO_MIN_X;
			else if (iAxisIndex == 1) //Y axis
				iState = PROBE_STATE_WEBCENTER_TO_MIN_Y;
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected axis selected.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		break;

		case PROBE_STATE_WEBCENTER_TO_MIN_X:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", (fWebWidth / 2.0f) + fClearance, iProbingSpeedFast); //Move to the min x.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;


		case PROBE_STATE_WEBCENTER_LOWER_MIN_X:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MIN_X_FAST:
			sprintf(sCmd, "G38.3 X%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe in towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MIN_X_SLOW:
			sprintf(sCmd, "G38.5 X-%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the left, at slow speed
			RunProbeStep(sCmd, &ProbePos1);
		break;

		case PROBE_STATE_WEBCENTER_CLEAR_MIN_X:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance x pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_RAISE_MIN_X:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_START_X:
			sprintf(sCmd, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			xyz = { StartPos.x,  MachineStatus.Coord.Working.y,  MachineStatus.Coord.Working.z }; //We only really care about X
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_TO_MAX_X:
			sprintf(sCmd, "G38.3 X%0.2f F%d", (fWebWidth / 2.0f) + fClearance, iProbingSpeedFast); //Move to the max x.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_LOWER_MAX_X:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MAX_X_FAST:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe in towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MAX_X_SLOW:
			sprintf(sCmd, "G38.5 X%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the right, at slow speed
			RunProbeStep(sCmd, &ProbePos2);
		break;

		case PROBE_STATE_WEBCENTER_CLEAR_MAX_X:
			sprintf(sCmd, "G38.3 X%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance x pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_RAISE_MAX_X:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_CENTER_X:
			//Calculate center in Machine coords
				EndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;
				EndPos.y = MachineStatus.Coord.Machine.y; //Not used
				EndPos.z = MachineStatus.Coord.Machine.z; //Not used

			sprintf(sCmd, "G53 G0 X%0.2f F%d", EndPos.x, iProbingSpeedFast); //Move back to the real center, in MCS

			if (RunMoveStep(sCmd, EndPos, true))
				iState = PROBE_STATE_WEBCENTER_FINISH;
		break;

		case PROBE_STATE_WEBCENTER_TO_MIN_Y:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", (fWebWidth / 2.0f) + fClearance, iProbingSpeedFast); //Move to the min y.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_LOWER_MIN_Y:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MIN_Y_FAST:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe forward towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MIN_Y_SLOW:
			sprintf(sCmd, "G38.5 Y-%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the rear, at slow speed
			RunProbeStep(sCmd, &ProbePos3);
		break;

		case PROBE_STATE_WEBCENTER_CLEAR_MIN_Y:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance y pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_RAISE_MIN_Y:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_START_Y:
			sprintf(sCmd, "G0 Y%0.2f F%d", StartPos.y, iProbingSpeedFast); //All the way back to center
			xyz = { MachineStatus.Coord.Working.x,  StartPos.y,  MachineStatus.Coord.Working.z }; //We only really care about Y
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_TO_MAX_Y:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", (fWebWidth / 2.0f) + fClearance, iProbingSpeedFast); //Move to the max y.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_LOWER_MAX_Y:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MAX_Y_FAST:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe rearwards towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_WEBCENTER_PROBE_MAX_Y_SLOW:
			sprintf(sCmd, "G38.5 Y%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off forward, at slow speed
			RunProbeStep(sCmd, &ProbePos4);
		break;

		case PROBE_STATE_WEBCENTER_CLEAR_MAX_Y:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance y pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_WEBCENTER_RAISE_MAX_Y:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_WEBCENTER_CENTER_Y:
			//Calculate center in Machine coords
				EndPos.x = MachineStatus.Coord.Machine.x; //Not used
				EndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;
				EndPos.z = MachineStatus.Coord.Machine.z; //Not used

			sprintf(sCmd, "G53 G0 Y%0.2f F%d", EndPos.y, iProbingSpeedFast); //All the way back to center, in MCS

			if (RunMoveStep(sCmd, EndPos, true))
				iState = PROBE_STATE_WEBCENTER_FINISH;
		break;

		case PROBE_STATE_WEBCENTER_FINISH:
			sprintf(sCmd, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
			Comms_SendString(sCmd);

			if (iAxisIndex == 0) //X axis
			{
				//Zero the WCS if requested
				if (bZeroWCS)
					ZeroWCS(true, false, false);	//Set this location to X0

				EndPos.x = MachineStatus.Coord.Working.x;
				Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Feature center X: %0.03f", EndPos.x);
			}
			else if (iAxisIndex == 1) //Y axis
			{
				//Zero the WCS if requested
				if (bZeroWCS)
					ZeroWCS(false, true, false);	//Set this location to X0

				EndPos.y = MachineStatus.Coord.Working.y;
				Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Feature center Y: %0.03f", EndPos.y);
			}

			iState = PROBE_STATE_COMPLETE;
		break;
	}
}


void ProbeOperation_WebCenter::DrawSubwindow()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat(sUnits, "in"); //Inches

	ImGui::Text("Probe outside a web feature to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of feature at the clearance Z height");


	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ScaledByWindowScale(450.0f)) * 0.5f);	//Center the image in the window
	ImGui::Image((void*)(intptr_t)imgPreview[iAxisIndex], ScaledByWindowScale(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(ScaledByWindowScale(200));	//Set the width of the textboxes

	//Axis
	const char szAxisChoices[][2] = { "X", "Y" };
	if (ImGui::BeginCombo("Probe Axis##WebCenter", szAxisChoices[iAxisIndex]))
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

	//Web Diameter
		sprintf(sString, "%.6g%s##Width", fWebWidth, sUnits);
		BUTTON_TO_KEYPAD("Feature width", sString, ScaledByWindowScale(120, 0), &fWebWidth, "Nominal width of the feature, in current machine units.")

	//Clearance Distance
		sprintf(sString, "%.6g%s##Clearance", fClearance, sUnits);
		BUTTON_TO_KEYPAD("Clearance distance", sString, ScaledByWindowScale(120, 0), &fClearance, "Distance traveled outside the nominal width before lowering and lowering and probing back in.")

	//Overtravel Distance
		sprintf(sString, "%.6g%s##Overtravel", fOvertravel, sUnits);
		BUTTON_TO_KEYPAD("Overtravel distance", sString, ScaledByWindowScale(120, 0), &fOvertravel, "Distance inside the nominal width to continue probing before failing.")

	//Z Depth
		sprintf(sString, "%.6g%s##Depth", fZDepth, sUnits);
		BUTTON_TO_KEYPAD("Z probing depth", sString, ScaledByWindowScale(120, 0), &fZDepth, "How far to lower the probe when we start measuring.\nNote: Negative numbers are lower.")

	//Feed rate
		sprintf(sString, "%d##Fast", iProbingSpeedFast);
		BUTTON_TO_KEYPAD("Probing Speed (Fast)", sString, ScaledByWindowScale(120, 0), &iProbingSpeedFast, "Speed at which the probe moves towards the edge.")

		sprintf(sString, "%d##Slow", iProbingSpeedSlow);
		BUTTON_TO_KEYPAD("Probing Speed (Slow)", sString, ScaledByWindowScale(120, 0), &iProbingSpeedSlow, "Speed at which the probe moves off the edge, for high accuracy.")


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

			if (ImGui::BeginCombo("##ProbingWebCenter_WCS", szWCSChoices[iWCSIndex]))
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
	HelpMarker("If selected, after completion of the probing operation the desired WCS axis will be zero'd");
}

void ProbeOperation_WebCenter::ParseToJSON(json& j)
{
	ProbeOperation::ParseToJSON(j);

	j["Feature Width"] = fWebWidth;
	j["Clearance"] = fClearance;
	j["Overtravel"] = fOvertravel;
	j["Z Depth"] = fZDepth;

	if (iAxisIndex == 0)
		j["Axis"] = "X";
	else if (iAxisIndex == 1)
		j["Axis"] = "Y";
}

void ProbeOperation_WebCenter::ParseFromJSON(const json& j)
{
	ProbeOperation::ParseFromJSON(j);

	fWebWidth = j.value("Feature Width", 0.0f);
	fClearance = j.value("Clearance", 5.0f);
	fOvertravel = j.value("Overtravel", 5.0f);
	fZDepth = j.value("Z Depth", -5.0f);

	std::string strAxis = j.value("Axis", "");

	iAxisIndex = -1;
	if (strAxis == "X")
		iAxisIndex = 0;
	else if (strAxis == "Y")
		iAxisIndex = 1;
}