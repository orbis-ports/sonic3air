# Sonic 3 A.I.R. - PlayStation 4 (OpenOrbis) build.
#
#   Oxygen/sonic3air/build/_ps4/build.sh        <- use this; it passes the toolchain and the paths
#
# Derived from ../_cmake/CMakeLists.txt. What differs, and why:
#
#   * SDL2 is NOT add_subdirectory()'d. SDL's own CMake does not know a CMAKE_SYSTEM_NAME of Generic
#     with a FreeBSD triple and a musl libc; the target is assembled here from an explicit source list
#     and configured by sdl/SDL_config_orbis.h (selected by include/SDL_config.h on __ORBIS__).
#     Platform drivers are opt-in, see the PS4_SDL_ORBIS_* options below.
#   * No ImGui, Discord, curl, OxygenApp or server. No find_package(OpenGL/CURL/Threads), no stdc++fs:
#     std::filesystem is in the SDK's libc++.a, pthreads are in its libc.a.
#   * GL: there is no libGL.a. Desktop GL is reached through Mesa's EGL (platform orbis) + zink + RADV,
#     statically linked, and GLEW resolves every gl* entry point with eglGetProcAddress (GLEW_EGL).
#   * The executable also carries orbis-compat's optional log channel (ps4_app.cpp, orbis_netlog.cpp)
#     and the runtime corrections copied from RetroArch into orbis/.
#
# The toolchain file (orbis-compat/cmake/ps4-openorbis.cmake) supplies the target triple, the include
# order (libc++ -> orbis-compat -> SDK), -D__ORBIS__ -D__PS4__, the link script and liborbis-compat.a.

cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

project(Sonic3AIR_PS4 C CXX)

if (NOT PS4)
	message(FATAL_ERROR "This CMakeLists.txt builds only with orbis-compat/cmake/ps4-openorbis.cmake - use build.sh")
endif()

# The one check that cannot live in the toolchain file (it needs a working compiler).
if (COMMAND orbis_compat_verify)
	orbis_compat_verify()
endif()


# ---------------------------------------------------------------- options

option(PS4_SDL_ORBIS_AUDIO    "SDL: build src/audio/orbis (SDL_AUDIO_DRIVER_ORBIS) instead of dummy audio only" OFF)
option(PS4_SDL_ORBIS_JOYSTICK "SDL: build src/joystick/orbis (SDL_JOYSTICK_ORBIS) instead of the dummy joystick" OFF)
option(PS4_SDL_ORBIS_VIDEO    "SDL: build src/video/orbis (SDL_VIDEO_DRIVER_ORBIS + EGL/GL/GLES2) instead of dummy video only" OFF)

# "S3AR00001" was the planned id, but make-pkg.sh (and param.sfo) require 4 LETTERS + 5 digits
set(PS4_TITLE_ID      "SAIR00001"      CACHE STRING "PS4 title id (AAAA00000: 4 letters, 5 digits)")
set(PS4_TITLE         "Sonic 3 A.I.R." CACHE STRING "PS4 title shown on the home screen")
set(PS4_CONTENT_LABEL "SONIC3AIR"      CACHE STRING "content label, <=16 chars [A-Z0-9]")
# Directory holding the packed release data (enginedata.bin, gamedata.bin, audiodata.bin, optionally
# audioremaster.bin, metadata.json, scripts.bin). Each file that exists there goes into the .pkg at
# /app0/data/<name>. Empty: the package carries no game data and it has to be provided another way.
set(PS4_DATA_DIR "" CACHE PATH "directory with enginedata.bin / gamedata.bin / audiodata.bin / scripts.bin to package")

set(ORBIS_MESA_BUILD "$ENV{ORBIS_MESA_BUILD}" CACHE PATH "mesa-ps4 build-orbis directory (static archives)")
set(ORBIS_MESA_SRC   "$ENV{ORBIS_MESA_DIR}"   CACHE PATH "mesa-ps4 source checkout (for include/)")

message(STATUS "PS4_SDL_ORBIS_AUDIO    = ${PS4_SDL_ORBIS_AUDIO}")
message(STATUS "PS4_SDL_ORBIS_JOYSTICK = ${PS4_SDL_ORBIS_JOYSTICK}")
message(STATUS "PS4_SDL_ORBIS_VIDEO    = ${PS4_SDL_ORBIS_VIDEO}")
message(STATUS "ORBIS_MESA_BUILD       = ${ORBIS_MESA_BUILD}")
message(STATUS "ORBIS_MESA_SRC         = ${ORBIS_MESA_SRC}")
message(STATUS "PS4_DATA_DIR           = ${PS4_DATA_DIR}")

