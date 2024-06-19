#include "Platforms/Platforms.h"

#include <imgui.h>

#include <ImGuiFileDialog.h>

#include "Clout.h"
#include "Helpers.h"
#include "Comms.h"
#include "Console.h"

#include "CloutProgram.h"
#include "OperationQueue.h"

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
		return;

	CloutProgram_Op &op = GetOp(0);

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

	bool bDisabled = bIsRunning;	//This can't be updated in the middle of the draw or we'll corrupt the BeginDisabled/EndDisabled stack

	ImGui::Begin("Queue");

	//Control
		ImGui::SeparatorText("Control");

		if (bDisabled)
			ImGui::BeginDisabled();

		//Load button
			if (ImGui::Button("Load##Queue"))
			{
				//Setup the file dialog
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.flags = ImGuiFileDialogFlags_Modal;
				const char* filters = "Clout Program (*.clout){.clout},All files (*.*){.*}";
				GuiFileDialog->OpenDialog("OpQueueLoadFileDlgKey", "Load File", filters, config);
			}

			// Show the File dialog
			ImVec2 MinSize(ScaledByWindowScale(750, 450));
			if (GuiFileDialog->Display("OpQueueLoadFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
			{
				if (GuiFileDialog->IsOk())
				{
					CloutProgram Prog(GuiFileDialog->GetFilePathName().c_str());
					AddProgramToQueue(Prog);
				}

				// Close it
				GuiFileDialog->Close();
			}

		//Run button
			if (ImGui::Button("Run##Queue"))
				Start();

		if (bDisabled)
			ImGui::EndDisabled();

		ImGui::SameLine();

		if (!bDisabled)
			ImGui::BeginDisabled();

		ImGui::Button("Pause##Queue");

		if (!bDisabled)
			ImGui::EndDisabled();

	//Operations list
		ImGui::SeparatorText("Operation List");

	//Draw each item
		if (ImGui::BeginTable("table_OpsList", 1, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ScaledByWindowScale(0.0, 15));

			for (x = 0; x < Ops.size(); x++)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				
				sprintf(szString, "%s##OpsList%d", GetOp(x).FullText.c_str(), x);
				ImGui::Selectable(szString);
			}

			ImGui::PopStyleVar();

			ImGui::EndTable();
		}

	//All done
		ImGui::End();
}

void _OperationQueue::AddOpToQueue(CloutProgram_Op_Datatypes& NewOp)
{
	Ops.push_back(NewOp);
}

void _OperationQueue::AddProgramToQueue(CloutProgram& Program)
{
	/*std::for_each(Program.Ops.begin(), Program.Ops.end(), [this](const CloutProgram_Op_Datatypes NewOp)
	{ 
		Ops.push_back(NewOp);
	});*/

	for (int x = 0; x < Program.Ops.size(); x++)
	{
		Ops.push_back(Program.Ops.at(x));
	}
}
