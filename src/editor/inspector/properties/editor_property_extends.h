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

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/line_edit.hpp>

using namespace godot;

class OrchestratorSelectClassSearchDialog;

class OrchestratorEditorPropertyExtends : public EditorProperty {
    GDCLASS(OrchestratorEditorPropertyExtends, EditorProperty);

    OrchestratorSelectClassSearchDialog* _dialog = nullptr;
    Button* _select_class_button = nullptr;
    Button* _select_path_button = nullptr;
    LineEdit* _extends = nullptr;
    bool _allow_path = false;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Signal Handlers
    void _select_extends_class();
    void _select_extends_path();
    void _extends_class_selected();
    void _extends_path_selected(const String& p_path);
    //~ End Signal Handlers

    void _handle_selection(const StringName& p_property, const String& p_value);

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    //~ End EditorProperty Interface

    /// Setup the editor property
    /// @param p_allow_path whether the path option is selectable
    void setup(bool p_allow_path = true);
};
