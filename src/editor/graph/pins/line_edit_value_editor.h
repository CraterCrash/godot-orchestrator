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

#include "editor/graph/pins/pin_value_editor.h"

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/popup_menu.hpp>

/// An implementation of <code>OrchestratorEditorGraphPinValueEditor</code> wrapping a <code>LineEdit</code>.
/// This provides users with the ability to specify a string value, based on an optional list of suggestions.
///
class OrchestratorEditorGraphPinValueEditorLineEdit : public OrchestratorEditorGraphPinValueEditor {
    GDCLASS(OrchestratorEditorGraphPinValueEditorLineEdit, OrchestratorEditorGraphPinValueEditor);

    Ref<OrchestrationGraphPin> _pin;
    LineEdit* _control = nullptr;
    PopupMenu* _popup = nullptr;

    void _focus_entered();
    void _popup_window_input(const Ref<InputEvent>& p_event);
    void _popup_index_pressed(int p_index);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorEditorGraphPinValueEditor Interface
    void configure(const PropertyInfo& p_property) override;
    void set_value(const Variant& p_value) override;
    void set_pin_ref(const Ref<OrchestrationGraphPin>& p_pin) override;
    //~ End OrchestratorEditorGraphPinValueEditor Interface
};
