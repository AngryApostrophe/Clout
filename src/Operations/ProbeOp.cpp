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



CloutProgram_Op_ProbeOp::CloutProgram_Op_ProbeOp()
{
	ProbeOp.reset();
}

void CloutProgram_Op_ProbeOp::Change_ProbeOp_Type(int iNewType)
{
	Probing_InstantiateNewOp(ProbeOp, iNewType);
}

void CloutProgram_Op_ProbeOp::StateMachine()
{
}

void CloutProgram_Op_ProbeOp::DrawDetailTab()
{
	int iNewProbeOpType = -1;

	if (ProbeOp == 0)
		return;

	//Description
		ImGui::Text("Run a probe operation");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	//Probe op type
		if (ImGui::BeginCombo("Op Type##ProbeOp", szProbeOpNames[ProbeOp->bProbingType]))
		{
			for (int x = 0; x < szProbeOpNames.size(); x++)
			{
				//Add the item
				const bool is_selected = ((ProbeOp->bProbingType) == x);
				if (ImGui::Selectable(szProbeOpNames[x], is_selected))
				{
					if (ProbeOp->bProbingType != x)
						iNewProbeOpType = x;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
		HelpMarker("Select which type of probe operation to run.");

	ImGui::Separator();

	//Draw the subwindow for this probe op
		ProbeOp->DrawSubwindow();

	//Change this probe operation if requested
		if (iNewProbeOpType != -1)
			Change_ProbeOp_Type(iNewProbeOpType);
}

void CloutProgram_Op_ProbeOp::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_ProbeOp::ParseFromJSON(const json& j)
{
	ProbeOp.reset();

	//Read the operation type
		int iProbeOpType = 0;
		std::string ProbeOpType = j.value("Probe Op Type", "");

	//Make sure it's in our list
		while (iProbeOpType < szProbeOpNames.size())	//Loop through all the available types and find this one
		{
			if (_stricmp(ProbeOpType.c_str(), szProbeOpNames[iProbeOpType]) == 0)
				break;
			iProbeOpType++;
		}
		if (iProbeOpType >= szProbeOpNames.size())	//If we didn't find it, default back to -1
		{
			iProbeOpType = -1;
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Read invalid probe operation of type: %s", ProbeOpType.c_str());
		}

	//TODO: If not found, abort this operation and alert user

	//Create the ProbeOp object that has all the data
		Probing_InstantiateNewOp(ProbeOp, iProbeOpType);
}