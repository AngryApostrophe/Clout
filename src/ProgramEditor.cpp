#include "Platforms/Platforms.h"

#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "Clout.h"
#include "Helpers.h"
#include "Console.h"
#include "CloutProgram.h"
#include "Probing/Probing.h"



CloutProgram EditorProg;	//The program we're currently editing


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
				CloutProgram_Op& ActiveOp = EditorProg.GetOp(iActive);

				ActiveOp.bEditorExpanded = !ActiveOp.bEditorExpanded; //Toggle this page

				if (ActiveOp.bEditorExpanded) //If we opened it, close all the others
				{
					for (x = 0; x < EditorProg.Ops.size(); x++)
					{
						if (x != iActive)
							EditorProg.GetOp(x).bEditorExpanded = false;
					}
				}
				
				bTriggerToggle = false;
			}

		//Check if we're hoving over the table but not an item
			if (ImGui::IsWindowHovered())
				iHovered = -1;

		//List of operations
			static int iClickedDelete = -1;

			for (x = 0; x < EditorProg.Ops.size(); x++)
			{
				CloutProgram_Op &BaseOp = EditorProg.GetOp(x);

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ScaledByWindowScale(0.0, OP_TITLE_PADDING));
			
				ImGui::SetNextItemOpen(BaseOp.bEditorExpanded);
				//ImGui::SetNextItemOpen(true);
			
				bool b = BaseOp.bKeepItem || (x == iClickedDelete); //Show the item if it's armed for deletion, this is different from ImGui style
				
				char szName[50];
				sprintf(szName, "%s##%d", strOperationName[BaseOp.iType], x);

				if (ImGui::CollapsingHeader(szName, &b))
				//if (ImGui::Selectable(strOperationName[BaseOp.iType]))
				{
					ImGui::PopStyleVar();	

					//Keep track of active/hovered for mouse drag later.  This also has to go inside because IsItemActive() doesn't work outside of here if the header is open
						if (ImGui::IsItemActive())
							iActive = x;
						if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
							iHovered = x;
				
					//Show the summary info
						BaseOp.DrawEditorSummaryInfo();
				}
				else
					ImGui::PopStyleVar();

				//Arm it for deletion
					if (!b && iClickedDelete == -1)
					{
						ImGui::OpenPopup("Delete Operation?");
						iClickedDelete = x;
					}

				//Keep track of active/hovered for mouse drag later
					if (ImGui::IsItemActive())
						iActive = x;
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
						iHovered = x;
			}

		//Remove an item
				if (ImGui::BeginPopupModal("Delete Operation?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Are you sure you wish to delete this operation?\n\nThis cannot cannot be undone!\n");
					ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space before the separator
					ImGui::Separator();
					ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space before the buttons

					if (ImGui::Button("OK", ScaledByWindowScale(120, 0)))
					{
						EditorProg.Ops.erase(EditorProg.Ops.begin() + iClickedDelete);

						if (iActive == iClickedDelete) //If we deleted the one we're viewing, "unview" it
							iActive = -1;

						ImGui::CloseCurrentPopup(); 
						iClickedDelete = -1;
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					
					if (ImGui::Button("Cancel", ScaledByWindowScale(120, 0)))
					{ 
						ImGui::CloseCurrentPopup();
						EditorProg.GetOp(iClickedDelete).bKeepItem = true;
						iClickedDelete = -1;
					}
					
					ImGui::EndPopup();
				}

		//Drag to reorder
			if (iActive != -1 && iActive != iHovered && iHovered != -1)
			{
				int iHigherItem = -1;
				if (ImGui::GetMouseDragDelta(0).y < -OP_TITLE_PADDING) //We're dragging up, so the next one is higher. 
				{
					iHigherItem = iActive;
					iActive--;
					iHovered--;
				}
				else if (ImGui::GetMouseDragDelta(0).y > OP_TITLE_PADDING) //We're dragging down, so the next one is higher
				{
					iHigherItem = iActive + 1;
					iActive++;
					iHovered++;
				}

				if (iHigherItem >= 0 && iHigherItem < EditorProg.Ops.size())
				{
					EditorProg.MoveOperationUp(iHigherItem);
					ImGui::ResetMouseDragDelta();
					bHaveDragged = true;
				}
			}

			//Arm the mouse click event for next time through
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && bHaveDragged)
					bTriggerToggle = false; //Don't toggle the view on this item when we release
				else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && bHaveDragged)
					bHaveDragged = false;
				if (iActive != -1 && iHovered != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !bTriggerToggle && !bHaveDragged && iClickedDelete == -1)
				{
					bTriggerToggle = true;
					iActive = iHovered;
				}

		//Add Operation button
			ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f));

			if (ImGui::Button("Add Operation##ProgramEditor", ScaledByWindowScale(120, 0)))
				ImGui::OpenPopup("popup_add_operation");

			if (ImGui::BeginPopup("popup_add_operation"))
			{
				for (x = 0; x < strOpsSections.size(); x++)
				{
					ImGui::SeparatorText(strOpsSections[x].c_str());
					
					for (int y = 0; y < OrganizedOps[x].size(); y++)
					{
						if (ImGui::Selectable(strOperationName[OrganizedOps[x][y]]))
						{
							EditorProg.AddNewOperation(OrganizedOps[x][y]);
						}
					}
				}

				/*
				ImGui::SeparatorText("Add Operation");
				for (x = 1; x < strOperationName.size(); x++)
				{
					if (ImGui::Selectable(strOperationName[x]))
					{
						EditorProg.AddNewOperation(x);
					}
				}*/
				ImGui::EndPopup();
			}

		
		//Draw the editor tab
			ImGui::TableSetColumnIndex(1);

			ImGui::BeginChild("ProgramEditorChild");

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
				iHovered = -1;
			
			if (iActive != -1)
				EditorProg.GetOp(iActive).DrawDetailTab();

			ImGui::EndChild();
		
		ImGui::EndTable();

		//ImGui::Text("Active:  %d", iActive);
		//ImGui::Text("Hovered:  %d", iHovered);

		

		//Load Program
			if (ImGui::Button("Load Program##ProgramEditor", ScaledByWindowScale(120, 0)))
			{
				//Setup the file dialog
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.flags = ImGuiFileDialogFlags_Modal;
				const char* filters = "Clout Program (*.clout){.clout},All files (*.*){.*}";
				GuiFileDialog->OpenDialog("ProgramEditorLoadFileDlgKey", "Load File", filters, config);
			}

			// Show the File dialog
			ImVec2 MinSize(ScaledByWindowScale(750, 450));
			if (GuiFileDialog->Display("ProgramEditorLoadFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
			{
				if (GuiFileDialog->IsOk())
				{
					EditorProg.LoadFromFile(GuiFileDialog->GetFilePathName().c_str());
				}

				// Close it
				GuiFileDialog->Close();
			}

		ImGui::SameLine();
		

		//Save Program
			if (ImGui::Button("Save Program##ProgramEditor", ScaledByWindowScale(120, 0)))
			{
				//Setup the file dialog
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
				const char* filters = "Clout Program (*.clout){.clout},All files (*.*){.*}";
				GuiFileDialog->OpenDialog("ProgramEditorSaveFileDlgKey", "Save File", filters, config);
			}

			// Show the File dialog
				if (GuiFileDialog->Display("ProgramEditorSaveFileDlgKey", ImGuiWindowFlags_NoCollapse, MinSize))
				{
					if (GuiFileDialog->IsOk())
					{
						EditorProg.SaveToFile(GuiFileDialog->GetFilePathName().c_str());
					}

					// Close it
					GuiFileDialog->Close();
				}
	}


	ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Exit##ProgramEditor", ScaledByWindowScale(120, 0)))
	{
		ImGui::CloseCurrentPopup();
	}


	ImGui::EndPopup();
}