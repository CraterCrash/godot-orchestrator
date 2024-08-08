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
#ifndef ORCHESTRATOR_SCRIPT_CONNECTIONS_H
#define ORCHESTRATOR_SCRIPT_CONNECTIONS_H

#include <godot_cpp/classes/accept_dialog.hpp>

using namespace godot;

/// Foward declarations
namespace godot
{
    class Tree;
}

/// A dialog that displays the script's active connections.
class OrchestratorScriptConnectionsDialog : public AcceptDialog
{
    GDCLASS(OrchestratorScriptConnectionsDialog, AcceptDialog);

    static void _bind_methods() { }

    Label* _method{ nullptr };
    Tree* _tree{ nullptr };

protected:

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Dispatched when the user confirmed the dialog
    void _on_confirmed();

public:
    OrchestratorScriptConnectionsDialog() = default;

    /// Popup and open the dialog
    /// @param p_method the method name
    /// @param p_nodes the nodes associated with the script
    void popup_connections(const String& p_method, const Vector<Node*>& p_nodes);
};

#endif // ORCHESTRATOR_SCRIPT_CONNECTIONS_H