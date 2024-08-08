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

FUNCTION( TRIM text out )
    STRING(REGEX REPLACE "^[ \t\n\r]+" "" text "${text}")
    SET(${out} "${text}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION( READ_SECTION_LIST lines index max_index out )
    SET(is_match 0)
    SET(result "")
    MATH(EXPR index "${index} + 1")
    WHILE(index LESS max_index AND NOT is_match)
        LIST(GET lines ${index} line)
        STRING(REGEX MATCH "^## " is_match "${line}")
        IF (is_match STREQUAL "")
            TRIM("${line}" line)
            IF (NOT line STREQUAL "")
                SET(result "${result}\t\"${line}\",\n")
            ENDIF()
        ENDIF()
        MATH(EXPR index "${index} + 1")
    ENDWHILE()
    SET(result "${result}\t0")
    SET(${out} "${result}" PARENT_SCOPE)
ENDFUNCTION()