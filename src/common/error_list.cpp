// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "common/error_list.h"

#include "core/typedefs.h"

#include <godot_cpp/classes/global_constants.hpp>

namespace godot {
    const char *error_names[] = {
        "OK", // OK
        "Failed", // FAILED
        "Unavailable", // ERR_UNAVAILABLE
        "Unconfigured", // ERR_UNCONFIGURED
        "Unauthorized", // ERR_UNAUTHORIZED
        "Parameter out of range", // ERR_PARAMETER_RANGE_ERROR
        "Out of memory", // ERR_OUT_OF_MEMORY
        "File not found", // ERR_FILE_NOT_FOUND
        "File: Bad drive", // ERR_FILE_BAD_DRIVE
        "File: Bad path", // ERR_FILE_BAD_PATH
        "File: Permission denied", // ERR_FILE_NO_PERMISSION
        "File already in use", // ERR_FILE_ALREADY_IN_USE
        "Can't open file", // ERR_FILE_CANT_OPEN
        "Can't write file", // ERR_FILE_CANT_WRITE
        "Can't read file", // ERR_FILE_CANT_READ
        "File unrecognized", // ERR_FILE_UNRECOGNIZED
        "File corrupt", // ERR_FILE_CORRUPT
        "Missing dependencies for file", // ERR_FILE_MISSING_DEPENDENCIES
        "End of file", // ERR_FILE_EOF
        "Can't open", // ERR_CANT_OPEN
        "Can't create", // ERR_CANT_CREATE
        "Query failed", // ERR_QUERY_FAILED
        "Already in use", // ERR_ALREADY_IN_USE
        "Locked", // ERR_LOCKED
        "Timeout", // ERR_TIMEOUT
        "Can't connect", // ERR_CANT_CONNECT
        "Can't resolve", // ERR_CANT_RESOLVE
        "Connection error", // ERR_CONNECTION_ERROR
        "Can't acquire resource", // ERR_CANT_ACQUIRE_RESOURCE
        "Can't fork", // ERR_CANT_FORK
        "Invalid data", // ERR_INVALID_DATA
        "Invalid parameter", // ERR_INVALID_PARAMETER
        "Already exists", // ERR_ALREADY_EXISTS
        "Does not exist", // ERR_DOES_NOT_EXIST
        "Can't read database", // ERR_DATABASE_CANT_READ
        "Can't write database", // ERR_DATABASE_CANT_WRITE
        "Compilation failed", // ERR_COMPILATION_FAILED
        "Method not found", // ERR_METHOD_NOT_FOUND
        "Link failed", // ERR_LINK_FAILED
        "Script failed", // ERR_SCRIPT_FAILED
        "Cyclic link detected", // ERR_CYCLIC_LINK
        "Invalid declaration", // ERR_INVALID_DECLARATION
        "Duplicate symbol", // ERR_DUPLICATE_SYMBOL
        "Parse error", // ERR_PARSE_ERROR
        "Busy", // ERR_BUSY
        "Skip", // ERR_SKIP
        "Help", // ERR_HELP
        "Bug", // ERR_BUG
        "Printer on fire" // ERR_PRINTER_ON_FIRE
    };

    static_assert(std_size(error_names) == ERR_PRINTER_ON_FIRE + 1);
};