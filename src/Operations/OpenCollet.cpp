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
}

void CloutProgram_Op_OpenCollet::DrawDetailTab()
{
}

void CloutProgram_Op_OpenCollet::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_OpenCollet::ParseFromJSON(const json& j)
{
	CloutProgram_Op::ParseFromJSON(j);
}

void CloutProgram_Op_OpenCollet::ParseToJSON(json& j)
{
	CloutProgram_Op::ParseToJSON(j);
}