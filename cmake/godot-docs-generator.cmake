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

FUNCTION( GENERATE_GODOT_DOCUMENTATION )
    # Grab all documentation XML files
    FILE(GLOB XML_FILES "${CMAKE_CURRENT_SOURCE_DIR}/doc_classes/*.xml")
    STRING(JOIN "," XML_FILES_STR ${XML_FILES})
    # Generate the target file
    SET(DOC_DATA_CPP_FILE "${CMAKE_BINARY_DIR}/_generated/doc_data.cpp")
    STRING(JOIN "," DOC_DATA_CPP_STR ${DOC_DATA_CPP_FILE})
    # Run python to generate the doc_data.cpp file
    EXECUTE_PROCESS(
            COMMAND cmd /c py ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_godot_docs.py ${DOC_DATA_CPP_STR} ${XML_FILES_STR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
ENDFUNCTION()