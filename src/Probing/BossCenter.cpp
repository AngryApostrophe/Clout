#include "../Platforms/Platforms.h"

#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

#include "../Resources/ProbeBoss.h"
static GLuint imgProbeBossCenter = 0;	//Only load it once, rather than every time we open a window


ProbeOperation_BossCenter::ProbeOperation_BossCenter()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bProbingType = PROBE_OP_TYPE_BOSS;

	szWindowIdent = "BossCenter";

	if (imgProbeBossCenter == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeBoss_compressed_data, ProbeBoss_compressed_size, &imgProbeBossCenter);
	imgPreview = imgProbeBossCenter;

	fBossDiameter = 25.0f;
	fClearance = 5.0f;
	fOvertravel = 5.0f;
	fZDepth = -10.0f;
}


void ProbeOperation_BossCenter::StateMachine()
{
	char sCmd[50];
	CarveraMessage msg;
	int iRet;
	DOUBLE_XYZ xyz;

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
		iState = PROBE_STATE_BOSSCENTER_TO_MIN_X;
		break;

		case PROBE_STATE_BOSSCENTER_TO_MIN_X:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", (fBossDiameter / 2.0f) + fClearance, iProbingSpeedFast); //Move to the min x.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;


		case PROBE_STATE_BOSSCENTER_LOWER_MIN_X:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_X_FAST:
			sprintf(sCmd, "G38.3 X%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe in towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_X_SLOW:
			sprintf(sCmd, "G38.5 X-%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the left, at slow speed
			RunProbeStep(sCmd, &ProbePos1);
		break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MIN_X:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance x pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_RAISE_MIN_X:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_START_X:
			sprintf(sCmd, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			xyz = { StartPos.x,  MachineStatus.Coord.Working.y,  MachineStatus.Coord.Working.z }; //We only really care about X
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_TO_MAX_X:
			sprintf(sCmd, "G38.3 X%0.2f F%d", (fBossDiameter / 2.0f) + fClearance, iProbingSpeedFast); //Move to the max x.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_LOWER_MAX_X:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_X_FAST:
			sprintf(sCmd, "G38.3 X-%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe in towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_X_SLOW:
			sprintf(sCmd, "G38.5 X%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the right, at slow speed
			RunProbeStep(sCmd, &ProbePos2);
		break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MAX_X:
			sprintf(sCmd, "G38.3 X%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance x pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_RAISE_MAX_X:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_CENTER_X:
			sprintf(sCmd, "G0 X%0.2f F%d", StartPos.x, iProbingSpeedFast); //All the way back to center
			xyz = { StartPos.x,  MachineStatus.Coord.Working.y,  MachineStatus.Coord.Working.z }; //We only really care about X
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_TO_MIN_Y:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", (fBossDiameter / 2.0f) + fClearance, iProbingSpeedFast); //Move to the min y.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_LOWER_MIN_Y:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_FAST:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe forward towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MIN_Y_SLOW:
			sprintf(sCmd, "G38.5 Y-%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off to the rear, at slow speed
			RunProbeStep(sCmd, &ProbePos3);
		break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MIN_Y:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance y pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_RAISE_MIN_Y:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_START_Y:
			sprintf(sCmd, "G0 Y%0.2f F%d", StartPos.y, iProbingSpeedFast); //All the way back to center
			xyz = { MachineStatus.Coord.Working.x,  StartPos.y,  MachineStatus.Coord.Working.z }; //We only really care about Y
			RunMoveStep(sCmd, xyz);
		break;

		case PROBE_STATE_BOSSCENTER_TO_MAX_Y:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", (fBossDiameter / 2.0f) + fClearance, iProbingSpeedFast); //Move to the max y.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_LOWER_MAX_Y:
			sprintf(sCmd, "G38.3 Z%0.2f F%d", fZDepth, iProbingSpeedFast); //Lower to the probing depth.  Treat it as a probe just in case we hit something we shouldn't
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_FAST:
			sprintf(sCmd, "G38.3 Y-%0.2f F%d", fClearance + fOvertravel, iProbingSpeedFast); //Probe rearwards towards the center
			RunProbeStep(sCmd);
		break;

		case PROBE_STATE_BOSSCENTER_PROBE_MAX_Y_SLOW:
			sprintf(sCmd, "G38.5 Y%0.2f F%d", fOvertravel, iProbingSpeedSlow); //Back off forward, at slow speed
			RunProbeStep(sCmd, &ProbePos4);
		break;

		case PROBE_STATE_BOSSCENTER_CLEAR_MAX_Y:
			sprintf(sCmd, "G38.3 Y%0.2f F%d", fClearance, iProbingSpeedFast); //Move to clearance y pos
			RunProbeStep(sCmd, 0, true);
		break;

		case PROBE_STATE_BOSSCENTER_RAISE_MAX_Y:
			sprintf(sCmd, "G0 Z%0.2f F%d", StartPos.z, iProbingSpeedFast); //Raise back up to the start Z
			xyz = { MachineStatus.Coord.Working.x,  MachineStatus.Coord.Working.y,  StartPos.z }; //We only really care about Z
			RunMoveStep(sCmd, xyz); 
		break;

		case PROBE_STATE_BOSSCENTER_CENTER:
			//Calculate center in Machine coords
				EndPos.x = (ProbePos1.x + ProbePos2.x) / 2.0;
				EndPos.y = (ProbePos3.y + ProbePos4.y) / 2.0;
				EndPos.z = MachineStatus.Coord.Machine.z; //Not used

			sprintf(sCmd, "G53 G0 X%0.2f Y%0.2f F%d", EndPos.x, EndPos.y, iProbingSpeedFast); //All the way back to center, in MCS
			RunMoveStep(sCmd, EndPos, true);
		break;

		case PROBE_STATE_BOSSCENTER_FINISH:
			EndPos.x = MachineStatus.Coord.Working.x;
			EndPos.y = MachineStatus.Coord.Working.y;

			//Zero the WCS if requested
			if (bZeroWCS)
				ZeroWCS(true, true, false);	//Set this location to 0,0

			sprintf(sCmd, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
			Comms_SendString(sCmd);

			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  Boss center: (%0.03f, %0.03f)", EndPos.x, EndPos.y);
			iState = PROBE_STATE_COMPLETE;
		break;
	}
}


void ProbeOperation_BossCenter::DrawSubwindow()	//Could be drawn in either its own modal dialog or in the program editor
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat(sUnits, "in"); //Inches

	ImGui::Text("Probe outside a boss to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of boss at the clearance Z height");

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ScaledByWindowScale(450.0f)) * 0.5f);	//Center the image in the window
	ImGui::Image((void*)(intptr_t)imgPreview, ScaledByWindowScale(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(ScaledByWindowScale(200));	//Set the width of the textboxes

	//Boss Diameter
		sprintf(sString, "%%0.3f%s", sUnits);
		ImGui::InputFloat("Boss diameter", &fBossDiameter, 0.01f, 0.1f, sString);
		ImGui::SameLine(); HelpMarker("Nominal diameter of the boss, in current machine units.");

	//Clearance Distance
		sprintf(sString, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Clearance distance", &fClearance, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance traveled outside the nominal diameter before lowering and probing back in.");

	//Overtravel Distance
		sprintf(sString, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Overtravel distance", &fOvertravel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance inside the nominal diameter to continue probing before failing.");

	//Z Depth
		sprintf(sString, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Z probing depth", &fZDepth, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("How far to lower the probe when we start measuring.\nNote: Negative numbers are lower.");

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
	x = MachineStatus.WCS;
	if (x < Carvera::CoordSystem::G54)
		x = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

	//The combo box
				//iWCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
	if (iWCSIndex < Carvera::CoordSystem::G54)
		iWCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

	if (ImGui::BeginCombo("##ProbingBossCenter_WCS", szWCSChoices[iWCSIndex]))
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
bool ProbeOperation_BossCenter::DrawPopup()
{
	bool bRetVal = true;	//The operation continues to run

	BeginPopup();

	DrawSubwindow();

	bRetVal = EndPopup();

	StateMachine(); //Run the state machine if an operation is going

	return bRetVal;
}*/