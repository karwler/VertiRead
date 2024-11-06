cmake_minimum_required(VERSION 3.16.9)
include(CheckIncludeFileCXX)
project(VertiRead VERSION 1.0.0 DESCRIPTION "Comic/Manga reader" HOMEPAGE_URL "https://github.com/karwler/VertiRead" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

option(FORCE_INLINE_SHADERS "Put shaders in the executable." OFF)
option(OPENGL "Build with OpenGL 1.1, 3.0 and ES 3.0 support." ON)
option(SDL3 "Build with SDL3" OFF)
option(VULKAN "Build with Vulkan 1.0 support." ON)
if(WIN32)
	option(DIRECT3D "Build with Direct3D 11 support." ON)
	option(FTP "Build with FTP support." OFF)
else()
	option(APPIMAGE "Package as an AppImage." OFF)
	option(FTP "Build with FTP support." ON)
	option(USE_MUPDF "Build with MuPDF support." ON)
	option(USE_SECRET "Build with libsecret if available." ON)
	option(USE_SMBCLIENT "Build with smbclient if available." ON)
	option(USE_SSH2 "Build with libssh2 if available." ON)
endif()
if(MSVC)
	option(USE_ARCHIVE "Build with libarchive." OFF)
	option(USE_ICU "Build with ICU." OFF)
else()
	option(NATIVE "Build for the current CPU." OFF)
	option(USE_ARCHIVE "Build with libarchive." ON)
	option(USE_FONTCONFIG "Build with fontconfig if available." ON)
	option(USE_GNUTLS "Build with gnutls if available." ON)
	option(USE_ICU "Build with ICU." ON)
	option(USE_OPENSSL "Build with openssl if available." ON)
	option(USE_POPPLER "Build with Poppler support." ON)
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
	"src/utils/sdlCompat.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/sthandle.h"
	"src/utils/stvector.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h"
	"src/utils/widgets.cpp"
	"src/utils/widgets.h")

