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
set(VER_ARC "3.5.1" CACHE STRING "libarchive version.")
set(VER_SDL "2.0.14" CACHE STRING "SDL2 version.")
set(VER_IMG "2.0.5" CACHE STRING "SDL2_image version.")
set(VER_TTF "2.0.15" CACHE STRING "SDL2_ttf version.")
set(VER_GLM "0.9.9.8" CACHE STRING "glm version.")
set(VER_CURL "7.74.0" CACHE STRING "libcurl version.")
set(VER_LXML "2.9.10" CACHE STRING "libxml2 version.")
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")
set(DIR_ICN "${DIR_RSC}/icons")

option(DOWNLOADER "Build with downloader. (currently mostly broken)" OFF)
if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	option(NATIVE "Build for the current CPU." OFF)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	option(APPIMAGE "Package as an AppImage." OFF)
endif()

function(downloadLib URL LIBDIR ORIG_LIBDIR)
	if(EXISTS "${LIBDIR}")
		message(STATUS "Found ${LIBDIR}")
		return()
	endif()

	get_filename_component(FILE "${URL}" NAME)
	get_filename_component(DST "${LIBDIR}" DIRECTORY)
	if(EXISTS "${DST}/${FILE}")
		message(STATUS "Found ${DST}/${FILE}")
	else()
		message(STATUS "Downloading ${URL}")
		file(DOWNLOAD "${URL}" "${DST}/${FILE}")
	endif()

	if(FILE MATCHES "\.(tar\.gz|tgz|zip)$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${FILE}" WORKING_DIRECTORY "${DST}")
		if(ORIG_LIBDIR AND NOT LIBDIR STREQUAL ORIG_LIBDIR)
			file(RENAME "${ORIG_LIBDIR}" "${LIBDIR}")
		endif()
	elseif(FILE MATCHES "\.dmg$")
		execute_process(COMMAND hdiutil attach "${DST}/${FILE}")
		string(FIND "${FILE}" "-" FID)
		string(SUBSTRING "${FILE}" 0 ${FID} FNAME)
		file(COPY "/Volumes/${FNAME}/${FNAME}.framework" DESTINATION "${DST}")
	endif()
endfunction()

function(compileLib URL LIBDIR CMDV WDIR ORIG_LIBDIR)
	if(EXISTS "${LIBDIR}")
		message(STATUS "Found ${LIBDIR}")
	else()
		downloadLib("${URL}" "${LIBDIR}" "${ORIG_LIBDIR}")
		foreach(CMD IN LISTS CMDV)
			execute_process(COMMAND ${CMD} WORKING_DIRECTORY "${WDIR}")
		endforeach()
	endif()
endfunction()

