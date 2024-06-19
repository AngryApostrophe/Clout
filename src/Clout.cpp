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


//Global vars
	ImGuiIO* io = 0;
	extern ImGuiID dockspace_id;	//winmain.cpp

//Machine status
	MACHINE_STATUS MachineStatus;
	char szWCSChoices[9][20] = { "Unknown", "G53", "G54", "G55", "G56", "G57", "G58", "G59", "" }; //The active one will have (Active) appended to it
	const char szWCSNames[9][10] = { "Unknown", "G53", "G54", "G55", "G56", "G57", "G58", "G59", "" };


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
		const ImVec2 WindowSize(800, 525);
		ImGui::SetNextWindowSize(ScaledByWindowScale(WindowSize));

	//Create the modal popup
		if (!ImGui::BeginPopupModal("Jog Axis", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			return;

//	ImGui::SeparatorText("Setup");
	//ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space

	//Jog buttons
	ImGui::SetCursorPosX(ScaledByWindowScale(50));	//Bump in the left side a bit
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ScaledByWindowScale(0, 25));	//Space above and below each button.  Leaves 50 in between them in Y
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
				ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]);
				for (x = 0; x < JogVals.size(); x++)
				{
					if (ImGui::Selectable(JogVals[x]))
						XYJogIdx = x;
				}
				ImGui::PopFont();
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
				ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]);
				for (x = 0; x < JogVals.size(); x++)
				{
					if (ImGui::Selectable(JogVals[x]))
						ZJogIdx = x;
				}
				ImGui::PopFont();
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

	ImGui::Dummy(ScaledByWindowScale(0.0f, 8.0f)); //Extra empty space before the buttons

	
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

void Window_Connection_Draw()
{
	int x;

	//Create the connection window
		if (!ImGui::Begin("Connection", 0))
		{
			ImGui::End();
			return;
		}

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

		if (ImGui::Button("Connect", ScaledByWindowScale(90,45)))
			Comms_ConnectDevice(item_current_idx);
	}

	ImGui::End();
}

