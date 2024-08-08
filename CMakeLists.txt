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
CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

SET(GDEXTENSION_LIB_NAME orchestrator)
SET(GDEXTENSION_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/project/addons/orchestrator")

# This doesn't work for editor plugins
# ADD_COMPILE_DEFINITIONS(HOT_RELOAD_ENABLED)
add_compile_definitions(TOOLS_ENABLED)

OPTION(
        AUTOFORMAT_SRC_ON_CONFIGURE
        "If enabled, clang-format will be used to format all sources in src/ during configuration"
        OFF
)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_EXTENSIONS ON)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_COLOR_DIAGNOSTICS ON)
SET(CMAKE_MESSAGE_LOG_LEVEL STATUS)

FIND_PACKAGE(Git QUIET)
IF(GIT_FOUND)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE()
    SET(GIT_COMMIT_HASH "<Unknown>")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/_generated)

LIST(APPEND CMAKE_MODULE_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp/cmake/"
)

INCLUDE(generate-authors)
INCLUDE(generate-license)
INCLUDE(generate-version)
INCLUDE(generate-donors)
INCLUDE(godot-extension-db-generator)
INCLUDE(godot-docs-generator)

# Generation steps
GENERATE_LICENSE()
GENERATE_VERSION()
GENERATE_AUTHORS()
GENERATE_DONORS()
GENERATE_GODOT_EXTENSION_DB()
GENERATE_GODOT_DOCUMENTATION()

# MacOS universal binary support
IF (APPLE)
    SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for OSX" FORCE)
ENDIF ()

PROJECT("${GDEXTENSION_LIB_NAME}" LANGUAGES C CXX VERSION ${RESOLVED_VERSION})

# Compiler Identification
SET(compiler_is_clang "$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>")
SET(compiler_is_gnu "$<CXX_COMPILER_ID:GNU>")
SET(compiler_is_msvc "$<CXX_COMPILER_ID:MSVC>")

# Configure godot-cpp - a statically linked library to this project.
INCLUDE(godot-dev-configuration)

# Library sources
FILE(GLOB_RECURSE gdext_sources
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]pp"
        # Includes the generated doc data from /doc_classes
        "${CMAKE_BINARY_DIR}/_generated/*.cpp"
)

# GDExtension library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${gdext_sources})

## Import compiler warnings from godot-cpp
INCLUDE(GodotCompilerWarnings)

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
        /MD
        /O2
        >
        >
        $<$<NOT:${compiler_is_msvc}>:
        -g
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
        -fno-omit-frame-pointer
        -O0
        >
        $<$<CONFIG:Release>:
        -O3
        >
        >
)

find_program(ccache_exe ccache)
if (ccache_exe)
    set(CMAKE_VS_GLOBALS
            "TrackFileAccess=false"
            "UseMultiToolTask=true"
            "DebugInformationFormat=OldStyle"
    )
endif()

# Include directories for GDExtension library
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

IF (NOT APPLE)
    # Linker options for the GDExtension library
    TARGET_LINK_OPTIONS(${PROJECT_NAME} PRIVATE
            $<$<NOT:${compiler_is_msvc}>:
            -static-libgcc
            -static-libstdc++
            -Wl,-R,'$$ORIGIN'
            >
    )
ENDIF ()

if (AUTOFORMAT_SRC_ON_CONFIGURE MATCHES ON)
    include(clang-format)
ENDIF ()

# Dependency linking
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        PUBLIC godot::cpp
)

SET(GDEXTENSION_LIB_PREFIX "")
IF (NOT ANDROID)
    IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
        SET(system_bits 64)
    ELSE ()
        SET(system_bits 32)
    ENDIF ()
ELSE ()
    SET(GDEXTENSION_LIB_PREFIX "lib")
    IF (CMAKE_ANDROID_ARCH STREQUAL "arm64")
        SET(system_bits 64)
    ELSE ()
        SET(system_bits 32)
    ENDIF ()
ENDIF ()

STRING(TOLOWER "${PROJECT_NAME}.${CMAKE_SYSTEM_NAME}.${system_bits}.${CMAKE_BUILD_TYPE}" gde_lib_name)

# Generate library resource
IF(WIN32)
    SET(win32_product_name "${RESOLVED_VERSION_NAME}")
    SET(win32_file_version "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    SET(win32_product_version "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    SET(win32_file_description "Godot ${win32_product_name}")
    CONFIGURE_FILE(cmake/windows.rc.in ${CMAKE_CURRENT_BINARY_DIR}/_generated/version.rc @ONLY)
ENDIF()

IF (WIN32)
    LIST(APPEND gdext_sources "${CMAKE_CURRENT_BINARY_DIR}/_generated/version.rc")
ENDIF()

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
        OUTPUT_NAME "${gde_lib_name}"
)

INCLUDE(cmake-utils)
PRINT_PROJECT_VARIABLES()
