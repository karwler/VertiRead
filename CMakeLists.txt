# project properties
cmake_minimum_required(VERSION 3.8.2)
project(VertiRead LANGUAGES CXX)
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

if(NOT CMAKE_BUILD_TYPE OR NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set(CMAKE_BUILD_TYPE "Release")
endif()

# source files
set(SRC_FILES
	"src/engine/drawSys.cpp"
	"src/engine/drawSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/prog/browser.cpp"
	"src/prog/browser.h"
	"src/prog/defaults.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/prog/progs.cpp"
	"src/prog/progs.h"
	"src/utils/layouts.cpp"
	"src/utils/layouts.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h"
	"src/utils/vec2.h"
	"src/utils/widgets.cpp"
	"src/utils/widgets.h")
	
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND SRC_FILES "rsc/resource.rc")
endif()

# library/framework directories
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(MSVC)
		if(${CMAKE_SIZEOF_VOID_P} MATCHES "8")
			set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/msvc64")
		else()
			set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/msvc32")
		endif()
	endif()
	
	link_directories(${LIB_DIR})
	set(DLL_FILES
		"archive.dll"
		"libfreetype-6.dll"
		"libjpeg-9.dll"
		"libpng16-16.dll"
		"libtiff-5.dll"
		"libwebp-7.dll"
		"SDL2.dll"
		"SDL2_image.dll"
		"SDL2_ttf.dll"
		"zlib1.dll")	
endif()

# definitions
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-D_UNICODE)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

# set main target
add_executable(VertiRead ${SRC_FILES})
target_compile_features(VertiRead PUBLIC cxx_std_11)

# include directories and link libraries
include_directories("${CMAKE_SOURCE_DIR}/src")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	include_directories("${CMAKE_SOURCE_DIR}/lib/include")
endif()
target_link_libraries(VertiRead SDL2 SDL2_image SDL2_ttf archive)

# target properties
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set_target_properties(VertiRead PROPERTIES
								RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_OUTPUT_PATH}
								RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_OUTPUT_PATH})

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	if(${CMAKE_BUILD_TYPE} MATCHES "Debug")	# for some reason half of this shit doesn't work with set_target_properties
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	endif()
	
	if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
	endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_target_properties(VertiRead PROPERTIES
									OUTPUT_NAME VertiRead
									LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
									LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
else()
	set_target_properties(VertiRead PROPERTIES
									OUTPUT_NAME vertiread)
endif()

# post buils commands
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	foreach(dll_file ${DLL_FILES})
		add_custom_command(TARGET VertiRead POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DIR}/${dll_file}" ${EXECUTABLE_OUTPUT_PATH})
	endforeach()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_custom_command(TARGET VertiRead POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/rsc/vertiread.desktop" ${EXECUTABLE_OUTPUT_PATH})
endif()
add_custom_command(TARGET VertiRead POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/rsc/icon.ico" ${EXECUTABLE_OUTPUT_PATH}
					COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/rsc/data" ${EXECUTABLE_OUTPUT_PATH})

# filter file for ms visual studio
foreach(source IN LISTS SRC_FILES)
	get_filename_component(source_dir ${source} PATH)
	string(REPLACE "/" ";" dirs "${source_dir}")
	list(GET dirs 0 dir0)
	
	if(${dir0} MATCHES "rsc")
		set(filter_path "Source")
	else()
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" filter_path "${dirs}")
	endif()
	source_group("${filter_path}" FILES ${source})
endforeach()
