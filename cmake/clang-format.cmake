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
INCLUDE_GUARD()

find_program(CLANG_FORMAT_PROGRAM NAMES clang-format)
IF (CLANG_FORMAT_PROGRAM)
    EXECUTE_PROCESS(
            COMMAND "${CLANG_FORMAT_PROGRAM}" --version
            OUTPUT_VARIABLE CLANG_FORMAT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    MESSAGE("Using clang-format: ${CLANG_FORMAT_PROGRAM} (${CLANG_FORMAT_VERSION})")

    FILE(GLOB_RECURSE
            format_src_list
            RELATIVE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "src/*.[hc]"
            "src/*.[hc]pp"
    )

    FOREACH (_src_file ${format_src_list})
        MESSAGE("    formatting => ${_src_file}")
        EXECUTE_PROCESS(
                COMMAND "${CLANG_FORMAT_PROGRAM}" --style=file -i "${_src_file}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    ENDFOREACH ()

    UNSET(CLANG_FORMAT_VERSION)
ENDIF ()