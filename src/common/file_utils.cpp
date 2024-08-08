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
#include "common/file_utils.h"

#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>

namespace FileUtils
{
    Ref<FileAccess> open_project_settings_file(const String& p_file_name, FileAccess::ModeFlags p_flags)
    {
        const EditorPaths* ep = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_paths();
        return FileAccess::open(ep->get_project_settings_dir().path_join(p_file_name), p_flags);
    }

    void for_each_line(const Ref<FileAccess>& p_file, std::function<void(const String&)> p_callback)
    {
        if (p_file.is_valid() && p_file->is_open())
        {
            while (!p_file->eof_reached())
            {
                p_callback(p_file->get_line());
            }
        }
    }
}