file(MAKE_DIRECTORY "${DIR_LIB}")
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}/glm" "")
include_directories("${CMAKE_SOURCE_DIR}/src" "${DIR_LIB}/glm")
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	set(DATA_DIR "${PBOUT_DIR}")
	set(FONTS_DIR "${PBOUT_DIR}")
	set(ICONS_DIR "${PBOUT_DIR}/icons")
	set(LICN_DIR "${PBOUT_DIR}/licenses")
	set(LIBS_DIR "${PBOUT_DIR}")
	set(EXECUTABLE_OUTPUT_PATH "${PBOUT_DIR}")

	if(MSVC)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "x64")
		else()
			set(VMACHINE "x86")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-msvc")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-msvc")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-msvc")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-msvc")
		set(DIR_LXML "${DIR_LIB}/libxml2-${VER_LXML}-${VMACHINE}-msvc")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(DIR_ARC "${DIR_LIB}/libarchive")
			set(BIN_ARC "${DIR_ARC}/bin")
			set(LIB_ARC "${DIR_ARC}/lib")
			set(INC_ARC "${DIR_ARC}/include")
		else()
			set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}")
			set(BIN_ARC "${DIR_ARC}/build/bin")
			set(LIB_ARC "${DIR_ARC}/build/libarchive")
			set(INC_ARC "${DIR_ARC}/libarchive")
		endif()
		set(OUT_CURL "${DIR_CURL}/builds/libcurl-vc-${VMACHINE}-release-dll-ipv6-sspi-schannel")
		set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
		set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
		set(BIN_TTF "${DIR_TTF}/lib/${VMACHINE}")
		set(BIN_CURL "${OUT_CURL}/bin")
		set(BIN_LXML "${DIR_LXML}/win32/bin.msvc")
		set(LIC_SDL "${DIR_SDL}")
		set(LIC_IMG "${DIR_IMG}")
		set(LIC_TTF "${DIR_TTF}")

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			downloadLib("https://github.com/libarchive/libarchive/releases/download/${VER_ARC}/libarchive-v${VER_ARC}-win64.zip" "${DIR_ARC}" "")
		else()
			compileLib("https://github.com/libarchive/libarchive/releases/download/${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "cmake\;..\;-G\;NMake Makefiles\;-DENABLE_WERROR=0;nmake" "${DIR_ARC}/build" "")
		endif()
		downloadLib("https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-VC.zip" "${DIR_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		link_directories("${LIB_ARC}" "${BIN_SDL}" "${BIN_IMG}" "${BIN_TTF}")
		include_directories("${INC_ARC}" "${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_TTF}/include")
		if(DOWNLOADER)
			compileLib("https://curl.haxx.se/download/curl-${VER_CURL}.zip" "${DIR_CURL}" "nmake\;/f\;Makefile.vc\;mode=dll\;MACHINE=${VMACHINE}" "${DIR_CURL}/winbuild" "${DIR_LIB}/curl-${VER_CURL}")
			compileLib("ftp://xmlsoft.org/libxml2/libxml2-${VER_LXML}.tar.gz" "${DIR_LXML}" "cscript\;configure.js\;compiler=msvc\;iconv=no;nmake\;/f\;Makefile.msvc" "${DIR_LXML}/win32" "${DIR_LIB}/libxml2-${VER_LXML}")
			link_directories("${OUT_CURL}/lib" "${BIN_LXML}")
			include_directories("${OUT_CURL}/include" "${DIR_LXML}/include")
		endif()
	else()
		set(LIC_SDL "${DIR_LIB}/SDL2-${VER_SDL}-mingw")
		set(LIC_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-mingw")
		set(LIC_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-mingw")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "win64")
			set(DIR_SDL "${LIC_SDL}/x86_64-w64-mingw32")
			set(DIR_IMG "${LIC_IMG}/x86_64-w64-mingw32")
			set(DIR_TTF "${LIC_TTF}/x86_64-w64-mingw32")
		else()
			set(VMACHINE "win32")
			set(DIR_SDL "${LIC_SDL}/i686-w64-mingw32")
			set(DIR_IMG "${LIC_IMG}/i686-w64-mingw32")
			set(DIR_TTF "${LIC_TTF}/i686-w64-mingw32")
		endif()
		set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}-${VMACHINE}-mingw")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-mingw")
		set(DIR_LXML "${DIR_LIB}/libxml2-${VER_LXML}-${VMACHINE}-mingw")
		set(BIN_ARC "${DIR_ARC}/build/bin")
		set(BIN_SDL "${DIR_SDL}/bin")
		set(BIN_IMG "${DIR_IMG}/bin")
		set(BIN_TTF "${DIR_TTF}/bin")
		set(BIN_CURL "${DIR_CURL}/bin")
		set(BIN_LXML "${DIR_LXML}/win32/bin.mingw")

		compileLib("https://github.com/libarchive/libarchive/releases/download/${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "cmake\;..\;-G\;MinGW Makefiles\;-DENABLE_WERROR=0;mingw32-make" "${DIR_ARC}/build" "${DIR_LIB}/libarchive-${VER_ARC}")
		downloadLib("https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-mingw.tar.gz" "${LIC_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-mingw.tar.gz" "${LIC_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-mingw.tar.gz" "${LIC_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		link_directories("${DIR_ARC}/build/libarchive" "${DIR_SDL}/lib" "${DIR_IMG}/lib" "${DIR_TTF}/lib")
		include_directories("${DIR_ARC}/libarchive" "${DIR_SDL}/include/SDL2" "${DIR_IMG}/include/SDL2" "${DIR_TTF}/include/SDL2")

		if(DOWNLOADER)
			downloadLib("https://curl.haxx.se/windows/dl-${VER_CURL}/curl-${VER_CURL}-${VMACHINE}-mingw.zip" "${DIR_CURL}" "")
			compileLib("ftp://xmlsoft.org/libxml2/libxml2-${VER_LXML}.tar.gz" "${DIR_LXML}" "cscript\;configure.js\;compiler=mingw\;iconv=no;mingw32-make\;/f\;Makefile.mingw" "${DIR_LXML}/win32" "${DIR_LIB}/libxml2-${VER_LXML}")
			link_directories("${OUT_CURL}/lib" "${BIN_LXML}")
			include_directories("${OUT_CURL}/include" "${DIR_LXML}/include")
		endif()
	endif()
else()
	if(APPIMAGE)
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.AppDir")
	else()
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	endif()
	set(DATA_DIR "${PBOUT_DIR}/share")
	set(FONTS_DIR "${DATA_DIR}/fonts")
	set(ICONS_DIR "${DATA_DIR}/icons")
	set(LICN_DIR "${DATA_DIR}/licenses")
	set(LIBS_DIR "${PBOUT_DIR}/lib")
	set(EXECUTABLE_OUTPUT_PATH "${PBOUT_DIR}/bin")

	if(APPIMAGE)
		set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}")
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
		set(DIR_LXML "${DIR_LIB}/libxml2-${VER_LXML}")

		compileLib("https://github.com/libarchive/libarchive/releases/download/${VER_ARC}/libarchive-${VER_ARC}.tar.gz" "${DIR_ARC}" "./configure;make" "${DIR_ARC}" "")
		compileLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.tar.gz" "${DIR_SDL}" "./configure;make" "${DIR_SDL}" "")
		compileLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.tar.gz" "${DIR_IMG}" "./configure;make" "${DIR_IMG}" "")
		compileLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_TTF}" "./configure;make" "${DIR_TTF}" "")
		link_directories("${LIBS_DIR}")
		include_directories("${DIR_ARC}/libarchive" "${DIR_SDL}/include" "${DIR_IMG}" "${DIR_TTF}")
		if(DOWNLOADER)
			compileLib("https://curl.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "./configure;make" "${DIR_CURL}" "")
			compileLib("ftp://xmlsoft.org/libxml2/libxml2-${VER_LXML}.tar.gz" "${DIR_LXML}" "./configure;make" "${DIR_LXML}" "")
			include_directories("${DIR_CURL}/include" "${DIR_LXML}/include")
		endif()
	elseif(DOWNLOADER)
		include_directories("/usr/include/libxml2")
	endif()
endif()

# compiler flags
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()
if(APPIMAGE)
	add_definitions(-DAPPIMAGE)
endif()
if(DOWNLOADER)
	add_definitions(-DDOWNLOADER)
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
	set(CXX_FLG_ANY "-Wall -Wextra -pedantic-errors -Wdouble-promotion -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code -Wno-implicit-fallthrough -Wno-old-style-cast -Wno-switch")
	set(FLG_DEBUG "-g")
	set(FLG_RELEASE "-O3")

	if(NATIVE)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -march=native")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -mmmx -msse -msse2")
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(FLG_RELEASE "${FLG_RELEASE} -flto -fuse-linker-plugin -s")
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
if(DOWNLOADER)
	if(CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${PROJECT_NAME} libcurl libxml2)
	else()
		target_link_libraries(${PROJECT_NAME} curl xml2)
	endif()
endif()
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
						RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXECUTABLE_OUTPUT_PATH}"
						RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXECUTABLE_OUTPUT_PATH}")
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
	set_target_properties(${PROJECT_NAME} PROPERTIES
							OUTPUT_NAME ${PROJECT_NAME_LOWER})