void Window_Status_Draw()
{
	int x;		//General purpose int
	char s[50];	//General purpose string
	ImVec2 vec;
	float fColWidth;

	ImVec2 TextSize;

	//Used for zeroing an axis
		int iAxisHover = -1;
		static int iAxisZero;

	//Create the status window
		if (!ImGui::Begin("Status", 0))
		{
			ImGui::End();
			return;
		}

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space

	ImVec4 MachineFontColor(0.8f, 1.0f, 1.0f, 1.0f);
	ImVec4 TableBgColor(0.1f, 0.1f, 0.1f, 1.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
	//ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));	//		ImGui::PopStyleVar();	//ImGuiStyleVar_CellPadding
	ImGui::PushStyleColor(ImGuiCol_ChildBg, TableBgColor);

	//Status
		const unsigned char StatusHeight = 50;
		
		//Begin the child window so we get the rounded border
			ImGui::BeginChild("child_status", ImVec2(ImGui::GetContentRegionAvail().x, ScaledByWindowScale(StatusHeight)), ImGuiChildFlags_Border);

		//Get the string
			if (!bCommsConnected)
				sprintf(s, "Offline");
			else if (MachineStatus.Status == Carvera::Status::Idle)
				sprintf(s, "Idle");
			else if (MachineStatus.Status == Carvera::Status::Busy)
				sprintf(s, "Busy");
			else if (MachineStatus.Status == Carvera::Status::Homing)
				sprintf(s, "Homing");
			else if (MachineStatus.Status == Carvera::Status::Alarm)
				sprintf(s, "Alarm");
			else if (MachineStatus.Status == Carvera::Status::Hold)
				sprintf(s, "Hold");

		//Swtitch to the correct font
			ImGui::PushFont(io->Fonts->Fonts[FONT_POS_LARGE]);

		//Move to the center of the child window
			TextSize = ImGui::CalcTextSize(s);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetWindowWidth() - TextSize.x) * 0.5f));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ((ImGui::GetWindowHeight() - TextSize.y) * 0.5f) - ScaledByWindowScale(3)); //Need a little bump upwards, maybe something to do with padding?

		//Draw the text
			ImGui::TextColored(MachineFontColor, s);

		//Switch back to default font
			ImGui::PopFont();
		
		//End the child window
			ImGui::EndChild();
		
	ImGui::Dummy(ScaledByWindowScale(0.0f, 10.0f)); //Extra empty space

	//Positions
		//Create the child window for the rounded border
			ImGui::BeginChild("child_pos", ImVec2(ImGui::GetContentRegionAvail().x, ScaledByWindowScale(0.0f)), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

		//Create the table to get everything aligned
			if (ImGui::BeginTable("table_pos", 2, /*ImGuiTableFlags_BordersOuter |*/ ImGuiTableFlags_BordersInnerH))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(30));	//The column with the axis label
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				//X Position
					ImGui::PushFont(io->Fonts->Fonts[FONT_POS_LARGE]);	//Switch to large font
					ImGui::TextColored(MachineFontColor, " X");

					ImGui::TableSetColumnIndex(1);	//Jump to data column
					fColWidth = ImGui::GetColumnWidth();	//Store the width of this column

					//Create another table, this time to make rows for aligning the WCS and MCS coords
						if (ImGui::BeginTable("table_posx", 1 /*, ImGuiTableFlags_Borders*/))
						{
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);

							//Working X coord
								sprintf(s, "%0.04f", MachineStatus.Coord.Working.x); //Get the text
						
								//Center in column
									TextSize = ImGui::CalcTextSize(s);
									ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));
						
								ImGui::TextColored(MachineFontColor, s); //Draw it
								ImGui::PopFont(); //Back to default font

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);

							//Machine X coord
								ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]); //Switch to this font
								sprintf(s, "%0.04f", MachineStatus.Coord.Machine.x); //Get the text
						
								//Center in column
									TextSize = ImGui::CalcTextSize(s);
									ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));

								ImGui::TextColored(MachineFontColor, s); //Draw it
								ImGui::PopFont(); //Back to default font

							ImGui::EndTable();
						}
						if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped))
							iAxisHover = 0;

				//Move to Y position
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

				//Y Position
					ImGui::PushFont(io->Fonts->Fonts[FONT_POS_LARGE]);	//Switch to large font
					ImGui::TextColored(MachineFontColor, " Y");

					ImGui::TableSetColumnIndex(1);	//Jump to data column
					fColWidth = ImGui::GetColumnWidth();	//Store the width of this column

					//Create another table, this time to make rows for aligning the WCS and MCS coords
						if (ImGui::BeginTable("table_posy", 1/*, ImGuiTableFlags_Borders*/))
						{
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);

							//Working Y coord
								sprintf(s, "%0.04f", MachineStatus.Coord.Working.y); //Get the text

							//Center in column
								TextSize = ImGui::CalcTextSize(s);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));

								ImGui::TextColored(MachineFontColor, s); //Draw it
								ImGui::PopFont(); //Back to default font

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);

							//Machine Y coord
								ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]); //Switch to this font
								sprintf(s, "%0.04f", MachineStatus.Coord.Machine.y); //Get the text

							//Center in column
								TextSize = ImGui::CalcTextSize(s);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));

								ImGui::TextColored(MachineFontColor, s); //Draw it
								ImGui::PopFont(); //Back to default font

								ImGui::EndTable();
						}
						if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped))
							iAxisHover = 1;

				//Move to Z position
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

				//Z Position
					ImGui::PushFont(io->Fonts->Fonts[FONT_POS_LARGE]);	//Switch to large font
					ImGui::TextColored(MachineFontColor, " Z");

					ImGui::TableSetColumnIndex(1);	//Jump to data column
					fColWidth = ImGui::GetColumnWidth();	//Store the width of this column

					if (ImGui::BeginTable("table_posz", 1/*, ImGuiTableFlags_Borders*/))
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						//Working Z coord
							sprintf(s, "%0.04f", MachineStatus.Coord.Working.z); //Get the text

						//Center in column
							TextSize = ImGui::CalcTextSize(s);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));

							ImGui::TextColored(MachineFontColor, s); //Draw it
							ImGui::PopFont(); //Back to default font

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);

						//Machine Z coord
							ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]); //Switch to this font
							sprintf(s, "%0.04f", MachineStatus.Coord.Machine.z); //Get the text

						//Center in column
							TextSize = ImGui::CalcTextSize(s);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x) * 0.5f));

							ImGui::TextColored(MachineFontColor, s); //Draw it
							ImGui::PopFont(); //Back to default font

						ImGui::EndTable();
					}
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped))
						iAxisHover = 2;

				ImGui::EndTable();
			}

		//End the child window
			ImGui::EndChild();
		
		ImGui::Dummy(ScaledByWindowScale(0.0f, 10.0f)); //Extra empty space

		//ImGui::Text("%d", iAxisHover);

	//Zero axis on mouse click
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && iAxisHover != -1)
			{
				iAxisZero = iAxisHover;
				ImGui::OpenPopup("Zero Axis##Status");
			}

			if (ImGui::BeginPopupModal("Zero Axis##Status", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				char sAxis[2] = "";
				if (iAxisZero == 0)
					strcpy(sAxis, "X");
				else if (iAxisZero == 1)
					strcat(sAxis, "Y");
				else if (iAxisZero == 2)
					strcat(sAxis, "Z");

				sprintf(s, "Confirm you wish to set the %s axis to 0?", sAxis);

				ImGui::Text(s);
				ImGui::Separator();
				ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space

				if (ImGui::Button("OK", ScaledByWindowScale(120, 30)))
				{
					sprintf(s, "G10 L20 P0 %s0", sAxis);
					Comms_SendString(s); //Send the command
					ImGui::CloseCurrentPopup();										
				}
				ImGui::SameLine();
				ImGui::Dummy(ScaledByWindowScale(15.0f, 0.0f)); //Extra empty space
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ScaledByWindowScale(120, 30)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::EndPopup();
			}

	//Feedrate
		if (ImGui::BeginTable("table_feedrates", 2))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			
			fColWidth = ImGui::GetColumnWidth();
			sprintf(s, "Feed Rate");
			TextSize = ImGui::CalcTextSize(s);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x)));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ScaledByWindowScale(3));
			ImGui::Text(s);

			ImGui::TableSetColumnIndex(1);

			ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]); //Switch to this font
			ImGui::TextColored(MachineFontColor, "%0.0f", MachineStatus.FeedRates.x); //Draw it
			ImGui::PopFont(); //Back to default font

			ImGui::EndTable();
		}

	//Spindle
		if (ImGui::BeginTable("table_spindle", 2))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			fColWidth = ImGui::GetColumnWidth();
			sprintf(s, "Spindle");
			TextSize = ImGui::CalcTextSize(s);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fColWidth - TextSize.x)));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ScaledByWindowScale(3));
			ImGui::Text(s);

			ImGui::TableSetColumnIndex(1);

			ImGui::PushFont(io->Fonts->Fonts[FONT_POS_SMALL]); //Switch to this font
			ImGui::TextColored(MachineFontColor, "%0.0f", 12000); //Draw it
			ImGui::PopFont(); //Back to default font

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

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the combo box
	
	//Emergency Stop button
		x = 1; //Disabled
		if (MachineStatus.Status != Carvera::Status::Busy && MachineStatus.Status != Carvera::Status::Homing)
			x = 0; //Active

		if (!x)
			ImGui::BeginDisabled();

		if (ImGui::Button("Emergency Stop##Status", ScaledByWindowScale(105,45)))
		{
			s[0] = 0x18; //Abort command
			s[1] = 0x0;
			Comms_SendString(s);
		}

		if (!x)
			ImGui::EndDisabled();

	ImGui::SameLine();

	//Unlock button.  TODO: Maybe click on ALARM status up top instead?
		x = 0; //Disabled
		if (MachineStatus.Status == Carvera::Status::Alarm)
			x = 1; //Active

		if (!x)
			ImGui::BeginDisabled();
	
		if (ImGui::Button("Unlock##Status", ScaledByWindowScale(90, 45)))
		{
			Comms_SendString("$X");
		}

		if (!x)
			ImGui::EndDisabled();

	ImGui::PopStyleColor(); //ImGuiCol_ChildBg
	ImGui::PopStyleVar();	//ImGuiStyleVar_ChildRounding


	ImGui::End(); //End Status window
}


