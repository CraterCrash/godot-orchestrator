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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H

#include "editor/graph/graph_node_pin.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/script.hpp>

/// Forward declarations
namespace godot
{
    class Button;
    class LineEdit;
    class Node;
    class Tree;
    class TreeItem;
}

/// A confirmation dialog heavily based after the engine's SceneTreeDialog
/// If the editor would expose SceneTreeDialog, it would be nice to use it instead.
class OrchestratorSceneTreeDialog : public ConfirmationDialog
{
    GDCLASS(OrchestratorSceneTreeDialog, ConfirmationDialog);
    static void _bind_methods();

    LineEdit* _filter{ nullptr };   //! The node filter
    Tree* _tree{ nullptr };         //! The node tree
    Node* _script_node{ nullptr };  //! The node that the script is attached
    NodePath _node_path;            //! The current node path for selections
    Ref<Script> _script;            //! The script

    // Callback helpers
    void _on_close_requested();
    void _on_confirmed();
    void _on_filter_changed(const String& p_text);
    void _on_item_activated();
    void _on_item_selected();

    /// Updates the state of the tree after a filter change
    /// @param p_item the node item to update
    /// true if the item is to be kept, false otherwise
    bool _update_tree(TreeItem* p_item);

    /// Populates the tree
    void _populate_tree();

    /// Populates the tree
    /// @param p_parent the parent tree item
    /// @param p_node the node associated with the tree item
    /// @param p_root the scene node root
    void _populate_tree(TreeItem* p_parent, Node* p_node, Node* p_root);

public:
    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Set the node path selection in the dialog
    /// @param p_node_path the node path
    void set_node_path(const NodePath& p_node_path) { _node_path = p_node_path; }

    /// Sets the logical script that is triggering this dialog
    /// @param p_script the script instance
    void set_script(const Ref<Script>& p_script) { _script = p_script; }
};

/// An implementation of OrchestratorGraphNodePin for node-path pin types.
class OrchestratorGraphNodePinNodePath : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinNodePath, OrchestratorGraphNodePin);
    static void _bind_methods();

protected:
    Button* _button{ nullptr };  //! The button widget

    OrchestratorGraphNodePinNodePath() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Dispatched to show the scene tree dialog.
    void _on_show_scene_tree_dialog();

    /// Dispatched when a node is selected in the scene tree dialog.
    /// @param p_node_path the selected node path
    void _on_node_selected(const NodePath& p_node_path);

public:
    OrchestratorGraphNodePinNodePath(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H
