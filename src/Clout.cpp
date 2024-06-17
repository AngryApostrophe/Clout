using namespace std;

#include "Platforms/Platforms.h"

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

		ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the combo box

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

	ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space before the buttons

	if (ImGui::Button("Run##ChangeTool", ScaledByWindowScale(120, 0)))
	{
		char szString[8];
		sprintf(szString, "M6 T%d", iNewToolChoice - 1);
		Comms_SendString(szString);

		ImGui::CloseCurrentPopup();
	}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	if (ImGui::Button("Cancel##ChangeTool", ScaledByWindowScale(120, 0)))
	{
		ImGui::CloseCurrentPopup();
	}

	//All done
		ImGui::End();
}

void Window_Jog_Draw()
{
	int x;
	std::string String; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	static int XYJogIdx = 3;
	static int ZJogIdx = 3;
	std::vector<const char*> JogVals = {"0.001", "0.01", "0.1", "1", "10", "100"};

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat(sUnits, "in"); //Inches

	// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	//Size the window
		const ImVec2 WindowSize(800, 500);
		ImGui::SetNextWindowSize(ScaledByWindowScale(WindowSize));

	//Create the modal popup
		if (!ImGui::BeginPopupModal("Jog Axis", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		

//	ImGui::SeparatorText("Setup");
	ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space

	//Jog buttons
	ImGui::SetCursorPosX(ScaledByWindowScale(50));	//Bump in the left side a bit
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 25));	//Space above and below each button.  Leaves 50 in between them in Y
	if (ImGui::BeginTable("table_jogaxis", 5 /*, ImGuiTableFlags_Borders*/))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);

		ImVec2 ButtonSize = ScaledByWindowScale(100, 100);
		ImVec2 DistanceButtonSize = ScaledByWindowScale(75, 50);

		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(150));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(150));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(250));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(100));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(10));	//This last column is just a filler

		//Y+
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);		

		if (ImGui::Button("Y+##JogAxis", ButtonSize))
		{
			String = "G91 G0 Y";
			String += JogVals[XYJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}

		//Z+
		ImGui::TableSetColumnIndex(3);
		if (ImGui::Button("Z+##JogAxis", ButtonSize))
		{
			String = "G91 G0 Z";
			String += JogVals[ZJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}

		// X-
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("X-##JogAxis", ButtonSize))
		{
			String = "G91 G0 X-";
			String += JogVals[XYJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}

		//XY Value
			ImGui::TableSetColumnIndex(1);
			String = JogVals[XYJogIdx];
			String += " ";
			String += sUnits;
			String += "##xyvalbtn";
			
			//Position the button in the center.  TODO: This shouldn't be hardcoded.  I really don't even know if this aligns on other users' computers, but this is quick and dirty.
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ScaledByWindowScale(12));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ScaledByWindowScale(25));
			
			if (ImGui::Button(String.c_str(), DistanceButtonSize))
				ImGui::OpenPopup("popup_change_xyval");

			if (ImGui::BeginPopup("popup_change_xyval"))
			{
				ImGui::SeparatorText("Distance");
				for (x = 0; x < JogVals.size(); x++)
				{
					if (ImGui::Selectable(JogVals[x]))
						XYJogIdx = x;
				}
				ImGui::EndPopup();
			}

		// X+
		ImGui::TableSetColumnIndex(2);

		if (ImGui::Button("X+##JogAxis", ButtonSize))
		{
			String = "G91 G0 X";
			String += JogVals[XYJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}

		//Z Value
		ImGui::TableSetColumnIndex(3);
			String = JogVals[ZJogIdx];
			String += " ";
			String += sUnits;
			String += "##zvalbtn";

			//Position the button in the center.  TODO: This shouldn't be hardcoded.  I really don't even know if this aligns on other users' computers, but this is quick and dirty.
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ScaledByWindowScale(12));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ScaledByWindowScale(25));

			if (ImGui::Button(String.c_str(), DistanceButtonSize))
				ImGui::OpenPopup("popup_change_zval");

			if (ImGui::BeginPopup("popup_change_zval"))
			{
				ImGui::SeparatorText("Distance");
				for (x = 0; x < JogVals.size(); x++)
				{
					if (ImGui::Selectable(JogVals[x]))
						ZJogIdx = x;
				}
				ImGui::EndPopup();
			}

		// Y-
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);

		if (ImGui::Button("Y-##JogAxis", ButtonSize))
		{
			String = "G91 G0 Y-";
			String += JogVals[XYJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}

		// Z-
		ImGui::TableSetColumnIndex(3);
		if (ImGui::Button("Z-##JogAxis", ButtonSize))
		{
			String = "G91 G0 Z-";
			String += JogVals[ZJogIdx];
			Comms_SendString(String.c_str());
			Comms_SendString("G90");	//Back to absolute mode
		}


		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	ImGui::Dummy(ScaledByWindowScale(0.0f, 12.0f)); //Extra empty space before the buttons

	
	ImGui::SetCursorPosX(ScaledByWindowScale((WindowSize.x/2)-50));
	if (ImGui::Button("Ok##Jog", ScaledByWindowScale(100, 25)))
	{
		ImGui::CloseCurrentPopup();
	}

	//All done
	ImGui::End();
}


