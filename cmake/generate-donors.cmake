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

INCLUDE(markdown-utils)

FUNCTION( GENERATE_DONORS )
    FILE(STRINGS "DONORS.md" lines ENCODING UTF-8)
    SET(index 0)
    LIST(LENGTH lines max_index)
    WHILE (index LESS max_index)
        LIST(GET lines ${index} line)
        STRING(REGEX MATCH "^## " is_match "${line}")
        IF(is_match)
            IF (line STREQUAL "## Gold donors")
                READ_SECTION_LIST( "${lines}" ${index} ${max_index} gold_donors )
            ELSEIF(line STREQUAL "## Silver donors")
                READ_SECTION_LIST( "${lines}" ${index} ${max_index} silver_donors )
            ELSEIF(line STREQUAL "## Bronze donors")
                READ_SECTION_LIST( "${lines}" ${index} ${max_index} bronze_donors )
            ELSEIF(line STREQUAL "## Supporters")
                READ_SECTION_LIST( "${lines}" ${index} ${max_index} supporters )
            ENDIF()
        ENDIF()
        MATH(EXPR index "${index} + 1")
    ENDWHILE()
    CONFIGURE_FILE(cmake/templates/donors.h.in _generated/donors.gen.h @ONLY)
ENDFUNCTION()