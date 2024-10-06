#include "Platforms/Platforms.h"

#include <imgui.h>

#include <ImGuiFileDialog.h>

#include "Clout.h"
#include "Helpers.h"
#include "Comms.h"
#include "Console.h"

#include "CloutScript.h"
#include "OperationQueue.h"

#include "ScriptEditor.h"

_OperationQueue OperationQueue;  //The one and only queue for Clout


_OperationQueue::_OperationQueue()
{
	Ops.clear();
}

void _OperationQueue::Init()
{
	bIsRunning = 0;
}

void _OperationQueue::Start()
{
	bIsRunning = true;
}

void _OperationQueue::Run()
{
	if (!bIsRunning || Ops.empty())
	{
		bIsRunning = false;
		return;
	}

	CloutScript_Op &op = GetOp(0);

	if (op.iState != STATE_OP_COMPLETE)
		op.StateMachine();

	if (op.iState == STATE_OP_COMPLETE)
	{
		Ops.pop_front(); //Remove this op, since it's complete
	}

	if (Ops.empty())
		bIsRunning = false;
}

void _OperationQueue::DrawList()
{
	int x;
	char szString[50];

	bool bDisabled = bIsRunning || !bCommsConnected;	//This can't be updated in the middle of the draw or we'll corrupt the BeginDisabled/EndDisabled stack

	if (!ImGui::Begin("Queue"))
	{
		ImGui::End();
		return;
	}

	//Control
		ImGui::SeparatorText("Control");

		if (bDisabled)
			ImGui::BeginDisabled();

		//Run button
			if (ImGui::Button("Run##Queue", ScaledByWindowScale(50, 35)))
				Start();

		if (bDisabled)
			ImGui::EndDisabled();

		ImGui::SameLine();

		if (!bDisabled || 1) //This button doesn't do anything yet
			ImGui::BeginDisabled();

		ImGui::Button("Pause##Queue", ScaledByWindowScale(50, 35));

		if (!bDisabled || 1)
			ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::Dummy(ScaledByWindowScale(50.0f, 0.0f)); //Extra empty space between the buttons
		ImGui::SameLine();

		//Load button
			if (ImGui::Button("Load##Queue", ScaledByWindowScale(50, 35)))
			{
				//Setup the file dialog
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.flags = ImGuiFileDialogFlags_Modal;
				const char* filters = "Clout Script (*.clout){.clout},All files (*.*){.*}";
				GuiFileDialog->OpenDialog("OpQueueLoadFileDlgKey", "Load File", filters, config);
			}

			// Show the File dialog
			ImVec2 MinSize(ScaledByWindowScale(750, 450));
			if (GuiFileDialog->Display("OpQueueLoadFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
			{
				if (GuiFileDialog->IsOk())
				{
					CloutScript Script(GuiFileDialog->GetFilePathName().c_str());
					AddScriptToQueue(Script);
				}

				// Close it
				GuiFileDialog->Close();
			}

		ImGui::SameLine();

		ImGui::Dummy(ScaledByWindowScale(10.0f, 0.0f)); //Extra empty space between the buttons
		ImGui::SameLine();

		if (ImGui::Button("Editor", ScaledByWindowScale(50, 35)))
			ImGui::OpenPopup("Script Editor");
		ScriptEditor_Draw();

	//Operations list
		ImGui::SeparatorText("Operation List");

	//Draw each item
		if (ImGui::BeginTable("table_OpsList", 1, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ScaledByWindowScale(0.0, 15));
			ImGui::PushFont(io->Fonts->Fonts[FONT_SELECTION]);			

			for (x = 0; x < Ops.size(); x++)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				
				//Draw a progress bar if this should have one
				if (x == 0 && (GetOp(x).iType == CLOUT_OP_RUN_GCODE_FILE || GetOp(x).iType == CLOUT_OP_CUSTOM_GCODE))
				{
					ImVec2 curpos = ImGui::GetCursorScreenPos();	//Save current screen location
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg)));	//Bar color to blue
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32( ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)  ));	//Background to black
					ImGui::ProgressBar(MachineStatus.Playing.iPercentComplete/100.0f, ImVec2(ImGui::GetColumnWidth(),ScaledByWindowScale(18)), "");	//Draw the progress
					ImGui::PopStyleColor(2);
					ImGui::SetNextItemAllowOverlap();
					ImGui::SetCursorScreenPos(curpos);
				}

				//If we're in the future, make it gray
				if (x > 0)
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ScaledByWindowScale(3.0f)); //Bump it right just a bit to make room for that progress bar above
				sprintf(szString, "%s##OpsList%d", GetOp(x).FullText.c_str(), x);
				ImGui::Selectable(szString);

				if (x > 0)
					ImGui::PopStyleColor(); //ImGuiCol_Text
			}

			ImGui::PopFont();
			ImGui::PopStyleVar();

			ImGui::EndTable();
		}

	//All done
		ImGui::End();
}

void _OperationQueue::AddOpToQueue(CloutScript_Op_Datatypes& NewOp)
{
	Ops.push_back(NewOp);
}

void _OperationQueue::AddScriptToQueue(CloutScript& Script)
{
	/*std::for_each(Program.Ops.begin(), Program.Ops.end(), [this](const CloutProgram_Op_Datatypes NewOp)
	{ 
		Ops.push_back(NewOp);
	});*/

	for (int x = 0; x < Script.Ops.size(); x++)
	{
		Ops.push_back(Script.Ops.at(x));
	}
}
