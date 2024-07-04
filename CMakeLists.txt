cmake_minimum_required(VERSION 3.16.9)
include(CheckIncludeFileCXX)
project(VertiRead VERSION 1.0.0 DESCRIPTION "Comic/Manga reader" HOMEPAGE_URL "https://github.com/karwler/VertiRead" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPENGL "Build with OpenGL 1.1, 3.0 and ES 3.0 support." ON)
option(VULKAN "Build with Vulkan 1.0 support." ON)
if(UNIX)
	option(APPIMAGE "Package as an AppImage." OFF)
	option(USE_MUPDF "Build with MuPDF support." ON)
	option(USE_SECRET "Build with libsecret if available." ON)
	option(USE_SMBCLIENT "Build with smbclient if available." ON)
	option(USE_SSH2 "Build with libssh2 if available." ON)
endif()
if(MSVC)
	option(DIRECT3D "Build with Direct3D 11 support." ON)
	option(USE_ARCHIVE "Build with libarchive." OFF)
	option(USE_ICU "Build with ICU." OFF)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		option(PREBUILT_ARCHIVE "Use the pre-built libarchive binaries." OFF)
	endif()
else()
	option(USE_ARCHIVE "Build with libarchive." ON)
	option(USE_FONTCONFIG "Build with fontconfig if available." ON)
	option(USE_ICU "Build with ICU." ON)
	option(USE_POPPLER "Build with Poppler support." ON)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	option(NATIVE "Build for the current CPU." OFF)
endif()

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
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/engine/optional/internal.h"
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

if(DIRECT3D)
	CHECK_INCLUDE_FILE_CXX("d3d11.h" FOUND_D3D11)
	if(FOUND_D3D11)
		list(APPEND SRC_FILES
			"src/engine/rendererDx.cpp"
			"src/engine/rendererDx.h"
			"src/engine/optional/d3d.cpp"
			"src/engine/optional/d3d.h"
			"src/engine/shaders/dxBgrCs.dbg.h"
			"src/engine/shaders/dxBgrCs.rel.h"
			"src/engine/shaders/dxGuiPs.dbg.h"
			"src/engine/shaders/dxGuiPs.rel.h"
			"src/engine/shaders/dxGuiVs.dbg.h"
			"src/engine/shaders/dxGuiVs.rel.h"
			"src/engine/shaders/dxIdxCs.dbg.h"
			"src/engine/shaders/dxIdxCs.rel.h"
			"src/engine/shaders/dxRgbCs.dbg.h"
			"src/engine/shaders/dxRgbCs.rel.h"
			"src/engine/shaders/dxSelPs.dbg.h"
			"src/engine/shaders/dxSelPs.rel.h"
			"src/engine/shaders/dxSelVs.dbg.h"
			"src/engine/shaders/dxSelVs.rel.h")
		add_compile_definitions(WITH_DIRECT3D)
	endif()
endif()
if(OPENGL)
	list(APPEND SRC_FILES
		"src/engine/rendererGl.cpp"
		"src/engine/rendererGl.h"
		"src/engine/shaders/glDefs.h"
		"src/engine/shaders/glGui.frag.dbg.h"
		"src/engine/shaders/glGui.frag.rel.h"
		"src/engine/shaders/glGui.vert.dbg.h"
		"src/engine/shaders/glGui.vert.rel.h"
		"src/engine/shaders/glSel.frag.dbg.h"
		"src/engine/shaders/glSel.frag.rel.h"
		"src/engine/shaders/glSel.vert.dbg.h"
		"src/engine/shaders/glSel.vert.rel.h")
	add_compile_definitions(WITH_OPENGL)

	foreach(GLFILE "rsc/shaders/glGui.frag" "rsc/shaders/glGui.vert" "rsc/shaders/glSel.frag" "rsc/shaders/glSel.vert")
		file(READ "${CMAKE_SOURCE_DIR}/${GLFILE}" GL_CODE)
		get_filename_component(GLFILE_NAME ${GLFILE} NAME)
		file(WRITE "${CMAKE_SOURCE_DIR}/src/engine/shaders/${GLFILE_NAME}.dbg.h" "R\"r(${GL_CODE})r\"")
	endforeach()
