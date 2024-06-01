# Properties->C/C++->General->Additional Include Directories
include_directories ("${PROJECT_SOURCE_DIR}/src/external/imgui"
"${PROJECT_SOURCE_DIR}/src/external/imgui/backends"
"${PROJECT_SOURCE_DIR}/src/external/assimp/include"
"${PROJECT_SOURCE_DIR}/src/external/stb"
"${PROJECT_SOURCE_DIR}/src/external/glm/glm"
"${PROJECT_SOURCE_DIR}/src/external/glew/include"
)

set  (IMGUI_SOURCES
      "${PROJECT_SOURCE_DIR}/src/external/imgui/imgui.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_draw.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_tables.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_widgets.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_win32.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imconfig.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3_loader.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_win32.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_internal.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_rectpack.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_textedit.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_truetype.h"
	)

set  (CLOUT_SOURCES
      "${PROJECT_SOURCE_DIR}/src/winmain.cpp"
	"${PROJECT_SOURCE_DIR}/src/Clout.cpp"
	"${PROJECT_SOURCE_DIR}/src/Probing.cpp"
	"${PROJECT_SOURCE_DIR}/src/ModelViewer.cpp"
	"${PROJECT_SOURCE_DIR}/src/Helpers.cpp"
	"${PROJECT_SOURCE_DIR}/src/Console.cpp"
	"${PROJECT_SOURCE_DIR}/src/Comms.cpp"
	)
	
set  (CLOUT_HEADERS
	"${PROJECT_SOURCE_DIR}/src/Clout.h"
	"${PROJECT_SOURCE_DIR}/src/Probing.h"
	"${PROJECT_SOURCE_DIR}/src/ModelViewer.h"
	"${PROJECT_SOURCE_DIR}/src/Helpers.h"
	"${PROJECT_SOURCE_DIR}/src/Console.h"
	"${PROJECT_SOURCE_DIR}/src/Comms.h"
	)

source_group("ImgUi" FILES ${IMGUI_SOURCES})
source_group("Header Files" FILES ${CLOUT_HEADERS})
source_group("Source Files" FILES ${CLOUT_SOURCES})

# Set Properties->General->Configuration Type to Application(.exe)
# Creates app.exe with the listed sources (main.cxx)
# Adds sources to the Solution Explorer
add_executable (CloutProj WIN32 ${CLOUT_SOURCES} ${IMGUI_SOURCES} ${CLOUT_HEADERS})

# Properties->Linker->Input->Additional Dependencies
target_link_libraries (CloutProj 
"${PROJECT_SOURCE_DIR}/src/external/assimp/lib/Release/assimp-vc143-mt.lib"
"${PROJECT_SOURCE_DIR}/src/external/glew/lib/Release/x64/glew32.lib"
)

# Creates a folder "executables" and adds target 
# project (Clout.vcproj) under it
# set_property(TARGET Clout PROPERTY FOLDER "Clout")

# Properties->General->Output Directory
set_target_properties(CloutProj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Adds logic to INSTALL.vcproj to copy app.exe to destination directory
install (TARGETS CloutProj RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin)

# Set the Visual Studio working directory so we can run the app
set_property(TARGET CloutProj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

#Set the Visual Studio startup project, so we can run and debug the app
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CloutProj)
