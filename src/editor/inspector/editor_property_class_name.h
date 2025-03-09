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
#ifndef ORCHESTRATOR_EDITOR_PROPERTY_CLASS_NAME_H
#define ORCHESTRATOR_EDITOR_PROPERTY_CLASS_NAME_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>

using namespace godot;

class OrchestratorSelectClassSearchDialog;

/// An EditorProperty that works identical to Godot's EditorPropertyClassName, with the difference
/// being that we use our own implementation of <code>CreateDialog</code>.
class OrchestratorEditorPropertyClassName : public EditorProperty
{
    GDCLASS(OrchestratorEditorPropertyClassName, EditorProperty);

    OrchestratorSelectClassSearchDialog* _dialog{ nullptr };
    Button* _property{ nullptr };
    String _selected_type;
    String _base_type;

protected:
    //~ Begin Wrapped Interface
    static void _bind_methods();
    void _notification(int p_what);
    //~ End Wrapped Interface

    Variant _get_edited_property_value();

    void _property_selected();
    void _dialog_selected();

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    void _set_read_only(bool p_read_only) override;
    //~ End EditorProperty Interface

    /// Configures the editor
    /// @param p_base_type the base class type
    /// @param p_selected_type the type to be selected when opened
    /// @param p_allow_abstract whether to allow selecting abstract types, defaults to <code>false</code>
    void setup(const String& p_base_type, const String& p_selected_type, bool p_allow_abstract = false);

    OrchestratorEditorPropertyClassName();
};

#endif // ORCHESTRATOR_EDITOR_PROPERTY_CLASS_NAME_H