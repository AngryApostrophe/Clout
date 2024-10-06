#include "../Platforms/Platforms.h"

#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutScript.h"


#define STATE_HOMEXY_START		0
#define STATE_HOMEXY_STARTING	1
#define STATE_HOMEXY_RUNNING	2


CloutScript_Op_HomeXY::CloutScript_Op_HomeXY()
{
	bRemoveOffsets = false;
}

void CloutScript_Op_HomeXY::StateMachine()
{
	switch (iState)
	{
		case STATE_HOMEXY_START:
			Comms_SendString("$H"); //Send the command
			iState = STATE_HOMEXY_STARTING;
		break;

		case STATE_HOMEXY_STARTING:
			if (MachineStatus.Status == Carvera::Status::Homing)
				iState = STATE_HOMEXY_RUNNING;
		break;

		case STATE_HOMEXY_RUNNING:
			if (MachineStatus.Status == Carvera::Status::Idle)
				iState = STATE_OP_COMPLETE;
		break;
	}
}

void CloutScript_Op_HomeXY::DrawDetailTab()
{
	//Description
	ImGui::Text("Home the X&Y axis");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::BeginDisabled();
	ImGui::Checkbox("Remove WCS Offsets##DrawTab_HomeXY", &bRemoveOffsets);
	HelpMarker("If selected, all WCS offsets will be cleared and 0,0 will be placed back on bottom left corner");
	ImGui::EndDisabled();
}

void CloutScript_Op_HomeXY::DrawEditorSummaryInfo()
{
}

void CloutScript_Op_HomeXY::ParseFromJSON(const json& j)
{
	CloutScript_Op::ParseFromJSON(j);

	bRemoveOffsets = j.value("Remove Offsets", false);
}

void CloutScript_Op_HomeXY::ParseToJSON(json& j)
{
	CloutScript_Op::ParseToJSON(j);

	j["Remove Offsets"] = bRemoveOffsets;
}