#include "../Platforms/Platforms.h"

#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutProgram.h"


#define STATE_RAPIDTO_START		0
#define STATE_RAPIDTO_MOVING	1


CloutProgram_Op_RapidTo::CloutProgram_Op_RapidTo()
{
	CloutProgram_Op();

	bUseAxis[0] = false;
	bUseAxis[1] = false;
	bUseAxis[2] = false;
	Coords = {0,0,0};

	bUseFeedrate = 0;
	fFeedrate = 1000;

	bUseWCS = 0;
	WCS = Carvera::CoordSystem::G54;
}

void CloutProgram_Op_RapidTo::GenerateFullTitle()
{
	char szString[20];

	FullText.clear();
	FullText = "Rapid To:  ";

	if (bUseAxis[0])
	{
		sprintf(szString, "X%0.2f ", Coords.x);
		FullText += szString;
	}
	if (bUseAxis[1])
	{
		sprintf(szString, "Y%0.02f ", Coords.y);
		FullText += szString;
	}
	if (bUseAxis[2])
	{
		sprintf(szString, "Z%0.02f ", Coords.z);
		FullText += szString;
	}
	if (bUseFeedrate)
	{
		sprintf(szString, " @ %0.0f ", fFeedrate);
		FullText += szString;
	}
};

void CloutProgram_Op_RapidTo::StateMachine()
{
	char sCmd[50];
	char sTmp[20];

	switch (iState)
	{
		case STATE_RAPIDTO_START:
			Comms_SendString("G90"); //Ensure we're in absolute positioning
			if (bUseWCS && WCS >= Carvera::CoordSystem::G53)
				Comms_SendString(szWCSNames[WCS]);

			sprintf(sCmd, "G0 ");

			if (bUseAxis[0])
			{
				sprintf(sTmp, "X%0.0f ", Coords.x);
				strcat(sCmd, sTmp);
			}

			if (bUseAxis[1])
			{
				sprintf(sTmp, "Y%0.0f ", Coords.y);
				strcat(sCmd, sTmp);
			}

			if (bUseAxis[2])
			{
				sprintf(sTmp, "Z%0.0f ", Coords.z);
				strcat(sCmd, sTmp);
			}

			if (bUseFeedrate)
			{
				sprintf(sTmp, "F%0.0f", fFeedrate);
				strcat(sCmd, sTmp);
			}

			Comms_SendString(sCmd); //Send the move command
			iState++;
		break;

		case STATE_RAPIDTO_MOVING:
		{
			bool bArrived = true;

			DOUBLE_XYZ CurCoords = MachineStatus.Coord.Working;
			if (WCS == Carvera::CoordSystem::G53)
				CurCoords = MachineStatus.Coord.Machine;

			//Check each active axis to see if we haven't yet arrived
				if (bUseAxis[0] && fabs(CurCoords.x - Coords.x) > 0.1)
					bArrived = false;
				if (bUseAxis[1] && fabs(CurCoords.y - Coords.y) > 0.1)
					bArrived = false;
				if (bUseAxis[2] && fabs(CurCoords.z - Coords.z) > 0.1)
					bArrived = false;

			if (bArrived)
				iState = STATE_OP_COMPLETE;
		}
		break;
	}
}

