# Properties->C/C++->General->Additional Include Directories
include_directories ("${PROJECT_SOURCE_DIR}/src/external/imgui"
"${PROJECT_SOURCE_DIR}/src/external/imgui/backends"
"${PROJECT_SOURCE_DIR}/src/external/assimp/include"
"${PROJECT_SOURCE_DIR}/src/external/stb"
"${PROJECT_SOURCE_DIR}/src/external/glm/glm"
"${PROJECT_SOURCE_DIR}/src/external/glew/include"
"${PROJECT_SOURCE_DIR}/src/external/json/single_include/nlohmann"
"${PROJECT_SOURCE_DIR}/src/external/ImGuiFileDialog"
"${PROJECT_SOURCE_DIR}/src/external/glfw/include"
)

set  (IMGUI_SOURCES
      "${PROJECT_SOURCE_DIR}/src/external/imgui/imgui.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_draw.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_tables.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_widgets.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_glfw.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/misc/cpp/imgui_stdlib.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imconfig.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_opengl3_loader.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/backends/imgui_impl_glfw.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imgui_internal.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_rectpack.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_textedit.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/imstb_truetype.h"
	"${PROJECT_SOURCE_DIR}/src/external/imgui/misc/cpp/imgui_stdlib.h"
	)
	
set  (MISC_LIB_SOURCES
      "${PROJECT_SOURCE_DIR}/src/external/ImGuiFileDialog/ImGuiFileDialog.cpp"
	"${PROJECT_SOURCE_DIR}/src/external/ImGuiFileDialog/ImGuiFileDialog.h"
	"${PROJECT_SOURCE_DIR}/src/ImGuiFileDialogConfig.h"
	)

set  (CLOUT_SOURCES
      "${PROJECT_SOURCE_DIR}/src/Main.cpp"
	"${PROJECT_SOURCE_DIR}/src/Clout.cpp"
	"${PROJECT_SOURCE_DIR}/src/ModelViewer.cpp"
	"${PROJECT_SOURCE_DIR}/src/Helpers.cpp"
	"${PROJECT_SOURCE_DIR}/src/Console.cpp"
	"${PROJECT_SOURCE_DIR}/src/Comms.cpp"
	"${PROJECT_SOURCE_DIR}/src/ProgramEditor.cpp"
	"${PROJECT_SOURCE_DIR}/src/CloutProgram.cpp"
	"${PROJECT_SOURCE_DIR}/src/OperationQueue.cpp"
	)
	
set  (CLOUT_SOURCES_PROBING
      "${PROJECT_SOURCE_DIR}/src/Probing/Probing.cpp"
      "${PROJECT_SOURCE_DIR}/src/Probing/Probing.h"
      "${PROJECT_SOURCE_DIR}/src/Probing/BoreCenter.cpp"
      "${PROJECT_SOURCE_DIR}/src/Probing/BossCenter.cpp"
      "${PROJECT_SOURCE_DIR}/src/Probing/PocketCenter.cpp"
      "${PROJECT_SOURCE_DIR}/src/Probing/WebCenter.cpp"
      "${PROJECT_SOURCE_DIR}/src/Probing/SingleAxis.cpp"
	)
	
set  (CLOUT_SOURCES_OPERATIONS
      "${PROJECT_SOURCE_DIR}/src/Operations/RunGCodeFile.cpp"
      "${PROJECT_SOURCE_DIR}/src/Operations/ATCToolChange.cpp"
      "${PROJECT_SOURCE_DIR}/src/Operations/ProbeOp.cpp"
      "${PROJECT_SOURCE_DIR}/src/Operations/InstallTouchProbe.cpp"
      "${PROJECT_SOURCE_DIR}/src/Operations/RapidTo.cpp"
      "${PROJECT_SOURCE_DIR}/src/Operations/CustomGCode.cpp"
      )
	
set  (CLOUT_HEADERS
	"${PROJECT_SOURCE_DIR}/src/Clout.h"
	"${PROJECT_SOURCE_DIR}/src/ModelViewer.h"
	"${PROJECT_SOURCE_DIR}/src/Helpers.h"
	"${PROJECT_SOURCE_DIR}/src/Console.h"
	"${PROJECT_SOURCE_DIR}/src/Comms.h"
	"${PROJECT_SOURCE_DIR}/src/ProgramEditor.h"
	"${PROJECT_SOURCE_DIR}/src/CloutProgram.h"
	"${PROJECT_SOURCE_DIR}/src/OperationQueue.h"
	)

	
