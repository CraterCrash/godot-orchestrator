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
#ifndef ORCHESTRATOR_EDITOR_PROPERTY_EXTENDS_H
#define ORCHESTRATOR_EDITOR_PROPERTY_EXTENDS_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/line_edit.hpp>

using namespace godot;

class OrchestratorEditorPropertyExtends : public EditorProperty {
    GDCLASS(OrchestratorEditorPropertyExtends, EditorProperty);

    Button* _select_class_button = nullptr;
    Button* _select_path_button = nullptr;
    LineEdit* _extends = nullptr;
    String _base_type;
    String _selected_value;
    bool _allow_path = false;
    Control* _editor_property_class = nullptr;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Signal Handlers
    void _select_extends_class();
    void _select_extends_path();
    void _extends_path_selected(const String& p_path);
    //~ End Signal Handlers

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    //~ End EditorProperty Interface

    String get_selected_value() const { return _selected_value; }

    /// Setup the editor property
    /// @param p_base_type the base type
    /// @param p_allow_path whether the path option is selectable
    void setup(const String& p_base_type, bool p_allow_path = true);
};

#endif