if (NOT EXISTS "${ORBIS_MESA_SRC}/include/EGL/egl.h")
	message(FATAL_ERROR "No Mesa headers at ${ORBIS_MESA_SRC}/include - pass -DORBIS_MESA_SRC=<mesa-ps4 checkout>")
endif()
file(GLOB PS4_MESA_GALLIUM "${ORBIS_MESA_BUILD}/src/gallium/targets/dri/libgallium-*.a")
set(PS4_MESA_RADV "${ORBIS_MESA_BUILD}/src/amd/vulkan/libvulkan_radeon.a")
if (NOT PS4_MESA_GALLIUM OR NOT EXISTS "${PS4_MESA_RADV}" OR NOT EXISTS "${ORBIS_MESA_BUILD}/src/egl/libEGL.a")
	message(FATAL_ERROR "No Mesa archives under ${ORBIS_MESA_BUILD} (libEGL.a, libgallium-*.a, libvulkan_radeon.a) - build mesa-ps4 with the OpenGL arm")
endif()
file(GLOB PS4_MESA_ZLIB "${ORBIS_MESA_BUILD}/subprojects/zlib-*/libz.a")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(WORKSPACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(PS4_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SDL_DIR "${WORKSPACE_DIR}/framework/external/sdl/SDL2")

add_compile_options(-Wno-unused-function -Wno-unused-command-line-argument)
# Clang 22 against the SDK's libc++ 11 and a decade-old code base: keep the noise down so real errors
# are visible. None of these change code generation.
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
                    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-builtins>
                    $<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>
                    $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>)

# Mesa's include/ (GL, EGL, GLES2, KHR) goes AHEAD of the SDK's include directory, which carries
# Piglet's EGL/GLES2 headers. -I is searched before every -isystem the toolchain file adds, so a
# plain include_directories() is enough to win.
include_directories(BEFORE "${ORBIS_MESA_SRC}/include")


# ---------------------------------------------------------------- SDL2 (static, explicit source list)

