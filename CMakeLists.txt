# project properties
cmake_minimum_required(VERSION 3.10.2)
project(VertiRead LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
	"src/prog/downloader.cpp"
	"src/prog/downloader.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/prog/progs.cpp"
	"src/prog/progs.h"
	"src/utils/compare.h"
	"src/utils/layouts.cpp"
	"src/utils/layouts.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h"
	"src/utils/widgets.cpp"
	"src/utils/widgets.h")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	list(APPEND SRC_FILES "rsc/resource.rc")
endif()

# dependencies
set(VER_ARCH "3.4.3" CACHE STRING "libarchive version.")
set(VER_SDL "2.0.12" CACHE STRING "SDL2 version.")
set(VER_IMG "2.0.5" CACHE STRING "SDL2_image version.")
set(VER_TTF "2.0.15" CACHE STRING "SDL2_ttf version.")
set(VER_GLM "0.9.9.8" CACHE STRING "glm version.")
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")

option(BUILD_DOWNLOADER "Build with downloader. (currently mostly broken)" OFF)

function(downloadLib URL DST LIBDIR)
	get_filename_component(LIBDIR "${LIBDIR}" NAME)
	if(EXISTS "${DST}/${LIBDIR}")
		message(STATUS "Found ${DST}/${LIBDIR}")
		return()
	endif()

	get_filename_component(FILE "${URL}" NAME)
	if(EXISTS "${DST}/${FILE}")
		message(STATUS "Found ${DST}/${FILE}")
	else()
		message(STATUS "Downloading ${URL}")
		file(DOWNLOAD "${URL}" "${DST}/${FILE}")
	endif()

	if(FILE MATCHES ".zip$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf "${FILE}" WORKING_DIRECTORY "${DST}")
	elseif(FILE MATCHES ".tar.gz$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${FILE}" WORKING_DIRECTORY "${DST}")
	elseif(FILE MATCHES ".dmg$")
		execute_process(COMMAND hdiutil attach "${DST}/${FILE}")
		string(FIND "${FILE}" "-" FID)
		string(SUBSTRING "${FILE}" 0 ${FID} FNAME)
		file(COPY "/Volumes/${FNAME}/${FNAME}.framework" DESTINATION "${DST}")
	endif()
endfunction()

file(MAKE_DIRECTORY "${DIR_LIB}")
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}" "glm")
include_directories("${CMAKE_SOURCE_DIR}/src" "${DIR_LIB}/glm")
if(MSVC)
	set(DIR_ARCH "${DIR_LIB}/libarchive-${VER_ARCH}")
	set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}")
	set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}")
	set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
	set(OUT_ARCH "${DIR_ARCH}/bout")
	set(BIN_ARCH "${OUT_ARCH}/bin/Release")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BIN_SDL "${DIR_SDL}/lib/x64")
		set(BIN_IMG "${DIR_IMG}/lib/x64")
		set(BIN_TTF "${DIR_TTF}/lib/x64")
	else()
		set(BIN_SDL "${DIR_SDL}/lib/x86")
		set(BIN_IMG "${DIR_IMG}/lib/x86")
		set(BIN_TTF "${DIR_TTF}/lib/x86")
	endif()
	set(LIB_ARCH "${OUT_ARCH}/libarchive/Release")
	set(LIB_SDL "${BIN_SDL}")
	set(LIB_IMG "${BIN_IMG}")
	set(LIB_TTF "${BIN_TTF}")
	set(INC_ARCH "${DIR_ARCH}/libarchive")
	set(INC_SDL "${DIR_SDL}/include")
	set(INC_IMG "${DIR_IMG}/include")
	set(INC_TTF "${DIR_TTF}/include")
	set(URL_SDL "https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-VC.zip")
	set(URL_IMG "https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-VC.zip")
	set(URL_TTF "https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-VC.zip")

	downloadLib("${URL_SDL}" "${DIR_LIB}" "${DIR_SDL}")
	downloadLib("${URL_IMG}" "${DIR_LIB}" "${DIR_IMG}")
	downloadLib("${URL_TTF}" "${DIR_LIB}" "${DIR_TTF}")
	if(NOT EXISTS "${DIR_ARCH}")
		downloadLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARCH}/libarchive-${VER_ARCH}.zip" "${DIR_LIB}" "${DIR_ARCH}")

		# TODO: make this bullshit work
		#set(CMAKE_GENERATOR_CURRENT "${CMAKE_GENERATOR}")
		set(CMAKE_BUILD_TYPE_CURRENT "${CMAKE_BUILD_TYPE}")
		#set(CMAKE_GENERATOR "NMake Makefiles")
		set(CMAKE_BUILD_TYPE "Release")
		set(ENABLE_WERROR OFF)
		add_subdirectory("${DIR_ARCH}" "${OUT_ARCH}" EXCLUDE_FROM_ALL)
		#execute_process(COMMAND nmake archive WORKING_DIRECTORY "${OUT_ARCH}")
		#set(CMAKE_GENERATOR "${CMAKE_GENERATOR_CURRENT}")
		set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE_CURRENT}")
	endif()

	link_directories("${LIB_SDL}" "${LIB_IMG}" "${LIB_TTF}" "${LIB_ARCH}")
	include_directories("${INC_SDL}" "${INC_IMG}" "${INC_TTF}" "${INC_ARCH}")
