## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Vahera Studios LLC and its contributors.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##		http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

# =======================================================================
# Godot Engine submodule update/init
# =======================================================================

# Confirms that the Godot Engine source files exist.
# Assumes that if they don't, the submodule has not yet been initialized.

IF ( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/core" )
    MESSAGE( NOTICE "Godot engine sources not found" )
    MESSAGE( NOTICE "initializing/updating the engine submodule..." )

    # update the engine submodule to populate it with the
    # code necessary to build a debug version of the editor that
    # can be easily debugged along with the library
    EXECUTE_PROCESS(
            COMMAND git submodule update --init extern/godot-engine
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
    )
ENDIF ()

# =======================================================================
# Godot-cpp bindings submodule update/init
# =======================================================================

# Confirms that the Godot CPP extension source files exist.
# Assumes that if they don't, the submodule has not yet been initialized.

IF ( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp/src" )
    MESSAGE( NOTICE "godot-cpp bindings source not found" )
    MESSAGE( NOTICE "initializing/updating the godot-cpp submodule..." )

    # update the c++ bindings submodule to populate it with
    # the necessary source for the library
    EXECUTE_PROCESS(
            COMMAND git submodule update --init extern/godot-cpp
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY
    )
ENDIF ()

# =======================================================================
# Godot editor/engine debug build
# =======================================================================

STRING( TOLOWER "${CMAKE_SYSTEM_NAME}" host_os )
SET( cpu_arch "x86_64" )

# define variable to be used in the engine build when specifying platform.
SET( host_os_engine "${host_os}" )
IF ( APPLE )
    IF ( "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64" )
        SET( cpu_arch "arm64" )
    ENDIF ()
    # ${CMAKE_SYSTEM_NAME} returns Darwin, but the scons platform name will be macos
    SET( host_os_engine "macos" )
ELSEIF ( UNIX )
    # the scons build expects linuxbsd to be passed in as the platform
    # when building on linux, so just append bsd to CMAKE_SYSTEM_NAME
    SET( host_os_engine "${host_os}bsd" )
ENDIF ()


SET( godot_debug_editor_executable
     "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/bin/godot.${host_os_engine}.editor.dev.${cpu_arch}${CMAKE_EXECUTABLE_SUFFIX}"
)

FIND_PROGRAM( SCONS_PROGRAM NAMES scons )
IF ( NOT EXISTS "${SCONS_PROGRAM}" )
    MESSAGE( FATAL_ERROR
             "scons not found, it is required for the godot engine build. "
             "Please install scons and confirm it is in your system PATH."
    )
ENDIF ()

#MESSAGE( NOTICE "godot_debug_editor_executable = ${godot_debug_editor_executable}" )
#
## if the engine/editor executable isn't found in the
## engine's submodule bin folder, invoke the scons build.
#IF ( NOT EXISTS "${godot_debug_editor_executable}" )
#    MESSAGE( STATUS "Godot engine debug binaries not found, invoking debug build of engine..." )
#
#    IF ( WIN32 )
#        SET( SCONS_COMMAND powershell -c )
#    ENDIF ()
#
#    SET( SCONS_COMMAND
#         ${SCONS_COMMAND}
#         ${SCONS_PROGRAM}
#         target=editor
#         use_static_cpp=yes
#         dev_build=yes
#         debug_symbols=yes
#         optimize=none
#         use_lto=no
#    )
#
#    SET( GODOT_ENGINE_CLEAN_BUILD OFF )
#    IF ( GODOT_ENGINE_CLEAN_BUILD MATCHES ON )
#        MESSAGE( STATUS "Invoking scons clean: ${SCONS_COMMAND} --clean" )
#
#        EXECUTE_PROCESS(
#                COMMAND "${SCONS_PROGRAM}" --clean
#                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine"
#                COMMAND_ERROR_IS_FATAL ANY
#        )
#    ENDIF ()
#
#    MESSAGE( STATUS "Invoking scons build: ${SCONS_COMMAND}" )
#    # this build should only ever need to be run once (unless the enging debug binaries
#    # are deleted or you want to change the build configuration/command invoked below).
#    EXECUTE_PROCESS(
#            COMMAND ${SCONS_COMMAND}
#            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine"
#            COMMAND_ERROR_IS_FATAL ANY
#    )
#
#    # not necessary, the temp file in here just confuses Visual Studio
#    FILE( REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}}/extern/godot-engine/.sconf_temp" )
#
#    IF ( NOT EXISTS "${godot_debug_editor_executable}" )
#        MESSAGE( FATAL_ERROR "Couldn't find godot debug executable after scons build: ${godot_debug_editor_executable}" )
#    ENDIF ()
#ENDIF ()

# =======================================================================
# Godot C++ bindings library setup/configuration
# =======================================================================

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp )

# =======================================================================
# Godot engine library setup/configuration.
# Not necessary, just provides better support in multiple IDEs
# for engine source code browsing, intellisense, and debugging
# =======================================================================

# populate source file list for the godot engine submodule
#FILE( GLOB_RECURSE godot_engine_sources CONFIGURE_DEPENDS
#      "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/*.[hc]"
#      "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/*.[hc]pp"
#)

# add the engine sources as a library so intellisense works in VS and VSCode
# (and any other IDEs that support CMake in a way where the information from
# the CMake build is fed into the IDE for additional context about the code
# when browsing/debugging). even though the engine is being added as a library here,
# the EXCLUDE_FROM_ALL option will prevent it from compiling. This is done
# purely for IDE integration so it's able to properly navigate the engine
# source code using features like "go do definition", or typical tooltips.
#ADD_LIBRARY( godot_engine EXCLUDE_FROM_ALL ${godot_engine_sources} )

# this is just a handful of additional include directories used by the engine.
# this isn't a complete list, I just add them as needed whenever I venture into
# code where the IDE can't find certain header files during engine source browsing.
#TARGET_INCLUDE_DIRECTORIES( godot_engine PUBLIC
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/platform/windows"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/zlib"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/vulkan"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/vulkan/include"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/vulkan/include/vulkan"
#                            "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/drivers/vulkan"
#                            SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/glad"
#                            SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/volk"
#                            SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/zstd"
#                            SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-engine/thirdparty/mbedtls/include"
#)

# define a bunch of the same symbol definitions
# used when by the scons engine build. These build
# flags can different based on the engine's build for
# you system. Update as needed for your setup.
#TARGET_COMPILE_DEFINITIONS( godot_engine PUBLIC
#                            $<$<CONFIG:Debug>:
#                            DEBUG_ENABLED
#                            DEBUG_METHODS_ENABLED
#                            DEV_ENABLED
#                            >
#                            $<$<BOOL:UNIX>:
#                            UNIX_ENABLED
#                            VK_USE_PLATFORM_XLIB_KHR
#                            >
#                            $<$<BOOL:WIN32>:
#                            WINDOWS_ENABLED
#                            WASAPI_ENABLED
#                            WINMIDI_ENABLED
#                            TYPED_METHOD_BIND
#                            NOMINMAX
#                            WIN32
#                            VK_USE_PLATFORM_WIN32_KHR
#                            _SCRT_STARTUP_WINMAIN=1
#                            $<$<BOOL:MSVC>:
#                            MSVC
#                            >
#                            >
#                            TOOLS_ENABLED
#                            NO_EDITOR_SPLASH
#                            GLAD_ENABLED
#                            GLES3_ENABLED
#                            GLES_OVER_GL
#                            VULKAN_ENABLED
#                            USE_VOLK
#                            MINIZIP_ENABLED
#                            BROTLI_ENABLED
#                            ZSTD_STATIC_LINKING_ONLY
#)