endif()
if(VULKAN)
	find_package(Vulkan)
	if(Vulkan_FOUND)
		list(APPEND SRC_FILES
			"src/engine/rendererVk.cpp"
			"src/engine/rendererVk.h"
			"src/engine/shaders/vkGui.frag.dbg.h"
			"src/engine/shaders/vkGui.frag.rel.h"
			"src/engine/shaders/vkGui.vert.dbg.h"
			"src/engine/shaders/vkGui.vert.rel.h"
			"src/engine/shaders/vkIdx.comp.dbg.h"
			"src/engine/shaders/vkIdx.comp.rel.h"
			"src/engine/shaders/vkRgb.comp.dbg.h"
			"src/engine/shaders/vkRgb.comp.rel.h"
			"src/engine/shaders/vkSel.frag.dbg.h"
			"src/engine/shaders/vkSel.frag.rel.h"
			"src/engine/shaders/vkSel.vert.dbg.h"
			"src/engine/shaders/vkSel.vert.rel.h")
		add_compile_definitions(WITH_VULKAN)
		include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})
	endif()
endif()
if(WIN32)
	list(APPEND SRC_FILES "rsc/resource.rc")
	add_compile_definitions(UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
include_directories("${CMAKE_SOURCE_DIR}/src")

# dependencies
set(VER_SDL "2.30.5")
set(VER_IMG "2.8.2")
set(VER_FT2 "2.13.2")
set(VER_ARC "3.7.4")
set(VER_ICU "75-1")
set(VER_GLM "1.0.1")
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")
set(DIR_ICN "${DIR_RSC}/icons")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(MSVC)
	find_program(MAKE_CMD "nmake" REQUIRED)
else()
	if(MINGW)
		find_program(MAKE_CMD "mingw32-make")
	else()
		find_program(MAKE_CMD "make")
	endif()

	include(ProcessorCount)
	ProcessorCount(NJOBS)
	if(NJOBS GREATER 1)
		list(APPEND MAKE_CMD "-j${NJOBS}")
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
		find_program(HDIUTIL "hdiutil")
		execute_process(COMMAND "${HDIUTIL}" attach "${DST}/${FILE}")
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

if(USE_FONTCONFIG)
	CHECK_INCLUDE_FILE_CXX("fontconfig/fontconfig.h" FOUND_FONTCONFIG)
	if(FOUND_FONTCONFIG)
		list(APPEND SRC_FILES
			"src/engine/optional/fontconfig.cpp"
			"src/engine/optional/fontconfig.h")
		add_compile_definitions(CAN_FONTCFG)
	endif()
endif()
if(USE_MUPDF)
	CHECK_INCLUDE_FILE_CXX("mupdf/fitz.h" FOUND_MUPDF)
	if(FOUND_MUPDF)
		list(APPEND SRC_FILES
			"src/engine/optional/mupdf.cpp"
			"src/engine/optional/mupdf.h")
		add_compile_definitions(CAN_MUPDF)
	endif()
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
if(USE_POPPLER)
	list(APPEND PKGCONFIG_ARGS "poppler-glib")
endif()
if(PKGCONFIG_ARGS)
	find_package(PkgConfig)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(PKGCONFS ${PKGCONFIG_ARGS})
		include_directories(SYSTEM ${PKGCONFS_INCLUDE_DIRS})

		if((PKGCONFS_libsecret-1_FOUND OR PKGCONFS_libsecret-1_VERSION) AND (PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION OR PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION))
			list(APPEND SRC_FILES
				"src/engine/optional/secret.cpp"
				"src/engine/optional/secret.h")
			add_compile_definitions(CAN_SECRET)
			set(NEED_GLIB TRUE)
		endif()
		if(PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/smbclient.cpp"
				"src/engine/optional/smbclient.h")
			add_compile_definitions(CAN_SMB)
		endif()
		if(PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/ssh2.cpp"
				"src/engine/optional/ssh2.h")
			add_compile_definitions(CAN_SFTP)
		endif()
		if(PKGCONFS_poppler-glib_FOUND OR PKGCONFS_poppler-glib_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/poppler.cpp"
				"src/engine/optional/poppler.h")
			add_compile_definitions(CAN_POPPLER)
			set(NEED_GLIB TRUE)
		endif()
		if(NEED_GLIB)
			list(APPEND SRC_FILES
				"src/engine/optional/glib.cpp"
				"src/engine/optional/glib.h")
		endif()
	endif()
endif()

CHECK_INCLUDE_FILE_CXX("glm/glm.hpp" FOUND_GLM)
if(NOT FOUND_GLM)
	file(MAKE_DIRECTORY "${DIR_LIB}")
	downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}-light.zip" "${DIR_LIB}/glm" "")
	include_directories(SYSTEM "${DIR_LIB}")
