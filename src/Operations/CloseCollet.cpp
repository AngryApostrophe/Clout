#include "../Platforms/Platforms.h"

#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutProgram.h"


#define STATE_CLOSECOLLET_START		0
#define STATE_CLOSECOLLET_RUNNING	1



CloutProgram_Op_CloseCollet::CloutProgram_Op_CloseCollet()
{
}

void CloutProgram_Op_CloseCollet::StateMachine()
{
	switch (iState)
	{
		case STATE_CLOSECOLLET_START:
			if (bConfirmWithOperator)
			{
				ImGui::OpenPopup("Close Collet?");

				if (ImGui::BeginPopupModal("Close Collet?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Confirm ready to close collet and grab tool?");
					ImGui::Separator();

					if (ImGui::Button("OK", ScaledByWindowScale(120, 0)))
					{
						ImGui::CloseCurrentPopup();

						iState = STATE_CLOSECOLLET_RUNNING;
						Comms_SendString("M490.1"); //Send the command
					}
					ImGui::SetItemDefaultFocus();
					ImGui::EndPopup();
				}
			}
			else
			{
				Comms_SendString("M490.1"); //Send the command
				iState = STATE_CLOSECOLLET_RUNNING;
			}
		break;

		case STATE_CLOSECOLLET_RUNNING:
			if (Comms_PopMessageOfType(CARVERA_MSG_ATC_CLAMPED))
				iState = STATE_OP_COMPLETE;
		break;
	}
}

void CloutProgram_Op_CloseCollet::DrawDetailTab()
{
	//Description
		ImGui::Text("Clamp the collet onto a manually inserted tool");

		ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

		ImGui::Checkbox("Confirm with operator##DrawTab_CloseCollet", &bConfirmWithOperator);
		HelpMarker("If selected, operator will be asked to confirm ready to clamp the collet");
}

void CloutProgram_Op_CloseCollet::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_CloseCollet::ParseFromJSON(const json& j)
{
	CloutProgram_Op::ParseFromJSON(j);

	bConfirmWithOperator = j.value("Confirm", true);
}

void CloutProgram_Op_CloseCollet::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["Confirm"] = bConfirmWithOperator;
}