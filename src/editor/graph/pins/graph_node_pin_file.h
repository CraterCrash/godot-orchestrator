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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_FILE_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_FILE_H

#include "editor/graph/graph_node_pin.h"

/// Forward declarations
namespace godot
{
    class Button;
    class FileDialog;
}

/// A node pin for file selections
///
/// This handler renders a button that shows a file dialog window.  The selected filename will
/// be used as the text of the button.
class OrchestratorGraphNodePinFile : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinFile, OrchestratorGraphNodePin);
    static void _bind_methods();

protected:
    Button* _clear_button;

    OrchestratorGraphNodePinFile() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Get the default text for the button
    String _get_default_text() const;

    /// Dispatched when the clear button is clicked
    /// @param p_button the button control, should not be null
    void _on_clear_file(Button* p_button);

    /// Dispatched when the button is clicked
    /// @param p_button the button control, should not be null
    void _on_show_file_dialog(Button* p_button);

    /// Dispatched when a file is selected
    /// @param p_file_name the selected file
    /// @param p_dialog the file dialog, should not be null
    /// @param p_button the button control, should not be null
    void _on_file_selected(const String& p_file_name, FileDialog* p_dialog, Button* p_button);

    /// Dispatched when the file dialog window is closed or cancelled
    /// @param p_dialog the file dialog, should not be null
    /// @param p_button the button control, should not be null
    void _on_file_canceled(FileDialog* p_dialog, Button* p_button);

public:
    OrchestratorGraphNodePinFile(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif // ORCHESTRATOR_GRAPH_NODE_PIN_FILE_H