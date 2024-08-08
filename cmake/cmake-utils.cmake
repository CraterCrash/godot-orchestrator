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

# Creates a custom "graphviz" target that outputs useful information
# about the project's (and sub target) lib deps/linkage relationships
FUNCTION( RUN_ACTIVE_CMAKE_DIAGNOSTICS )
    # enabled with -D DEPENDENCY_DIAGNOSTICS=ON
    IF ( DEPENDENCY_DIAGNOSTICS MATCHES ON )
        # prints a dependency hierarchy for all targets in project
        SET_PROPERTY( GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE ON )
    ENDIF ()

    # enabled with -D GRAPHVIZ_OUTPUT=ON
    IF ( GRAPHVIZ_OUTPUT MATCHES ON )
        # Outputs graphviz dot files and generates png images showing dependency
        # relationships for top level project and all targets it contains.
        # All files will be generated in src/build/graphviz_output by default.
        #
        # Note: png image graph generation requires graphviz to be installed
        INCLUDE( ${CMAKE_SOURCE_DIR}/CMakeGraphVizOptions.cmake )
        ADD_CUSTOM_TARGET( graphviz ALL

                           # TODO: wipe out ${CMAKE_BINARY_DIR}/graphviz_output dir here
                           COMMAND ${CMAKE_COMMAND} "--graphviz=${CMAKE_BINARY_DIR}/graphviz_output/${PROJECT_NAME}.dot" .
                           COMMAND for dot_file in \$$\(find "${CMAKE_BINARY_DIR}/graphviz_output/*.dot*" ! -name \"*.png\" \)\; do echo \"Generating \$\${dot_file}.png\" && dot -Tpng \"\$$dot_file\" -o \"\$$dot_file.png\" \; done;
                           WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
    ENDIF ()
ENDFUNCTION( RUN_ACTIVE_CMAKE_DIAGNOSTICS )

# function to output all CMAKE variables along with their
# values using a case insensitive regex match
#
# examples:
# 1. print all cmake variables:
#    > dump_cmake_variables(".*")
# 2. print all bool cmake variables:
#    > dump_cmake_variables("^boost.*")
FUNCTION( DUMP_CMAKE_VARIABLES )
    GET_CMAKE_PROPERTY( _vars VARIABLES )
    LIST( SORT _vars )

    FOREACH ( _var ${_vars} )
        IF ( ARGV0 )
            UNSET( MATCHED )

            # case insenstitive match
            STRING( TOLOWER "${ARGV0}" ARGV0_lower )
            STRING( TOLOWER "${_var}" _var_lower )

            STRING( REGEX MATCH ${ARGV0_lower} MATCHED ${_var_lower} )

            IF ( NOT MATCHED )
                continue()
            ENDIF ()
        ENDIF ()


        SET( _value ${${_var}} )
        LIST( LENGTH _value _val_list_len )
        IF ( _val_list_len GREATER 1 )
            MESSAGE( DEBUG "    [${_var}] =>" )
            FOREACH ( _val ${_value} )
                MESSAGE( DEBUG "        - ${_val}" )
            ENDFOREACH ()
        ELSE ()
            MESSAGE( DEBUG "    [${_var}] => ${_value}" )
        ENDIF ()
    ENDFOREACH ()
ENDFUNCTION()

# prints a collection of useful C++ project configuration values
FUNCTION( PRINT_PROJECT_VARIABLES )
    MESSAGE( DEBUG "" )
    MESSAGE( DEBUG "DEBUG CMake Cache Variable Dump" )
    MESSAGE( DEBUG "=============================================" )
    MESSAGE( DEBUG "" )
    DUMP_CMAKE_VARIABLES( ".*" )

    MESSAGE( NOTICE "" )
    MESSAGE( NOTICE "Project Configuration Settings: " ${PROJECT_NAME} )
    MESSAGE( NOTICE "=============================================" )
    MESSAGE( NOTICE "" )
    MESSAGE( NOTICE "Build Configuration" )
    MESSAGE( NOTICE "    CMAKE_SYSTEM_PROCESSOR:..................: " ${CMAKE_SYSTEM_PROCESSOR} )
    MESSAGE( NOTICE "    CMAKE_HOST_SYSTEM_NAME:..................: " ${CMAKE_HOST_SYSTEM_NAME} )
    MESSAGE( NOTICE "    CMAKE_BUILD_TYPE:........................: " ${CMAKE_BUILD_TYPE} )
    MESSAGE( NOTICE "    CMAKE_CXX_COMPILER_ARCHITECTURE_ID:......: " ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID} )
    MESSAGE( NOTICE "    CMAKE_CXX_STANDARD:......................: " ${CMAKE_CXX_STANDARD} )
    MESSAGE( NOTICE "    CMAKE_CXX_COMPILER_VERSION:..............: " ${CMAKE_CXX_COMPILER_VERSION} )
    MESSAGE( NOTICE "    CMAKE_CXX_SIZEOF_DATA_PTR:...............: " ${CMAKE_CXX_SIZEOF_DATA_PTR} )
    MESSAGE( NOTICE "    CMAKE_GENERATOR:.........................: " ${CMAKE_GENERATOR} )
    MESSAGE( NOTICE "    CMAKE_VERSION:...........................: " ${CMAKE_VERSION} )
    MESSAGE( NOTICE "    CMAKE_MINIMUM_REQUIRED_VERSION:..........: " ${CMAKE_MINIMUM_REQUIRED_VERSION} )
    MESSAGE( NOTICE "    VCPKG_TARGET_TRIPLET.....................: " ${VCPKG_TARGET_TRIPLET} )
    MESSAGE( NOTICE "    CMAKE_DEBUG_POSTFIX......................: " ${CMAKE_DEBUG_POSTFIX} )
    MESSAGE( NOTICE "    GIT_COMMIT_HASH..........................: " ${GIT_COMMIT_HASH} )
    MESSAGE( NOTICE "" )
    MESSAGE( NOTICE "CMake Paths" )
    MESSAGE( NOTICE "    CMAKE_CURRENT_SOURCE_DIR.................: " ${CMAKE_CURRENT_SOURCE_DIR} )
    MESSAGE( NOTICE "    CMAKE_TOOLCHAIN_FILE:....................: " ${CMAKE_TOOLCHAIN_FILE} )
    MESSAGE( NOTICE "    CMAKE_SOURCE_DIR:........................: " ${CMAKE_SOURCE_DIR} )
    MESSAGE( NOTICE "    CMAKE_COMMAND:...........................: " ${CMAKE_COMMAND} )
    MESSAGE( NOTICE "    CLANG_FORMAT_PROGRAM:....................: " ${CLANG_FORMAT_PROGRAM} )
    MESSAGE( NOTICE "    SCONS_PROGRAM:...........................: " ${SCONS_PROGRAM} )
    MESSAGE( NOTICE "    CMAKE_CXX_COMPILER:......................: " ${CMAKE_CXX_COMPILER} )
    MESSAGE( NOTICE "    CMAKE_LINKER:............................: " ${CMAKE_LINKER} )
    MESSAGE( NOTICE "    CMAKE_BUILD_TOOL:........................: " ${CMAKE_BUILD_TOOL} )
    MESSAGE( NOTICE "    vcpkg_executable:........................: " ${vcpkg_executable} )
    MESSAGE( NOTICE "    godot_debug_editor_executable:...........: " ${godot_debug_editor_executable} )
    MESSAGE( NOTICE "    CMAKE_INSTALL_PREFIX:....................: " ${CMAKE_INSTALL_PREFIX} )
    MESSAGE( NOTICE "    CMAKE_BINARY_DIR:........................: " ${CMAKE_BINARY_DIR} )
    MESSAGE( NOTICE "    GDEXTENSION_LIB_PATH:....................: " ${GDEXTENSION_LIB_PATH} )
    MESSAGE( NOTICE "" )
ENDFUNCTION( PRINT_PROJECT_VARIABLES )