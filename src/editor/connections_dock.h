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
#ifndef ORCHESTRATOR_EDITOR_CONNECTIONS_DOCK_H
#define ORCHESTRATOR_EDITOR_CONNECTIONS_DOCK_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/tree_item.hpp>

using namespace godot;

/// A utility node that acts as a mediator for the real Godot <code>ConnectionsDock</code> node.
class OrchestratorEditorConnectionsDock : public Node
{
    GDCLASS(OrchestratorEditorConnectionsDock, Node);

    // Taken from connections_dialog.h
    enum SlotMenuOption {
        SLOT_MENU_EDIT,
        SLOT_MENU_GO_TO_METHOD,
        SLOT_MENU_DISCONNECT
    };

    static OrchestratorEditorConnectionsDock* _singleton;
    Node* _connections_dock = nullptr;
    Node* _scene_tree_editor = nullptr;
    Tree* _connections_tree = nullptr;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _slot_menu_option(int p_option);

    void _go_to_method(TreeItem* p_item);
    void _notify_connections_dock_changed();

public:

    static OrchestratorEditorConnectionsDock* get_singleton() { return _singleton; }

    /// Get a reference to the connections dock node in the editor
    /// @return the <code>ConnectionsDock</code> node
    Node* get_connections_dock() { return _connections_dock; }

    /// Disconnects the slot to the specified script function
    /// @param p_script the script
    /// @param p_method the method name
    bool disconnect_slot(const Ref<Script>& p_script, const StringName& p_method);

    OrchestratorEditorConnectionsDock();
    ~OrchestratorEditorConnectionsDock() override;
};

#endif // ORCHESTRATOR_EDITOR_CONNECTIONS_DOCK_H