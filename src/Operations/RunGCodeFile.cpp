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

CloutProgram_Op_Run_GCode_File::CloutProgram_Op_Run_GCode_File()
{
	sFilename.clear();
	iStartLineNum = 0;
	iLastLineNum = 0;
	sGCode_Line.clear();
}

void CloutProgram_Op_Run_GCode_File::StateMachine()
{
}

void CloutProgram_Op_Run_GCode_File::DrawDetailTab()
{
	int x;

	//Description
	ImGui::Text("Run all, or a portion, of an existing G Code file");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::InputText("Filename##RunGCodeFile", &sFilename);
	ImGui::SameLine();
	if (ImGui::Button("Open...##RunGCodeFile"))
	{
		//Setup the file dialog
		IGFD::FileDialogConfig config;
		config.path = ".";
		config.flags = ImGuiFileDialogFlags_Modal;
		const char* filters = "G Code (*.nc *.cnc *.gcode){.nc,.cnc,.gcode},All files (*.*){.*}";
		GuiFileDialog->OpenDialog("ChooseFileDlgKey", "Choose File", filters, config);
	}

	// Show the File dialog
	ImVec2 MinSize(750, 450);
	if (GuiFileDialog->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
	{
		if (GuiFileDialog->IsOk())
		{
			sFilename = GuiFileDialog->GetFilePathName();

			//Read in the file
			std::ifstream file(sFilename);
			if (file.is_open())
			{
				std::string line;
				while (getline(file, line))
				{
					sGCode_Line.push_back(line);
				}
				file.close();
			}
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error loading file");
			}
		}

		// Close it
		GuiFileDialog->Close();
	}



	ImGui::InputInt("Start Line##RunGCodeFile", &iStartLineNum);
	HelpMarker("First line in this G Code file to begin executing.");

	ImGui::InputInt("Final Line##RunGCodeFile", &iLastLineNum);
	HelpMarker("Final line in this G Code file to execute before moving on.");

	const ImVec2 TextViewSize(475, 500);
	const float fLineNumWidth = 35.0f;
	if (ImGui::BeginTable("table_RunGCodeFile", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, TextViewSize))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fLineNumWidth);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TextViewSize.x - fLineNumWidth);

		for (x = 0; x < sGCode_Line.size(); x++)
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%d", x);

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", sGCode_Line.at(x).c_str());
		}

		ImGui::EndTable();
	}
}

void CloutProgram_Op_Run_GCode_File::DrawEditorSummaryInfo()
{
	ImGui::Text("Filename:  %s", sFilename.c_str());
	ImGui::Text("Start Line:  %d", iStartLineNum);
	ImGui::Text("Last Line:  %d", iLastLineNum);
}

void CloutProgram_Op_Run_GCode_File::ParseFromJSON(const json& j)
{
	iStartLineNum = j.value("Start", -1);
	iLastLineNum = j.value("End", -1);
	sFilename = j.value("Filename", "");
	sGCode_Line.clear();
}