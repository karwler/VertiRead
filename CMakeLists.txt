cmake_minimum_required(VERSION 3.12.4)
project(VertiRead VERSION 1.0.0 DESCRIPTION "Comic/Manga reader" HOMEPAGE_URL "https://github.com/karwler/vertiread" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT GENERATOR_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
	option(DIRECTX "Build with DirectX 11 support." ON)
elseif(UNIX)
	option(APPIMAGE "Package as an AppImage." OFF)
	option(OPENGLES "Build for OpenGL ES 3.0 instead of OpenGL 3.0." OFF)
	option(USE_FONTCONFIG "Build with fontconfig if available." ON)
	option(USE_SECRET "Build with libsecret if available." ON)
	option(USE_SMBCLIENT "Build with smbclient if available." ON)
	option(USE_SSH2 "Build with libssh2 if available." ON)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	option(NATIVE "Build for the current CPU." OFF)
endif()
option(OPENGL "Build with OpenGL 3.0 support." ON)
option(USE_ICU "Build with ICU if available." OFF)
option(VULKAN "Build with Vulkan 1.0 support." ON)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# source files
set(SRC_FILES
	"src/engine/drawSys.cpp"
	"src/engine/drawSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/renderer.cpp"
	"src/engine/renderer.h"
	"src/engine/rendererDx.cpp"
	"src/engine/rendererDx.h"
	"src/engine/rendererGl.cpp"
	"src/engine/rendererGl.h"
	"src/engine/rendererVk.cpp"
	"src/engine/rendererVk.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/engine/optional/fontconfig.cpp"
	"src/engine/optional/fontconfig.h"
	"src/engine/optional/secret.cpp"
	"src/engine/optional/secret.h"
	"src/engine/optional/smbclient.cpp"
	"src/engine/optional/smbclient.h"
	"src/engine/optional/ssh2.cpp"
	"src/engine/optional/ssh2.h"
	"src/prog/browser.cpp"
	"src/prog/browser.h"
	"src/prog/fileOps.cpp"
	"src/prog/fileOps.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/prog/progs.cpp"
	"src/prog/progs.h"
	"src/prog/types.cpp"
	"src/prog/types.h"
	"src/utils/compare.cpp"
	"src/utils/compare.h"
	"src/utils/layouts.cpp"
	"src/utils/layouts.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h"
	"src/utils/widgets.cpp"
	"src/utils/widgets.h")
if(DIRECTX)
	list(APPEND SRC_FILES
		"src/engine/shaders/dxGuiPs.dbg.h"
		"src/engine/shaders/dxGuiPs.rel.h"
		"src/engine/shaders/dxGuiVs.dbg.h"
		"src/engine/shaders/dxGuiVs.rel.h"
		"src/engine/shaders/dxSelPs.dbg.h"
		"src/engine/shaders/dxSelPs.rel.h"
		"src/engine/shaders/dxSelVs.dbg.h"
		"src/engine/shaders/dxSelVs.rel.h")
endif()
if(OPENGL)
	list(APPEND SRC_FILES
		"src/engine/shaders/glGui.frag.dbg.h"
		"src/engine/shaders/glGui.frag.rel.h"
		"src/engine/shaders/glGui.vert.dbg.h"
		"src/engine/shaders/glGui.vert.rel.h"
		"src/engine/shaders/glSel.frag.dbg.h"
		"src/engine/shaders/glSel.frag.rel.h"
		"src/engine/shaders/glSel.vert.dbg.h"
		"src/engine/shaders/glSel.vert.rel.h")

	foreach(GLFILE "rsc/shaders/glGui.frag" "rsc/shaders/glGui.vert" "rsc/shaders/glSel.frag" "rsc/shaders/glSel.vert")
		file(READ "${CMAKE_SOURCE_DIR}/${GLFILE}" GL_CODE)
		get_filename_component(GLFILE_NAME ${GLFILE} NAME)
		file(WRITE "${CMAKE_SOURCE_DIR}/src/engine/shaders/${GLFILE_NAME}.dbg.h" "R\"r(${GL_CODE})r\"")
	endforeach()
endif()
if(VULKAN)
	list(APPEND SRC_FILES
		"src/engine/shaders/vkConv.comp.dbg.h"
		"src/engine/shaders/vkConv.comp.rel.h"
		"src/engine/shaders/vkGui.frag.dbg.h"
		"src/engine/shaders/vkGui.frag.rel.h"
		"src/engine/shaders/vkGui.vert.dbg.h"
		"src/engine/shaders/vkGui.vert.rel.h"
		"src/engine/shaders/vkSel.frag.dbg.h"
		"src/engine/shaders/vkSel.frag.rel.h"
		"src/engine/shaders/vkSel.vert.dbg.h"
		"src/engine/shaders/vkSel.vert.rel.h")
endif()
if(WIN32)
	list(APPEND SRC_FILES "rsc/resource.rc")
endif()

# dependencies
set(VER_ARC "3.6.2")
set(VER_SDL "2.28.2")
set(VER_IMG "2.6.3")
set(VER_FT2 "2.13.0")
set(VER_GLM "0.9.9.8")
set(VER_CURL "7.83.1")
set(VER_LXML "2.9.14")
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")
set(DIR_ICN "${DIR_RSC}/icons")

if(MSVC)
	set(ENV{CL} /MP)
	set(MAKE_CMD "nmake")
else()
	set(MAKE_CMD "make")
	if(MINGW)
		set(MAKE_CMD "mingw32-${MAKE_CMD}")
	endif()

	include(ProcessorCount)
	ProcessorCount(NJOBS)
	if(NJOBS GREATER 1)
		set(MAKE_CMD "${MAKE_CMD}\;-j${NJOBS}")
	endif()
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
		if(NOT EXISTS "${WDIR}")
			file(MAKE_DIRECTORY "${WDIR}")
		endif()
		foreach(CMD IN LISTS CMDV)
			execute_process(COMMAND ${CMD} WORKING_DIRECTORY "${WDIR}")
		endforeach()
	endif()
endfunction()

if(USE_ICU)
	if(WIN32)
		find_package(ICU COMPONENTS uc in)
	else()
		find_package(ICU COMPONENTS uc i18n)
	endif()
	if(ICU_FOUND)
		add_compile_definitions(WITH_ICU)
		include_directories(SYSTEM ${ICU_INCLUDE_DIRS})
		if(WIN32)
			string(REGEX REPLACE "\\.[0-9]+" "" ICU_VER_MAJ ${ICU_VERSION})
			get_filename_component(BIN_ICU "${ICU_ICUPKG_EXECUTABLE}" DIRECTORY)
			get_filename_component(LIC_ICU "${BIN_ICU}" DIRECTORY)
		endif()
	endif()
endif()

if(USE_FONTCONFIG)
	list(APPEND PKGCONFIG_ARGS "fontconfig")
endif()
if(USE_SECRET AND (USE_SMBCLIENT OR USE_SSH2))
	list(APPEND PKGCONFIG_ARGS "libsecret-1")
endif()
if(USE_SMBCLIENT)
	list(APPEND PKGCONFIG_ARGS "smbclient")
endif()
if(USE_SSH2)
	list(APPEND PKGCONFIG_ARGS "libssh2")
endif()
if(PKGCONFIG_ARGS)
	find_package(PkgConfig)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(PKGCONFS ${PKGCONFIG_ARGS})
		include_directories(SYSTEM ${PKGCONFS_INCLUDE_DIRS})

		if(PKGCONFS_fontconfig_FOUND OR PKGCONFS_fontconfig_VERSION)
			add_compile_definitions(CAN_FONTCFG)
		endif()
		if(PKGCONFS_libsecret-1_FOUND OR PKGCONFS_libsecret-1_VERSION AND (PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION OR PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION))
			add_compile_definitions(CAN_SECRET)
		endif()
		if(PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION)
			add_compile_definitions(CAN_SMB)
		endif()
		if(PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION)
			add_compile_definitions(CAN_SFTP)
		endif()
	endif()
endif()

if(VULKAN)
	find_package(Vulkan REQUIRED)
	string(REPLACE "/Include" "" VULKAN_PATH "${Vulkan_INCLUDE_DIRS}")
	include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})
	link_directories("${VULKAN_PATH}/Lib")
