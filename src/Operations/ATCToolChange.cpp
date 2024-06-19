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

CloutProgram_Op_ATC_Tool_Change::CloutProgram_Op_ATC_Tool_Change()
{
	iNewTool = -99;
}

void CloutProgram_Op_ATC_Tool_Change::StateMachine()
{
	char sCmd[30];

	switch (iState)
	{
		case STATE_ATCCHANGE_START:
			
			if (iNewTool >= -1 && iNewTool <= 6)
			{
				sprintf(sCmd, "M6 T%d", iNewTool);
				Comms_SendString(sCmd);
				iState = STATE_ATCCHANGE_RUNNING;
			}
			else
			{
				//TODO: Error abort
			}
		break;

	case STATE_ATCCHANGE_RUNNING:
		if (Comms_PopMessageOfType(CARVERA_MSG_ATC_DONE))
			iState = STATE_OP_COMPLETE;
		break;
	}
}

void CloutProgram_Op_ATC_Tool_Change::GenerateFullTitle()
{
	FullText.clear();
	FullText = "ATC Tool Change: ";

	if (iNewTool > 0)
	{
		FullText += "T" + std::to_string(iNewTool);
	}
	else if (iNewTool == 0)
		FullText += "Wireless Probe";
	else if (iNewTool == -1)
		FullText += "<Empty>";
};

void CloutProgram_Op_ATC_Tool_Change::DrawDetailTab()
{
	//Description
		ImGui::Text("Use the Automatic Tool Changer to switch to a new tool");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	//New tool
	const char szToolChoices[][15] = { "Empty", "Wireless Probe", "1", "2", "3", "4", "5", "6" };
	if (ImGui::BeginCombo("New Tool##ChangeTool", szToolChoices[iNewTool + 1]))
	{
		for (int x = 0; x < 8; x++)
		{
			//Add the item
			const bool is_selected = ((iNewTool + 1) == x);
			if (ImGui::Selectable(szToolChoices[x], is_selected))
			{
				iNewTool = x - 1;
				GenerateFullTitle();
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	HelpMarker("Select the new tool to change to.");
}

void CloutProgram_Op_ATC_Tool_Change::DrawEditorSummaryInfo()
{
	if (iNewTool > 0)
		ImGui::Text("New Tool:  %d", iNewTool);
	else if (iNewTool == 0)
		ImGui::Text("New Tool:  Wireless Probe");
	else if (iNewTool == -1)
		ImGui::Text("New Tool:  <Empty spindle>");
}

void CloutProgram_Op_ATC_Tool_Change::ParseFromJSON(const json& j)
{
	iNewTool = j.value("Tool", -99);

	GenerateFullTitle();
}

void CloutProgram_Op_ATC_Tool_Change::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["Tool"] = iNewTool;
}