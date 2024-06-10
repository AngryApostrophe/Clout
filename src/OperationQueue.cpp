#include "Platforms/Platforms.h"

#include <imgui.h>

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
	if (!bIsRunning)
		return;
}

void _OperationQueue::DrawList()
{
	int x;
	char szString[50];

	ImGui::Begin("Queue");

	//Control
		ImGui::SeparatorText("Control");

		if (bIsRunning)
			ImGui::BeginDisabled();

		if (ImGui::Button("Run"))
			Start();

		if (bIsRunning)
			ImGui::EndDisabled();

		ImGui::SameLine();

		if (!bIsRunning)
			ImGui::BeginDisabled();

		ImGui::Button("Pause");

		if (!bIsRunning)
			ImGui::EndDisabled();

	//Operations list
		ImGui::SeparatorText("Operation List");

	//Draw each item
		if (ImGui::BeginTable("table_OpsList", 1, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 15));

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
	std::for_each(Program.Ops.begin(), Program.Ops.end(), [this](const CloutProgram_Op_Datatypes NewOp)
	{ 
		Ops.push_back(NewOp);
	});
}
