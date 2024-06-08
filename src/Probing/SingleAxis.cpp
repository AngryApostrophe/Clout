#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

ProbeOperation_SingleAxis::ProbeOperation_SingleAxis()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bType = PROBE_OP_TYPE_SINGLEAXIS;

	szWindowIdent = "SingleAxis";

	imgPreview[0] = 0;
	imgPreview[1] = 0;
	imgPreview[2] = 0;
	imgPreview[3] = 0;
	imgPreview[4] = 0;
	LoadPreviewImage(&imgPreview[0], "./res/ProbeSingleAxisX0.png");
	LoadPreviewImage(&imgPreview[1], "./res/ProbeSingleAxisX1.png");
	LoadPreviewImage(&imgPreview[2], "./res/ProbeSingleAxisY2.png");
	LoadPreviewImage(&imgPreview[3], "./res/ProbeSingleAxisY3.png");
	LoadPreviewImage(&imgPreview[4], "./res/ProbeSingleAxisZ4.png");

	iAxisDirectionIndex = 0;
	fTravel = 5.0f;
}

void ProbeOperation_SingleAxis::StateMachine()
{
	char sCmd[50];

	switch (iState)
	{
	case PROBE_STATE_IDLE:
		bOperationRunning = false;
		break;

	case PROBE_STATE_START:
		//Save the feedrates for later
		StartFeedrate.x = MachineStatus.FeedRates.x;
		StartFeedrate.y = MachineStatus.FeedRates.y;
		StartFeedrate.z = MachineStatus.FeedRates.z;

		//Save the starting position
		StartPos.x = MachineStatus.Coord.Working.x;
		StartPos.y = MachineStatus.Coord.Working.y;
		StartPos.z = MachineStatus.Coord.Working.z;

		bOperationRunning = true; //Limit what we show on the console (from comms module)

		bStepIsRunning = false;
		iState++;
		break;

	case PROBE_STATE_SINGLEAXIS_PROBE_FAST:
		if (!bStepIsRunning)
		{
			strcpy_s(sCmd, 50, "G38.3 ");

			//Add the axis direction
			if (iAxisDirectionIndex == 0)
				strcat_s(sCmd, 50, "X-");
			else if (iAxisDirectionIndex == 1)
				strcat_s(sCmd, 50, "X");
			else if (iAxisDirectionIndex == 2)
				strcat_s(sCmd, 50, "Y");
			else if (iAxisDirectionIndex == 3)
				strcat_s(sCmd, 50, "Y-");
			else if (iAxisDirectionIndex == 4)
				strcat_s(sCmd, 50, "Z-");
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
				iState = PROBE_STATE_IDLE;
				break;
			}

			//Add the travel distance
			char szTemp[10];
			sprintf_s(szTemp, 10, "%0.2f ", fTravel);
			strcat_s(sCmd, 50, szTemp);

			//Add the feed rate
			sprintf_s(szTemp, 10, "F%d", iProbingSpeedFast);
			strcat_s(sCmd, 50, szTemp);

			//Send it out
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_SINGLEAXIS_PROBE_SLOW:
		if (!bStepIsRunning)
		{
			strcpy_s(sCmd, 50, "G38.5 ");

			//Add the axis direction (opposite this time)
			if (iAxisDirectionIndex == 0)
				strcat_s(sCmd, 50, "X");
			else if (iAxisDirectionIndex == 1)
				strcat_s(sCmd, 50, "X-");
			else if (iAxisDirectionIndex == 2)
				strcat_s(sCmd, 50, "Y-");
			else if (iAxisDirectionIndex == 3)
				strcat_s(sCmd, 50, "Y");
			else if (iAxisDirectionIndex == 4)
				strcat_s(sCmd, 50, "Z");
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
				iState = PROBE_STATE_IDLE;
				break;
			}

			//Add the travel distance
			char szTemp[10];
			sprintf_s(szTemp, 10, "%0.2f ", fTravel);
			strcat_s(sCmd, 50, szTemp);

			//Add the feed rate
			sprintf_s(szTemp, 10, "F%d", iProbingSpeedSlow);
			strcat_s(sCmd, 50, szTemp);

			//Do it
			Comms_SendString(sCmd);

			bStepIsRunning = TRUE;
		}
		else //Step is running.  Monitor for completion
		{
			DWORD dwRet = WaitForSingleObject(hProbeResponseEvent, 0);

			if (dwRet == WAIT_OBJECT_0) //Comms thread has triggered the event
			{
				if (ProbingSuccessOrFail(sProbeReplyMessage, &EndPos))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (dwRet != WAIT_TIMEOUT) //Windows error while waiting for response
			{
				//Abort anything going on, just in case it's running away
				sCmd[0] = 0x18; //Abort command
				sCmd[1] = 0x0;
				Comms_SendString(sCmd);

				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Windows error waiting for probe response.  Aborted.");
				iState = PROBE_STATE_IDLE;
			}
		}
		break;

	case PROBE_STATE_SINGLEAXIS_GETCOORDS:
		if (MachineStatus.Status == Carvera::Status::Idle && (fabs(MachineStatus.Coord.Machine.y - EndPos.y) < 0.1) && (fabs(MachineStatus.Coord.Machine.x - EndPos.x) < 0.1) && (fabs(MachineStatus.Coord.Machine.z - EndPos.z) < 0.1)) //Wait for the MachineStatus coords to catch up to the probe results
		{
			iState = PROBE_STATE_SINGLEAXIS_FINISH;
		}
		break;

	case PROBE_STATE_SINGLEAXIS_FINISH:
		sprintf_s(sCmd, 50, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		//Get the end position in WCS
		EndPos.x = MachineStatus.Coord.Working.x;
		EndPos.y = MachineStatus.Coord.Working.y;
		EndPos.z = MachineStatus.Coord.Working.z;

		sCmd[0] = 0x0;

		if (iAxisDirectionIndex == 0 || iAxisDirectionIndex == 1)
			sprintf_s(sCmd, 50, "Edge at X: %0.03f", EndPos.x);
		if (iAxisDirectionIndex == 2 || iAxisDirectionIndex == 3)
			sprintf_s(sCmd, 50, "Edge at Y: %0.03f", EndPos.y);
		if (iAxisDirectionIndex == 4)
			sprintf_s(sCmd, 50, "Bottom at Z: %0.03f", EndPos.z);

		Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Probe operation completed successfuly.  %s", sCmd);
		iState = PROBE_STATE_COMPLETE;
		break;
	}
}


void ProbeOperation_SingleAxis::DrawSubwindow()
{
	int x;
	char sString[50]; //General purpose string
	char sUnits[5] = "mm"; //Currently select machine units

	if (MachineStatus.Units != Carvera::Units::mm)
		strcat_s(sUnits, 5, "in"); //Inches

	ImGui::Text("Probe along a single axis to find an edge");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be at start position to move in one axis only");


	ImGui::Image((void*)(intptr_t)imgPreview[iAxisDirectionIndex], ImVec2(450, 342));

	//ImGui::Separator();

//	ImGui::SeparatorText("Setup");
	ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

	//Which Axis
		//ImGui::Text("Probe direction:");
	ImGui::SeparatorText("Probe direction");

	if (ImGui::BeginTable("table_singleaxis_axis", 4 /*, ImGuiTableFlags_Borders*/))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 140);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 65);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.0f);

		//Y+
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::PushID(0);

		//Make button red if it's selected
		if (iAxisDirectionIndex == 2)
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		else
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
		}

		if (ImGui::Button("Y+##ProbeSingleAxis", ImVec2(50, 50)))
			iAxisDirectionIndex = 2;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		ImGui::TableSetColumnIndex(3);
		ImGui::BeginDisabled(); //Can't probe Z+
		ImGui::Button("Z+##ProbeSingleAxis", ImVec2(50, 50));
		ImGui::EndDisabled();


		// X-
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID(0);

		//Calculations for right alignment
		auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - 50 - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		if (posX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(posX);

		//Make button red if it's selected
		if (iAxisDirectionIndex == 0)
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		else
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
		}

		if (ImGui::Button("X-##ProbeSingleAxis", ImVec2(50, 50)))
			iAxisDirectionIndex = 0;

		ImGui::PopStyleColor(1);
		ImGui::PopID();


		// X+
		ImGui::TableSetColumnIndex(2);

		ImGui::PushID(0);

		//Make button red if it's selected
		if (iAxisDirectionIndex == 1)
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		else
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
		}

		if (ImGui::Button("X+##ProbeSingleAxis", ImVec2(50, 50)))
			iAxisDirectionIndex = 1;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		// Y-
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::PushID(0);

		//Make button red if it's selected
		if (iAxisDirectionIndex == 3)
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		else
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
		}

		if (ImGui::Button("Y-##ProbeSingleAxis", ImVec2(50, 50)))
			iAxisDirectionIndex = 3;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		// Z-
		ImGui::TableSetColumnIndex(3);
		ImGui::PushID(0);

		//Make button red if it's selected
		if (iAxisDirectionIndex == 4)
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		else
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
		}

		if (ImGui::Button("Z-##ProbeSingleAxis", ImVec2(50, 50)))
			iAxisDirectionIndex = 4;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		ImGui::EndTable();
	}

	//ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(200);	//Set the width of the textboxes

	//Travel Distance
	sprintf_s(sString, 10, "%%0.2f%s", sUnits);
	ImGui::InputFloat("Travel distance", &fTravel, 0.1f, 1.0f, sString);
	ImGui::SameLine(); HelpMarker("How far to probe in the desired direction before failing.");

	//Feed rate
	ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
	ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
	ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");

	ImGui::PopItemWidth();


	ImGui::SeparatorText("Completion");

	//Zero WCS?
	ImGui::BeginDisabled(); //TODO: Need to know probe size so we can accurately set 0
	ImGui::Checkbox("Zero WCS", &bZeroWCS);
	ImGui::EndDisabled();

	ImGui::SameLine();

	//Disable the combo if the option isn't selected
	if (!bZeroWCS)
		ImGui::BeginDisabled();

	//The combo box
				//iWCSIndex = MachineStatus.WCS;					//TODO: Default to the current mode
	if (iWCSIndex < Carvera::CoordSystem::G54)
		iWCSIndex = Carvera::CoordSystem::G54; //If we're in an unknown WCS or G53, show G54 as default

	if (ImGui::BeginCombo("##ProbingSingleAxis_WCS", szWCSChoices[iWCSIndex]))
	{
		x = Carvera::CoordSystem::G54;
		while (szWCSChoices[x][0] != 0x0) //Add all available WCSs (starting at G54)
		{
			//Add the item
			const bool is_selected = (iWCSIndex == (x));
			if (ImGui::Selectable(szWCSChoices[x], is_selected))
				iWCSIndex = (x);

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
			x++;
		}

		ImGui::EndCombo();
	}

	if (!bZeroWCS)
		ImGui::EndDisabled();

	ImGui::SameLine();
	HelpMarker("If selected, after completion of the probing operation the desired WCS axis will be zero'd");
}

bool ProbeOperation_SingleAxis::DrawPopup()
{
	bool bRetVal = TRUE;	//The operation continues to run

	BeginPopup();

	DrawSubwindow();

	bRetVal = EndPopup();

	StateMachine(); //Run the state machine if an operation is going

	return bRetVal;
}