void CloutProgram_Op_RapidTo::DrawDetailTab()
{
	//Description
	ImGui::Text("G0 - Rapid move to a new location");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::SeparatorText("Coordinates");

	//X
	ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
	ImGui::Text("X: ");
	ImGui::SameLine();
	ImGui::Checkbox("##DrawTab_RapidTo_UseX", &bUseAxis[0]);
	ImGui::SameLine();
	if (!bUseAxis[0])
		ImGui::BeginDisabled();
	ImGui::InputDouble("##DrawTab_RapidTo_CoordX", &Coords.x, 0.1f, 1.0f);
	if (!bUseAxis[0])
		ImGui::EndDisabled();
	HelpMarker("(optional) X coordinate to move to.");

	//Y
	ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
	ImGui::Text("Y: ");
	ImGui::SameLine();
	ImGui::Checkbox("##DrawTab_RapidTo_UseY", &bUseAxis[1]);
	ImGui::SameLine();
	if (!bUseAxis[1])
		ImGui::BeginDisabled();
	ImGui::InputDouble("##DrawTab_RapidTo_CoordY", &Coords.y, 0.1f, 1.0f);
	if (!bUseAxis[1])
		ImGui::EndDisabled();
	HelpMarker("(optional) Y coordinate to move to.");

	//Z
	ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
	ImGui::Text("Z: ");
	ImGui::SameLine();
	ImGui::Checkbox("##DrawTab_RapidTo_UseZ", &bUseAxis[2]);
	ImGui::SameLine();
	if (!bUseAxis[2])
		ImGui::BeginDisabled();
	ImGui::InputDouble("##DrawTab_RapidTo_CoordZ", &Coords.z, 0.1f, 1.0f);
	if (!bUseAxis[2])
		ImGui::EndDisabled();
	HelpMarker("(optional) Z coordinate to move to.");

	ImGui::SeparatorText("Options");

	//Feedrate
	ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
	ImGui::Text("Feedrate: ");
	ImGui::SameLine();
	ImGui::Checkbox("##DrawTab_RapidTo_UseFeedrate", &bUseFeedrate);
	ImGui::SameLine();
	if (!bUseFeedrate)
		ImGui::BeginDisabled();
	ImGui::InputFloat("##DrawTab_RapidTo_Feedrate", &fFeedrate, 0.1f, 1.0f);
	if (!bUseFeedrate)
		ImGui::EndDisabled();
	HelpMarker("(optional) Supply a Feedrate for this movement.");

	//WCS
	ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
	ImGui::Text("WCS: ");
	ImGui::SameLine();
	ImGui::Checkbox("##DrawTab_RapidTo_UseWCS", &bUseWCS);
	ImGui::SameLine();
	if (!bUseWCS)
		ImGui::BeginDisabled();

	//const char szWCSChoices[9][20] = { "Unknown", "G53", "G54", "G55", "G56", "G57", "G58", "G59", "" };
	if (ImGui::BeginCombo("##DrawTab_RapidTo_WCS", szWCSChoices[WCS - 1]))
	{ 
		for (int x = 1; x < 8; x++)
		{
			//Add the item
			const bool is_selected = ((WCS - 1) == x);
			if (ImGui::Selectable(szWCSChoices[x], is_selected))
				WCS = (Carvera::CoordSystem::eCoordSystem)(x + 1);

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	if (!bUseWCS)
		ImGui::EndDisabled();
	HelpMarker("(optional) Which coordinate system to reference.");
}

void CloutProgram_Op_RapidTo::DrawEditorSummaryInfo()
{
	ImGui::Text("Coords:  (%0.03f,  %0.03f,  %0.03f)", Coords.x, Coords.y, Coords.z);

	if (bUseFeedrate)
		ImGui::Text("Feedrate: %0.1f", fFeedrate);

	if (bUseWCS)
		ImGui::Text("WCS: %s", szWCSChoices[WCS - 1]);
}

void CloutProgram_Op_RapidTo::ParseFromJSON(const json& j)
{
	Coords.x = j.value("X", 0.0f);
	Coords.y = j.value("Y", 0.0f);
	Coords.z = j.value("Z", 0.0f);

	bUseAxis[0] = j.value("Use X", false);
	bUseAxis[1] = j.value("Use Y", false);
	bUseAxis[2] = j.value("Use Z", false);

	bUseFeedrate = j.value("Supply Feedrate", false);
	fFeedrate = j.value("Feedrate", 300.0f);

	bUseWCS = j.value("Supply WCS", false);

	std::string strWCS = j.value("WCS", "G54");

	if (strWCS == "G53")
		WCS = Carvera::CoordSystem::G53;
	else if (strWCS == "G54")
		WCS = Carvera::CoordSystem::G54;
	else if (strWCS == "G55")
		WCS = Carvera::CoordSystem::G55;
	else if (strWCS == "G56")
		WCS = Carvera::CoordSystem::G56;
	else if (strWCS == "G57")
		WCS = Carvera::CoordSystem::G57;
	else if (strWCS == "G58")
		WCS = Carvera::CoordSystem::G58;
	else if (strWCS == "G59")
		WCS = Carvera::CoordSystem::G59;

	GenerateFullTitle();
}

void CloutProgram_Op_RapidTo::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["X"] = Coords.x;
	j["Y"] = Coords.y;
	j["Z"] = Coords.z;

	j["Use X"] = bUseAxis[0];
	j["Use Y"] = bUseAxis[1];
	j["Use Z"] = bUseAxis[2];

	j["Supply Feedrate"] = bUseFeedrate;
	j["Feedrate"] = fFeedrate;

	j["Supply WCS"] = bUseWCS;
	j["WCS"] = szWCSNames[WCS];
}