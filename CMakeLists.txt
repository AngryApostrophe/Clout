#ideas: https://stackoverflow.com/questions/68850556/cmake-how-to-stop-building-every-dependency-every-time-i-build-my-project
#set_target_properties(GLFW_LIBRARY PROPERTIES IMPORTED_LOCATION ${GLFW_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})


cmake_minimum_required (VERSION 3.8)

# Maps to a solution file (Clout.sln).
project (Clout)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
# Here it is setting the Visual Studio warning level to 4
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Command to output information to the console
# Useful for displaying errors, warnings, and debugging
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

include(ExternalProject)

option(CLOUT_SKIP_ASSIMP "Skip building Assimp source if restoring cache in a Github workflow" OFF)
option(CLOUT_SKIP_GLFW "Skip building GLFW source if restoring cache in a Github workflow" OFF)


#Required lib sources
IF(WIN32)
	#These are for MSVC which always adds /Release and /Debug
		set  (CLOUT_ASSIMP_LIBR "${PROJECT_SOURCE_DIR}/build/external/assimp/lib/Release/assimp-vc143-mt.lib" )
		set  (CLOUT_ASSIMP_LIBD "${PROJECT_SOURCE_DIR}/build/external/assimp/lib/Debug/assimp-vc143-mtd.lib" )
		set  (CLOUT_GLFW_LIBR "${PROJECT_SOURCE_DIR}/build/external/glfw/src/Release/glfw3.lib" )
		set  (CLOUT_GLFW_LIBD "${PROJECT_SOURCE_DIR}/build/external/glfw/src/Debug/glfw3.lib" )
		
	#These are the non-MSVC sources
		set  (CLOUT_ASSIMP_LIB "${PROJECT_SOURCE_DIR}/build/external/assimp/lib/assimp-vc143-mt.lib" )		
		set  (CLOUT_GLFW_LIB "${PROJECT_SOURCE_DIR}/build/external/glfw/src/glfw3.lib" )
		set  (CLOUT_GLEW_LIB "${PROJECT_SOURCE_DIR}/src/external/glew-win32/lib/Release/x64/glew32.lib" )
ELSE()
	find_package(ZLIB REQUIRED)
	set  (CLOUT_ASSIMP_LIB "${PROJECT_SOURCE_DIR}/build/external/assimp/lib/libassimp.a" ZLIB::ZLIB)
	set  (CLOUT_GLEW_LIB "${PROJECT_SOURCE_DIR}/src/external/glew/lib/libGLEW.a" )
	set  (CLOUT_GLFW_LIB "${PROJECT_SOURCE_DIR}/build/external/glfw/src/libglfw3.a" )
ENDIF()

#Build Assimp
	IF(WIN32)
		set (CLOUT_ASSIMP_SHARED ON)
		set (CLOUT_ASSIMP_ZLIB ON)
	ELSE()
		set (CLOUT_ASSIMP_SHARED OFF)
		set (CLOUT_ASSIMP_ZLIB OFF)
	ENDIF()

	IF (NOT CLOUT_SKIP_ASSIMP)
		ExternalProject_Add(assimp
			SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/external/assimp
			BINARY_DIR "${PROJECT_SOURCE_DIR}/build/external/assimp/"

			CMAKE_ARGS 
				   -DCMAKE_BUILD_TYPE=Release	#This is for non-MSVC compilers. In MSVC it depends on current config

				   -DBUILD_SHARED_LIBS=${CLOUT_ASSIMP_SHARED}
				   -DASSIMP_BUILD_ZLIB=${CLOUT_ASSIMP_ZLIB}
			   
				   #Select which importers/exporters are included, to reduce file size
					   -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF 
					   -DASSIMP_BUILD_3MF_IMPORTER=ON
					   -DASSIMP_NO_EXPORT=ON
				   			   
				   #Don't waste time building the extra stuff we don't need
					   -DASSIMP_BUILD_TESTS=OFF
					   -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
					   -DASSIMP_INSTALL=OFF
					   -DASSIMP_WARNINGS_AS_ERRORS=OFF

				  
			STEP_TARGETS   build
			BUILD_BYPRODUCTS ${CLOUT_ASSIMP_LIB} ${CLOUT_ASSIMP_LIBR} ${CLOUT_ASSIMP_LIBD} #The actual byproduct depends on our compiler
			INSTALL_COMMAND ""
		)
	ENDIF()

#Build GLFW
	IF (NOT CLOUT_SKIP_GLFW)
		ExternalProject_Add(glfw
			SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/external/glfw
			BINARY_DIR ${PROJECT_SOURCE_DIR}/build/external/glfw

			CMAKE_ARGS 
			   -DGLFW_BUILD_EXAMPLES=OFF
			   -DGLFW_BUILD_TESTS=OFF
			   -DGLFW_BUILD_DOCS=OFF
			   -DGLFW_INSTALL=OFF

			STEP_TARGETS   build
			BUILD_BYPRODUCTS ${CLOUT_GLFW_LIB} ${CLOUT_GLFW_LIBR} ${CLOUT_GLFW_LIBD}
			INSTALL_COMMAND ""
		)
	ENDIF()

#Build GLEW (Linux only)
	IF(NOT WIN32)
		#First generate the source files we need
			ExternalProject_Add(glewsrc
				SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/external/glew/auto
				BINARY_DIR ${PROJECT_SOURCE_DIR}/src/external/glew/auto
				CONFIGURE_COMMAND ""
				INSTALL_COMMAND ""
			)

		#Now build the source	
			ExternalProject_Add(glew
				SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/external/glew
				BINARY_DIR ${PROJECT_SOURCE_DIR}/src/external/glew

				DEPENDS glewsrc

				STEP_TARGETS   build
				BUILD_BYPRODUCTS ${CLOUT_GLEW_LIB}
				#No CMake, only Make
					CONFIGURE_COMMAND ""
					INSTALL_COMMAND ""
			)
	ENDIF(NOT WIN32)

#Build GLEW


# Sub-directories where more CMakeLists.txt exist
add_subdirectory("src")
