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

#include "../FileTransfer.h"


#define STATE_CUSTOMCODE_START		0
#define STATE_CUSTOMCODE_UPLOAD		1
#define STATE_CUSTOMCODE_RUNNING	2

static bool bStepRunning;


CloutProgram_Op_Custom_GCode::CloutProgram_Op_Custom_GCode()
{
	iState = 0;
	strGCode.clear();
}

void CloutProgram_Op_Custom_GCode::StateMachine()
{
	switch (iState)
	{
	case STATE_CUSTOMCODE_START:
		strTransmitted = strGCode;
		strTransmitted += "\nM400\nM2 (Clout sync)\n";
		FileTransfer_BeginUploadFromMemory(&strTransmitted);
		iState = STATE_CUSTOMCODE_UPLOAD;
		break;

	case STATE_CUSTOMCODE_UPLOAD:
		if (FileTransfer_DoTransfer())
		{
			bStepRunning = false;
			iState = STATE_CUSTOMCODE_RUNNING;
			Comms_SendString("play sd/gcodes/Clout.nc -v");	//Start running that file
		}
		break;

	case STATE_CUSTOMCODE_RUNNING:
		if (!bStepRunning)
		{
			//Wait for it to begin playing
			if (Comms_PopMessageOfType(CARVERA_MSG_PLAYING))
				bStepRunning = true;
		}
		else
		{	
			//Now wait for it to go idle
			if (Comms_PopMessageOfType(CARVERA_MSG_CLOUTSYNC))
			//if (MachineStatus.Status == Carvera::Status::Idle)
				iState = STATE_OP_COMPLETE;
			break;
		}
	}
}

void CloutProgram_Op_Custom_GCode::DrawDetailTab()
{
	//Description
	ImGui::Text("Run custom G Code as entered below");

	ImGui::Dummy(ScaledByWindowScale(0.0f, 5.0f)); //Extra empty space before the setup
	const ImVec2 TextViewSize(ScaledByWindowScale(475, 500));
	ImGui::InputTextMultiline("##CustomGCode", &strGCode, TextViewSize);
}

void CloutProgram_Op_Custom_GCode::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_Custom_GCode::ParseFromJSON(const json& j)
{
	strGCode = j.value("Code", "");
}

void CloutProgram_Op_Custom_GCode::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);

	j["Code"] = strGCode;
}