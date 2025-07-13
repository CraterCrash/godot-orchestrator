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
    EXECUTE_PROCESS(
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/generate_godot_extension_db.py ${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp/gdextension/extension_api.json
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE godot_extension_db
    )
    STRING(FIND "${godot_extension_db}" "//##" pos)
    IF(NOT ${pos} EQUAL -1)
        STRING(SUBSTRING "${godot_extension_db}" 0 ${pos} godot_extension_db_hpp)
        MATH(EXPR pos "${pos} + 5")
        STRING(SUBSTRING "${godot_extension_db}" ${pos} -1 godot_extension_db_cpp)
        CONFIGURE_FILE(cmake/templates/extension_db.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/api/extension_db.h @ONLY NEWLINE_STYLE LF)
        CONFIGURE_FILE(cmake/templates/extension_db.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/api/extension_db.cpp @ONLY NEWLINE_STYLE LF)
    ENDIF()
ENDFUNCTION()