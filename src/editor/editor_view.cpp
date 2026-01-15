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
#include "editor/editor_view.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/theme.hpp>

void OrchestratorEditorView::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_editor"), &OrchestratorEditorView::get_editor);

    ADD_SIGNAL(MethodInfo("name_changed"));
    ADD_SIGNAL(MethodInfo("edited_script_changed"));
    ADD_SIGNAL(MethodInfo("request_help", PropertyInfo(Variant::STRING, "topic")));
    ADD_SIGNAL(MethodInfo("request_open_script_at_line", PropertyInfo(Variant::OBJECT, "script"), PropertyInfo(Variant::INT, "node")));
    ADD_SIGNAL(MethodInfo("request_save_history"));
    ADD_SIGNAL(MethodInfo("request_save_previous_state", PropertyInfo(Variant::DICTIONARY, "state")));
    ADD_SIGNAL(MethodInfo("go_to_help", PropertyInfo(Variant::STRING, "what")));
    ADD_SIGNAL(MethodInfo("go_to_method", PropertyInfo(Variant::OBJECT, "script"), PropertyInfo(Variant::STRING, "method")));
    ADD_SIGNAL(MethodInfo("view_layout_restored"));
}

OrchestratorEditorView::~OrchestratorEditorView()
{
}