endif()

file(MAKE_DIRECTORY "${DIR_LIB}")
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}/glm" "")
include_directories(SYSTEM "${DIR_LIB}/glm")

include_directories("${CMAKE_SOURCE_DIR}/src")
if(WIN32)
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	set(TBIN_DIR "${PBOUT_DIR}")
	set(DATA_DIR "${PBOUT_DIR}")

	if(MSVC)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "x64")
			set(DIR_ARC "${DIR_LIB}/libarchive")
			set(BIN_ARC "${DIR_ARC}/bin")
			set(LIB_ARC "${DIR_ARC}/lib")
			set(INC_ARC "${DIR_ARC}/include")
		else()
			set(VMACHINE "x86")
			set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}")
			set(BIN_ARC "${DIR_ARC}/build/bin")
			set(LIB_ARC "${DIR_ARC}/build/libarchive")
			set(INC_ARC "${DIR_ARC}/libarchive")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-msvc")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-msvc")
		set(DIR_FT2 "${DIR_LIB}/freetype-${VER_FT2}-${VMACHINE}-msvc")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-msvc")
		set(DIR_LXML "${DIR_LIB}/libxml2-${VER_LXML}-${VMACHINE}-msvc")
		set(OUT_CURL "${DIR_CURL}/builds/libcurl-vc-${VMACHINE}-release-dll-ipv6-sspi-schannel")
		set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
		set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
		set(BIN_FT2 "${DIR_FT2}/build")
		set(BIN_CURL "${OUT_CURL}/bin")
		set(BIN_LXML "${DIR_LXML}/win32/bin.msvc")

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			downloadLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-v${VER_ARC}-amd64.zip" "${DIR_ARC}" "")
		else()
			compileLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "cmake\;..\;-G\;NMake Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DENABLE_WERROR=0;${MAKE_CMD}" "${DIR_ARC}/build" "")
		endif()
		downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		compileLib("https://download.savannah.gnu.org/releases/freetype/freetype-${VER_FT2}.tar.gz" "${DIR_FT2}" "cmake\;..\;-G\;NMake Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DFT_ENABLE_ERROR_STRINGS=1\;-DFT_DISABLE_ZLIB=1\;-DFT_DISABLE_BZIP2=1\;-DFT_DISABLE_PNG=1\;-DFT_DISABLE_HARFBUZZ=1\;-DFT_DISABLE_BROTLI=1;${MAKE_CMD}" "${BIN_FT2}" "${DIR_LIB}/freetype-${VER_FT2}")
		link_directories("${LIB_ARC}" "${BIN_SDL}" "${BIN_IMG}" "${BIN_FT2}")
		include_directories(SYSTEM "${INC_ARC}" "${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_FT2}/include")
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "win64")
			set(VMARCH "x86_64")
		else()
			set(VMACHINE "win32")
			set(VMARCH "i686")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-mingw")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-mingw")
		set(DIR_FT2 "${DIR_LIB}/freetype-${VER_FT2}-${VMACHINE}-mingw")
		set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}-${VMACHINE}-mingw")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-mingw")
		set(DIR_LXML "${DIR_LIB}/libxml2-${VER_LXML}-${VMACHINE}-mingw")
		set(BIN_ARC "${DIR_ARC}/build/bin")
		set(BIN_SDL "${DIR_SDL}/${VMARCH}-w64-mingw32/bin")
		set(BIN_IMG "${DIR_IMG}/${VMARCH}-w64-mingw32/bin")
		set(BIN_FT2 "${DIR_FT2}/build")
		set(BIN_CURL "${DIR_CURL}/bin")
		set(BIN_LXML "${DIR_LXML}/win32/bin.mingw")

		compileLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "cmake\;..\;-G\;MinGW Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DENABLE_WERROR=0;${MAKE_CMD}\;archive" "${DIR_ARC}/build" "${DIR_LIB}/libarchive-${VER_ARC}")
		downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-devel-${VER_SDL}-mingw.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-devel-${VER_IMG}-mingw.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		compileLib("https://download.savannah.gnu.org/releases/freetype/freetype-${VER_FT2}.tar.gz" "${DIR_FT2}" "cmake\;..\;-G\;MinGW Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DFT_ENABLE_ERROR_STRINGS=1\;-DFT_DISABLE_ZLIB=1\;-DFT_DISABLE_BZIP2=1\;-DFT_DISABLE_PNG=1\;-DFT_DISABLE_HARFBUZZ=1\;-DFT_DISABLE_BROTLI=1;${MAKE_CMD}" "${BIN_FT2}" "${DIR_LIB}/freetype-${VER_FT2}")
		link_directories("${DIR_ARC}/build/libarchive" "${DIR_SDL}/${VMARCH}-w64-mingw32/lib" "${DIR_IMG}/${VMARCH}-w64-mingw32/lib" "${BIN_FT2}")
		include_directories(SYSTEM "${DIR_ARC}/libarchive" "${DIR_SDL}/${VMARCH}-w64-mingw32/include/SDL2" "${DIR_IMG}/${VMARCH}-w64-mingw32/include/SDL2" "${DIR_FT2}/include")
	endif()
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(VMARCH "x86_64")
	else()
		set(VMARCH "i386")
	endif()
	if(APPIMAGE)
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.AppDir")
		set(TBIN_DIR "${PBOUT_DIR}/usr/bin")
		set(DATA_DIR "${PBOUT_DIR}/usr/share/${PROJECT_NAME_LOWER}")
	else()
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}")
		set(TBIN_DIR "${PBOUT_DIR}/bin")
		set(DATA_DIR "${PBOUT_DIR}/share/${PROJECT_NAME_LOWER}")
	endif()

	find_package(Freetype REQUIRED)
	find_package(LibArchive REQUIRED)
	include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS} ${LibArchive_INCLUDE_DIRS})
