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
#include "editor/gui/select_type_dialog.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

/// An implementation of <code>OrchestratorEditorGraphPinValueEditor</code> that provides an editor for
/// Variant data type pins, which have Variant::NIL and PROPERTY_USAGE_NIL_IS_VARIANT properties.
///
/// This editor renders a pencil button for type selection and dynamically hosts a type-specific inner
/// editor when an override type is chosen. Value changes from the inner editor are re-emitted as this
/// editor's own value_changed signal.
///
class OrchestratorEditorGraphPinValueEditorAny : public OrchestratorEditorGraphPinValueEditor {
    GDCLASS(OrchestratorEditorGraphPinValueEditorAny, OrchestratorEditorGraphPinValueEditor);

    OrchestratorSelectTypeSearchDialog* _type_dialog = nullptr;
    Button* _type_button = nullptr;
    HBoxContainer* _inner_container = nullptr;
    OrchestratorEditorGraphPinValueEditor* _inner_editor = nullptr;

    Ref<OrchestrationGraphPin> _pin;
    bool _is_input = true;
    Variant::Type _built_for_type = Variant::NIL;
    String _built_for_class;

    void _type_button_pressed();
    void _type_selected();
    void _rebuild_inner_editor(const PropertyInfo& p_override);
    void _on_inner_value_changed(const Variant& p_value);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorEditorGraphPinValueEditor Interface
    bool prefers_leading_placement() const override { return !_is_input; }
    void configure(const PropertyInfo& p_property) override;
    void set_value(const Variant& p_value) override;
    void set_linked(bool p_linked) override;
    void set_pin_ref(const Ref<OrchestrationGraphPin>& p_pin) override;
    //~ End OrchestratorEditorGraphPinValueEditor Interface
};
