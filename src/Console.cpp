//This is pretty much right out of the Dear ImgUi demo for now.  Lots of work to be done

#include "Platforms/Platforms.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "Clout.h"
#include "Console.h"
#include "Comms.h"

CommsConsole Console; //The main console object


CommsConsole::CommsConsole()
{
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPos = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	Commands.push_back("HELP");
	Commands.push_back("HISTORY");
	Commands.push_back("CLEAR");
	Commands.push_back("CLASSIFY");
	AutoScroll = true;
	ScrollToBottom = false;

	NewMutex(&ConsoleMutex);
}


CommsConsole::~CommsConsole()
{
	ClearLog();
	for (int i = 0; i < History.Size; i++)
		ImGui::MemFree(History[i]);

	CloseMutex(&ConsoleMutex);
}

void CommsConsole::ClearLog()
{
	for (int i = 0; i < Items.Size; i++)
		ImGui::MemFree(Items[i]);
	Items.clear();
}

void CommsConsole::Draw()
{
	if (WaitForMutex(&ConsoleMutex, true) != MUTEX_RESULT_SUCCESS)
		return;

	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console", 0))
	{
		ImGui::End();
		ReleaseMutex(&ConsoleMutex);
		return;
	}

	
	ImGui::TextWrapped("Enter 'HELP' for help.");

	if (ImGui::SmallButton("Add Debug Text")) { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
	ImGui::SameLine();
	if (ImGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear")) { ClearLog(); }
	ImGui::SameLine();
	bool copy_to_clipboard = ImGui::SmallButton("Copy");
	//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

	ImGui::Separator();

	// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &AutoScroll);
		ImGui::EndPopup();
	}

	// Options, Filter
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
	ImGui::Separator();

	// Reserve enough left-over height for 1 separator + 1 input text
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NavFlattened))
	{
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets.
		// If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
		// to only process visible items. The clipper will automatically measure the height of your first item and then
		// "seek" to display only items in the visible area.
		// To use the clipper we can replace your standard loop:
		//      for (int i = 0; i < Items.Size; i++)
		//   With:
		//      ImGuiListClipper clipper;
		//      clipper.Begin(Items.Size);
		//      while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// - That your items are evenly spaced (same height)
		// - That you have cheap random access to your elements (you can access them given their index,
		//   without processing all the ones before)
		// You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
		// We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
		// or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage
		// to improve this example code!
		// If your items are of variable height:
		// - Split them into same height items would be simpler and facilitate random-seeking into your list.
		// - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (copy_to_clipboard)
			ImGui::LogToClipboard();
		for (CONSOLE_ITEM *item : Items)
		{
			if (!Filter.PassFilter(item->string))
				continue;

			// Normally you would store more information in your item than just a string.
			// (e.g. make Items[] an array of structure, store color/type etc.)
			ImVec4 color;
			bool has_color = false;
			if (strstr(item->string, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
			else if (strncmp(item->string, "# ", 2) == 0) { color = ImVec4(0.75f, 0.5f, 0.4f, 1.0f); has_color = true; }
			else if (item->Type == ITEM_TYPE_ERROR) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
			else if (item->Type == ITEM_TYPE_RECV) { color = ImVec4(0.3f, 0.79f, 0.69f, 1.0f); has_color = true; }
			else if (item->Type == ITEM_TYPE_SENT) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(item->string);
			if (has_color)
				ImGui::PopStyleColor();
		}
		if (copy_to_clipboard)
			ImGui::LogFinish();

		// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
		// Using a scrollbar or mouse-wheel will take away from the bottom edge.
		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
	{
		char* s = InputBuf;
		Strtrim(s);
		if (s[0])
			ExecCommand(s);
		strcpy(s, "");
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

	ImGui::End();

	ReleaseMutex(&ConsoleMutex);
}

void CommsConsole::ExecCommand(const char* command_line)
{
	if (WaitForMutex(&ConsoleMutex, true) != MUTEX_RESULT_SUCCESS)
		return;

	AddLog("# %s\n", command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.Size - 1; i >= 0; i--)
		if (Stricmp(History[i], command_line) == 0)
		{
			ImGui::MemFree(History[i]);
			History.erase(History.begin() + i);
			break;
		}
	History.push_back(Strdup(command_line));

	// Process command
	if (command_line[0] != '>') //Send strings to carvera
	{
		Comms_SendString(command_line + 1); // +1 so we don't send the >
	}
	else
	{
		char *c = (char*)command_line + 1;

		if (Stricmp(c, "CLEAR") == 0)
		{
			ClearLog();
		}
		else if (Stricmp(c, "HELP") == 0)
		{
			AddLog("Commands:");
			for (int i = 0; i < Commands.Size; i++)
				AddLog("- %s", Commands[i]);
		}
		else if (Stricmp(c, "HISTORY") == 0)
		{
			int first = History.Size - 10;
			for (int i = first > 0 ? first : 0; i < History.Size; i++)
				AddLog("%3d: %s\n", i, History[i]);
		}
		else
		{
			AddLog("Unknown command: '%s'\n", c);
		}
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;

	ReleaseMutex(&ConsoleMutex);
}

void CommsConsole::AddLog(const char* fmt, ...)
{
	if (WaitForMutex(&ConsoleMutex, true) != MUTEX_RESULT_SUCCESS)
		return;

	// FIXME-OPT
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);

	CONSOLE_ITEM* NewItem = new CONSOLE_ITEM;
	NewItem->Type = ITEM_TYPE_NONE;
	strcpy(NewItem->string, buf);
	Items.push_back(NewItem);

	ReleaseMutex(&ConsoleMutex);
}

void CommsConsole::AddLog(CommsConsole::CONSOLE_ITEM_TYPE Type, const char* fmt, ...)
{
	if (WaitForMutex(&ConsoleMutex, true) != MUTEX_RESULT_SUCCESS)
		return;

	// FIXME-OPT
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);

	CONSOLE_ITEM* NewItem = new CONSOLE_ITEM;
	NewItem->Type = Type;
	strcpy(NewItem->string, buf);
	Items.push_back(NewItem);

	ReleaseMutex(&ConsoleMutex);
}