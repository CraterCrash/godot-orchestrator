## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
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
CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)

SET(GDEXTENSION_LIB_NAME orchestrator)
SET(GDEXTENSION_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/project/addons/orchestrator")

# Configurable options
OPTION(AUTOFORMAT_SRC_ON_CONFIGURE "If enabled, clang-format will be used to format all sources in /src during configuration" OFF)

# Set basic CMAKE properties
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_EXTENSIONS ON)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_COLOR_DIAGNOSTICS ON)
SET(CMAKE_MESSAGE_LOG_LEVEL STATUS)

# Get the current repository Git commit hash
# This is used when creating the data from VERSION to show the commit hash in the about box
FIND_PACKAGE(Git QUIET)
IF (GIT_FOUND)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp"
            OUTPUT_VARIABLE GODOT_CPP_GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
ELSE ()
    # In this case the about box will simply show an unknown git commit
    SET(GIT_COMMIT_HASH "<Unknown>")
    SET(GODOT_CPP_GIT_COMMIT_HASH "<Unknown>")
ENDIF ()

# Setup the CMAKE_MODULE_PATH
LIST(APPEND CMAKE_MODULE_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Include and execute various CMAKE modules
INCLUDE(generate-authors)
INCLUDE(generate-license)
INCLUDE(generate-version)
INCLUDE(generate-donors)
INCLUDE(godot-extension-db-generator)

# Generation steps
GENERATE_AUTHORS()
GENERATE_LICENSE()
GENERATE_VERSION()
GENERATE_DONORS()
GENERATE_GODOT_EXTENSION_DB()

# Configure project
PROJECT("${GDEXTENSION_LIB_NAME}" LANGUAGES C CXX VERSION ${RESOLVED_VERSION})

# Generate library resource
IF (WIN32)
    SET(win32_product_name "${RESOLVED_VERSION_NAME}")
    SET(win32_file_version "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    SET(win32_product_version "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    SET(win32_file_description "Godot ${win32_product_name} Plug-in")
    CONFIGURE_FILE(cmake/templates/windows.rc.in ${CMAKE_CURRENT_BINARY_DIR}/_generated/version.rc @ONLY)
ENDIF ()

# MacOS universal binary support
IF (APPLE)
    SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for OSX" FORCE)
ENDIF ()

# Compiler Identification
SET(compiler_is_clang "$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>")
SET(compiler_is_gnu "$<CXX_COMPILER_ID:GNU>")
SET(compiler_is_msvc "$<CXX_COMPILER_ID:MSVC>")

# Configure godot-cpp - a statically linked library to this project.
INCLUDE(godot-dev-configuration)

# Generate Doc Data
file( GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/doc_classes/*.xml" )
SET(DOC_DATA_CPP_FILE "${CMAKE_BINARY_DIR}/_generated/doc_data.cpp")
generate_doc_source( "${DOC_DATA_CPP_FILE}" "${DOC_XML}" )

# Library sources
FILE(GLOB_RECURSE gdext_sources
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]pp"
        # Includes the generated doc data from /doc_classes
        "${CMAKE_BINARY_DIR}/_generated/*.cpp"
        "${DOC_DATA_CPP_FILE}"
        # Include windows version resource, if exists
        "${CMAKE_CURRENT_BINARY_DIR}/_generated/version.rc")

# GDExtension library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${gdext_sources})

# Setup compiler options for GDExtension Library based on the compiler used
TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PUBLIC
        $<${compiler_is_msvc}:
            /EHsc
            /utf-8
            /Zc:preprocessor
            /wd5054 # operator '|' deprecated between enumerations of different types
            $<$<CONFIG:Debug>:
                /MDd
            >
            $<$<CONFIG:Release>:
                /MT
                /O2
            >
        >
        $<$<NOT:${compiler_is_msvc}>:
            -Wno-unused-value
            $<${compiler_is_gnu}:
                -Wno-attributes
                -Wno-attributes=r1::
            >
            $<${compiler_is_clang}:
                -Wno-ignored-attributes
                -Wno-unknown-attributes
            >
            $<$<CONFIG:Debug>:
                -g
                -fno-omit-frame-pointer
                -O0
            >
            $<$<CONFIG:Release>:
                -O3
            >
        >)

find_program(ccache_exe ccache)
IF (ccache_exe)
    SET(CMAKE_VS_GLOBALS "TrackFileAccess=false" "UseMultiToolTask=true" "DebugInformationFormat=OldStyle")
ENDIF ()

# Add the special "_generated" directory to the include list
# This is where the generator CMAKE module steps creates automated files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/_generated)

# Include directories for GDExtension library
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

IF (NOT APPLE)
    # Linker options for the GDExtension library
    TARGET_LINK_OPTIONS(${PROJECT_NAME} PRIVATE
            $<$<NOT:${compiler_is_msvc}>:
                -static-libgcc
                -static-libstdc++
                -Wl,-R,'$$ORIGIN'
                $<$<CONFIG:Release>:
                    $<$<PLATFORM_ID:Android>:-s>
                >
            >)
ENDIF ()

IF (AUTOFORMAT_SRC_ON_CONFIGURE MATCHES ON)
    include(clang-format)
ENDIF ()

# Dependency linking
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC godot-cpp.editor)

SET(GDEXTENSION_LIB_PREFIX "")

### Get useful properties of the library
get_target_property( GODOT_PLATFORM godot-cpp::editor GODOT_PLATFORM   )
get_target_property( GODOT_TARGET   godot-cpp::editor GODOT_TARGET     )
get_target_property( GODOT_ARCH     godot-cpp::editor GODOT_ARCH       )

# Converts GODOT_ARCH to the old system bits for unchanged file names.
IF (GODOT_ARCH STREQUAL "x86_64" OR GODOT_ARCH STREQUAL "arm64")
    SET(ORCHESTRATOR_ARCH "64")
ELSE ()
    SET(ORCHESTRATOR_ARCH "32")
ENDIF ()

IF (GODOT_PLATFORM STREQUAL "android")
    SET(GDEXTENSION_LIB_PREFIX "lib")
ENDIF ()

SET_TARGET_PROPERTIES(${PROJECT_NAME}
        PROPERTIES
        PREFIX "${GDEXTENSION_LIB_PREFIX}"
        POSITION_INDEPENDENT_CODE ON
        CMAKE_EXPORT_COMPILE_OPTIMIZATION_RELEASE ON
        CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
        ARCHIVE_OUTPUT_DIRECTORY "${GDEXTENSION_LIB_PATH}"
        LIBRARY_OUTPUT_DIRECTORY "${GDEXTENSION_LIB_PATH}"
        RUNTIME_OUTPUT_DIRECTORY "${GDEXTENSION_LIB_PATH}"
        CMAKE_PDB_OUTPUT_DIRECTORY "${GDEXTENSION_LIB_PATH}"
        CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${GDEXTENSION_LIB_PATH}"
        OUTPUT_NAME "${PROJECT_NAME}.${GODOT_PLATFORM}.${ORCHESTRATOR_ARCH}.$<LOWER_CASE:$<CONFIG>>")

INCLUDE(cmake-utils)

PRINT_PROJECT_VARIABLES()
