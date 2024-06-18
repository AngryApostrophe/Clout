#include "../Platforms/Platforms.h"

#include <stdio.h>
#include <math.h>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

#include "../Resources/ProbeSingleAxisX0.h"
#include "../Resources/ProbeSingleAxisX1.h"
#include "../Resources/ProbeSingleAxisY2.h"
#include "../Resources/ProbeSingleAxisY3.h"
#include "../Resources/ProbeSingleAxisZ4.h"
static GLuint imgProbeSingleAxis[5] = {0,0,0,0,0};	//Only load it once, rather than every time we open a window

ProbeOperation_SingleAxis::ProbeOperation_SingleAxis()
{
	ProbeOperation();	//Do the stuff in the base class constructor

	bProbingType = PROBE_OP_TYPE_SINGLEAXIS;

	szWindowIdent = "SingleAxis";

	if (imgProbeSingleAxis[0] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeSingleAxisX0_compressed_data, ProbeSingleAxisX0_compressed_size, &imgProbeSingleAxis[0]);
	imgPreview[0] = imgProbeSingleAxis[0];
	if (imgProbeSingleAxis[1] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeSingleAxisX1_compressed_data, ProbeSingleAxisX1_compressed_size, &imgProbeSingleAxis[1]);
	imgPreview[1] = imgProbeSingleAxis[1];
	if (imgProbeSingleAxis[2] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeSingleAxisY2_compressed_data, ProbeSingleAxisY2_compressed_size, &imgProbeSingleAxis[2]);
	imgPreview[2] = imgProbeSingleAxis[2];
	if (imgProbeSingleAxis[3] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeSingleAxisY3_compressed_data, ProbeSingleAxisY3_compressed_size, &imgProbeSingleAxis[3]);
	imgPreview[3] = imgProbeSingleAxis[3];
	if (imgProbeSingleAxis[4] == 0)
		LoadCompressedTextureFromMemory((const unsigned char*)ProbeSingleAxisZ4_compressed_data, ProbeSingleAxisZ4_compressed_size, &imgProbeSingleAxis[4]);
	imgPreview[4] = imgProbeSingleAxis[4];

	iAxisDirectionIndex = 0;
	fTravel = 5.0f;
	fProbeTipDiameter = 2.0f;
}

void ProbeOperation_SingleAxis::StateMachine()
{
	char sCmd[50];
	CarveraMessage msg;
	int iRet;


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
			strcpy(sCmd, "G38.3 ");

			//Add the axis direction
			if (iAxisDirectionIndex == 0)
				strcat(sCmd, "X-");
			else if (iAxisDirectionIndex == 1)
				strcat(sCmd, "X");
			else if (iAxisDirectionIndex == 2)
				strcat(sCmd, "Y");
			else if (iAxisDirectionIndex == 3)
				strcat(sCmd, "Y-");
			else if (iAxisDirectionIndex == 4)
				strcat(sCmd, "Z-");
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
				iState = PROBE_STATE_IDLE;
				break;
			}

			//Add the travel distance
			char szTemp[10];
			sprintf(szTemp, "%0.2f ", fTravel);
			strcat(sCmd, szTemp);

			//Add the feed rate
			sprintf(szTemp, "F%d", iProbingSpeedFast);
			strcat(sCmd, szTemp);

			//Send it out
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			iRet = Comms_PopMessageOfType(&msg, CARVERA_MSG_PROBE);

			if (iRet > 0) //We've received a probing event from Carvera
			{
				if (ProbingSuccessOrFail(msg.cData))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet < 0) //Error while waiting for response
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
			strcpy(sCmd, "G38.5 ");

			//Add the axis direction (opposite this time)
			if (iAxisDirectionIndex == 0)
				strcat(sCmd, "X");
			else if (iAxisDirectionIndex == 1)
				strcat(sCmd, "X-");
			else if (iAxisDirectionIndex == 2)
				strcat(sCmd, "Y-");
			else if (iAxisDirectionIndex == 3)
				strcat(sCmd, "Y");
			else if (iAxisDirectionIndex == 4)
				strcat(sCmd, "Z");
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unknown probe direction.  Aborted.");
				iState = PROBE_STATE_IDLE;
				break;
			}

			//Add the travel distance
			char szTemp[10];
			sprintf(szTemp, "%0.2f ", fTravel);
			strcat(sCmd, szTemp);

			//Add the feed rate
			sprintf(szTemp, "F%d", iProbingSpeedSlow);
			strcat(sCmd, szTemp);

			//Do it
			Comms_SendString(sCmd);

			bStepIsRunning = true;
		}
		else //Step is running.  Monitor for completion
		{
			iRet = Comms_PopMessageOfType(&msg, CARVERA_MSG_PROBE);

			if (iRet > 0) //We've received a probing event from Carvera
			{
				if (ProbingSuccessOrFail(msg.cData, &EndPos))
				{
					bStepIsRunning = 0;
					iState++;
				}
			}
			else if (iRet < 0) //Error while waiting for response
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
		sprintf(sCmd, "G0 F%f", StartFeedrate.x); //Restore the feedrate to what it was
		Comms_SendString(sCmd);

		//Get the end position in WCS
			EndPos.x = MachineStatus.Coord.Working.x;
			EndPos.y = MachineStatus.Coord.Working.y;
			EndPos.z = MachineStatus.Coord.Working.z;

		sCmd[0] = 0x0;

		if (iAxisDirectionIndex == 0)
		{
			sprintf(sCmd, "Edge at X: %0.03f", EndPos.x - (fProbeTipDiameter/2.0f));

			if (bZeroWCS)
				ZeroWCS(true, false, false, (fProbeTipDiameter / 2.0f));
		}
		else if (iAxisDirectionIndex == 1)
		{
			sprintf(sCmd, "Edge at X: %0.03f", EndPos.x + (fProbeTipDiameter / 2.0f));

			if (bZeroWCS)
				ZeroWCS(true, false, false, 0 - (fProbeTipDiameter / 2.0f));
		}
		else if (iAxisDirectionIndex == 2)
		{
			sprintf(sCmd, "Edge at Y: %0.03f", EndPos.y - (fProbeTipDiameter / 2.0f));

			if (bZeroWCS)
				ZeroWCS(false, true, false, 0, (fProbeTipDiameter / 2.0f));
		}
		else if (iAxisDirectionIndex == 3)
		{
			sprintf(sCmd, "Edge at Y: %0.03f", EndPos.y + (fProbeTipDiameter / 2.0f));

			if (bZeroWCS)
				ZeroWCS(false, true, false, 0, 0 - (fProbeTipDiameter / 2.0f));
		}
		else if (iAxisDirectionIndex == 4)
		{
			sprintf(sCmd, "Bottom at Z: %0.03f", EndPos.z);

			if (bZeroWCS)
				ZeroWCS(false, false, true);
		}

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
		strcat(sUnits, "in"); //Inches

	ImGui::Text("Probe along a single axis to find an edge");
	ImGui::Text("Setup:");
	ImGui::Text("	Probe already installed");
	ImGui::Text("	Probe must be at start position to move in one axis only");


	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ScaledByWindowScale(450.0f)) * 0.5f);	//Center the image in the window
	ImGui::Image((void*)(intptr_t)imgPreview[iAxisDirectionIndex], ScaledByWindowScale(450, 342));

	//ImGui::Separator();

