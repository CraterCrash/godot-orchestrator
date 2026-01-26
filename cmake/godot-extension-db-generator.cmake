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

FIND_PACKAGE( Python3 REQUIRED COMPONENTS Interpreter )

FUNCTION( GENERATE_GODOT_EXTENSION_DB )
    SET(EXTENSIONDB_CPP_FILE_BASE "${CMAKE_BINARY_DIR}/_generated")
    SET(EXTENSIONDB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp/gdextension/extension_api.json")
    EXECUTE_PROCESS(
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/generate_godot_api.py ${EXTENSIONDB_FILE} ${EXTENSIONDB_CPP_FILE_BASE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

ENDFUNCTION()