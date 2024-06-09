#include <Windows.h>
#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutProgram.h"

CloutProgram_Op_InstallTouchProbe::CloutProgram_Op_InstallTouchProbe()
{
	bConfirmFunction = TRUE;
}

void CloutProgram_Op_InstallTouchProbe::StateMachine()
{
}

void CloutProgram_Op_InstallTouchProbe::DrawDetailTab()
{
	//Description
	ImGui::Text("Work with the user to install a touch probe");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::Checkbox("Confirm function##DrawTab_InstallTouchProbe", &bConfirmFunction);
	HelpMarker("If selected, after installation of the probe you will be asked to touch the probe to confirm it works.");
}

void CloutProgram_Op_InstallTouchProbe::DrawEditorSummaryInfo()
{
	ImGui::Text("Confirm function: %s", (bConfirmFunction ? "Yes" : "No"));
}

void CloutProgram_Op_InstallTouchProbe::ParseFromJSON(const json& j)
{
	bConfirmFunction = j.value("Confirm_Function", true);
}