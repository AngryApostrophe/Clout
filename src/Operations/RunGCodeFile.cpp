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

#include "../FileTransfer.h"


#define STATE_RUNFILE_START			0
#define STATE_RUNFILE_UPLOAD		1
#define STATE_RUNFILE_RUNNING		2


static bool bStepRunning;


CloutProgram_Op_Run_GCode_File::CloutProgram_Op_Run_GCode_File()
{
	sFilename.clear();
	iStartLineNum = 0;
	iLastLineNum = -1;
	iState = 0;
	sGCode_Line.clear();
}

void CloutProgram_Op_Run_GCode_File::StateMachine()
{
	switch (iState)
	{
		case STATE_RUNFILE_START:
			ReadFromFile();
			FileTransfer_BeginUpload(sFilename.c_str(), iStartLineNum, iLastLineNum, true);
			iState = STATE_RUNFILE_UPLOAD;
		break;

		case STATE_RUNFILE_UPLOAD:
			if (FileTransfer_DoTransfer())
			{
				bStepRunning = false;
				iState = STATE_RUNFILE_RUNNING;
				Comms_SendString("play sd/gcodes/Clout.nc -v");	//Start running that file
			}
		break;

		case STATE_RUNFILE_RUNNING:
			if (!bStepRunning)
			{
				//Wait for it to begin playing
				if (Comms_PopMessageOfType(CARVERA_MSG_PLAYING))
					bStepRunning = true;
			}
			else
			{
				//Now wait for it to go idle
				if (Comms_PopMessageOfType(CARVERA_MSG_CLOUTSYNC))
					iState = STATE_OP_COMPLETE;
				break;
			}
		break;
		
		/*
		* This old way uses M28/M29 to save to a file.  It's easy but much slower.  It may still be useful in other circumstances so I'm leaving it here for now for documentation
		* 
		case STATE_RUNFILE_STARTUPLOAD:
			Comms_SendString("M28 gcodes/Clout.nc");	//Start writing file
			Comms_HideReply(CARVERA_MSG_OK);
			iState++;
		break;


		case STATE_RUNFILE_UPLOADLINE:
		{
			int count = sGCode_Line.size() - iUploaded;
			if (count > 5)
				count = 5;
			
			for (int x = 0; x < count; x++)
			{
				sentID = Comms_SendString(sGCode_Line[iUploaded+x].c_str(), false);

				//Hide all replies except for the last one
					if (count - x > 1)
						Comms_HideReply(CARVERA_MSG_OK);
			}

			iUploaded += count;

			Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Uploading file: %0.2f%%", ((float)iUploaded/(float)sGCode_Line.size())*100);

			iState++;
		}
		break;

		case STATE_RUNFILE_UPLOADWAIT:
			if (Comms_PopMessageOfType(CARVERA_MSG_OK, 0, sentID))
			{
				if (iUploaded >= sGCode_Line.size())
					iState = STATE_RUNFILE_ENDUPLOAD;
				else
					iState = STATE_RUNFILE_UPLOADLINE;
			}
		break;

		case STATE_RUNFILE_ENDUPLOAD:
			Comms_SendString("M29");	//Close the file
			iState++;
		break;

		*/
	}
}

void CloutProgram_Op_Run_GCode_File::ReadFromFile()
{
	std::ifstream file(sFilename);
	if (file.is_open())
	{
		std::string line;
		while (getline(file, line))
		{
			if (line.size() > 0)
				sGCode_Line.push_back(line);
		}
		file.close();
	}
	else
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error loading file %s", sFilename.c_str());
	}
}

void CloutProgram_Op_Run_GCode_File::DrawDetailTab()
{
	char szString[10];
	int x;

	//Description
	ImGui::Text("Run all, or a portion, of an existing G Code file");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

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
	ImVec2 MinSize(ScaledByWindowScale(750, 450));
	if (GuiFileDialog->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
	{
		if (GuiFileDialog->IsOk())
		{
			sGCode_Line.clear();
			sFilename = GuiFileDialog->GetFilePathName();
			ReadFromFile();			
		}

		// Close it
		GuiFileDialog->Close();
	}

	ImGui::InputInt("Start Line##RunGCodeFile", &iStartLineNum);
	HelpMarker("First line in this G Code file to begin executing.");

	ImGui::InputInt("Final Line##RunGCodeFile", &iLastLineNum);
	HelpMarker("Final line in this G Code file to execute before moving on.");

	const ImVec2 TextViewSize(ScaledByWindowScale(475, 500));
	const float fLineNumWidth = 35.0f;
	if (ImGui::BeginTable("table_RunGCodeFile", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, TextViewSize))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fLineNumWidth);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TextViewSize.x - fLineNumWidth);

		
		for (x = 0; x < sGCode_Line.size(); x++)
		{
			bool bGrayText = false;
			if (iStartLineNum > 0 && x < iStartLineNum)
				bGrayText = true;
			else if (iLastLineNum > -1 && x > iLastLineNum)
				bGrayText = true;

			ImGui::TableNextRow();

			if (bGrayText)
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 100, 100, 255));

			ImGui::TableSetColumnIndex(0);

			//Line number
				sprintf(szString, "%d", x);
				ImVec2 TextSize = ImGui::CalcTextSize(szString);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - TextSize.x));
				ImGui::Text(szString);

			ImGui::TableSetColumnIndex(1);
			ImGui::Text(" %s", sGCode_Line.at(x).c_str());

			if (bGrayText)
				ImGui::PopStyleColor();
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
	iStartLineNum = j.value("Start", 0);
	iLastLineNum = j.value("End", -1);
	sFilename = j.value("Filename", "");
	sGCode_Line.clear();

	if (!sFilename.empty())
		ReadFromFile();
}

void CloutProgram_Op_Run_GCode_File::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["Start"] = iStartLineNum;
	j["End"] = iLastLineNum;
	j["Filename"] = sFilename;
}