endif()
set(ICONS_DIR "${DATA_DIR}/icons")
set(LICN_DIR "${DATA_DIR}/licenses")

# compiler flags
add_compile_definitions($<$<BOOL:${DIRECTX}>:WITH_DIRECTX>
	"$<$<BOOL:${OPENGL}>:WITH_OPENGL;$<$<BOOL:${OPENGLES}>:OPENGLES>>"
	$<$<BOOL:${VULKAN}>:WITH_VULKAN>
	"$<$<BOOL:${WIN32}>:UNICODE;_UNICODE;_CRT_SECURE_NO_WARNINGS;NOMINMAX;$<$<NOT:$<BOOL:${MSVC}>>:_WIN32_WINNT=0x600>>")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-Wall -Wextra -pedantic-errors -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code
		-Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-switch $<$<NOT:$<BOOL:${MINGW}>>:-Wdouble-promotion>)
	if(NATIVE)
		add_compile_options(-march=native)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		add_compile_options(-march=x86-64)
	endif()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wcast-align=strict -Wduplicated-cond -Wlogical-op -Wuseless-cast
			"$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin>")
		add_link_options("$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin;-s>")
	else()
		add_compile_options(-Wcast-align -Wnull-dereference -Wshadow-all -Wno-final-dtor-non-final-class)
	endif()
