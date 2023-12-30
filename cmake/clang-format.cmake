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

find_program(CLANG_FORMAT_PROGRAM NAMES clang-format)

if (CLANG_FORMAT_PROGRAM)
    execute_process(
            COMMAND "${CLANG_FORMAT_PROGRAM}" --version
            OUTPUT_VARIABLE CLANG_FORMAT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message("Using clang-format: ${CLANG_FORMAT_PROGRAM} (${CLANG_FORMAT_VERSION})")

    file(GLOB_RECURSE
            format_src_list
            RELATIVE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "src/*.[hc]"
            "src/*.[hc]pp"
    )

    foreach (_src_file ${format_src_list})
        message("    formatting => ${_src_file}")
        execute_process(
                COMMAND "${CLANG_FORMAT_PROGRAM}" --style=file -i "${_src_file}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endforeach ()

    unset(CLANG_FORMAT_VERSION)
endif ()