void Window_Control_Draw()
{
	ImVec2 BtnSize = ScaledByWindowScale(110,25);

	//Create the window
		ImGui::Begin("Control");

	//General section
		ImGui::SeparatorText("General");

		if (ImGui::BeginTable("table", 2))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			if (ImGui::Button("Home X/Y", BtnSize))
				Comms_SendString("$H");

			if (ImGui::Button("Go to clearance", BtnSize))
				Comms_SendString("G28");

			ImGui::TableSetColumnIndex(1);
			
			if (ImGui::Button("Jog Axis", BtnSize))
				ImGui::OpenPopup("Jog Axis");
			Window_Jog_Draw();

			ImGui::BeginDisabled();
			ImGui::Button("Rapid To", BtnSize);
			ImGui::EndDisabled();

			ImGui::EndTable();
		}
			

	//Tool section
		ImGui::SeparatorText("Tool");
		if (ImGui::BeginTable("table", 2))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			if (ImGui::Button("ATC change tool", BtnSize))
				ImGui::OpenPopup("Change Tool");
			Window_ChangeTool_Draw();

			if (ImGui::Button("Open collet", BtnSize))
				Comms_SendString("M490.2");
		
			ImGui::TableSetColumnIndex(1);
		
			if (ImGui::Button("TLO Calibrate", BtnSize))
				Comms_SendString("M491");	
			
			if (ImGui::Button("Close collet", BtnSize))
				Comms_SendString("M490.1");

			ImGui::EndTable();
		}

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

	//Process comms stuff
		Comms_Update();

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
				if (ImGui::BeginTable("table", 2))
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
				if (ImGui::BeginTable("table", 2))
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
			if (ImGui::BeginListBox("##devices", ScaledByWindowScale(-FLT_MIN, 3 * ImGui::GetTextLineHeightWithSpacing())))
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

			ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space

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
							strcpy(s, "Unknown");
							if (MachineStatus.Positioning == Carvera::Positioning::Absolute)
								strcpy(s, "Absolute");
							else if (MachineStatus.Positioning == Carvera::Positioning::Relative)
								strcpy(s, "Relative");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text("%s", s);


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
							strcpy(s, "Unknown");
							if (MachineStatus.WCS == Carvera::CoordSystem::G53)
								strcpy(s, "G53");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G54)
								strcpy(s, "G54");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G55)
								strcpy(s, "G55");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G56)
								strcpy(s, "G56");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G57)
								strcpy(s, "G57");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G58)
								strcpy(s, "G58");
							else if (MachineStatus.WCS == Carvera::CoordSystem::G59)
								strcpy(s, "G59");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text("%s", s);

					

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
							strcpy(s, "Unknown");
							if (MachineStatus.Units == Carvera::Units::inch)
								strcpy(s, "Inch");
							else if (MachineStatus.Units == Carvera::Units::mm)
								strcpy(s, "Millimeter");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text("%s", s);
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
							strcpy(s, "Unknown");
							if (MachineStatus.Plane == Carvera::Plane::XYZ)
								strcpy(s, "XYZ");
							else if (MachineStatus.Plane == Carvera::Plane::XZY)
								strcpy(s, "XZY");
							else if (MachineStatus.Plane == Carvera::Plane::YZX)
								strcpy(s, "YZX");

						//Calculations for right alignment
							auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(s).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
							if (posX > ImGui::GetCursorPosX())
								ImGui::SetCursorPosX(posX);

						//Show it
							ImGui::Text("%s", s);
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
			ImGui::SameLine();
			ImGui::Text("<Not Functional>");
			ProgramEditor_Draw();

		
		ImGui::End(); //End Status window



     return false;
}