//Our main Init routine
void Clout_Init()
{
	Console.AddLog("Starting up");

	//Setup the global machine status
		MachineStatus.Status = Carvera::Status::Idle;
		MachineStatus.Coord.Working = {-1, -1, -1};
		MachineStatus.Coord.Machine = { -1, -1, -1 };
		MachineStatus.FeedRates = { 0, 0, 0 };

		MachineStatus.iCurrentTool = -1;
		MachineStatus.fToolLengthOffset = 0;
		MachineStatus.Positioning = Carvera::Positioning::Absolute;
		MachineStatus.bProbeTriggered = false;

	//Initialize comms
		CommsInit();

	//Load some GUI resources
		Probing_InitPage();
	
	//And initialize the model viewer
		ModelViewer.Init();

	//Other init stuff
		OperationQueue.Init();	
}


void Clout_CreateDefaultLayout()
{
	ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
	ImGui::DockBuilderAddNode(dockspace_id); // Add empty node
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

	auto dock_id_preview = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.8f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Preview", dock_id_preview);

	auto dock_id_status = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.0f, nullptr, &dockspace_id);
	ImGui::DockBuilderDockWindow("Status", dock_id_status);
	ImGui::DockBuilderDockWindow("Connection", dock_id_status);

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
	//Process comms stuff
		Comms_Update();

	//Process the operations queue
		OperationQueue.Run();

	//Draw the sub windows
		Console.Draw();
		Window_Control_Draw();
		Window_Connection_Draw();
		Window_Status_Draw();
		OperationQueue.DrawList();
		ModelViewer.Draw();

	//Setup some window defaults the first time through.  This has to be done after the first draw pass
		static bool bFirstRun = true;
		if (bFirstRun)
		{
			ImGui::SetWindowFocus("Connection");	//Make it default to the Connection page vice Status
			//ImGui::SetWindowFocus(0);
			//ImGui::SetWindowFocus("Queue");	//Make it default to the Queue page vice 3d settings
			bFirstRun = false;
		}
     return false;
}