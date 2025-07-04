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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_BUTTON_BASE_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_BUTTON_BASE_H

#include "editor/graph/graph_pin.h"

#include <godot_cpp/classes/button.hpp>

/// An abstract implementation of <code>OrchestratorEditorGraphPin</code> that displays a button
/// with text derived from an external source, with a secondary button to reset the value when the
/// value does not equal its default.
///
class OrchestratorEditorGraphPinButtonBase : public OrchestratorEditorGraphPin
{
    GDCLASS(OrchestratorEditorGraphPinButtonBase, OrchestratorEditorGraphPin);

    Button* _clear_button = nullptr;
    Button* _selector_button = nullptr;
    String _default_text;
    Variant _clear_default;
    Variant _button_value;

    void _clear_button_pressed();
    void _selector_button_pressed();

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    void _update_control_value(const Variant& p_value) override;
    Variant _read_control_value() override;
    Control* _create_default_value_widget() override;
    //~ End OrchestratorEditorGraphPin Interface

    void _set_button_visible(bool p_visible) { _selector_button->set_visible(p_visible); }

    Variant _get_button_value() const { return _button_value; }
    const Button* _get_selector_button() const { return _selector_button; }

    void _handle_selector_button_response(const Variant& p_value);
    virtual void _handle_selector_button_pressed() { }

public:
    void set_default_text(const String& p_default_text) { _default_text = p_default_text; }
    void set_clear_button_default_value(const Variant& p_clear_default) { _clear_default = p_clear_default; }

};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_BUTTON_BASE_H