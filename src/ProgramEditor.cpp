#include <Windows.h>
#include <imgui.h>

#include "Clout.h"
#include "Helpers.h"
#include "Console.h"
#include "CloutProgram.h"


CloutProgram prog;

#define OP_TITLE_PADDING	7.0f


void ProgramEditor_Draw()
{
	static int iActive = -1;
	static int iHovered = -1;
	static bool bTriggerToggle = false;
	static bool bHaveDragged = false; //True if we have dragged an item this mouse cycle

	int x;

	if (!ImGui::BeginPopupModal("Program Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		prog.jData.get_to(prog); //TODO: Temp.  This resets the data when window is closed
		return;
		}

	

	if (ImGui::BeginTable("table_ProgramEditor", 2 , ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, ImVec2(800, 700)))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 300);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 500);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		

		//Click on item to open/close.  This needs to go before we update it for this draw cycle
			if (iActive != -1 && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && bTriggerToggle) //We release a left mouse click
			{
				prog.Ops[iActive].bEditorExpanded = !prog.Ops[iActive].bEditorExpanded; //Toggle this page

				if (prog.Ops[iActive].bEditorExpanded) //If we opened it, close all the others
				{
					for (x = 0; x < prog.iOpsCount; x++)
					{
						if (x != iActive)
							prog.Ops[x].bEditorExpanded = false;
					}
				}
				
				bTriggerToggle = false;
			}


		//List of operations
		for (x = 0; x < prog.iOpsCount; x++)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, OP_TITLE_PADDING));
			
			ImGui::SetNextItemOpen(prog.Ops[x].bEditorExpanded);

			if (ImGui::CollapsingHeader(szOperationName[prog.Ops[x].iType]))
			{
				ImGui::PopStyleVar();	

				//Keep track of active/hovered for mouse drag later.  This also has to go inside becuase IsItemActive() doesn't work outside of here if the header is open
					if (ImGui::IsItemActive())
						iActive = x;
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
						iHovered = x;
				
				//Show the info
					switch (prog.Ops[x].iType)
					{
						case CLOUT_OP_RUN_GCODE_FILE:
						{
							CloutProgram_Op_Run_GCode_File &Data = std::any_cast<CloutProgram_Op_Run_GCode_File>(prog.Ops[x].Data);
							ImGui::Text("Filename:  %s", Data.sFilename.c_str());
							ImGui::Text("Start Line:  %d", Data.iStartLineNum);
							ImGui::Text("Last Line:  %d", Data.iLastLineNum);
						}
						break;

						case CLOUT_OP_ATC_TOOL_CHANGE:
						{
							CloutProgram_Op_ATC_Tool_Change &Data = std::any_cast<CloutProgram_Op_ATC_Tool_Change>(prog.Ops[x].Data);
							
							if (Data.iNewTool > 0)
								ImGui::Text("New Tool:  %d", Data.iNewTool);
							else if (Data.iNewTool == 0)
								ImGui::Text("New Tool:  Wireless Probe");
							else if (Data.iNewTool == -1)
								ImGui::Text("New Tool:  <Empty spindle>");
						}
						break;

						case CLOUT_OP_RAPID_TO:
						{
							CloutProgram_Op_Rapid_To& Data = std::any_cast<CloutProgram_Op_Rapid_To>(prog.Ops[x].Data);
							ImGui::Text("Coords:  (%0.03f,  %0.03f,  %0.03f)", Data.Coords.x, Data.Coords.y, Data.Coords.z);
						}
						break;
					}
			}
			else
				ImGui::PopStyleVar();

			//Keep track of active/hovered for mouse drag later
				if (ImGui::IsItemActive())
					iActive = x;
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
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

				if (iHigherItem >= 0 && iHigherItem < prog.iOpsCount)
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
			if (iActive != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !bTriggerToggle && !bHaveDragged)
				bTriggerToggle = true;

		

		
		ImGui::EndTable();

		ImGui::Text("Active:  %d", iActive);
		ImGui::Text("Hovered:  %d", iHovered);

		if (ImGui::Button("Add Operation##ProgramEditor", ImVec2(120, 0)))
		{
		}
	}


	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Exit##ProgramEditor", ImVec2(120, 0)))
	{
		ImGui::CloseCurrentPopup();
	}


	ImGui::EndPopup();
}