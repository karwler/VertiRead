cmake_minimum_required(VERSION 3.1.0)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0043 NEW)

project(VertiRead CXX)
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

# source files
set(SRC_FILES
	"src/engine/audioSys.cpp"
	"src/engine/audioSys.h"
	"src/engine/engine.cpp"
	"src/engine/engine.h"
	"src/engine/filer.cpp"
	"src/engine/filer.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/main.cpp"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/kklib/aliases.h"
	"src/kklib/sptr.h"
	"src/kklib/vec2.h"
	"src/kklib/vec3.h"
	"src/kklib/vec4.h"
	"src/prog/browser.cpp"
	"src/prog/browser.h"
	"src/prog/defaults.h"
	"src/prog/library.cpp"
	"src/prog/library.h"
	"src/prog/playlistEditor.cpp"
	"src/prog/playlistEditor.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/utils/capturers.cpp"
	"src/utils/capturers.h"
	"src/utils/items.cpp"
	"src/utils/items.h"
	"src/utils/objects.cpp"
	"src/utils/objects.h"
	"src/utils/popups.cpp"
	"src/utils/popups.h"
	"src/utils/scrollAreas.cpp"
	"src/utils/scrollAreas.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/types.cpp"
	"src/utils/types.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h")
	
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND SRC_FILES "rsc/resource.rc")
endif()

# set macros
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if (MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	endif()
endif()

# include directories
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	include_directories(/usr/local/include /Library/Frameworks)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# library/framework directories
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
		set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib/win64)
		set(DLL_FILES
			"libFLAC-8.dll"
			"libfreetype-6.dll"
			"libjpeg-9.dll"
			"libmikmod-2.dll"
			"libmodplug-1.dll"
			"libogg-0.dll"
			"libpng16-16.dll"
			"libtiff-5.dll"
			"libvorbis-0.dll"
			"libvorbisfile-3.dll"
			"libwebp-4.dll"
			"msvcp140.dll"
			"SDL2.dll"
			"SDL2_image.dll"
			"SDL2_mixer.dll"
			"SDL2_ttf.dll"
			"smpeg2.dll"
			"vcruntime140.dll"
			"zlib1.dll")
    else()
        set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib/win32)
		set(DLL_FILES
			"libFLAC-8.dll"
			"libfreetype-6.dll"
			"libjpeg-9.dll"
			"libmodplug-1.dll"
			"libogg-0.dll"
			"libpng16-16.dll"
			"libtiff-5.dll"
			"libvorbis-0.dll"
			"libvorbisfile-3.dll"
			"libwebp-4.dll"
			"msvcp140.dll"
			"SDL2.dll"
			"SDL2_image.dll"
			"SDL2_mixer.dll"
			"SDL2_ttf.dll"
			"smpeg2.dll"
			"vcruntime140.dll"
			"zlib1.dll")
    endif()	
	link_directories(${LIB_DIR})

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	find_library(LIB_SDL2 SDL2)
	find_library(LIB_SDL2_IMG SDL2_image)
	find_library(LIB_SDL2_TTF SDL2_ttf)
	find_library(LIB_SDL2_MIX SDL2_mixer)
	
endif()
	
# set main target
add_executable(VertiRead ${SRC_FILES})
target_compile_features(VertiRead PUBLIC cxx_nullptr cxx_enum_forward_declarations cxx_variadic_templates cxx_alias_templates cxx_raw_string_literals cxx_unicode_literals cxx_long_long_type cxx_rvalue_references)

# linker flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(VertiRead SDL2.lib SDL2_mixer.lib SDL2_image.lib SDL2_ttf.lib)
	
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(VertiRead ${LIB_SDL2} ${LIB_SDL2_IMG} ${LIB_SDL2_TTF} ${LIB_SDL2_MIX})

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_link_libraries(VertiRead SDL2 SDL2_mixer SDL2_image SDL2_ttf)

endif()

# target path
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/VertiRead.app/Contents/MacOS")
else()
	set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
endif()

# target properties
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_target_properties(VertiRead PROPERTIES
									OUTPUT_NAME VertiRead
									RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
									RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXECUTABLE_OUTPUT_PATH}"
									RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXECUTABLE_OUTPUT_PATH}"
									LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
									LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
else()
	set_target_properties(VertiRead PROPERTIES
									OUTPUT_NAME vertiread
									RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
									RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXECUTABLE_OUTPUT_PATH}"
									RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXECUTABLE_OUTPUT_PATH}")
endif()

# post buils commands
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	foreach(dll_file ${DLL_FILES})
		add_custom_command(TARGET VertiRead POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_DIR}/${dll_file} ${EXECUTABLE_OUTPUT_PATH})
	endforeach()
	add_custom_command(TARGET VertiRead POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH})
	
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_custom_command(TARGET VertiRead POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory ${EXECUTABLE_OUTPUT_PATH}/../Resources
					COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/rsc/Info.plist ${EXECUTABLE_OUTPUT_PATH}/..
					COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/rsc/icon.icns ${EXECUTABLE_OUTPUT_PATH}/../Resources
					COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}/../Resources)

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_custom_command(TARGET VertiRead POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/rsc/vertiread.desktop ${EXECUTABLE_OUTPUT_PATH}
					COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH})
endif()

# filter file for ms visual studio
foreach(source IN LISTS SRC_FILES)
	get_filename_component(source_dir ${source} PATH)
	string(REPLACE "/" ";" dirs "${source_dir}")
	list(GET dirs 0 dir0)
	
	if (${dir0} MATCHES "rsc")
		set(filter_path "Source")
	else()
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" filter_path "${dirs}")
	endif()
	source_group("${filter_path}" FILES ${source})
endforeach()

