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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H

#include "editor/graph/graph_node_pin.h"

namespace godot
{
    class LineEdit;
    class TextEdit;
}

/// An implementation of OrchestratorGraphNodePin for types that want to represent their default values
/// using a string-based text field for data entry.
class OrchestratorGraphNodePinString : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinString, OrchestratorGraphNodePin);

    static void _bind_methods();

protected:
    OrchestratorGraphNodePinString() = default;

    /// Sets the default value on the pin
    /// @param p_value the default value to set
    void _set_default_value(const String& p_value);

    /// Called when the text edit's (multi-line text) text changes
    /// @param p_text_edit the text edit widget
    void _on_text_changed(TextEdit* p_text_edit);

    /// Called when the line edit's text submitted handler.
    /// @param p_value the text value submitted
    /// @param p_line_edit the line edit widget
    void _on_text_submitted(const String& p_value, LineEdit* p_line_edit);

    /// Called when focus is lost on the line edit widget.
    /// @param p_line_edit the line edit widget
    void _on_focus_lost(LineEdit* p_line_edit);

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    bool _render_default_value_below_label() const override;
    //~ End OrchestratorGraphNodePin Interface

public:
    OrchestratorGraphNodePinString(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);

};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H