endif()

if(WIN32)
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	set(TBIN_DIR "${PBOUT_DIR}")
	set(DATA_DIR "${PBOUT_DIR}")
	if(MSVC)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "x64")
		else()
			set(VMACHINE "x86")
		endif()
		set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}-${VMACHINE}-msvc")
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-msvc")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-msvc")
		set(DIR_FT2 "${DIR_LIB}/freetype-${VER_FT2}-${VMACHINE}-msvc")
		set(DIR_ICU "${DIR_LIB}/icu-${VER_ICU}-${VMACHINE}-msvc")
		set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
		set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
		set(BIN_FT2 "${DIR_FT2}/build")

		file(MAKE_DIRECTORY "${DIR_LIB}")
		downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		compileLib("https://download.savannah.gnu.org/releases/freetype/freetype-${VER_FT2}.tar.gz" "${DIR_FT2}" "${CMAKE_COMMAND}\;..\;-G\;NMake Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DFT_ENABLE_ERROR_STRINGS=1\;-DFT_DISABLE_ZLIB=1\;-DFT_DISABLE_BZIP2=1\;-DFT_DISABLE_PNG=1\;-DFT_DISABLE_HARFBUZZ=1\;-DFT_DISABLE_BROTLI=1;${MAKE_CMD}" "${BIN_FT2}" "${DIR_LIB}/freetype-${VER_FT2}")
		link_directories("${BIN_SDL}" "${BIN_IMG}" "${BIN_FT2}")
		include_directories(SYSTEM "${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_FT2}/include")

		if(USE_ARCHIVE)
			if(PREBUILT_ARCHIVE)
				set(BIN_ARC "${DIR_ARC}/bin")
				downloadLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-v${VER_ARC}-amd64.zip" "${DIR_ARC}" "${DIR_LIB}/libarchive")
				link_directories("${DIR_ARC}/lib")
				include_directories(SYSTEM "${DIR_ARC}/include")
			else()
				set(BIN_ARC "${DIR_ARC}/build/bin")
				compileLib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "${CMAKE_COMMAND}\;..\;-G\;NMake Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DENABLE_WERROR=0\;-DENABLE_MBEDTLS=0\;-DENABLE_NETTLE=0\;-DENABLE_OPENSSL=0\;-DENABLE_LIBB2=0\;-DENABLE_LZ4=0\;-DENABLE_LZO=0\;-DENABLE_LZMA=0\;-DENABLE_ZSTD=0\;-DENABLE_ZLIB=0\;-DENABLE_BZip2=0\;-DENABLE_LIBXML2=0\;-DENABLE_EXPAT=0\;-DENABLE_PCREPOSIX=0\;-DENABLE_CNG=0\;-DENABLE_XATTR=0\;-DENABLE_ACL=0\;-DENABLE_ICONV=0\;-DENABLE_TEST=0\;-DENABLE_COVERAGE=0\;-DENABLE_INSTALL=0;${MAKE_CMD}\;archive" "${DIR_ARC}/build" "${DIR_LIB}/libarchive-${VER_ARC}")
				link_directories("${DIR_ARC}/build/libarchive")
				include_directories(SYSTEM "${DIR_ARC}/libarchive")
			endif()
			add_compile_definitions(WITH_ARCHIVE)
		endif()

		if(USE_ICU)
			string(REGEX REPLACE "-[0-9]+" "" ICU_VER_MAJ ${VER_ICU})
			string(REPLACE "-" "_" VER_ICU_UDS ${VER_ICU})
			file(MAKE_DIRECTORY "${DIR_ICU}")
			if(CMAKE_SIZEOF_VOID_P EQUAL 8)
				set(BIN_ICU "${DIR_ICU}/bin64")
				downloadLib("https://github.com/unicode-org/icu/releases/download/release-${VER_ICU}/icu4c-${VER_ICU_UDS}-Win64-MSVC2022.zip" "${BIN_ICU}" "")
				link_directories("${DIR_ICU}/lib64")
			else()
				set(BIN_ICU "${DIR_ICU}/bin")
				downloadLib("https://github.com/unicode-org/icu/releases/download/release-${VER_ICU}/icu4c-${VER_ICU_UDS}-Win32-MSVC2022.zip" "${BIN_ICU}" "")
				link_directories("${DIR_ICU}/lib")
			endif()
			include_directories(SYSTEM "${DIR_ICU}/include")
			add_compile_definitions(WITH_ICU)
		endif()
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
endif()
if(NOT MSVC)
	find_package(SDL2 REQUIRED COMPONENTS SDL2)
	find_package(SDL2_image REQUIRED)
	find_package(Freetype REQUIRED)
	link_directories(${FREETYPE_LIBRARY_DIRS})
	include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})

	if(USE_ARCHIVE)
		find_package(LibArchive)
		if(LibArchive_FOUND)
			link_directories(${LibArchive_LIBRARY_DIRS})
			include_directories(SYSTEM ${LibArchive_INCLUDE_DIRS})
			add_compile_definitions(WITH_ARCHIVE)
		endif()
	endif()

	if(USE_ICU)
		if(WIN32)
			find_package(ICU COMPONENTS uc in)
		else()
			find_package(ICU COMPONENTS uc i18n)
		endif()
		if(ICU_FOUND)
			add_compile_definitions(WITH_ICU)
			include_directories(SYSTEM ${ICU_INCLUDE_DIRS})
		endif()
	endif()