elseif(MSVC)
	add_compile_options(/wd4244 /wd4267 $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/GL>)
endif()

# set main target
add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} SDL2 SDL2_image
	"$<IF:$<BOOL:${WIN32}>,freetype;archive,${FREETYPE_LIBRARIES};${LibArchive_LIBRARIES}>"
	"$<$<BOOL:${UNIX}>:pthread;dl>"
	"$<$<BOOL:${DIRECTX}>:d3d11.lib;dxgi.lib>"
	$<$<BOOL:${OPENGL}>:$<IF:$<BOOL:${WIN32}>,opengl32,$<IF:$<BOOL:${OPENGLES}>,GLESv2,GL>>>
	$<$<BOOL:${VULKAN}>:$<IF:$<BOOL:${WIN32}>,vulkan-1,vulkan>>
	"$<$<BOOL:${ICU_FOUND}>:${ICU_UC_LIBRARIES};$<IF:$<BOOL:${WIN323}>,${ICU_IN_LIBRARIES},${ICU_I18N_LIBRARIES}>>")

set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${TBIN_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TBIN_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TBIN_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TBIN_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${TBIN_DIR}")
if(WIN32)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
	set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME ${PROJECT_NAME_LOWER})
endif()

# build commands
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${ICONS_DIR}"
	COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/licenses" "${LICN_DIR}"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/BrisaSans.otf" "${DIR_RSC}/themes.ini" "${DATA_DIR}"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.svg" "${DIR_ICN}/center.svg" "${DIR_ICN}/cross.svg" "${DIR_ICN}/file.svg" "${DIR_ICN}/fit.svg" "${DIR_ICN}/folder.svg" "${DIR_ICN}/left.svg" "${DIR_ICN}/minus.svg" "${DIR_ICN}/plus.svg" "${DIR_ICN}/reset.svg" "${DIR_ICN}/right.svg" "${DIR_ICN}/search.svg" "${ICONS_DIR}")
