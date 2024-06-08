#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

ProbeOperation_WebCenter::ProbeOperation_WebCenter()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bProbingType = PROBE_OP_TYPE_WEB;

	szWindowIdent = "WebCenter";

	imgPreview[0] = 0;
	imgPreview[1] = 0;
	LoadPreviewImage(&imgPreview[0], "./res/ProbeWebX.png");
	LoadPreviewImage(&imgPreview[1], "./res/ProbeWebY.png");
	
	iAxisIndex = 0;
	fWebWidth = 15.0f;
	fClearance = 5.0f;
	fOvertravel = 5.0f;
	fZDepth = -10.0f;
}

void ProbeOperation_WebCenter::StateMachine()
{
}


void ProbeOperation_WebCenter::DrawSubwindow()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	ImGui::Text("Probe outside a web feature to find the center");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be near center of feature at the clearance Z height");


	ImGui::Image((void*)(intptr_t)imgPreview[iAxisIndex], ImVec2(450, 342));

	//ImGui::Separator();

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(200);	//Set the width of the textboxes

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
		sprintf_s(sString, 10, "%%0.3f%s", sUnits);
		ImGui::InputFloat("Feature width##WebCenter", &fWebWidth, 0.01f, 0.1f, sString);
		ImGui::SameLine(); HelpMarker("Nominal width of the feature, in current machine units.");

	//Clearance Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Clearance distance##WebCenter", &fClearance, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance traveled outside the nominal width before lowering and lowering and probing back in.");

	//Overtravel Distance
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Overtravel distance##WebCenter", &fOvertravel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Distance inside the nominal width to continue probing before failing.");

	//Z Depth
		sprintf_s(sString, 10, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Z probing depth##WebCenter", &fZDepth, 0.1f, 1.0f, sString);
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
/*
bool ProbeOperation_WebCenter::DrawPopup()
{
	bool bRetVal = TRUE;	//The operation continues to run

	BeginPopup();

	DrawSubwindow();

	bRetVal = EndPopup();

	StateMachine(); //Run the state machine if an operation is going

	return bRetVal;
}
*/