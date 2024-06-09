#include <Windows.h>
#include <stdio.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiFileDialog.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "../CloutProgram.h"

CloutProgram_Op_Custom_GCode::CloutProgram_Op_Custom_GCode()
{
	szGCode.clear();
}

void CloutProgram_Op_Custom_GCode::StateMachine()
{
}

void CloutProgram_Op_Custom_GCode::DrawDetailTab()
{
}

void CloutProgram_Op_Custom_GCode::DrawEditorSummaryInfo()
{
}

void CloutProgram_Op_Custom_GCode::ParseFromJSON(const json& j)
{
}