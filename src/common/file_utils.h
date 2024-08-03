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
#ifndef ORCHESTRATOR_FILE_UTILS_H
#define ORCHESTRATOR_FILE_UTILS_H

#include <functional>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace FileUtils
{
    /// Opens a file in the Godot project's <code>.godot</code> directory.
    /// @param p_file_name the name of the file
    /// @param p_flags the mode flags to use when opening the file
    /// @return a reference to the file access object or an invalid reference if the file could not be opened
    Ref<FileAccess> open_project_settings_file(const String& p_file_name, FileAccess::ModeFlags p_flags);

    /// For the specified file, reads each line and calls the specified callback function with the line.
    /// @param p_file the file to read
    /// @param p_callback the callback function to call for each line
    void for_each_line(const Ref<FileAccess>& p_file, const std::function<void(const String&)>& p_callback);
}

#endif // ORCHESTRATOR_FILE_UTILS_H