//	ImGui::SeparatorText("Setup");
	ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

	//Which Axis
		//ImGui::Text("Probe direction:");
	ImGui::SeparatorText("Probe direction");

	if (ImGui::BeginTable("table_singleaxis_axis", 4 /*, ImGuiTableFlags_Borders*/))
	{
		ImVec2 ButtonSize = ScaledByWindowScale(50, 50);

		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(140));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(65));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(80));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ScaledByWindowScale(100));

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

		if (ImGui::Button("Y+##ProbeSingleAxis", ButtonSize))
			iAxisDirectionIndex = 2;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		ImGui::TableSetColumnIndex(3);
		ImGui::BeginDisabled(); //Can't probe Z+
		ImGui::Button("Z+##ProbeSingleAxis", ButtonSize);
		ImGui::EndDisabled();


		// X-
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID(0);

		//Calculations for right alignment
		auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ScaledByWindowScale(50) - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
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

		if (ImGui::Button("X-##ProbeSingleAxis", ButtonSize))
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

		if (ImGui::Button("X+##ProbeSingleAxis", ButtonSize))
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

		if (ImGui::Button("Y-##ProbeSingleAxis", ButtonSize))
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

		if (ImGui::Button("Z-##ProbeSingleAxis", ButtonSize))
			iAxisDirectionIndex = 4;

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		ImGui::EndTable();
	}

	//ImGui::Dummy(ImVec2(0.0f, 5)); //Extra empty space

	ImGui::SeparatorText("Setup");

	ImGui::PushItemWidth(ScaledByWindowScale(200));	//Set the width of the textboxes

	//Travel Distance
		sprintf(sString, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Travel distance", &fTravel, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("How far to probe in the desired direction before failing.");

	//Probe size
		sprintf(sString, "%%0.2f%s", sUnits);
		ImGui::InputFloat("Probe tip diameter", &fProbeTipDiameter, 0.1f, 1.0f, sString);
		ImGui::SameLine(); HelpMarker("Diameter of the tip of the probe, required to accurately calculate the edge");

	//Feed rate
		ImGui::InputInt("Probing Speed (Fast)", &iProbingSpeedFast);
		ImGui::SameLine(); HelpMarker("Speed at which the probe moves towards the edge.");
		ImGui::InputInt("Probing Speed (Slow)", &iProbingSpeedSlow);
		ImGui::SameLine(); HelpMarker("Speed at which the probe moves off the edge, for high accuracy.");

	ImGui::PopItemWidth();


	ImGui::SeparatorText("Completion");

	//Zero WCS?
		ImGui::Checkbox("Zero WCS", &bZeroWCS);

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

void ProbeOperation_SingleAxis::ParseToJSON(json& j)
{
	ProbeOperation::ParseToJSON(j);

	j["Probe Diameter"] = fProbeTipDiameter;
	j["Travel"] = fTravel;

	if (iAxisDirectionIndex == 0)
		j["Axis"] = "X-";
	else if (iAxisDirectionIndex == 1)
		j["Axis"] = "X+";
	else if (iAxisDirectionIndex == 2)
		j["Axis"] = "Y-";
	else if (iAxisDirectionIndex == 3)
		j["Axis"] = "Y+";
	else if (iAxisDirectionIndex == 4)
		j["Axis"] = "Z-";
}

void ProbeOperation_SingleAxis::ParseFromJSON(const json& j)
{
	ProbeOperation::ParseFromJSON(j);

	fProbeTipDiameter = j.value("Probe Diameter", 2.0f);
	fTravel = j.value("Travel", 5.0f);

	std::string strAxis = j.value("Axis", "");

	iAxisDirectionIndex = -1;
	if (strAxis == "X-")
		iAxisDirectionIndex = 0;
	else if (strAxis == "X+")
		iAxisDirectionIndex = 1;
	else if (strAxis == "Y-")
		iAxisDirectionIndex = 2;
	else if (strAxis == "Y+")
		iAxisDirectionIndex = 3;
	else if (strAxis == "Z-")
		iAxisDirectionIndex = 4;
}