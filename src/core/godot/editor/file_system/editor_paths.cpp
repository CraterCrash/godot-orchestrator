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
#include "core/godot/editor/file_system/editor_paths.h"

#ifdef TOOLS_ENABLED

#include "common/macros.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine.hpp>

String GDE::EditorPaths::get_project_script_templates_dir() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return EI->get_editor_settings()->get_setting("editor/script/templates_search_path");
    }
    return {};
}

#endif // TOOLS_ENABLED