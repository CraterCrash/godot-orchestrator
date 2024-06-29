// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "editor/file_dialog.h"

#include "plugins/orchestrator_editor_plugin.h"
#include <godot_cpp/classes/line_edit.hpp>

void OrchestratorFileDialog::_focus_file_text()
{
    if (Node* node = find_child("LineEdit", true, false))
    {
        if (LineEdit* file = Object::cast_to<LineEdit>(node))
        {
            int lp = file->get_text().rfind(".");
            if (lp != -1)
            {
                file->select(0, lp);
                file->grab_focus();
            }
        }
    }
}

void OrchestratorFileDialog::popup_file_dialog()
{
    const float EDSCALE = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_scale();
    popup_centered_clamped(Size2(1050, 700) * EDSCALE, 0.8);
    _focus_file_text();
}

void OrchestratorFileDialog::_bind_methods()
{
}