set(SDL_SRC_GLOBS
	${SDL_DIR}/src/*.c
	${SDL_DIR}/src/atomic/*.c
	${SDL_DIR}/src/audio/*.c
	${SDL_DIR}/src/audio/dummy/*.c
	${SDL_DIR}/src/audio/disk/*.c
	${SDL_DIR}/src/cpuinfo/*.c
	${SDL_DIR}/src/dynapi/*.c
	${SDL_DIR}/src/events/*.c
	${SDL_DIR}/src/file/*.c
	${SDL_DIR}/src/filesystem/dummy/*.c
	${SDL_DIR}/src/haptic/*.c
	${SDL_DIR}/src/haptic/dummy/*.c
	${SDL_DIR}/src/hidapi/SDL_hidapi.c
	${SDL_DIR}/src/joystick/*.c
	${SDL_DIR}/src/libm/*.c
	${SDL_DIR}/src/loadso/dummy/*.c
	${SDL_DIR}/src/locale/*.c
	${SDL_DIR}/src/locale/dummy/*.c
	${SDL_DIR}/src/misc/*.c
	${SDL_DIR}/src/misc/dummy/*.c
	${SDL_DIR}/src/power/*.c
	${SDL_DIR}/src/render/*.c
	${SDL_DIR}/src/render/software/*.c
	${SDL_DIR}/src/sensor/*.c
	${SDL_DIR}/src/sensor/dummy/*.c
	${SDL_DIR}/src/stdlib/*.c
	${SDL_DIR}/src/thread/*.c
	${SDL_DIR}/src/thread/pthread/SDL_syscond.c
	${SDL_DIR}/src/thread/pthread/SDL_sysmutex.c
	${SDL_DIR}/src/thread/pthread/SDL_systhread.c
	${SDL_DIR}/src/thread/pthread/SDL_systls.c
	# NOT pthread/SDL_syssem.c: the SDK's sem_* are FreeBSD ksem_* and fail on the console (EINVAL);
	# the generic mutex+condvar semaphore is used instead (agent B, STATUS.md 2026-09-15).
	${SDL_DIR}/src/thread/generic/SDL_syssem.c
	${SDL_DIR}/src/timer/*.c
	${SDL_DIR}/src/timer/unix/*.c
	${SDL_DIR}/src/video/*.c
	${SDL_DIR}/src/video/dummy/*.c
	${SDL_DIR}/src/video/yuv2rgb/*.c)

if (PS4_SDL_ORBIS_AUDIO)
	list(APPEND SDL_SRC_GLOBS ${SDL_DIR}/src/audio/orbis/*.c)
endif()
if (PS4_SDL_ORBIS_JOYSTICK)
	list(APPEND SDL_SRC_GLOBS ${SDL_DIR}/src/joystick/orbis/*.c)
else()
	list(APPEND SDL_SRC_GLOBS ${SDL_DIR}/src/joystick/dummy/*.c)
endif()
if (PS4_SDL_ORBIS_VIDEO)
	list(APPEND SDL_SRC_GLOBS ${SDL_DIR}/src/video/orbis/*.c ${SDL_DIR}/src/render/opengles2/*.c)
endif()

file(GLOB SDL_SOURCES CONFIGURE_DEPENDS ${SDL_SRC_GLOBS})

add_library(SDL2-static STATIC ${SDL_SOURCES})
target_include_directories(SDL2-static
	PUBLIC  "${PS4_DIR}/sdl" "${SDL_DIR}/include"
	PRIVATE "${SDL_DIR}/src" "${SDL_DIR}/src/video/khronos")
# The config header reads these, and they are PUBLIC so an engine TU including SDL.h sees the same
# SDL_config as the library was built with.
target_compile_definitions(SDL2-static PUBLIC
	PS4_SDL_ORBIS_AUDIO=$<BOOL:${PS4_SDL_ORBIS_AUDIO}>
	PS4_SDL_ORBIS_JOYSTICK=$<BOOL:${PS4_SDL_ORBIS_JOYSTICK}>
	PS4_SDL_ORBIS_VIDEO=$<BOOL:${PS4_SDL_ORBIS_VIDEO}>)
# SDL_dynapi.h cannot be switched off from the command line, but it honours "needs dlopen and there
# is none" - which is exactly this platform.
target_compile_definitions(SDL2-static PRIVATE DYNAPI_NEEDS_DLOPEN=1)
target_compile_options(SDL2-static PRIVATE -Wno-declaration-after-statement)


# ---------------------------------------------------------------- zlib, ogg/vorbis, minizip

set(ZLIB_SRC_DIR "${WORKSPACE_DIR}/framework/external/zlib/zlib")
file(GLOB ZLIB_SOURCES ${ZLIB_SRC_DIR}/*.c)
add_library(zlibstatic STATIC ${ZLIB_SOURCES})
target_include_directories(zlibstatic PUBLIC "${ZLIB_SRC_DIR}")
target_compile_definitions(zlibstatic PRIVATE HAVE_UNISTD_H=1 _LARGEFILE64_SOURCE=1)

include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib)
include_directories(${ZLIB_SRC_DIR})
include_directories(${ZLIB_SRC_DIR}/contrib/minizip)
include_directories(${WORKSPACE_DIR}/librmx/source)
include_directories(${WORKSPACE_DIR}/librmx/source/rmxmedia/_glew)
include_directories(${WORKSPACE_DIR}/Oxygen/lemonscript/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenengine/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source/external)
# orbis-compat's headers are on every TU through the toolchain flags already; ps4_app.h and
# orbis_netlog.h live in the same include/ directory.

file(GLOB OGGVORBIS_SOURCES ${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/src/*.c
							${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib/*.c)
add_library(oggvorbis STATIC ${OGGVORBIS_SOURCES})

file(GLOB MINIZIP_SOURCES ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
						  ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
						  ${ZLIB_SRC_DIR}/contrib/minizip/zip.c)
add_library(minizip STATIC ${MINIZIP_SOURCES})
# musl has no fopen64 family; minizip's ioapi.c uses the plain calls with this.
target_compile_definitions(minizip PRIVATE USE_FILE32API=1)
target_link_libraries(minizip zlibstatic)


# ---------------------------------------------------------------- rmx

file(GLOB_RECURSE RMXBASE_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxbase/*.cpp)
add_library(rmxbase STATIC ${RMXBASE_SOURCES})
target_precompile_headers(rmxbase PRIVATE ${WORKSPACE_DIR}/librmx/source/rmxbase.h)
target_link_libraries(rmxbase zlibstatic)

file(GLOB_RECURSE RMXMEDIA_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/*.cpp
								   ${WORKSPACE_DIR}/librmx/source/rmxmedia/_glew/*.c)
add_library(rmxmedia STATIC ${RMXMEDIA_SOURCES})
target_link_libraries(rmxmedia rmxbase SDL2-static)
# GLEW over EGL: glewGetProcAddress == eglGetProcAddress (see _glew/glew.c). glew.c does not include
# rmxmedia_externals.h, so the switches have to be compile definitions and not only #defines there.
target_compile_definitions(rmxmedia PUBLIC GLEW_EGL GLEW_STATIC)
file(GLOB RMXMEDIA_GLEW_C_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/_glew/*.c)
set_source_files_properties(${RMXMEDIA_GLEW_C_SOURCES} PROPERTIES COMPILE_DEFINITIONS GLEW_NO_GLU)	# rmxmedia_externals.h defines it for the C++ side

file(GLOB RMXEXT_OGGVORBIS_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxext_oggvorbis/*.cpp)
add_library(rmxext_oggvorbis STATIC ${RMXEXT_OGGVORBIS_SOURCES})
target_link_libraries(rmxext_oggvorbis rmxmedia oggvorbis)


# ---------------------------------------------------------------- lemonscript, netcore, oxygen

file(GLOB_RECURSE LEMONSCRIPT_SOURCES ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/*.cpp)
add_library(lemonscript STATIC ${LEMONSCRIPT_SOURCES})
target_precompile_headers(lemonscript PRIVATE ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/pch.h)
target_link_libraries(lemonscript rmxbase)

file(GLOB_RECURSE OXYGEN_NETCORE_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp)
add_library(oxygen_netcore STATIC ${OXYGEN_NETCORE_SOURCES})
target_precompile_headers(oxygen_netcore PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/pch.h)
target_link_libraries(oxygen_netcore rmxbase)

file(GLOB_RECURSE OXYGEN_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/*.cpp)
add_library(oxygen STATIC ${OXYGEN_SOURCES})
target_link_libraries(oxygen minizip rmxmedia rmxext_oggvorbis lemonscript oxygen_netcore)


# ---------------------------------------------------------------- orbis-compat optional: log channel

set(PS4_NETLOG_HOST "192.168.100.1" CACHE STRING "dev host that receives netlog datagrams")
set(PS4_NETLOG_PORT "18194"         CACHE STRING "udp port scripts/ps4/logs.sh binds")
add_subdirectory("${ORBIS_COMPAT_DIR}/optional" ps4-common)
target_compile_definitions(ps4-netlog PRIVATE NETLOG_TAG="sonic3air")


# ---------------------------------------------------------------- Sonic3AIR

file(GLOB_RECURSE SONIC3AIR_SOURCES ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/*.cpp)
set(PS4_ORBIS_SOURCES
	${PS4_DIR}/orbis/orbis_preinit.c		# FIRST: its constructor is the first of the game's (.init_array is in link order)
	${PS4_DIR}/orbis/orbis_cxa_guard.c
	${PS4_DIR}/orbis/orbis_thread_atexit.c
	${PS4_DIR}/orbis/orbis_abort_report.c
	${PS4_DIR}/orbis/orbis_wchar32.c
	${PS4_DIR}/orbis/orbis_cv_fix.cpp
	${PS4_DIR}/orbis/orbis_crash_handlers.cpp)

# ⚠ 16-bit wchar_t in the SDK's libc.a - see orbis/orbis_wchar32.c, which replaces the wide-character
# members with 32-bit ones. Three libc.a members that stay (regcomp and regexec - Mesa's driconf -
# and vfscanf) call mbtowc/mbrtowc with a pointer to a 2-byte wchar_t on their stack; the 32-bit
# versions would write 4 bytes there (regcomp keeps a saved register right behind it). They are
# linked as copies whose call is renamed to the SDK's ORIGINAL 16-bit mbtowc/mbrtowc, which are
# linked under that new name. Objects, not an archive: an object always wins over libc.a.
set(PS4_SDK_LIBC "${OO_PS4_TOOLCHAIN}/lib/libc.a")
set(PS4_LIBC16_DIR "${CMAKE_BINARY_DIR}/libc16")
set(PS4_LIBC16_OBJECTS
	${PS4_LIBC16_DIR}/libc16_regcomp.o ${PS4_LIBC16_DIR}/libc16_regexec.o ${PS4_LIBC16_DIR}/libc16_vfscanf.o
	${PS4_LIBC16_DIR}/libc16_mbtowc.o ${PS4_LIBC16_DIR}/libc16_mbrtowc.o)
add_custom_command(
	OUTPUT ${PS4_LIBC16_OBJECTS}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PS4_LIBC16_DIR}"
	COMMAND "${CMAKE_AR}" x "--output=${PS4_LIBC16_DIR}" "${PS4_SDK_LIBC}" regcomp.lo regexec.lo vfscanf.lo mbtowc.lo mbrtowc.lo
	COMMAND "${CMAKE_OBJCOPY}" --redefine-sym mbtowc=__orbis_libc16_mbtowc   "${PS4_LIBC16_DIR}/regcomp.lo" "${PS4_LIBC16_DIR}/libc16_regcomp.o"
	COMMAND "${CMAKE_OBJCOPY}" --redefine-sym mbtowc=__orbis_libc16_mbtowc   "${PS4_LIBC16_DIR}/regexec.lo" "${PS4_LIBC16_DIR}/libc16_regexec.o"
	COMMAND "${CMAKE_OBJCOPY}" --redefine-sym mbrtowc=__orbis_libc16_mbrtowc "${PS4_LIBC16_DIR}/vfscanf.lo" "${PS4_LIBC16_DIR}/libc16_vfscanf.o"
	COMMAND "${CMAKE_OBJCOPY}" --redefine-sym mbtowc=__orbis_libc16_mbtowc   "${PS4_LIBC16_DIR}/mbtowc.lo"  "${PS4_LIBC16_DIR}/libc16_mbtowc.o"
	COMMAND "${CMAKE_OBJCOPY}" --redefine-sym mbrtowc=__orbis_libc16_mbrtowc "${PS4_LIBC16_DIR}/mbrtowc.lo" "${PS4_LIBC16_DIR}/libc16_mbrtowc.o"
	DEPENDS "${PS4_SDK_LIBC}"
	COMMENT "libc16: renamed copies of libc.a regcomp/regexec/vfscanf + the 16-bit mbtowc/mbrtowc they call"
	VERBATIM)
set_source_files_properties(${PS4_LIBC16_OBJECTS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sonic3air")
add_executable(Sonic3AIR ${PS4_ORBIS_SOURCES} ${SONIC3AIR_SOURCES} ${PS4_LIBC16_OBJECTS})
set_target_properties(Sonic3AIR PROPERTIES SUFFIX ".elf")
target_precompile_headers(Sonic3AIR PRIVATE ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/pch.h)
# The C runtime corrections must not pick up the C++ PCH.
set_source_files_properties(${PS4_ORBIS_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

target_link_libraries(Sonic3AIR oxygen ps4-app)

# ⚠ THE MESA LINK LINE IS mesa-ps4's OWN (build-support/orbis/tools/gllinkprobe.sh), NOT A GUESS:
#   * one group, RADV inside it - zink calls the ICD and the ICD's runtime calls back out;
#   * --whole-archive on RADV - Mesa's dispatch tables reference entry points WEAKLY, and a weak
#     undefined does not pull an archive member: without it the table slots are NULL on the console;
#   * --allow-multiple-definition - libgallium carries its own copy of parts of the Vulkan runtime.
# Passed as link OPTIONS (SHELL: keeps the group intact, and CMake never de-duplicates the repeated
# --whole-archive flags there). lld resolves archives independently of their position on the line.
set(PS4_MESA_LINK
	"SHELL:-Wl,--allow-multiple-definition"
	"SHELL:-Wl,--start-group ${ORBIS_MESA_BUILD}/src/egl/libEGL.a ${ORBIS_MESA_BUILD}/src/mesa/glapi/es2api/libGLESv2.a ${ORBIS_MESA_BUILD}/src/mesa/glapi/es1api/libGLESv1_CM.a ${PS4_MESA_GALLIUM} -Wl,--whole-archive ${PS4_MESA_RADV} -Wl,--no-whole-archive ${PS4_MESA_ZLIB} -Wl,--end-group")
target_link_options(Sonic3AIR PRIVATE ${PS4_MESA_LINK})
# Records which archive member was extracted for which symbol; check-wchar-link.sh reads it (POST_BUILD).
set(PS4_WHY_EXTRACT "${CMAKE_BINARY_DIR}/Sonic3AIR.why-extract.txt")
target_link_options(Sonic3AIR PRIVATE "-Wl,--why-extract=${PS4_WHY_EXTRACT}")

target_link_libraries(Sonic3AIR
	-lSceGnmDriver -lSceVideoOut
	-lScePad -lSceUserService -lSceAudioOut
	-lSceSysmodule -lSceSystemService -lSceNet)


# ---------------------------------------------------------------- eboot + pkg

# ⚠ create-fself EXITS 0 WHEN IT FAILS. So the previous eboot.bin is deleted first and its existence
# checked afterwards; otherwise the build would report success and the pkg step would package a stale
# eboot (or fail later with a less useful message). POST_BUILD commands run in the order added.
set(PS4_EBOOT "${CMAKE_BINARY_DIR}/sonic3air/eboot.bin")		# == $<TARGET_FILE_DIR:Sonic3AIR>/eboot.bin
add_custom_command(TARGET Sonic3AIR POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:Sonic3AIR>/eboot.bin"
	VERBATIM)
# Before the eboot exists: a 16-bit wchar_t libc member in the ELF must not become a package.
add_custom_command(TARGET Sonic3AIR POST_BUILD
	COMMAND /bin/sh "${PS4_DIR}/check-wchar-link.sh" "${PS4_WHY_EXTRACT}"
	VERBATIM
	COMMENT "checking that no 16-bit wchar_t libc.a member was linked")
ps4_create_eboot(Sonic3AIR)
# BYPRODUCTS: tells the generator that the link step produces eboot.bin, so the pkg command below can
# depend on the FILE (Ninja otherwise has no rule for it).
add_custom_command(TARGET Sonic3AIR POST_BUILD
	COMMAND /bin/sh -c "test -s \"$1\" || { echo \"!! create-fself produced no eboot.bin at $1\" >&2; exit 1; }" sh "$<TARGET_FILE_DIR:Sonic3AIR>/eboot.bin"
	BYPRODUCTS "${PS4_EBOOT}" "${CMAKE_BINARY_DIR}/sonic3air/Sonic3AIR.oelf"
	VERBATIM
	COMMENT "checking eboot.bin")

# What goes into /app0 besides eboot.bin. The ROM never does (check-pkg-no-rom.sh enforces it).
#
#   PS4_DATA_DIR set        packed release data from a host `sonic3air_linux -pack` (+ scripts.bin)
#   PS4_DATA_DIR empty and  the UNPACKED data straight from this repository: Oxygen/sonic3air/data,
#   PS4_PKG_UNPACKED_DATA   Oxygen/oxygenengine/data (merged into /app0/data, like the dev setup mounts
#   (default ON)            it) and the script sources Oxygen/sonic3air/scripts -> /app0/scripts. The
#                           engine accepts missing packages when data/images/icon.png exists unpacked,
#                           and compiles the scripts from source when data/scripts.bin is missing.
#                           No host build needed - this is what makes [KONSOLA T1] testable today.
#
# The remastered soundtrack (~126 MB) is NOT packaged by default, in either mode. The engine also finds
# audioremaster.bin at /data/sonic3air/audioremaster.bin (user upload via FTP, see EngineMain.cpp).
#   PS4_PKG_REMASTERED_AUDIO=ON  packed mode: PS4_REMASTER_BIN (default ${PS4_DATA_DIR}/audioremaster.bin)
#                                -> /app0/data/audioremaster.bin; unpacked mode: data/audio/remastered.
option(PS4_PKG_UNPACKED_DATA     "pkg: ship the repository's unpacked data + script sources when PS4_DATA_DIR is empty" ON)
option(PS4_PKG_REMASTERED_AUDIO  "pkg: also ship the remastered soundtrack (audioremaster.bin / data/audio/remastered, ~126 MB)" OFF)
set(PS4_REMASTER_BIN "" CACHE FILEPATH "audioremaster.bin to package with PS4_PKG_REMASTERED_AUDIO (empty: PS4_DATA_DIR/audioremaster.bin)")

set(PS4_PKG_MANIFEST_LINES "${WORKSPACE_DIR}/Oxygen/sonic3air/_master_image_template/config.json:config.json")
if (PS4_DATA_DIR)
	foreach(_f enginedata.bin gamedata.bin audiodata.bin metadata.json scripts.bin)
		if (EXISTS "${PS4_DATA_DIR}/${_f}")
			list(APPEND PS4_PKG_MANIFEST_LINES "${PS4_DATA_DIR}/${_f}:data/${_f}")
			message(STATUS "pkg: data/${_f}")
		else()
			message(STATUS "pkg: data/${_f} NOT FOUND in ${PS4_DATA_DIR}")
		endif()
	endforeach()
	if (PS4_PKG_REMASTERED_AUDIO)
		set(_remaster "${PS4_REMASTER_BIN}")
		if (NOT _remaster)
			set(_remaster "${PS4_DATA_DIR}/audioremaster.bin")
		endif()
		if (NOT EXISTS "${_remaster}")
			message(FATAL_ERROR "PS4_PKG_REMASTERED_AUDIO=ON, but there is no ${_remaster} (set PS4_REMASTER_BIN)")
		endif()
		list(APPEND PS4_PKG_MANIFEST_LINES "${_remaster}:data/audioremaster.bin")
		message(STATUS "pkg: data/audioremaster.bin <- ${_remaster}")
	else()
		message(STATUS "pkg: data/audioremaster.bin not packaged (upload it to /data/sonic3air/ or use PS4_PKG_REMASTERED_AUDIO)")
	endif()
elseif (PS4_PKG_UNPACKED_DATA)
	set(_count 0)
	foreach(_pair "Oxygen/oxygenengine/data:data" "Oxygen/sonic3air/data:data" "Oxygen/sonic3air/scripts:scripts")
		string(REPLACE ":" ";" _pair "${_pair}")
		list(GET _pair 0 _srcrel)
		list(GET _pair 1 _targ)
		file(GLOB_RECURSE _files CONFIGURE_DEPENDS RELATIVE "${WORKSPACE_DIR}/${_srcrel}" "${WORKSPACE_DIR}/${_srcrel}/*")
		foreach(_f IN LISTS _files)
			if (NOT PS4_PKG_REMASTERED_AUDIO AND _f MATCHES "^audio/remastered/")
				continue()
			endif()
			if (_f MATCHES "(^|/)(\\.|_reference/)")		# dot files, script reference dumps
				continue()
			endif()
			list(APPEND PS4_PKG_MANIFEST_LINES "${WORKSPACE_DIR}/${_srcrel}/${_f}:${_targ}/${_f}")
			math(EXPR _count "${_count} + 1")
		endforeach()
	endforeach()
	message(STATUS "pkg: ${_count} unpacked data/script files from the repository (remastered audio: ${PS4_PKG_REMASTERED_AUDIO})")
endif()

# Every packaged source file is a dependency of the pkg step: touching one rebuilds only the pkg.
set(PS4_PKG_EXTRA_SOURCES)
foreach(_line IN LISTS PS4_PKG_MANIFEST_LINES)
	string(REGEX REPLACE ":[^:]*$" "" _src "${_line}")
	list(APPEND PS4_PKG_EXTRA_SOURCES "${_src}")
endforeach()

# Compare-and-write: the manifest's timestamp only moves when its contents change, so a re-configure
# alone does not rebuild the pkg, but a different --data / remaster choice does.
string(REPLACE ";" "\n" _manifest "${PS4_PKG_MANIFEST_LINES}")
set(PS4_PKG_MANIFEST "${CMAKE_BINARY_DIR}/pkg-assets/extra-files.txt")
file(WRITE "${PS4_PKG_MANIFEST}.tmp" "${_manifest}\n")
file(COPY_FILE "${PS4_PKG_MANIFEST}.tmp" "${PS4_PKG_MANIFEST}" ONLY_IF_DIFFERENT)
file(REMOVE "${PS4_PKG_MANIFEST}.tmp")

# The same steps ps4_create_pkg() (orbis-compat cmake/ps4-package.cmake) adds, but as a real custom
# command + target (Sonic3AIR_pkg) instead of a POST_BUILD step of the executable: the pkg is rebuilt
# when eboot.bin, the manifest, any packaged file or the icon changes - and only then. It goes through
# make-pkg-manifest.sh so the file list does not have to fit into one shell argument.
if (PS4_BUILD_PKG)
	find_program(PS4_PKGTOOL NAMES PkgTool.Core PATHS "${OO_PS4_BINDIR}" NO_DEFAULT_PATH)
	if (NOT PS4_PKGTOOL)
		message(WARNING "PkgTool.Core not found in ${OO_PS4_BINDIR}; no .pkg, the ELF and eboot.bin still build.")
	else()
		# icon0.png: the game's own 512x512 app icon, flattened onto white because the console wants it opaque.
		# Falls back to the generated placeholder when ImageMagick is missing or the conversion fails.
		set(PS4_ICON_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../___internal/icon/app-no-border.png" CACHE FILEPATH "Source image for sce_sys/icon0.png")
		set(_icon "${CMAKE_BINARY_DIR}/pkg-assets/icon0.png")
		find_program(PS4_MAGICK NAMES magick)
		set(_icon_depends)
		if (PS4_MAGICK AND EXISTS "${PS4_ICON_SOURCE}")
			set(_icon_depends "${PS4_ICON_SOURCE}")
		else()
			set(PS4_MAGICK "false")	# straight to the placeholder
			message(WARNING "icon0.png: no ImageMagick or no ${PS4_ICON_SOURCE}, using the generated placeholder")
		endif()
		add_custom_command(
			OUTPUT "${_icon}"
			COMMAND /bin/sh -c "\"$1\" \"$2\" -resize 512x512! -background white -alpha remove -alpha off -strip \"PNG24:$3\" >/dev/null 2>&1 || { echo \"icon0.png: could not convert $2, using the generated placeholder\" >&2; python3 \"$4\" --out \"$3\" --title-id \"$5\" >/dev/null; }"
			        sh "${PS4_MAGICK}" "${PS4_ICON_SOURCE}" "${_icon}" "${PS4_PACKAGE_SCRIPT_DIR}/gen-icon0.py" "${PS4_TITLE_ID}"
			DEPENDS ${_icon_depends}
			VERBATIM
			COMMENT "pkg: sce_sys/icon0.png")

		# Output name exactly as make-pkg.sh derives it: IV0000-<TITLE_ID>_00-<label: A-Z0-9, 0-padded to 16>
		string(TOUPPER "${PS4_CONTENT_LABEL}" _label)
		string(REGEX REPLACE "[^A-Z0-9]" "" _label "${_label}")
		string(SUBSTRING "${_label}0000000000000000" 0 16 _label)
		set(PS4_PKG_FILE "${CMAKE_BINARY_DIR}/sonic3air/IV0000-${PS4_TITLE_ID}_00-${_label}.pkg")

		add_custom_command(
			OUTPUT "${PS4_PKG_FILE}"
			BYPRODUCTS "${CMAKE_BINARY_DIR}/sonic3air/${PS4_TITLE_ID}.pkg"
			COMMAND ${CMAKE_COMMAND} -E rm -f "${PS4_PKG_FILE}"
			COMMAND "${PS4_DIR}/make-pkg-manifest.sh" "${PS4_PACKAGE_SCRIPT_DIR}/make-pkg.sh" "${PS4_PKG_MANIFEST}"
			        --eboot "${PS4_EBOOT}"
			        --out-dir "${CMAKE_BINARY_DIR}/sonic3air"
			        --title-id "${PS4_TITLE_ID}"
			        --title "${PS4_TITLE}"
			        --version "01.00"
			        --content-label "${PS4_CONTENT_LABEL}"
			        --sdk "${OO_PS4_TOOLCHAIN}"
			        --icon "${_icon}"
			COMMAND /bin/sh -c "test -s \"$1\" || { echo \"!! make-pkg.sh produced no $1\" >&2; exit 1; }" sh "${PS4_PKG_FILE}"
			DEPENDS "${PS4_EBOOT}" Sonic3AIR "${PS4_PKG_MANIFEST}" "${_icon}" ${PS4_PKG_EXTRA_SOURCES}
			        "${PS4_DIR}/make-pkg-manifest.sh" "${PS4_DIR}/check-pkg-no-rom.sh" "${PS4_PACKAGE_SCRIPT_DIR}/make-pkg.sh"
			VERBATIM
			COMMENT "pkg: Sonic3AIR -> IV0000-${PS4_TITLE_ID}_00-${_label}.pkg")
		add_custom_target(Sonic3AIR_pkg ALL DEPENDS "${PS4_PKG_FILE}")
	endif()
endif()