set(DX_SHADERS "dxGuiVs" "dxGuiPs" "dxFinVs" "dxFinPs" "dxRgbCs" "dxBgrCs" "dxRedCs" "dxIdxCs")
set(GL_SHADERS "glGui.vert" "glGui.frag" "glFin.vert" "glFin.frag")
set(VK_SHADERS "vkGui.vert" "vkGui.frag" "vkFin.vert" "vkFin.frag" "vkRgb.comp" "vkIdx.comp")
set(SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")
if(DIRECT3D)
	CHECK_INCLUDE_FILE_CXX("d3d11.h" FOUND_D3D11)
	if(FOUND_D3D11)
		list(APPEND SRC_FILES
			"src/engine/rendererDx.cpp"
			"src/engine/rendererDx.h"
			"src/engine/optional/d3d.cpp"
			"src/engine/optional/d3d.h")
		add_compile_definitions(WITH_DIRECT3D)

		if(NOT FORCE_INLINE_SHADERS)
			find_program(FXC_CMD "fxc")
		endif()
		if(FXC_CMD)
			file(MAKE_DIRECTORY "${SHADER_DIR}")
			foreach(DFILE IN LISTS DX_SHADERS)
				string(LENGTH ${DFILE} DFILE_NUM)
				math(EXPR DFILE_NUM "${DFILE_NUM} - 2")
				string(SUBSTRING ${DFILE} ${DFILE_NUM} 2 DEXT)
				string(TOLOWER ${DEXT} DEXT)
				if(IS_MULTI_CONFIG OR CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
					execute_process(COMMAND "${FXC_CMD}" /Ges /Gis /T "${DEXT}_5_0" /Od /Zi /Fo "${SHADER_DIR}/${DFILE}.dbg.fxc" "rsc/shaders/${DFILE}.hlsl" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
				endif()
				if(IS_MULTI_CONFIG OR NOT(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"))
					execute_process(COMMAND "${FXC_CMD}" /Ges /Gis /T "${DEXT}_5_0" /O1 /Fo "${SHADER_DIR}/${DFILE}.rel.fxc" "rsc/shaders/${DFILE}.hlsl" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
				endif()
			endforeach()
			add_compile_definitions(EXT_DIRECT3D_SHADERS)
		else()
			foreach(DFILE IN LISTS DX_SHADERS)
				list(APPEND SRC_FILES
					"src/engine/shaders/${DFILE}.dbg.h"
					"src/engine/shaders/${DFILE}.rel.h")
			endforeach()
		endif()
	endif()
endif()
if(OPENGL)
	list(APPEND SRC_FILES
		"src/engine/rendererGl.cpp"
		"src/engine/rendererGl.h"
		"src/engine/shaders/glDefs.h")
	add_compile_definitions(WITH_OPENGL)

	foreach(GFILE IN LISTS GL_SHADERS)
		file(READ "${CMAKE_SOURCE_DIR}/rsc/shaders/${GFILE}" GL_CODE)
		file(WRITE "${CMAKE_SOURCE_DIR}/src/engine/shaders/${GFILE}.dbg.h" "R\"r(${GL_CODE})r\"")
		list(APPEND SRC_FILES
			"src/engine/shaders/${GFILE}.dbg.h"
			"src/engine/shaders/${GFILE}.rel.h")
	endforeach()
endif()
if(VULKAN)
	find_package(Vulkan)
	if(Vulkan_FOUND)
		list(APPEND SRC_FILES
			"src/engine/rendererVk.cpp"
			"src/engine/rendererVk.h")
		add_compile_definitions(WITH_VULKAN)
		include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})

		if(NOT FORCE_INLINE_SHADERS)
			find_program(GLSLC_CMD "glslc")
		endif()
		if(GLSLC_CMD)
			file(MAKE_DIRECTORY "${SHADER_DIR}")
			foreach(VFILE IN LISTS VK_SHADERS)
				if(IS_MULTI_CONFIG OR CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
					execute_process(COMMAND "${GLSLC_CMD}" --target-env=vulkan1.0 --target-spv=spv1.0 -g -o "${SHADER_DIR}/${VFILE}.dbg.spv" "rsc/shaders/${VFILE}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
				endif()
				if(IS_MULTI_CONFIG OR NOT(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"))
					execute_process(COMMAND "${GLSLC_CMD}" --target-env=vulkan1.0 --target-spv=spv1.0 -O -o "${SHADER_DIR}/${VFILE}.rel.spv" "rsc/shaders/${VFILE}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
				endif()
			endforeach()
			add_compile_definitions(EXT_VULKAN_SHADERS)
		else()
			foreach(VFILE IN LISTS VK_SHADERS)
				list(APPEND SRC_FILES
					"src/engine/shaders/${VFILE}.dbg.h"
					"src/engine/shaders/${VFILE}.rel.h")
			endforeach()
		endif()
	endif()
endif()
if(FTP)
	list(APPEND SRC_FILES
		"src/engine/network.cpp"
		"src/engine/network.h"
		"src/prog/fileOpsFtp.cpp"
		"src/prog/fileOpsFtp.h")
	add_compile_definitions(WITH_FTP)
endif()
if(WIN32)
	list(APPEND SRC_FILES "rsc/resource.rc")
	add_compile_definitions(UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
include_directories("${CMAKE_SOURCE_DIR}/src")

# dependencies
if(SDL3)
	set(VER_SDL "3.2.4")
	set(VER_IMG "3.2.0")
else()
	set(VER_SDL "2.32.0")
	set(VER_IMG "2.8.5")
endif()
string(SUBSTRING ${VER_SDL} 0 1 SDL_VER)
set(VER_FT2 "2.13.3")
set(VER_ARC "3.7.7")
set(VER_ICU "76-1")
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

function(download_lib URL LIBDIR ORIG_LIBDIR)
	if(EXISTS "${LIBDIR}")
		set(DOWNLOAD_LIB_FRESH FALSE PARENT_SCOPE)
		message(STATUS "Found ${LIBDIR}")
		return()
	endif()
	set(DOWNLOAD_LIB_FRESH TRUE PARENT_SCOPE)

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

CHECK_INCLUDE_FILE_CXX("format" FOUND_FORMAT)
if(NOT FOUND_FORMAT)
	add_compile_definitions(WITH_LIBFMT)
endif()
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
if(USE_SECRET AND (FTP OR USE_SMBCLIENT OR USE_SSH2))
	list(APPEND PKGCONFIG_ARGS "libsecret-1")
endif()
if(FTP AND USE_GNUTLS)
	list(APPEND PKGCONFIG_ARGS "gnutls")
endif()
if(FTP AND USE_OPENSSL)
	list(APPEND PKGCONFIG_ARGS "openssl")
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

		if((PKGCONFS_libsecret-1_FOUND OR PKGCONFS_libsecret-1_VERSION) AND (FTP OR PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION OR PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION))
			list(APPEND SRC_FILES
				"src/engine/optional/secret.cpp"
				"src/engine/optional/secret.h")
			add_compile_definitions(CAN_SECRET)
			set(NEED_GLIB TRUE)
		endif()
		if(PKGCONFS_gnutls_FOUND OR PKGCONFS_gnutls_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/gnutls.cpp"
				"src/engine/optional/gnutls.h")
			add_compile_definitions(CAN_GNUTLS)
		endif()
		if(PKGCONFS_openssl_FOUND OR PKGCONFS_openssl_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/openssl.cpp"
				"src/engine/optional/openssl.h")
			add_compile_definitions(CAN_OPENSSL)
		endif()
		if(PKGCONFS_smbclient_FOUND OR PKGCONFS_smbclient_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/smbclient.cpp"
				"src/engine/optional/smbclient.h"
				"src/prog/fileOpsSmb.cpp"
				"src/prog/fileOpsSmb.h")
			add_compile_definitions(CAN_SMB)
		endif()
		if(PKGCONFS_libssh2_FOUND OR PKGCONFS_libssh2_VERSION)
			list(APPEND SRC_FILES
				"src/engine/optional/ssh2.cpp"
				"src/engine/optional/ssh2.h"
				"src/prog/fileOpsSftp.cpp"
				"src/prog/fileOpsSftp.h")
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
	download_lib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}-light.zip" "${DIR_LIB}/glm" "")
	include_directories(SYSTEM "${DIR_LIB}")
endif()

if(WIN32 AND NOT IS_MULTI_CONFIG)
	set(TBIN_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
endif()
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(VMACHINE "x64")
	else()
		set(VMACHINE "x86")
	endif()
	set(DIR_ARC "${DIR_LIB}/libarchive-${VER_ARC}-${VMACHINE}-msvc")
	set(DIR_SDL "${DIR_LIB}/SDL${SDL_VER}-${VER_SDL}-msvc")
	set(DIR_IMG "${DIR_LIB}/SDL${SDL_VER}_image-${VER_IMG}-msvc")
	set(DIR_FT2 "${DIR_LIB}/freetype-${VER_FT2}-${VMACHINE}-msvc")
	set(DIR_ICU "${DIR_LIB}/icu-${VER_ICU}-${VMACHINE}-msvc")
	set(BIN_ARC "${DIR_ARC}/build/bin")
	set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
	set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
	set(BIN_FT2 "${DIR_FT2}/build")

	file(MAKE_DIRECTORY "${DIR_LIB}")
	download_lib("https://download.savannah.gnu.org/releases/freetype/freetype-${VER_FT2}.tar.gz" "${DIR_FT2}" "${DIR_LIB}/freetype-${VER_FT2}")
	if(DOWNLOAD_LIB_FRESH)
		file(MAKE_DIRECTORY "${BIN_FT2}")
		execute_process(COMMAND "${CMAKE_COMMAND}" ".." -G "NMake Makefiles" "-DCMAKE_BUILD_TYPE=Release" "-DFT_ENABLE_ERROR_STRINGS=ON" "-DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON" "-DFT_DISABLE_PNG=ON" "-DFT_DISABLE_HARFBUZZ=ON" "-DFT_DISABLE_BROTLI=ON" WORKING_DIRECTORY "${BIN_FT2}")
		execute_process(COMMAND "${MAKE_CMD}" WORKING_DIRECTORY "${BIN_FT2}")
	endif()
	link_directories("${BIN_FT2}")
	include_directories(SYSTEM "${DIR_FT2}/include")

	download_lib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL${SDL_VER}-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL${SDL_VER}-${VER_SDL}")
	download_lib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL${SDL_VER}_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL${SDL_VER}_image-${VER_IMG}")
	if(SDL3)
		include_directories(SYSTEM "${DIR_SDL}/include/SDL3" "${DIR_IMG}/include/SDL3_image")
	endif()
	link_directories("${BIN_SDL}" "${BIN_IMG}")
	include_directories(SYSTEM "${DIR_SDL}/include" "${DIR_IMG}/include")

	if(USE_ARCHIVE)
		download_lib("https://github.com/libarchive/libarchive/releases/download/v${VER_ARC}/libarchive-${VER_ARC}.zip" "${DIR_ARC}" "${DIR_LIB}/libarchive-${VER_ARC}")
		if(DOWNLOAD_LIB_FRESH)
			file(MAKE_DIRECTORY "${DIR_ARC}/build")
			execute_process(COMMAND "${CMAKE_COMMAND}" ".." -G "NMake Makefiles" "-DCMAKE_BUILD_TYPE=Release" "-DENABLE_WERROR=OFF" "-DENABLE_MBEDTLS=OFF" "-DENABLE_NETTLE=OFF" "-DENABLE_OPENSSL=OFF" "-DENABLE_LIBB2=OFF" "-DENABLE_LZ4=OFF" "-DENABLE_LZO=OFF" "-DENABLE_LZMA=OFF" "-DENABLE_ZSTD=OFF" "-DENABLE_ZLIB=OFF" "-DENABLE_BZip2=OFF" "-DENABLE_LIBXML2=OFF" "-DENABLE_EXPAT=OFF" "-DENABLE_PCREPOSIX=OFF" "-DENABLE_CNG=OFF" "-DENABLE_XATTR=OFF" "-DENABLE_ACL=OFF" "-DENABLE_ICONV=OFF" "-DENABLE_TEST=OFF" "-DENABLE_COVERAGE=OFF" "-DENABLE_INSTALL=OFF" WORKING_DIRECTORY "${DIR_ARC}/build")
			execute_process(COMMAND "${MAKE_CMD}" "archive" WORKING_DIRECTORY "${DIR_ARC}/build")
		endif()
		link_directories("${DIR_ARC}/build/libarchive")
		include_directories(SYSTEM "${DIR_ARC}/libarchive")
		add_compile_definitions(WITH_ARCHIVE)
	endif()

	if(USE_ICU)
		string(REGEX REPLACE "-[0-9]+" "" ICU_VER_MAJ ${VER_ICU})
		string(REPLACE "-" "_" VER_ICU_UDS ${VER_ICU})
		file(MAKE_DIRECTORY "${DIR_ICU}")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(BIN_ICU "${DIR_ICU}/bin64")
			download_lib("https://github.com/unicode-org/icu/releases/download/release-${VER_ICU}/icu4c-${VER_ICU_UDS}-Win64-MSVC2022.zip" "${BIN_ICU}" "")
			link_directories("${DIR_ICU}/lib64")
		else()
			set(BIN_ICU "${DIR_ICU}/bin")
			download_lib("https://github.com/unicode-org/icu/releases/download/release-${VER_ICU}/icu4c-${VER_ICU_UDS}-Win32-MSVC2022.zip" "${BIN_ICU}" "")
			link_directories("${DIR_ICU}/lib")
		endif()
		include_directories(SYSTEM "${DIR_ICU}/include")
		add_compile_definitions(WITH_ICU)
	endif()
else()
	if(UNIX)
		if(APPIMAGE)
			set(TBIN_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.AppDir/usr/bin")
			set(BASE_SUF "/../..")
			set(DATA_SUF "${BASE_SUF}/usr/share/${PROJECT_NAME_LOWER}")
		else()
			set(TBIN_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/bin")
			set(BASE_SUF "/..")
			set(DATA_SUF "${BASE_SUF}/share/${PROJECT_NAME_LOWER}")
		endif()
	endif()

	if(SDL3)
		find_package(SDL3 REQUIRED COMPONENTS SDL3)
		find_package(SDL3_image REQUIRED)
		include_directories(SYSTEM "$<TARGET_FILE_DIR:SDL3::SDL3>/../include/SDL3" "$<TARGET_FILE_DIR:SDL3_image::SDL3_image>/../include/SDL3_image")
	else()
		find_package(SDL2 REQUIRED COMPONENTS SDL2)
		find_package(SDL2_image REQUIRED)
	endif()
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
set(ICONS_SUF "${DATA_SUF}/icons")
set(LICN_SUF "${DATA_SUF}/licenses")
set(DVSHD_SUF "${DATA_SUF}/shaders")

# compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
	add_compile_options(-Wall -Wextra -pedantic-errors -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code
		-Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-switch)
	if(NATIVE)
		add_compile_options(-march=native)
	endif()
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		add_compile_options(-Wcast-align=strict -Wduplicated-cond -Wlogical-op -Wuseless-cast
			"$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin>")
		add_link_options("$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin;-s>")
	else()
		add_compile_options(-Wcast-align -Wnull-dereference -Wshadow-all -Wno-final-dtor-non-final-class)
	endif()
	if(MINGW)
		add_compile_options(-Wno-cast-function-type -Wno-unused-parameter -Wno-unused-variable)
	endif()
elseif(MSVC)
	add_compile_options(/wd4244 /wd4267 $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/GL>)
endif()

# set main target
add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
if(WIN32 AND FTP)
	target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
if(MSVC)
	target_link_libraries(${PROJECT_NAME}
		"$<IF:$<BOOL:${SDL3}>,SDL3;SDL3_image,SDL2;SDL2_image>"
		freetype
		$<$<BOOL:${USE_ARCHIVE}>:archive>
		"$<$<BOOL:${USE_ICU}>:icuuc;icuin>")
else()
	target_link_libraries(${PROJECT_NAME}
		"$<IF:$<BOOL:${SDL3}>,SDL3::SDL3;SDL3_image::SDL3_image,SDL2::SDL2;SDL2_image::SDL2_image>"
		${FREETYPE_LIBRARIES}
		"$<$<BOOL:${LibArchive_FOUND}>:${LibArchive_LIBRARIES}>"
		"$<$<BOOL:${ICU_FOUND}>:ICU::uc;$<IF:$<BOOL:${WIN32}>,ICU::in,ICU::i18n>>"
		"$<$<BOOL:${UNIX}>:pthread;dl>"
		$<$<NOT:$<BOOL:${FOUND_FORMAT}>>:fmt>)
endif()
if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)	# because it fucks with ALE
	target_precompile_headers(${PROJECT_NAME} PUBLIC "src/utils/widgets.h")
endif()

if(TBIN_DIR)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${TBIN_DIR}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TBIN_DIR}"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TBIN_DIR}"
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TBIN_DIR}"
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${TBIN_DIR}")
endif()
if(WIN32)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
	set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME ${PROJECT_NAME_LOWER})
endif()

# build commands
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>${ICONS_SUF}"
	COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_RSC}/licenses" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}"
	COMMAND "$<$<OR:$<BOOL:${FXC_CMD}>,$<BOOL:${GLSLC_CMD}>>:${CMAKE_COMMAND};-E;make_directory;$<TARGET_FILE_DIR:${PROJECT_NAME}>${DVSHD_SUF}>"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/BrisaSans.otf" "${DIR_RSC}/themes.ini" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DATA_SUF}"
	COMMAND "$<$<NOT:$<BOOL:${FOUND_GLM}>>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_LIB}/glm/copying.txt;$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}/COPYING.glm.txt>"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.svg" "${DIR_ICN}/center.svg" "${DIR_ICN}/cross.svg" "${DIR_ICN}/file.svg" "${DIR_ICN}/fit.svg" "${DIR_ICN}/folder.svg" "${DIR_ICN}/left.svg" "${DIR_ICN}/minus.svg" "${DIR_ICN}/plus.svg" "${DIR_ICN}/reset.svg" "${DIR_ICN}/right.svg" "${DIR_ICN}/search.svg" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${ICONS_SUF}"
	COMMAND_EXPAND_LISTS)
if(FXC_CMD)
	foreach(DFILE IN LISTS DX_SHADERS)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SHADER_DIR}/${DFILE}.$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,dbg,rel>.fxc" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DVSHD_SUF}/${DFILE}.fxc")
	endforeach()
endif()
if(GLSLC_CMD)
	foreach(VFILE IN LISTS VK_SHADERS)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SHADER_DIR}/${VFILE}.$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,dbg,rel>.spv" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DVSHD_SUF}/${VFILE}.spv")
	endforeach()
endif()
if(MSVC)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL${SDL_VER}.dll" "${BIN_IMG}/SDL${SDL_VER}_image.dll" "${BIN_IMG}/optional/libavif-16.dll" "${BIN_IMG}/optional/libtiff-$<IF:$<BOOL:${SDL3}>,6,5>.dll" "${BIN_IMG}/optional/libwebp-7.dll" "${BIN_IMG}/optional/libwebpdemux-2.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/optional/LICENSE.avif.txt" "${BIN_IMG}/optional/LICENSE.dav1d.txt" "${BIN_IMG}/optional/LICENSE.tiff.txt" "${BIN_IMG}/optional/LICENSE.webp.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/LICENSE.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}/LICENSE.SDL.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/LICENSE.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}/LICENSE.SDL_image.txt"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_FT2}/LICENSE.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}/LICENSE.freetype.txt")
	if(USE_ARCHIVE)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ARC}/archive.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endif()
	if(USE_ICU)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_ICU}/icudt${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuin${ICU_VER_MAJ}.dll" "${BIN_ICU}/icuuc${ICU_VER_MAJ}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_ICU}/LICENSE" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}/LICENSE.icu.txt")
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
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.desktop" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${BASE_SUF}")

	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
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
			set(LDPEXE_NAME "linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
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
			COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/bin/${PROJECT_NAME_LOWER}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${BASE_SUF}/AppRun"
			COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/share/${PROJECT_NAME_LOWER}/icons/${PROJECT_NAME_LOWER}.svg" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${BASE_SUF}/${PROJECT_NAME_LOWER}.svg"
			COMMAND "${CMD_BASH}" -c "VERSION='${PROJECT_VERSION}' '${LINUXDEPLOY}' --appdir '$<TARGET_FILE_DIR:${PROJECT_NAME}>${BASE_SUF}' --output appimage")
	endif()
endif()

# install target
if(WIN32)
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${DST_DIR}")
	install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DATA_SUF}" DESTINATION "${CMAKE_INSTALL_PREFIX}" FILES_MATCHING PATTERN "*.dll")
elseif(APPIMAGE)
	install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${CMAKE_SYSTEM_PROCESSOR}.AppImage" DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
	set(DST_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}")
	install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()
if(NOT APPIMAGE)
	install(FILES "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DATA_SUF}/BrisaSans.otf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DATA_SUF}/themes.ini" DESTINATION "${DST_DIR}")
	install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>${ICONS_SUF}" DESTINATION "${DST_DIR}")
	install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>${LICN_SUF}" DESTINATION "${DST_DIR}")
	if(FXC_CMD OR GLSLC_CMD)
		install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>${DVSHD_SUF}" DESTINATION "${DST_DIR}")
	endif()
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
