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

CloutProgram_Op_ATC_Tool_Change::CloutProgram_Op_ATC_Tool_Change()
{
	iNewTool = -99;
}

void CloutProgram_Op_ATC_Tool_Change::StateMachine()
{
}

void CloutProgram_Op_ATC_Tool_Change::GenerateFullTitle()
{
	FullText.clear();
	FullText = "ATC Tool Change:  ";

	if (iNewTool > 0)
		FullText += "T" + iNewTool;
	else if (iNewTool == 0)
		FullText += "Wireless Probe";
	else if (iNewTool == -1)
		FullText += "<Empty>";
};

void CloutProgram_Op_ATC_Tool_Change::DrawDetailTab()
{
	//Description
	ImGui::Text("Use the Automatic Tool Changer to switch to a new tool");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	//New tool
	const char szToolChoices[][15] = { "Empty", "Wireless Probe", "1", "2", "3", "4", "5", "6" };
	if (ImGui::BeginCombo("New Tool##ChangeTool", szToolChoices[iNewTool + 1]))
	{
		for (int x = 0; x < 8; x++)
		{
			//Add the item
			const bool is_selected = ((iNewTool + 1) == x);
			if (ImGui::Selectable(szToolChoices[x], is_selected))
				iNewTool = x - 1;

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