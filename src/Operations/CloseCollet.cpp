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

CloutProgram_Op_CloseCollet::CloutProgram_Op_CloseCollet()
{
}

void CloutProgram_Op_CloseCollet::StateMachine()
{
}

void CloutProgram_Op_CloseCollet::DrawDetailTab()
{
}

void CloutProgram_Op_CloseCollet::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_CloseCollet::ParseFromJSON(const json& j)
{
	CloutProgram_Op::ParseFromJSON(j);
}

void CloutProgram_Op_CloseCollet::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);
}