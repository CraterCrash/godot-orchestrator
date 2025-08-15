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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_OPTION_PICKER_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_OPTION_PICKER_H

#include "editor/graph/graph_pin.h"

#include <godot_cpp/classes/option_button.hpp>

/// An implementation of <code>OrchestratorEditorGraphPin</code> wrapping a <code>OptionButton</code>
/// that provides selecting a series of predefined options from a list.
///
/// The current design is based on a <code>PackedStringArray</code> and a string-based value. This may need to
/// change in the future with a much more robust and versatile approach.
///
class OrchestratorEditorGraphPinOptionPicker : public OrchestratorEditorGraphPin
{
    GDCLASS(OrchestratorEditorGraphPinOptionPicker, OrchestratorEditorGraphPin);

    OptionButton* _control = nullptr;

    void _option_item_selected(int p_index);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    void _update_control_value(const Variant& p_value) override;
    Variant _read_control_value() override;
    Control* _create_default_value_widget() override;
    //~ End OrchestratorEditorGraphPin Interface

public:

    void add_item(const String& p_item, bool p_selected = false) { add_item(p_item, p_item, p_selected); }
    void add_item(const String& p_item, const Variant& p_value, bool p_selected = false);

    void clear();

    void set_tooltip_text(const String& p_tooltip_text);
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_OPTION_PICKER_H