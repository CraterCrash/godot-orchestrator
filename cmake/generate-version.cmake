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

FUNCTION( GENERATE_VERSION )
    FILE(READ VERSION version_text)
    STRING(REPLACE "\n" ";" version_lines "${version_text}")
    SET(version_max_length 0)
    FOREACH(line ${version_lines})
        IF(NOT line STREQUAL "")
            STRING(REGEX MATCH "^[^=]+" variable_name "${line}")
            STRING(LENGTH "${variable_name}" variable_name_length)
            IF(version_max_length LESS variable_name_length)
                SET(version_max_length ${variable_name_length})
            ENDIF()
        ENDIF()
    ENDFOREACH()

    FUNCTION(PAD_STRING input length output)
        SET(padded ${input})
        STRING(LENGTH "${padded}" current_length)
        WHILE(${current_length} LESS ${length})
            SET(padded "${padded} ")
            STRING(LENGTH "${padded}" current_length)
        ENDWHILE()
        SET(${output} "${padded}" PARENT_SCOPE)
    ENDFUNCTION()

    FOREACH(line ${version_lines})
        IF (NOT line STREQUAL "")
            STRING(REGEX MATCH "^[^=]+" variable_name "${line}")
            STRING(REPLACE "${variable_name}=" "" variable_value "${line}")
            STRING(REGEX REPLACE "^[ \t\n\r]+" "" variable_value "${variable_value}")
            STRING(TOUPPER "${variable_name}" variable_name_upper)
            STRING(REPLACE " " "" variable_name_upper "${variable_name_upper}")
            PAD_STRING("${variable_name_upper}" ${version_max_length} variable_name_padded)
            STRING(CONFIGURE "${variable_value}" variable_value_configured)
            SET(version_formatted "${version_formatted}#define ${variable_name_padded}\t${variable_value_configured}\n")
            SET("V${variable_name_upper}" ${variable_value_configured})
        ENDIF()
    ENDFOREACH()
    PAD_STRING("VERSION_HASH" ${version_max_length} version_hash)
    SET(version_formatted "${version_formatted}#define ${version_hash}\t\"${GIT_COMMIT_HASH}\"")
    CONFIGURE_FILE(cmake/templates/version.h.in _generated/version.gen.h @ONLY)

    # Pass certain values up to the parent scope
    # These are used to create windows DLL resource file details
    STRING(REPLACE "\"" "" VVERSION_NAME "${VVERSION_NAME}")
    SET("RESOLVED_VERSION" "${VVERSION_MAJOR}.${VVERSION_MINOR}.${VVERSION_MAINTENANCE}" PARENT_SCOPE)
    SET("RESOLVED_VERSION_NAME" "${VVERSION_NAME}" PARENT_SCOPE)
    SET("RESOLVED_VERSION_YEAR" "${VVERSION_YEAR}" PARENT_SCOPE)

ENDFUNCTION()

