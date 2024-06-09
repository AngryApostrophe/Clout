//#include <Windows.h>
using namespace std;

#include <imgui.h>
#include <imgui_internal.h>

#include "Clout.h"
#include "Comms.h"
#include "Console.h"
#include "ModelViewer.h"
#include "Probing/Probing.h"
#include "CloutProgram.h"
#include "ProgramEditor.h"
#include "OperationQueue.h"


extern CloutProgram prog;

//Global vars
	ImGuiIO* io = 0;
	extern ImGuiID dockspace_id;	//winmain.cpp

//Machine status
	MACHINE_STATUS MachineStatus;
	char szWCSChoices[9][20] = { "Unknown", "G53", "G54", "G55", "G56", "G57", "G58", "G59", "" }; //The active one will have (Active) appended to it


//Show the tooltip Help marker
void HelpMarker(const char *sString)
{
	ImGui::SameLine();
	ImGui::TextDisabled("[?]");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(sString);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void Window_ChangeTool_Draw()
{
	//Create the window
		if (!ImGui::BeginPopupModal("Change Tool", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			return;

	//Description
		ImGui::Text("Use the Automatic Tool Changer to switch to a new tool");

		ImGui::Dummy(ImVec2(0.0f, 5.0f)); //Extra empty space before the combo box

	//New tool
		static int iNewToolChoice = 2;
		const char szToolChoices[][15] = { "Empty", "Wireless Probe", "1", "2", "3", "4", "5", "6" }; //TODO: Make this default to the current tool.
		if (ImGui::BeginCombo("New Tool##ChangeTool", szToolChoices[iNewToolChoice]))
		{
			for (int x = 0; x < 8; x++)
			{
				//Add the item
				const bool is_selected = (iNewToolChoice == x);
				if (ImGui::Selectable(szToolChoices[x], is_selected))
					iNewToolChoice = x;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##ChangeTool", ImVec2(120, 0)))
	{
		char szString[8];
		sprintf_s(szString, 8, "M6 T%d", iNewToolChoice - 1);
		Comms_SendString(szString);

		ImGui::CloseCurrentPopup();
	}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##ChangeTool", ImVec2(120, 0)))
	{
		ImGui::CloseCurrentPopup();
	}

	//All done
		ImGui::End();
}


void Window_Control_Draw()
{
	//Create the window
		ImGui::Begin("Control");

	//General section
		ImGui::SeparatorText("General");

		if (ImGui::Button("Home X/Y"))
			Comms_SendString("$H");

		ImGui::Button("Rapid to location");

		if (ImGui::Button("Go to clearance"))
			Comms_SendString("G28");

	//Tool section
		ImGui::SeparatorText("Tool");
		if (ImGui::Button("ATC change tool"))
			ImGui::OpenPopup("Change Tool");
		Window_ChangeTool_Draw();
		
		ImGui::SameLine(); ImGui::Text("   "); ImGui::SameLine(); 		
		
		if (ImGui::Button("TLO Calibrate"))
			Comms_SendString("M491");
		
		if (ImGui::Button("Open collet"))
			Comms_SendString("M490.2");

		ImGui::SameLine(); ImGui::Text("       "); ImGui::SameLine();		
		
		if (ImGui::Button("Close collet"))
			Comms_SendString("M490.1");

	//Probing section
		ImGui::SeparatorText("Probing");
		Probing_Draw();

	//All done
		ImGui::End();
}


//Our main Init routine
void Clout_Init()
{
	Console.AddLog("Starting up");

	prog.jData.get_to(prog); //TODO: Temp.

	//Setup the global machine status
		MachineStatus.Status = Carvera::Status::Idle;
		MachineStatus.Coord.Working = {-1, -1, -1};
		MachineStatus.Coord.Machine = { -1, -1, -1 };
		MachineStatus.FeedRates = { 0, 0, 0 };

		MachineStatus.bCurrentTool = 0;
		MachineStatus.fToolLengthOffset = 0;
		MachineStatus.Positioning = Carvera::Positioning::Absolute;

	//Initialize comms
		CommsInit();

	//Load some GUI resources
		Probing_InitPage();
	
	//And initialize the model viewer
		ModelViewer.Init();

	//Other init stuff
		OperationQueue.Init();

	/*OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);
	OperationQueue.AddProgramToQueue(prog);*/		
}


void Clout_CreateDefaultLayout()
{
	ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
	ImGui::DockBuilderAddNode(dockspace_id); // Add empty node
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

	auto dock_id_preview = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.75f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Preview", dock_id_preview);

	auto dock_id_status = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Status", dock_id_status);

	auto dock_id_control = ImGui::DockBuilderSplitNode(dock_id_status, ImGuiDir_Down, 0.4f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Control", dock_id_control);

	auto dock_id_console = ImGui::DockBuilderSplitNode(dock_id_preview, ImGuiDir_Down, 0.43f, nullptr, &dock_id_preview);
	ImGui::DockBuilderDockWindow("Console", dock_id_console);

	auto dock_id_queue= ImGui::DockBuilderSplitNode(dock_id_preview, ImGuiDir_Right, 0.32f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Queue", dock_id_queue);
	ImGui::DockBuilderDockWindow("Preview Controls", dock_id_queue);	
	
	ImGui::DockBuilderFinish(dockspace_id);
}

//Our main Shutdown routine
void Clout_Shutdown()
{
}

//Our main app loop.  This gets called every draw frame
bool Clout_MainLoop()
{
	int x;		//General purpose int
	const BYTE bSlen = 20;
	char s[bSlen];	//General purpose string

	//Process the operations queue
		OperationQueue.Run();

	//Draw the sub windows
		Console.Draw();
		Window_Control_Draw();
		OperationQueue.DrawList();
		ModelViewer.Draw();

	//Status window
		ImGui::Begin("Status", 0); //Create the status window

		ImGui::SeparatorText("Connection");
			if (bCommsConnected)
			{
				if (ImGui::BeginTable("table_pos", 2))
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Status");

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("Connected");

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("IP Address");

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%s:%d", sConnectedIP, wConnectedPort);

					ImGui::EndTable();
				}
			
				if (ImGui::Button("Disconnect"))
					Comms_Disconnect();
			}
			else
			{
				if (ImGui::BeginTable("table_pos", 2))
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Status");

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("Disconnected");

					ImGui::EndTable();
				}
			}          

	
		if (!bCommsConnected)
		{
			ImGui::SeparatorText("Devices");

			static int item_current_idx = 0; // Here we store our selection data as an index.
			if (ImGui::BeginListBox("##devices", ImVec2(-FLT_MIN, 3 * ImGui::GetTextLineHeightWithSpacing())))
			{
				for (x = 0; x < bDetectedDevices; x++)
				{
					const bool is_selected = (item_current_idx == x);
					if (ImGui::Selectable(sDetectedDevices[x][0], is_selected))
						item_current_idx = x;
				}
				ImGui::EndListBox();
			}

			if (ImGui::Button("Connect"))
				Comms_ConnectDevice(item_current_idx);
		}

		ImGui::SeparatorText("Status");
		
			if (!bCommsConnected)
				ImGui::Text("Offline");
			else if (MachineStatus.Status == Carvera::Status::Idle)
				ImGui::Text("Idle");
			else if (MachineStatus.Status == Carvera::Status::Busy)
				ImGui::Text("Busy");
			else if (MachineStatus.Status == Carvera::Status::Homing)
				ImGui::Text("Homing");
			else if (MachineStatus.Status == Carvera::Status::Alarm)
				ImGui::Text("Alarm");
			else if (MachineStatus.Status == Carvera::Status::Hold)
				ImGui::Text("Hold");

			if (ImGui::BeginTable("table_pos", 3))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("X:  %0.04f", MachineStatus.Coord.Working.x);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Y:  %0.04f", MachineStatus.Coord.Working.y);

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("Z:  %0.04f", MachineStatus.Coord.Working.z);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("   (%0.04f)", MachineStatus.Coord.Machine.x);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("   (%0.04f)", MachineStatus.Coord.Machine.y);

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("   (%0.04f)", MachineStatus.Coord.Machine.z);
				
				ImGui::EndTable();
			}

			if (ImGui::BeginTable("table_feedrates", 3))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("X:  %0.1f", MachineStatus.FeedRates.x);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Y:  %0.1f", MachineStatus.FeedRates.y);

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("Z:  %0.1f", MachineStatus.FeedRates.z);

				ImGui::EndTable();
			}

			ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space

			if (ImGui::BeginTable("table_status", 3))// , ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH))
			{
				//Positioning mode
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding(); //Vertical align for the button coming later
					ImGui::Text("Positioning");

					ImGui::TableSetColumnIndex(1);

					if (bCommsConnected)
					{
						ImGui::AlignTextToFramePadding();

						//The string to show
							strcpy_s(s, bSlen, "Unknown");
							if (MachineStatus.Positioning == Carvera::Positioning::Absolute)
								strcpy_s(s, bSlen, "Absolute");
							else if (MachineStatus.Positioning == Carvera::Positioning::Relative)
								strcpy_s(s, bSlen, "Relative");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text(s);


						ImGui::TableSetColumnIndex(2);

						if (ImGui::SmallButton("...##pos"))
							ImGui::OpenPopup("popup_change_positioning");

						if (ImGui::BeginPopup("popup_change_positioning"))
						{
							const char* sPositioning[] = { "Absolute", "Relative" };
							int iPosIdx = MachineStatus.Positioning;

							ImGui::SeparatorText("Positioning");
							for (x = 0; x < 2; x++)
							{
								if (ImGui::Selectable(sPositioning[x]))
									iPosIdx = x;
							}
							ImGui::EndPopup();

							if (iPosIdx != MachineStatus.Positioning)
							{
								if (iPosIdx == 0) //Change to Absolute
									Comms_SendString("G90");
								else if (iPosIdx == 1) //Change to Relative
									Comms_SendString("G91");
							}
							
						}
					}

				//WCS
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding(); //Vertical align for the button coming later
					ImGui::Text("WCS");

					ImGui::TableSetColumnIndex(1);

					if (bCommsConnected)
					{
						ImGui::AlignTextToFramePadding();

						//The string to show
							strcpy_s(s, bSlen, "Unknown");
							if (MachineStatus.WCS == Carvera::CoordSystem::G53)
								strcpy_s(s, bSlen, "G53");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G54)
								strcpy_s(s, bSlen, "G54");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G55)
								strcpy_s(s, bSlen, "G55");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G56)
								strcpy_s(s, bSlen, "G56");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G57)
								strcpy_s(s, bSlen, "G57");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G58)
								strcpy_s(s, bSlen, "G58");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G59)
								strcpy_s(s, bSlen, "G59");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text(s);

					

						ImGui::TableSetColumnIndex(2);


						if (ImGui::SmallButton("...##wcs"))
							ImGui::OpenPopup("popup_change_wcs");

						if (ImGui::BeginPopup("popup_change_wcs"))
						{
							int iNewWCS = MachineStatus.WCS;
							ImGui::SeparatorText("Change WCS");
							x = Carvera::CoordSystem::G54; //Only show G54 and up
							while (szWCSChoices[x][0] != 0x0)
							{
								if (ImGui::Selectable(szWCSChoices[x]))
									iNewWCS = x;

								x++;
							}
							ImGui::EndPopup();

							if (iNewWCS != MachineStatus.WCS)
								Comms_SendString(szWCSChoices[iNewWCS]);
						}
					}

				//Units
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding(); //Vertical align for the button coming later
					ImGui::Text("Units");

					ImGui::TableSetColumnIndex(1);

					if (bCommsConnected)
					{
						ImGui::AlignTextToFramePadding();

						//The string to show
							strcpy_s(s, bSlen, "Unknown");
							if (MachineStatus.Units == Carvera::Units::inch)
								strcpy_s(s, bSlen, "Inch");
							else if (MachineStatus.Units == Carvera::Units::mm)
								strcpy_s(s, bSlen, "Millimeter");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text(s);
					}
					
				//Plane
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding(); //Vertical align for the button coming later
					ImGui::Text("Plane");

					ImGui::TableSetColumnIndex(1);

					if (bCommsConnected)
					{
						ImGui::AlignTextToFramePadding();

						//The string to show
							strcpy_s(s, bSlen, "Unknown");
							if (MachineStatus.Plane == Carvera::Plane::XYZ)
								strcpy_s(s, bSlen, "XYZ");
							else if (MachineStatus.Plane == Carvera::Plane::XZY)
								strcpy_s(s, bSlen, "XZY");
							else if (MachineStatus.Plane == Carvera::Plane::YZX)
								strcpy_s(s, bSlen, "YZX");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text(s);
					}

				ImGui::EndTable();
			}

			//if (MachineStatus.Status == MACHINE_STATUS::STATUS_BUSY) //Disable button if not busy
			if (ImGui::Button("Emergency Stop"))
			{
				s[0] = 0x18; //Abort command
				s[1] = 0x0;
				Comms_SendString(s);
			}
			if (ImGui::Button("Unlock"))
			{
				Comms_SendString("$X");
			}


			if (ImGui::Button("Program Editor"))
				ImGui::OpenPopup("Program Editor");
			ProgramEditor_Draw();

		
		ImGui::End(); //End Status window



     return false;
}