endif()

# build commands
find_program(PYTHON "python3" NAMES "python")
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
					COMMAND "${PYTHON}" "${CMAKE_SOURCE_DIR}/pictool.py" "${DIR_RSC}/vertiread.svg")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E make_directory "${ICONS_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/fonts" "${FONTS_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/licenses" "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/themes.ini" "${DATA_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt"
					COMMAND "${PYTHON}" "${CMAKE_SOURCE_DIR}/pictool.py" "${ICONS_DIR}" "${DIR_ICN}/center.svg" "${DIR_ICN}/cross.svg" "${DIR_ICN}/file.svg" "${DIR_ICN}/folder.svg" "${DIR_ICN}/left.svg" "${DIR_ICN}/minus.svg" "${DIR_ICN}/plus.svg" "${DIR_ICN}/reset.svg" "${DIR_ICN}/right.svg" "${DIR_ICN}/search.svg")
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/vertiread.png" "${DATA_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ARC}/archive.dll" "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_IMG}/libjpeg-9.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_IMG}/libtiff-5.dll" "${BIN_IMG}/libwebp-7.dll" "${BIN_TTF}/SDL2_ttf.dll" "${BIN_TTF}/libfreetype-6.dll" "${BIN_TTF}/zlib1.dll" "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.jpeg.txt" "${BIN_IMG}/LICENSE.png.txt" "${BIN_IMG}/LICENSE.tiff.txt" "${BIN_IMG}/LICENSE.webp.txt" "${BIN_TTF}/LICENSE.freetype.txt" "${BIN_TTF}/LICENSE.zlib.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt")
	if(DOWNLOADER)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_LXML}/libxml2.dll" "${PBOUT_DIR}"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LXML}/COPYING" "${LICN_DIR}/COPYING.libxml.txt")
		if(MSVC)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt")
		else()
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING.txt" "${LICN_DIR}/COPYING.curl.txt"
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING-brotli.txt" "${DIR_CURL}/COPYING-libssh2.txt" "${DIR_CURL}/COPYING-nghttp2.txt" "${DIR_CURL}/COPYING-openssl.txt" "${DIR_CURL}/COPYING-zstd.txt" "${LICN_DIR}")
		endif()
		if(MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl.dll" "${PBOUT_DIR}")
		else()
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl-x64.dll" "${PBOUT_DIR}/libcurl.dll")
		endif()
	endif()
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/vertiread.desktop" "${PBOUT_DIR}")

	if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND (NOT CMAKE_BUILD_TYPE STREQUAL "Debug"))
		find_program(STRIPPER "strip")
		if(STRIPPER)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${STRIPPER}" -s "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME_LOWER}")
		endif()
	endif()
	if(APPIMAGE)
		add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
							COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_ARC}/.libs/libarchive*.so*" "${DIR_SDL}/build/.libs/libSDL2*.so*" "${DIR_IMG}/.libs/libSDL2_image*.so*" "${DIR_TTF}/.libs/libSDL2_ttf*.so*" "${LIBS_DIR}")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E create_symlink "bin/${PROJECT_NAME_LOWER}" "${PBOUT_DIR}/AppRun"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/vertiread.png" "${PBOUT_DIR}"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_ARC}/COPYING" "${LICN_DIR}/COPYING.libarchive.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt")
		if(DOWNLOADER)
			add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
				                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/lib/.libs/libcurl*.so*" "${DIR_LXML}/.libs/libxml2*.so*" "${LIBS_DIR}")
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt"
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LXML}/COPYING" "${LICN_DIR}/COPYING.libxml.txt")
		endif()

		find_program(APPIMAGETOOL "appimagetool")
		if(APPIMAGETOOL)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${APPIMAGETOOL}" "${PBOUT_DIR}")
		endif()
	else()
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/vertiread.png" "${DATA_DIR}")
	endif()
endif()

# group files
foreach(FSRC IN LISTS SRC_FILES)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	string(REPLACE ";" "/" FGRP "${FGRP}")
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
