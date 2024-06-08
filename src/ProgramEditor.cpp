#include <Windows.h>
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


#define OP_TITLE_PADDING	7.0f


void ProgramEditor_Change_ProbeOp_Type(CloutProgram_Op_ProbeOp& Data, int iNewType)
{
	if (Data.ProbeOp != 0)
	{
		delete Data.ProbeOp;
		Data.ProbeOp = 0;
	}

	Data.iProbeOpType = iNewType;
	Data.ProbeOp = Probing_InstantiateNewOp(iNewType);
}


void ProgramEditor_DrawTab_Empty(CloutProgram_Op &Op)
{
}

void ProgramEditor_DrawTab_ATCToolChange(CloutProgram_Op& Op)
{
	CloutProgram_Op_ATC_Tool_Change& Data = std::any_cast<CloutProgram_Op_ATC_Tool_Change>(Op.Data);

	//Description
		ImGui::Text("Use the Automatic Tool Changer to switch to a new tool");

		ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	//New tool
		const char szToolChoices[][15] = { "Empty", "Wireless Probe", "1", "2", "3", "4", "5", "6" };
		if (ImGui::BeginCombo("New Tool##ChangeTool", szToolChoices[Data.iNewTool+1]))
		{
			for (int x = 0; x < 8; x++)
			{
				//Add the item
				const bool is_selected = ((Data.iNewTool+1) == x);
				if (ImGui::Selectable(szToolChoices[x], is_selected))
					Data.iNewTool = x-1;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
		HelpMarker("Select the new tool to change to.");

	Op.Data = Data;
}

void ProgramEditor_DrawTab_InstallTouchProbe(CloutProgram_Op& Op)
{
	CloutProgram_Op_InstallTouchProbe& Data = std::any_cast<CloutProgram_Op_InstallTouchProbe>(Op.Data);

	//Description
	ImGui::Text("Work with the user to install a touch probe");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::Checkbox("Confirm function##DrawTab_InstallTouchProbe", &Data.bConfirmFunction);
	HelpMarker("If selected, after installation of the probe you will be asked to touch the probe to confirm it works.");

	Op.Data = Data;
}

void ProgramEditor_DrawTab_RapidTo(CloutProgram_Op& Op)
{
	CloutProgram_Op_RapidTo& Data = std::any_cast<CloutProgram_Op_RapidTo>(Op.Data);

	//Description
	ImGui::Text("G0 - Rapid move to a new location");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::SeparatorText("Coordinates");

	//X
		ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
		ImGui::Text("X: ");
		ImGui::SameLine();
		ImGui::Checkbox("##DrawTab_RapidTo_UseX", &Data.bUseAxis[0]);
		ImGui::SameLine();
		if (!Data.bUseAxis[0])
			ImGui::BeginDisabled();
		ImGui::InputDouble("##DrawTab_RapidTo_CoordX", &Data.Coords.x, 0.1f, 1.0f);
		if (!Data.bUseAxis[0])
			ImGui::EndDisabled();
		HelpMarker("(optional) X coordinate to move to.");

	//Y
		ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
		ImGui::Text("Y: ");
		ImGui::SameLine();
		ImGui::Checkbox("##DrawTab_RapidTo_UseY", &Data.bUseAxis[1]);
		ImGui::SameLine();
		if (!Data.bUseAxis[1])
			ImGui::BeginDisabled();
		ImGui::InputDouble("##DrawTab_RapidTo_CoordY", &Data.Coords.y, 0.1f, 1.0f);
		if (!Data.bUseAxis[1])
			ImGui::EndDisabled();
		HelpMarker("(optional) Y coordinate to move to.");

	//Z
		ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
		ImGui::Text("Z: ");
		ImGui::SameLine();
		ImGui::Checkbox("##DrawTab_RapidTo_UseZ", &Data.bUseAxis[2]);
		ImGui::SameLine();
		if (!Data.bUseAxis[2])
			ImGui::BeginDisabled();
		ImGui::InputDouble("##DrawTab_RapidTo_CoordZ", &Data.Coords.z, 0.1f, 1.0f);
		if (!Data.bUseAxis[2])
			ImGui::EndDisabled();
		HelpMarker("(optional) Z coordinate to move to.");

	ImGui::SeparatorText("Options");

	//Feedrate
		ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
		ImGui::Text("Feedrate: ");
		ImGui::SameLine();
		ImGui::Checkbox("##DrawTab_RapidTo_UseFeedrate", &Data.bUseFeedrate);
		ImGui::SameLine();
		if (!Data.bUseFeedrate)
			ImGui::BeginDisabled();
		ImGui::InputFloat("##DrawTab_RapidTo_Feedrate", &Data.fFeedrate, 0.1f, 1.0f);
		if (!Data.bUseFeedrate)
			ImGui::EndDisabled();
		HelpMarker("(optional) Supply a Feedrate for this movement.");

	//WCS
		ImGui::AlignTextToFramePadding(); //Vertical align for the widget coming later
		ImGui::Text("WCS: ");
		ImGui::SameLine();
		ImGui::Checkbox("##DrawTab_RapidTo_UseWCS", &Data.bUseWCS);
		ImGui::SameLine();
		if (!Data.bUseWCS)
			ImGui::BeginDisabled();
	
		//const char szWCSChoices[9][20] = { "Unknown", "G53", "G54", "G55", "G56", "G57", "G58", "G59", "" };
		if (ImGui::BeginCombo("##DrawTab_RapidTo_WCS", szWCSChoices[Data.WCS-1]))
		{
			for (int x = 1; x < 8; x++)
			{
				//Add the item
				const bool is_selected = ((Data.WCS-1) == x);
				if (ImGui::Selectable(szWCSChoices[x], is_selected))
					Data.WCS = (Carvera::CoordSystem::eCoordSystem)(x+1);

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		if (!Data.bUseWCS)
			ImGui::EndDisabled();
		HelpMarker("(optional) Which coordinate system to reference.");




	Op.Data = Data;
}
void ProgramEditor_DrawTab_RunGCodeFile(CloutProgram_Op& Op)
{
	int x;

	CloutProgram_Op_Run_GCode_File& Data = std::any_cast<CloutProgram_Op_Run_GCode_File>(Op.Data);

	//Description
	ImGui::Text("Run all, or a portion, of an existing G Code file");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::InputText("Filename##RunGCodeFile", &Data.sFilename);
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
				Data.sFilename = GuiFileDialog->GetFilePathName();

				//Read in the file
					std::ifstream file(Data.sFilename);
					if (file.is_open())
					{
						std::string line;
						while (getline(file, line))
						{
							Data.sGCode_Line.push_back(line);
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



	ImGui::InputInt("Start Line##RunGCodeFile", &Data.iStartLineNum);
	HelpMarker("First line in this G Code file to begin executing.");

	ImGui::InputInt("Final Line##RunGCodeFile", &Data.iLastLineNum);
	HelpMarker("Final line in this G Code file to execute before moving on.");

	const ImVec2 TextViewSize(475, 500);
	const float fLineNumWidth = 35.0f;
	if (ImGui::BeginTable("table_RunGCodeFile", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, TextViewSize))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fLineNumWidth);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TextViewSize.x - fLineNumWidth);

		for (x = 0; x < Data.sGCode_Line.size(); x++)
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%d", x);

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", Data.sGCode_Line.at(x).c_str());
		}

		ImGui::EndTable();
	}

	Op.Data = Data; //Update it
}

void ProgramEditor_DrawTab_ProbeOp(CloutProgram_Op& Op)
{
	CloutProgram_Op_ProbeOp& Data = std::any_cast<CloutProgram_Op_ProbeOp>(Op.Data);

	//Description
	ImGui::Text("Run a probe operation");

	ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the setup

	//Probe op type
		if (ImGui::BeginCombo("Op Type##ProbeOp", szProbeOpTypes[Data.iProbeOpType]))
		{
			for (int x = 0; x < szProbeOpTypes.size(); x++)
			{
				//Add the item
				const bool is_selected = ((Data.iProbeOpType) == x);
				if (ImGui::Selectable(szProbeOpTypes[x], is_selected))
				{
					if (Data.iProbeOpType != x)
					{
						Data.iProbeOpType = x;
						ProgramEditor_Change_ProbeOp_Type(Data, x);
					}
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
		HelpMarker("Select which type of probe operation to run.");

	ImGui::Separator();

	//Draw the subwindow for this probe op
		if (Data.ProbeOp != 0)
			Data.ProbeOp->DrawSubwindow();

	Op.Data = Data;
}





void (*funcEditorDrawTabs[])(CloutProgram_Op&) = {
ProgramEditor_DrawTab_Empty,				//CLOUT_OP_NULL			0
ProgramEditor_DrawTab_ATCToolChange,		//CLOUT_OP_ATC_TOOL_CHANGE	1
ProgramEditor_DrawTab_RapidTo,			//CLOUT_OP_RAPID_TO			2
ProgramEditor_DrawTab_Empty,				//CLOUT_OP_HOME_XY			3
ProgramEditor_DrawTab_InstallTouchProbe,	//CLOUT_OP_INSTALL_PROBE		4
ProgramEditor_DrawTab_ProbeOp,			//CLOUT_OP_PROBE_OP			5
ProgramEditor_DrawTab_RunGCodeFile,		//CLOUT_OP_RUN_GCODE_FILE	6
ProgramEditor_DrawTab_Empty,				//CLOUT_OP_CUSTOM_GCODE		7
ProgramEditor_DrawTab_Empty				//CLOUT_OP_ALERT_USER		8
};


void ProgramEditor_Draw()
{
	static int iActive = -1;
	static int iHovered = -1;
	static bool bTriggerToggle = false;
	static bool bHaveDragged = false; //True if we have dragged an item this mouse cycle

	int x;

	if (!ImGui::BeginPopupModal("Program Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
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

		//Check if we're hoving over the table but not an item
			if (ImGui::IsWindowHovered())
				iHovered = -1;

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
							CloutProgram_Op_RapidTo& Data = std::any_cast<CloutProgram_Op_RapidTo>(prog.Ops[x].Data);
							ImGui::Text("Coords:  (%0.03f,  %0.03f,  %0.03f)", Data.Coords.x, Data.Coords.y, Data.Coords.z);

							if (Data.bUseFeedrate)
								ImGui::Text("Feedrate: %0.1f", Data.fFeedrate);

							if (Data.bUseWCS)
								ImGui::Text("WCS: %s", szWCSChoices[Data.WCS-1]);
						}
						break;

						case CLOUT_OP_INSTALL_PROBE:
						{
							CloutProgram_Op_InstallTouchProbe& Data = std::any_cast<CloutProgram_Op_InstallTouchProbe>(prog.Ops[x].Data);
							ImGui::Text("Confirm function: %s", (Data.bConfirmFunction ? "Yes" : "No"));
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
			if (iActive != -1 && iHovered != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !bTriggerToggle && !bHaveDragged)
				bTriggerToggle = true;

		
		//Draw the editor tab
			ImGui::TableSetColumnIndex(1);

			ImGui::BeginChild("ProgramEditorChild");
			
			if (iActive != -1)
				(*funcEditorDrawTabs[prog.Ops[iActive].iType])(prog.Ops[iActive]);

			ImGui::EndChild();
		
		ImGui::EndTable();

		//ImGui::Text("Active:  %d", iActive);
		//ImGui::Text("Hovered:  %d", iHovered);

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