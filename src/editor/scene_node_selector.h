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
#ifndef ORCHESTRATOR_SCENE_NODE_SELECTOR_H
#define ORCHESTRATOR_SCENE_NODE_SELECTOR_H

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/tree.hpp>

using namespace godot;

/// Displays a dialog of the current edited scene, allowing the user to select a specific node.
class OrchestratorSceneNodeSelector : public ConfirmationDialog
{
    GDCLASS(OrchestratorSceneNodeSelector, ConfirmationDialog);
    static void _bind_methods();

protected:
    LineEdit* _filter{ nullptr };   //! Filter text input box
    Tree* _tree{ nullptr };         //! Tree of scene nodes
    Node* _selected{ nullptr };     //! The selected node
    bool _show_all_nodes{ false };  //! Whether to show all nodes

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Signal Handlers
    void _close_requested();
    void _confirmed();
    void _filter_changed(const String& p_text);
    void _item_activated();
    void _item_selected();
    //~ End Signal Handlers

    /// Get the current edited scene root node
    /// @return the current edited scene root node, nullptr if no scene is open.
    Node* _get_scene_node() const;

    /// Updates the tree, optionally scrolling to the selected node
    /// @param p_scroll_to_selected whether to scroll to the selected node
    void _update_tree(bool p_scroll_to_selected);

    /// Adds the node and its children to the tree
    /// @param p_node the node to add
    /// @param p_parent the parent tree item to add the node
    void _add_nodes(Node* p_node, TreeItem* p_parent);

    /// Checks whether the item marks all the specified terms.
    /// @param p_parent the tree item
    /// @param p_terms the terms to check
    /// @return true if it matches, false otherwise.
    bool _item_matches_all_terms(TreeItem* p_parent, const PackedStringArray& p_terms);

    /// Updates the tree based on the filter
    /// @param p_parent the item
    /// @param p_scroll_to_selected whether to scroll to the selected node
    /// @return true if successful, false otherwise
    bool _update_filter(TreeItem* p_parent, bool p_scroll_to_selected);

public:
    /// Set the selected node
    /// @param p_selected the selected node
    void set_selected(Node* p_selected) { _selected = p_selected; }

    /// Constructor
    OrchestratorSceneNodeSelector();
};

#endif  // ORCHESTRATOR_SCENE_NODE_SELECTOR_H