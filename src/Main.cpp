#include "Platforms/Platforms.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <stdio.h>

#include "Clout.h"

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DEFINE
#include "deprecated/stb.h"  //This is supposedly going away, but it works for now.  I use it for decompressing images stored in header files


// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif


//Global vars
	ImGuiID dockspace_id;
	GLFWwindow* glfwWindow;	//The main glfw Window object

//Compressed resources
	#include "Resources/ProggyClean.h"


// Main code
int main(int, char**)
{
	//Initialize glfw
		if (!glfwInit())
			return 1;

		float xscale, yscale;
		glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);

	// Decide GL+GLSL versions
		#if defined(IMGUI_IMPL_OPENGL_ES2)
		    // GL ES 2.0 + GLSL 100
			const char* glsl_version = "#version 100";
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
		#elif defined(__APPLE__)
		    // GL 3.2 + GLSL 150
			const char* glsl_version = "#version 150";
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
		#else
		    // GL 3.0 + GLSL 130
			const char* glsl_version = "#version 130";
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		#endif

    // Create window with graphics context
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, true);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindow = glfwCreateWindow(1280, 910, "Clout", nullptr, nullptr);
		if (glfwWindow == 0)
			return 1;

		glfwMakeContextCurrent(glfwWindow);
		glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();


	//Set some ImGui flags
		//IO Flags
			io = &ImGui::GetIO();
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		//Enable Keyboard Controls
			io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;		//Enable Docking
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;	//Allow us to recenter the mouse when looking around in Model Viewer

			io->IniFilename = ""; //Disable loading of the ini file

		//Main window flags
			ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_MenuBar |*/ ImGuiWindowFlags_NoDocking; // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into, because it would be confusing to have two docking targets within each others.
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		//Dock Node flags
			ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_NoWindowMenuButton;
			//Useful flags for later:
				//ImGuiDockNodeFlags_NoUndocking - Prevents undocking or moving of windows
				//ImGuiDockNodeFlags_NoResize - No resize of windows, though still adjusts on main window resize

	//Set ImGui styles
		ImGui::StyleColorsDark();
		ImVec4 WindowBGcolor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		//style.ScaleAllSizes(xscale);
		if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.FrameRounding = 3;

	// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
		#ifdef __EMSCRIPTEN__
			ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
		#endif
			ImGui_ImplOpenGL3_Init(glsl_version);

	//Call our Init routine
		Clout_Init();

		float prev_scale = 0;

	// Main loop
	#ifdef __EMSCRIPTEN__
	    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
		io.IniFilename = nullptr;
		EMSCRIPTEN_MAINLOOP_BEGIN
	#else
		while (!glfwWindowShouldClose(glfwWindow))
	#endif
		{
			// Poll and handle events (inputs, window resize, etc.)
				glfwPollEvents();

			//Scale the window appropriately
				glfwGetWindowContentScale(glfwWindow, &xscale, &yscale);
				if (xscale != prev_scale) 
				{
					prev_scale = xscale;
					io->Fonts->Clear();
					io->Fonts->AddFontFromMemoryCompressedTTF(ProggyClean_compressed_data, ProggyClean_compressed_size, xscale * 13.0f);	//FONT_MAIN
					io->Fonts->AddFontFromMemoryCompressedTTF(ProggyClean_compressed_data, ProggyClean_compressed_size, xscale * 26.0f);	//FONT_POS_LARGE
					io->Fonts->AddFontFromMemoryCompressedTTF(ProggyClean_compressed_data, ProggyClean_compressed_size, xscale * 18.0f);	//FONT_POS_SMALL

					io->Fonts->Build();
					ImGui_ImplOpenGL3_DestroyFontsTexture();
					ImGui_ImplOpenGL3_CreateFontsTexture();
				}

			// Start the Dear ImGui frame
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

			//Create the dockspace for all other windows to dock to
				const ImGuiViewport* viewport = ImGui::GetMainViewport();

			//Make the dockspace window fill the entire app window
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("Dockspace Window", 0, window_flags);
				ImGui::PopStyleVar(3);

				dockspace_id = ImGui::GetID("dockspace");

			//Default window layout if the .ini isn't present
				if (ImGui::DockBuilderGetNode(dockspace_id) == NULL)
					Clout_CreateDefaultLayout();
				else
					ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			//Call the main processing function in Clout.cpp
				Clout_MainLoop();

			//End the dockspace container
				ImGui::End();

			// Rendering
				ImGui::Render();
				int display_w, display_h;
				glfwGetFramebufferSize(glfwWindow, &display_w, &display_h);
				glViewport(0, 0, display_w, display_h);
				glClearColor(WindowBGcolor.x, WindowBGcolor.y, WindowBGcolor.z, WindowBGcolor.w);
				glClear(GL_COLOR_BUFFER_BIT);
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
				if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					GLFWwindow* backup_current_context = glfwGetCurrentContext();
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					glfwMakeContextCurrent(backup_current_context);
				}

			//Show on screen
				glfwSwapBuffers(glfwWindow);
		}
	#ifdef __EMSCRIPTEN__
		EMSCRIPTEN_MAINLOOP_END;
	#endif

	//Shutdown our stuff
		Clout_Shutdown();

	// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

	//Shutdown
		glfwDestroyWindow(glfwWindow);
		glfwTerminate();

	return 0;
}
