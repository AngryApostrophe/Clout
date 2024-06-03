#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Ws2_32.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <Windows.h>

#include "Clout.h"

//Global vars
	ImGuiID dockspace_id;

// Data stored per platform window
	struct WGL_WindowData { HDC hDC; };

// Data
	static HGLRC            g_hRC;
	static WGL_WindowData   g_MainWindow;
	static int              g_Width;
	static int              g_Height;

// Forward declarations of helper functions
	bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
	void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
	void ResetDeviceWGL();
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Main code
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	// Create application window
		WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Clout", nullptr };
		RegisterClassExW(&wc);
	
		HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Clout", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 950, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize OpenGL
		if (!CreateDeviceWGL(hwnd, &g_MainWindow))
		{
			CleanupDeviceWGL(hwnd, &g_MainWindow);
			DestroyWindow(hwnd);
			UnregisterClassW(wc.lpszClassName, wc.hInstance);

			return 1;
		}
		wglMakeCurrent(g_MainWindow.hDC, g_hRC);

	// Show the window
		ShowWindow(hwnd, SW_SHOWDEFAULT);

	// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
	
	//Set some ImGui flags
		//IO Flags
			io = &ImGui::GetIO();
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
			io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;		//Enable Docking
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos; //Allow us to recenter the mouse when looking around in Model Viewer

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

	// Setup Platform/Renderer backends
		ImGui_ImplWin32_InitForOpenGL(hwnd);
		ImGui_ImplOpenGL3_Init();

    	//Call our Init routine
		Clout_Init();

    // Main loop
	    bool bLoop = true;
	    while (bLoop)
	    {
		//Process Windows message s
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
					bLoop = false;
			}

		//Begin Drawing	
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplWin32_NewFrame();
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


				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Options"))
					{
						ImGui::MenuItem("Settings");
						ImGui::Separator();
						ImGui::MenuItem("Exit");

						ImGui::EndMenu();
					}
				
					ImGui::EndMenuBar();
				}
		
		//Call the main processing function in Clout.cpp
			Clout_MainLoop();

		//End the dockspace container
			ImGui::End();

		//Rendering
			ImGui::Render();
			glViewport(0, 0, g_Width, g_Height);
			glClearColor(WindowBGcolor.x, WindowBGcolor.y, WindowBGcolor.z, WindowBGcolor.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//Show on screen
			SwapBuffers(g_MainWindow.hDC);
	    }

	//Shutdown our stuff
	    Clout_Shutdown();

	//Shutdown ImGui
	    ImGui_ImplOpenGL3_Shutdown();
	    ImGui_ImplWin32_Shutdown();
	    ImGui::DestroyContext();

	//Clean up everything else
		CleanupDeviceWGL(hwnd, &g_MainWindow);
		wglDeleteContext(g_hRC);
		DestroyWindow(hwnd);
		UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	HDC hDc = ::GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	const int pf = ::ChoosePixelFormat(hDc, &pfd);
	if (pf == 0)
		return false;
	if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
		return false;
	::ReleaseDC(hWnd, hDc);

	data->hDC = ::GetDC(hWnd);
	if (!g_hRC)
		g_hRC = wglCreateContext(data->hDC);
	return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	wglMakeCurrent(nullptr, nullptr);
	::ReleaseDC(hWnd, data->hDC);
}

/// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			g_Width = LOWORD(lParam);
			g_Height = HIWORD(lParam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