IF(WIN32)
    set  (CLOUT_SOURCES_PLATFORM
	"${PROJECT_SOURCE_DIR}/src/Platforms/Platforms.h"
	"${PROJECT_SOURCE_DIR}/src/Platforms/Win32.cpp"
	"${PROJECT_SOURCE_DIR}/src/Platforms/Win32.h"
      )
ELSE(WIN32)
    set  (CLOUT_SOURCES_PLATFORM
    	"${PROJECT_SOURCE_DIR}/src/Platforms/Platforms.h"
	"${PROJECT_SOURCE_DIR}/src/Platforms/Linux.cpp"
	"${PROJECT_SOURCE_DIR}/src/Platforms/Linux.h"
      )
ENDIF(WIN32)

source_group("ImgUi" FILES ${IMGUI_SOURCES})
source_group("Misc Libraries" FILES ${MISC_LIB_SOURCES})
source_group("Header Files" FILES ${CLOUT_HEADERS})
source_group("Source Files" FILES ${CLOUT_SOURCES})
source_group("Source Files/Probing" FILES ${CLOUT_SOURCES_PROBING})
source_group("Source Files/Operations" FILES ${CLOUT_SOURCES_OPERATIONS})
source_group("Source Files/Platforms" FILES ${CLOUT_SOURCES_PLATFORM})



# Set Properties->General->Configuration Type to Application(.exe)
# Creates app.exe with the listed sources (main.cxx)
# Adds sources to the Solution Explorer

IF(WIN32)
	# Set the subsystem and entry point for no console and main()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	add_executable (Clout WIN32 ${CLOUT_SOURCES} ${CLOUT_SOURCES_PROBING} ${CLOUT_SOURCES_OPERATIONS} ${IMGUI_SOURCES} ${MISC_LIB_SOURCES} ${CLOUT_HEADERS} ${CLOUT_SOURCES_PLATFORM})
ELSE(WIN32)
	add_executable (Clout WIN32 ${CLOUT_SOURCES} ${CLOUT_SOURCES_PROBING} ${CLOUT_SOURCES_OPERATIONS} ${IMGUI_SOURCES} ${MISC_LIB_SOURCES} ${CLOUT_HEADERS} ${CLOUT_SOURCES_PLATFORM})
ENDIF(WIN32)


#Add opengl
#Find OpenGL
FIND_PACKAGE(OpenGL)
IF (OPENGL_FOUND)
	MESSAGE("OPENGL FOUND")
	MESSAGE(${OPENGL_LIBRARIES})
ELSE (OPENGL_FOUND)
	MESSAGE("OPENGL NOT FOUND")
	SET(OPENGL_gl_LIBRARY opengl32)
	SET(OPENGL_glu_LIBRARY glu32)
ENDIF (OPENGL_FOUND)


#Other libraries
IF(WIN32)
    set  (CLOUT_EXTERN_LIBS
	"${PROJECT_SOURCE_DIR}/src/external/assimp/lib/Release/assimp-vc143-mt.lib"
	"${PROJECT_SOURCE_DIR}/src/external/glew/lib/Release/x64/glew32.lib"
	"${PROJECT_SOURCE_DIR}/src/external/glfw/src/Release/glfw3.lib"
      )
ELSE(WIN32)
    set  (CLOUT_EXTERN_LIBS
    	"${PROJECT_SOURCE_DIR}/src/external/assimp/bin/libassimp.so"
	"${PROJECT_SOURCE_DIR}/src/external/glfw/src/libglfw3.a"
      )
ENDIF(WIN32)


# Properties->Linker->Input->Additional Dependencies
target_link_libraries (Clout ${CLOUT_EXTERN_LIBS} ${OPENGL_LIBRARIES} )

# Creates a folder "executables" and adds target 
# project (Clout.vcproj) under it
# set_property(TARGET Clout PROPERTY FOLDER "Clout")

# Set C++ version
set_property(TARGET Clout PROPERTY CXX_STANDARD 17)

# Properties->General->Output Directory
set_target_properties(Clout PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Adds logic to INSTALL.vcproj to copy app.exe to destination directory
install (TARGETS Clout RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin)

# Set the Visual Studio working directory so we can run the app
set_property(TARGET Clout PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

#Set the Visual Studio startup project, so we can run and debug the app
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Clout)

#Tell ImGuiFileDialog where to get the config
add_compile_definitions(CUSTOM_IMGUIFILEDIALOG_CONFIG="${PROJECT_SOURCE_DIR}/src/ImGuiFileDialogConfig.h")

#String _s functions aren't available on linux, and I'm not writing a wrapper for something as stupid as this
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