endif()
set(ICONS_DIR "${DATA_DIR}/icons")
set(LICN_DIR "${DATA_DIR}/licenses")

# compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-Wall -Wextra -pedantic-errors -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code
		-Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-switch)
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
		add_compile_options(-Wcast-align -Wimplicit-int-float-conversion -Wnull-dereference -Wshadow-all -Wno-final-dtor-non-final-class)
	endif()
	if(MINGW)
		add_compile_options(-Wno-cast-function-type -Wno-unused-parameter -Wno-unused-variable)
	endif()
elseif(MSVC)
	add_compile_options(/wd4244 /wd4267 $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/GL>)
endif()

# set main target
add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
if(MSVC)
	target_link_libraries(${PROJECT_NAME}
		SDL2 SDL2_image freetype
		$<$<BOOL:${USE_ARCHIVE}>:archive>
		"$<$<BOOL:${USE_ICU}>:icuuc;icuin>")
else()
	target_link_libraries(${PROJECT_NAME}
		SDL2::SDL2 SDL2_image::SDL2_image ${FREETYPE_LIBRARIES}
		"$<$<BOOL:${LibArchive_FOUND}>:${LibArchive_LIBRARIES}>"
		"$<$<BOOL:${ICU_FOUND}>:ICU::uc;$<IF:$<BOOL:${WIN32}>,ICU::in,ICU::i18n>>"
		"$<$<BOOL:${UNIX}>:pthread;dl>")
endif()
if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)	# because it fucks with ALE
	target_precompile_headers(${PROJECT_NAME} PUBLIC "src/utils/widgets.h")
endif()

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
	COMMAND "$<$<NOT:$<BOOL:${FOUND_GLM}>>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_LIB}/glm/copying.txt;${LICN_DIR}/COPYING.glm.txt>"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.svg" "${DIR_ICN}/center.svg" "${DIR_ICN}/cross.svg" "${DIR_ICN}/file.svg" "${DIR_ICN}/fit.svg" "${DIR_ICN}/folder.svg" "${DIR_ICN}/left.svg" "${DIR_ICN}/minus.svg" "${DIR_ICN}/plus.svg" "${DIR_ICN}/reset.svg" "${DIR_ICN}/right.svg" "${DIR_ICN}/search.svg" "${ICONS_DIR}"
	COMMAND_EXPAND_LISTS)
