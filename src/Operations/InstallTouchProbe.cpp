#include "../Platforms/Platforms.h"

#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <chrono>
using namespace std::chrono;

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutScript.h"

#include "../Probing/Probing.h"


#define STATE_INSTALLPROBE_START		0
#define STATE_INSTALLPROBE_RETURNTOOL	1	//Return a tool currently in the spindle
#define STATE_INSTALLPROBE_CLEARANCE	2	//Go to clearance position
#define STATE_INSTALLPROBE_OPENCOLLET	3	//Open the collet
#define STATE_INSTALLPROBE_CONFIRM		4	//Confirm ready
#define STATE_INSTALLPROBE_CLOSECOLLET	5	//Close the collet
#define STATE_INSTALLPROBE_CALIBRATE	6	//Calibrate the TLO
#define STATE_INSTALLPROBE_TEST			7	//Test functionality


static steady_clock::time_point LastStatusRqst;
static bool bStepRunning;


CloutScript_Op_InstallTouchProbe::CloutScript_Op_InstallTouchProbe()
{
	bConfirmFunction = true;
	bProbeStatus = false;
}

void CloutScript_Op_InstallTouchProbe::StateMachine()
{
	switch (iState)
	{
		case STATE_INSTALLPROBE_START:
			bProbeStatus = false;
			if (MachineStatus.iCurrentTool != -1)
			{
				Comms_SendString("M6 T-1");		//Return this tool
				iState = STATE_INSTALLPROBE_RETURNTOOL;
			}
			else
			{
				Comms_SendString("G28");	//Go to clearance position
				Comms_HideReply(CARVERA_MSG_G28);
				iState = STATE_INSTALLPROBE_CLEARANCE;
			}
		break;

		case STATE_INSTALLPROBE_RETURNTOOL:
			if (Comms_PopMessageOfType(CARVERA_MSG_ATC_DONE))
			{
				Comms_SendString("G28");	//Go to clearance position
				Comms_HideReply(CARVERA_MSG_G28);
				iState = STATE_INSTALLPROBE_CLEARANCE;
			}
		break;

		case STATE_INSTALLPROBE_CLEARANCE:  //Currently moving to clearance position
			if (MachineStatus.Status == Carvera::Status::Idle)
			{
				Comms_SendString("M490.2"); //It should already be opened, but just in case
				iState = STATE_INSTALLPROBE_OPENCOLLET;
			}
		break;

		case STATE_INSTALLPROBE_OPENCOLLET:  //Currently opening the collet
			if (Comms_PopMessageOfType(CARVERA_MSG_ATC_LOOSED))
			{
				ImGui::OpenPopup("Close Collet?##InstallTouchProbe");
				iState = STATE_INSTALLPROBE_CONFIRM;
			}
		break;

		case STATE_INSTALLPROBE_CONFIRM: //Waiting for user to confirm ready
			if (ImGui::BeginPopupModal("Close Collet?##InstallTouchProbe", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Confirm probe installed and ready to close collet?");
				ImGui::Separator();

				if (ImGui::Button("OK", ScaledByWindowScale(120, 0)))
				{
					ImGui::CloseCurrentPopup();

					iState = STATE_INSTALLPROBE_CLOSECOLLET;
					Comms_SendString("M490.1"); //Send the command
				}
				ImGui::SetItemDefaultFocus();
				ImGui::EndPopup();
			}
		break;

		case STATE_INSTALLPROBE_CLOSECOLLET:
			if (Comms_PopMessageOfType(CARVERA_MSG_ATC_CLAMPED))
			{
				Comms_SendString("M491");
				bStepRunning = false;
				iState = STATE_INSTALLPROBE_CALIBRATE;
			}
		break;

		case STATE_INSTALLPROBE_CALIBRATE:
			if (!bStepRunning)
			{
				//Wait for the update that shows it's in progress
					if (MachineStatus.Status == Carvera::Status::Busy)
						bStepRunning = true;
			}
			else
			{
				//Wait for the update that shows it's done
				if (MachineStatus.Status == Carvera::Status::Idle)
				{
					if (bConfirmFunction)
					{
						ImGui::OpenPopup("Probe Test##InstallTouchProbe");
						LastStatusRqst = steady_clock::now();
						iState = STATE_INSTALLPROBE_TEST;
					}
					else
						iState = STATE_OP_COMPLETE;
				}
			}
		break;

		case STATE_INSTALLPROBE_TEST:
		{
			//Request status
				if (TimeSince_ms(LastStatusRqst) > 300)
				{
					Comms_SendString("*", false);
					Comms_HideReply(CARVERA_MSG_DEBUG);
					LastStatusRqst = steady_clock::now();
				}

			//Check status of the probe
			if (ImGui::BeginPopupModal("Probe Test##InstallTouchProbe", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Test functionality of touch probe here.\nWhen finished, click OK.");
				ImGui::Separator();

				ImGui::Text("Probe status:  ");
				ImGui::SameLine();
				if (MachineStatus.bProbeTriggered)
					ImGui::Text("ON");
				else
					ImGui::Text("OFF");

				ImGui::Dummy(ScaledByWindowScale(0.0f, 15.0f)); //Extra empty space before the buttons

				//TODO: Add an abort button in case it doesn't work
				if (ImGui::Button("OK", ScaledByWindowScale(120, 0)))
				{
					ImGui::CloseCurrentPopup();

					iState = STATE_OP_COMPLETE;
				}
				ImGui::SetItemDefaultFocus();
				ImGui::EndPopup();
			}
		}
		break;
	}
}

void CloutScript_Op_InstallTouchProbe::DrawDetailTab()
{
	//Description
	ImGui::Text("Work with the user to install a touch probe");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::Checkbox("Confirm function##DrawTab_InstallTouchProbe", &bConfirmFunction);
	HelpMarker("If selected, after installation of the probe you will be asked to touch the probe to confirm it works.");
}

void CloutScript_Op_InstallTouchProbe::DrawEditorSummaryInfo()
{
	ImGui::Text("Confirm function: %s", (bConfirmFunction ? "Yes" : "No"));
}

void CloutScript_Op_InstallTouchProbe::ParseFromJSON(const json& j)
{
	bConfirmFunction = j.value("Confirm Function", true);
}

void CloutScript_Op_InstallTouchProbe::ParseToJSON(json& j)
{
	CloutScript_Op::ParseToJSON(j);

	j["Confirm Function"] = bConfirmFunction;
}