endif()

# compiler flags
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-DDEBUG -D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	set(CXX_FLG_ANY "-Wall -Wextra -pedantic-errors -Wdouble-promotion -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code -Wno-implicit-fallthrough -Wno-parentheses -Wno-switch")
	set(FLG_DEBUG "-g")
	set(FLG_RELEASE "-O3")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(FLG_RELEASE "${FLG_RELEASE} -s")
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align=strict -Wduplicated-cond -Wlogical-op -Wuseless-cast")
	else()
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align -Wnull-dereference -Wshadow-all")
	endif()
elseif(MSVC)
	set(CXX_FLG_ANY "/W4 /wd4100 /wd4201 /wd4244 /wd4458 /wd4706 /wd4805")
	set(FLG_DEBUG "/Od /Ob0")
	set(FLG_RELEASE "/O2 /Ob2 /Oi /Ot /GL")
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLG_ANY} ${FLG_DEBUG}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLG_ANY} ${FLG_RELEASE}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLG_ANY} ${FLG_DEBUG}")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLG_ANY} ${FLG_RELEASE}")
endif()

# set main target
add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} SDL2 SDL2_image SDL2_ttf archive)
if(BUILD_DOWNLOADER)
	add_definitions(-DBUILD_DOWNLOADER)
	if(NOT MSVC)
		include_directories("/usr/include/libxml2")
		target_link_libraries(${PROJECT_NAME} curl xml2)
	endif()
endif()
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# target properties
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LICN_DIR "${EXECUTABLE_OUTPUT_PATH}/licenses")
set_target_properties(${PROJECT_NAME} PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
						RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXECUTABLE_OUTPUT_PATH}"
						RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXECUTABLE_OUTPUT_PATH}")
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
	set_target_properties(${PROJECT_NAME} PROPERTIES
							OUTPUT_NAME ${PROJECT_NAME_LOWER})
endif()

# post buils commands
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/icon.ico" "${EXECUTABLE_OUTPUT_PATH}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/data" "${EXECUTABLE_OUTPUT_PATH}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/licenses" "${LICN_DIR}")
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ARCH}/archive.dll" "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_IMG}/libjpeg-9.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_IMG}/libtiff-5.dll" "${BIN_IMG}/libwebp-7.dll" "${BIN_TTF}/SDL2_ttf.dll" "${BIN_TTF}/libfreetype-6.dll" "${BIN_TTF}/zlib1.dll" "${EXECUTABLE_OUTPUT_PATH}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_ARCH}/COPYING" "${LICN_DIR}/COPYING.libarchive.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.jpeg.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.png.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.tiff.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.webp.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_TTF}/LICENSE.freetype.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_TTF}/LICENSE.zlib.txt" "${LICN_DIR}")
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/vertiread.desktop" "${EXECUTABLE_OUTPUT_PATH}")
	if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND (NOT CMAKE_BUILD_TYPE STREQUAL "Debug"))
		find_program(STRIPPER "strip")
		if(STRIPPER)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${STRIPPER}" -s "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME_LOWER}")
		endif()
	endif()
endif()

# group files
foreach(FSRC IN LISTS SRC_FILES)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	if(MSVC)
		string(REPLACE ";" "\\" FGRP "${FGRP}")
	else()
		string(REPLACE ";" "/" FGRP "${FGRP}")
	endif()
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