if(MSVC)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "$<$<BOOL:${MSVC}>:${BIN_IMG}/optional/libavif-16.dll;${BIN_IMG}/optional/libtiff-5.dll;${BIN_IMG}/optional/libwebp-7.dll;${BIN_IMG}/optional/libwebpdemux-2.dll>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
		COMMAND "$<$<BOOL:${MSVC}>:${CMAKE_COMMAND};-E;copy_if_different;${BIN_IMG}/optional/LICENSE.avif.txt;${BIN_IMG}/optional/LICENSE.dav1d.txt;${BIN_IMG}/optional/LICENSE.tiff.txt;${BIN_IMG}/optional/LICENSE.webp.txt;${LICN_DIR}>"
		COMMAND "$<$<BOOL:${MINGW}>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_ARC}/COPYING;${LICN_DIR}/COPYING.libarchive.txt>"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/LICENSE.txt" "${LICN_DIR}/LICENSE.SDL_image.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_FT2}/LICENSE.txt" "${LICN_DIR}/LICENSE.freetype.txt"
		COMMAND_EXPAND_LISTS)
	if(USE_ARCHIVE)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ARC}/archive.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endif()
	if(USE_ICU)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ICU}/icudt${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuin${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuuc${ICU_VER_MAJ}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_ICU}/LICENSE" "${LICN_DIR}/LICENSE.icu.txt")
	endif()
elseif(MINGW)
	find_program(CMD_BASH "bash")
	find_program(CMD_LDD "ldd")
	find_program(CMD_GREP "grep")
	find_program(CMD_XARGS "xargs")
	if(CMD_BASH AND CMD_LDD AND CMD_GREP AND CMD_XARGS)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMD_BASH}" -c "'${CMD_LDD}' '$<TARGET_FILE:${PROJECT_NAME}>' | '${CMD_GREP}' -E '/(clang|mingw|ucrt)[0-9]+/.*\\.dll' -o | '${CMD_XARGS}' -I{} '${CMAKE_COMMAND}' -E copy_if_different '{}' '$<TARGET_FILE_DIR:${PROJECT_NAME}>'")
	else()
		message(WARNING "Can't copy DLL files after build.")
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
		find_program(CMD_BASH "bash")
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
			COMMAND "${CMD_BASH}" -c "VERSION='${PROJECT_VERSION}' '${LINUXDEPLOY}' --appdir '${PBOUT_DIR}' --output appimage")
	endif()
endif()

# install target
if(WIN32)
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${DST_DIR}")
	install(DIRECTORY "${DATA_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}" FILES_MATCHING PATTERN "*.dll")
	install(FILES "${DATA_DIR}/BrisaSans.otf" "${DATA_DIR}/themes.ini" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${ICONS_DIR}" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${LICN_DIR}" DESTINATION "${DST_DIR}")
elseif(APPIMAGE)
	install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VMARCH}.AppImage" DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
	install(FILES "${DATA_DIR}/BrisaSans.otf" "${DATA_DIR}/themes.ini" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${ICONS_DIR}" DESTINATION "${DST_DIR}")
	install(DIRECTORY "${LICN_DIR}" DESTINATION "${DST_DIR}")
endif()

# group files
if(MSVC)
	foreach(FSRC IN LISTS SRC_FILES)
		get_filename_component(FGRP "${FSRC}" DIRECTORY)
		string(REPLACE "/" ";" FGRP "${FGRP}")
		list(REMOVE_AT FGRP 0)
		string(REPLACE ";" "/" FGRP "${FGRP}")
		source_group("${FGRP}" FILES "${FSRC}")
	endforeach()
	foreach(CFG IN LISTS CMAKE_CONFIGURATION_TYPES)
		source_group("_pch" FILES "${CMAKE_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/${CFG}/cmake_pch.hxx")
	endforeach()
	source_group("_pch" FILES "${CMAKE_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/cmake_pch.cxx")
else()
	foreach(FSRC IN LISTS SRC_FILES)
		source_group("" FILES "${FSRC}")	# Qt Creator is a bitch
	endforeach()
endif()
