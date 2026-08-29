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
#pragma once

#include <functional>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace FileUtils {
    /// Returns the path to a file in Orchestrator's dedicated editor cache directory, creating
    /// the directory when it does not yet exist.
    /// @param p_file_name the name of the file
    /// @return the absolute path to the file
    String get_editor_cache_file(const String& p_file_name);

    /// Relocates caches written by earlier Orchestrator versions directly into the Godot project
    /// settings directory. The ConfigFile backed caches move into the dedicated editor cache
    /// directory, while the line based favorite and history caches are folded into sections of the
    /// metadata file and their files discarded. This is safe to call on every start-up; it does
    /// nothing once no legacy files remain.
    void migrate_editor_cache_files();

    /// For the specified file, reads each line and calls the specified callback function with the line.
    /// @param p_file the file to read
    /// @param p_callback the callback function to call for each line
    void for_each_line(const Ref<FileAccess>& p_file, const std::function<void(const String&)>& p_callback);
}
