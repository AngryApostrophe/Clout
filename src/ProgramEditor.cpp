#include "Platforms/Platforms.h"

#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "Clout.h"
#include "Helpers.h"
#include "Console.h"
#include "CloutProgram.h"
#include "Probing\Probing.h"



CloutProgram prog;	//Simple program for testing purposes only


#define OP_TITLE_PADDING	7.0f	//This effects the size of the operations in the list


void ProgramEditor_Draw()
{
	static int iActive = -1;
	static int iHovered = -1;
	static bool bTriggerToggle = false;
	static bool bHaveDragged = false; //True if we have dragged an item this mouse cycle

	int x;

	if (!ImGui::BeginPopupModal("Program Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		return;
	
	const ImVec2 TableSize = ScaledByWindowScale(800, 700);
	if (ImGui::BeginTable("table_ProgramEditor", 2 , ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, TableSize))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(300));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(500));

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		//Click on item to open/close.  This needs to go before we update it for this draw cycle
			if (iActive != -1 && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && bTriggerToggle) //We release a left mouse click
			{
				CloutProgram_Op& ActiveOp = prog.GetOp(iActive);

				ActiveOp.bEditorExpanded = !ActiveOp.bEditorExpanded; //Toggle this page

				if (ActiveOp.bEditorExpanded) //If we opened it, close all the others
				{
					for (x = 0; x < prog.Ops.size(); x++)
					{
						if (x != iActive)
							prog.GetOp(x).bEditorExpanded = false;
					}
				}
				
				bTriggerToggle = false;
			}

		//Check if we're hoving over the table but not an item
			if (ImGui::IsWindowHovered())
				iHovered = -1;

		//List of operations
		for (x = 0; x < prog.Ops.size(); x++)
		{
			CloutProgram_Op &BaseOp = prog.GetOp(x);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ScaledByWindowScale(OP_TITLE_PADDING)));
			
			ImGui::SetNextItemOpen(BaseOp.bEditorExpanded);

			if (ImGui::CollapsingHeader(szOperationName[BaseOp.iType]))
			{
				ImGui::PopStyleVar();	

				//Keep track of active/hovered for mouse drag later.  This also has to go inside becuase IsItemActive() doesn't work outside of here if the header is open
					if (ImGui::IsItemActive())
						iActive = x;
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))// | ImGuiHoveredFlags_ChildWindows))
						iHovered = x;
				
				//Show the summary info
					BaseOp.DrawEditorSummaryInfo();
			}
			else
				ImGui::PopStyleVar();

			//Keep track of active/hovered for mouse drag later
				if (ImGui::IsItemActive())
					iActive = x;
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))// | ImGuiHoveredFlags_ChildWindows))
					iHovered = x;
		}

		
		//Drag to reorder
			if (iActive != -1 && iActive != iHovered && iHovered != -1)
			{
				int iHigherItem = -1;
				if (ImGui::GetMouseDragDelta(0).y < -OP_TITLE_PADDING) //We're dragging up, so the next one is higher. 
					iHigherItem = iActive;
				else if (ImGui::GetMouseDragDelta(0).y > OP_TITLE_PADDING) //We're dragging down, so the next one is higher
					iHigherItem = iActive + 1;

				if (iHigherItem >= 0 && iHigherItem < prog.Ops.size())
				{
					prog.MoveOperationUp(iHigherItem);
					ImGui::ResetMouseDragDelta();
					bHaveDragged = true;
				}
			}

			//Arm the mouse click event for next time through
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && bHaveDragged)
				bTriggerToggle = false; //Don't toggle the view on this item when we release
			else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && bHaveDragged)
				bHaveDragged = false;
			if (iActive != -1 && iHovered != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !bTriggerToggle && !bHaveDragged)
				bTriggerToggle = true;

		
		//Draw the editor tab
			ImGui::TableSetColumnIndex(1);

			ImGui::BeginChild("ProgramEditorChild");

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
				iHovered = -1;
			
			if (iActive != -1)
				prog.GetOp(iActive).DrawDetailTab();

			ImGui::EndChild();
		
		ImGui::EndTable();

		ImGui::Text("Active:  %d", iActive);
		ImGui::Text("Hovered:  %d", iHovered);

		if (ImGui::Button("Add Operation##ProgramEditor", ScaledByWindowScale(120, 0)))
		{
		}
	}


	ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Exit##ProgramEditor", ScaledByWindowScale(120, 0)))
	{
		ImGui::CloseCurrentPopup();
	}


	ImGui::EndPopup();
}