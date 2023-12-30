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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_ACTION_MENU_H
#define ORCHESTRATOR_EDITOR_GRAPH_ACTION_MENU_H

#include "action_menu_filter.h"
#include "action_menu_item.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorGraphEdit;

/// An action menu that provides the user with all available actions that can be performed
/// within an OrchestratorGraphEdit workspace.
class OrchestratorGraphActionMenu : public ConfirmationDialog
{
    GDCLASS(OrchestratorGraphActionMenu, ConfirmationDialog);

    static void _bind_methods();

    Tree* _tree_view{ nullptr };                  //! Results tree view
    LineEdit* _filters_text_box{ nullptr };       //! Filter text box
    CheckBox* _context_sensitive{ nullptr };      //! Context-sensitive check box
    Button* _expand{ nullptr };                   //! Expand button
    Button* _collapse{ nullptr };                 //! Collapse button
    bool _force_refresh{ false };                 //! Forces refresh of action items
    HashMap<String, Ref<Texture2D>> _icon_cache;  //! Cache of icons
    List<Ref<OrchestratorGraphActionMenuItem>> _items;
    OrchestratorGraphActionFilter _filter;

public:

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Applies the specified filter and shows the results
    /// @param p_filter the action filter
    void apply_filter(const OrchestratorGraphActionFilter& p_filter);

private:
    /// Refreshes the action list
    void _refresh_actions();

    /// Populates the action items list
    /// @param p_context the action context
    void _generate_actions(const OrchestratorGraphActionContext& p_context);

    /// Populates the tree with filtered actions
    /// @param p_context the action context
    void _generate_filtered_actions([[maybe_unused]] [[maybe_unused]] const OrchestratorGraphActionContext& p_context);

    /// Removes all empty action nodes from the specified parent item
    /// @param p_parent the parent tree item
    void _remove_empty_action_nodes(TreeItem* p_parent);

    /// Notify of selection and close window
    /// @param p_selected the selected item
    void _notify_and_close(TreeItem* p_selected);

    /// Dispatched when the context sensitive checkbox is toggled
    /// @param p_new_state the new checkbox state
    void _on_context_sensitive_toggled(bool p_new_state);

    /// Dispatched when the filter search box text is changed.
    /// @param p_new_text the new filter text
    void _on_filter_text_changed(const String& p_new_text);

    /// Dispatched when a tree element is clicked
    void _on_tree_item_selected();

    /// Dispatched when a tree element is double-clicked
    void _on_tree_item_activated();

    /// Dispatched when the user selects the cancel or close window button
    void _on_close_requested();

    /// Dispatched when the user selects the OK button
    void _on_confirmed();

    /// Dispatched when the user clicks the collapse tree button
    /// @param p_collapsed the current collapse state
    void _on_collapse_tree(bool p_collapsed);

    /// Dispatched when the user clicks the expand tree button
    /// @param p_expanded the current expand state
    void _on_expand_tree(bool p_expanded);

};

#endif  // ORCHESTRATOR_EDITOR_GRAPH_ACTION_MENU_H
