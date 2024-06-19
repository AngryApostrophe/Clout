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

CloutProgram_Op_OpenCollet::CloutProgram_Op_OpenCollet()
{
}

void CloutProgram_Op_OpenCollet::StateMachine()
{
	switch (iState)
	{
		case STATE_OPENCOLLET_START:
			if (bConfirmWithOperator)
			{
				ImGui::OpenPopup("Open Collet?");
				//iState = STATE_OPENCOLLET_CONFIRM;

				if (ImGui::BeginPopupModal("Open Collet?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Confirm ready to open collet and release tool?");
					ImGui::Separator();

					if (ImGui::Button("OK", ScaledByWindowScale(120, 0)))
					{
						ImGui::CloseCurrentPopup();
						
						iState = STATE_OPENCOLLET_RUNNING;
						Comms_SendString("M490.2"); //Send the command
					}
					ImGui::SetItemDefaultFocus();
					ImGui::EndPopup();
				}
			}
			else
			{
				Comms_SendString("M490.2"); //Send the command
				iState = STATE_OPENCOLLET_RUNNING;
			}
		break;

		case STATE_OPENCOLLET_RUNNING:
			if (Comms_PopMessageOfType(CARVERA_MSG_ATC_LOOSED))
				iState = STATE_OP_COMPLETE;
		break;
	}
}

void CloutProgram_Op_OpenCollet::DrawDetailTab()
{
	//Description
	ImGui::Text("Open the collet to manually remove a tool");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup

	ImGui::Checkbox("Confirm with operator##DrawTab_OpenCollet", &bConfirmWithOperator);
	HelpMarker("If selected, operator will be asked to confirm ready to open collet to avoid dropping a tool");
}

void CloutProgram_Op_OpenCollet::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_OpenCollet::ParseFromJSON(const json& j)
{
	CloutProgram_Op::ParseFromJSON(j);

	bConfirmWithOperator = j.value("Confirm", true);
}

void CloutProgram_Op_OpenCollet::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["Confirm"] = bConfirmWithOperator;
}