if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ARC}/$<IF:$<BOOL:${MSVC}>,archive.dll,libarchive.dll>" "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "$<$<BOOL:${MSVC}>:${BIN_IMG}/optional/libtiff-5.dll;${BIN_IMG}/optional/libwebp-7.dll>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
		COMMAND "$<$<BOOL:${MSVC}>:${CMAKE_COMMAND};-E;copy_if_different;${BIN_IMG}/optional/LICENSE.tiff.txt;${BIN_IMG}/optional/LICENSE.webp.txt;${LICN_DIR}>"
		COMMAND "$<$<BOOL:${MINGW}>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_ARC}/COPYING;${LICN_DIR}/COPYING.libarchive.txt>"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/LICENSE.txt" "${LICN_DIR}/LICENSE.SDL_image.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_FT2}/LICENSE.txt" "${LICN_DIR}/LICENSE.freetype.txt"
		COMMAND_EXPAND_LISTS)
	if(ICU_FOUND)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ICU}/icudt${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuin${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuuc${ICU_VER_MAJ}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_ICU}/LICENSE" "${LICN_DIR}/LICENSE.icu.txt")
	endif()
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.desktop" "${PBOUT_DIR}")

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		find_program(STRIPPER "strip")
		if(STRIPPER)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				COMMAND "$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:${STRIPPER};-s;$<TARGET_FILE:${PROJECT_NAME}>>"
				COMMAND_EXPAND_LISTS)
		else()
			message(WARNING "Can't strip the executable: Failed to find stip")
		endif()
	endif()
	if(APPIMAGE)
		find_program(LINUXDEPLOY "linuxdeploy")
		if(NOT LINUXDEPLOY)
			set(LDPEXE_NAME "linuxdeploy-${VMARCH}.AppImage")
			set(LINUXDEPLOY "${DIR_LIB}/${LDPEXE_NAME}")
			if(EXISTS "${LINUXDEPLOY}")
				message(STATUS "Found ${LINUXDEPLOY}")
			else()
				message(STATUS "Downloading ${LINUXDEPLOY}")
				file(DOWNLOAD "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${LDPEXE_NAME}" "${LINUXDEPLOY}")
				file(CHMOD "${LINUXDEPLOY}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
			endif()
		endif()
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/bin/${PROJECT_NAME_LOWER}" "${PBOUT_DIR}/AppRun"
			COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/share/${PROJECT_NAME_LOWER}/icons/${PROJECT_NAME_LOWER}.svg" "${PBOUT_DIR}/${PROJECT_NAME_LOWER}.svg"
			COMMAND bash -c "VERSION='${PROJECT_VERSION}' '${LINUXDEPLOY}' --appdir '${PBOUT_DIR}' --output appimage")
	endif()
endif()

# install target
if(WIN32)
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${DST_DIR}")
	install(DIRECTORY "${ICONS_DIR}" DESTINATION "${DST_DIR}")
	install(FILES "${DATA_DIR}/BrisaSans.otf" "${DATA_DIR}/themes.ini" "${PBOUT_DIR}/${PROJECT_NAME_LOWER}.png" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${LICN_DIR}" DESTINATION "${DST_DIR}")
	install(FILES "${PBOUT_DIR}/archive.dll" "${PBOUT_DIR}/libfreetype-6.dll" "${PBOUT_DIR}/libjpeg-9.dll" "${PBOUT_DIR}/libpng16-16.dll" "${PBOUT_DIR}/libtiff-5.dll" "${PBOUT_DIR}/libwebp-7.dll" "${PBOUT_DIR}/SDL2.dll" "${PBOUT_DIR}/SDL2_image.dll" DESTINATION "${DST_DIR}")
elseif(APPIMAGE)
	install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VMARCH}.AppImage" DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
	install(DIRECTORY "${ICONS_DIR}" DESTINATION "${DST_DIR}")
	install(FILES "${DATA_DIR}/BrisaSans.otf" "${DATA_DIR}/themes.ini" "${PBOUT_DIR}/${PROJECT_NAME_LOWER}.png" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${LICN_DIR}" DESTINATION "${DST_DIR}")
endif()

# group files
foreach(FSRC IN LISTS SRC_FILES)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	string(REPLACE ";" "/" FGRP "${FGRP}")
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
