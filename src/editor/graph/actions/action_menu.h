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

#include "action_db.h"
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
    HashMap<String, Ref<Texture2D>> _icon_cache;  //! Cache of icons
    OrchestratorGraphEdit* _graph_edit;           //! The graph edit
    OrchestratorGraphActionDB _action_db;         //! Action database
    OrchestratorGraphActionFilter _filter;        //! The filter
    String _selection;                            //! Stores current selected item's category

protected:
    OrchestratorGraphActionMenu() = default;

public:

    /// Constructs a graph action menu
    /// @param p_graph_edit the graph edit control, should not be {@code null}
    OrchestratorGraphActionMenu(OrchestratorGraphEdit* p_graph_edit);

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Requests to clear any persisted state maintained by the menu
    void clear();

    /// Applies the specified filter and shows the results
    /// @param p_filter the action filter
    void apply_filter(const OrchestratorGraphActionFilter& p_filter);

private:
    /// Populates the tree with filtered actions
    void _generate_filtered_actions();

    /// Traverse tree
    /// @param p_item the tree item to traverse from
    /// @param r_highest_score the highest score
    /// @return the tree item with the higest score if found, nullptr otherwise
    TreeItem* _traverse_tree(TreeItem* p_item, float& r_highest_score);

    /// Calculate the score for a tree item
    /// @param p_item the item to calculate the score for
    /// @return the calculated score
    float _calculate_score(TreeItem* p_item);

    /// Common functionality for creating action tree items
    /// @param p_parent the parent item, should not be <code>null</code>
    /// @param p_menu_item the menu item to be called, should be valid
    /// @param p_text the text to be added to the menu item
    /// @param p_favorite_icon whether the favorite icon is added to the item
    /// @param p_is_favorite whether the favorite icon should be filled as a favorite
    /// @return the constructed tree item, never <code>null</code>
    TreeItem* _make_item(TreeItem* p_parent, const Ref<OrchestratorGraphActionMenuItem>& p_menu_item, const String& p_text, bool p_favorite_icon = false, bool p_is_favorite = false);

    /// Creates the favorite item's text
    /// @param p_parent the parent item, should not be <code>null</code>
    /// @param p_menu_item the menu item, should be valid
    /// @return the constructed favorite item text
    String _create_favorite_item_text(TreeItem* p_parent, const Ref<OrchestratorGraphActionMenuItem>& p_menu_item);

    /// Removes all empty action nodes from the specified parent item
    /// @param p_parent the parent tree item
    void _remove_empty_action_nodes(TreeItem* p_parent);

    /// Notify of selection and close window
    /// @param p_selected the selected item
    void _notify_and_close(TreeItem* p_selected);

    /// Looks through the tree and applies the category in <code>_selection</code> to the appropriate item.
    /// @param p_item the tree item hierarchy to traverse, should not be <code>null</code>
    /// @return true if the selection was applied, false otherwise
    bool _apply_selection(TreeItem* p_item);

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

    /// Dispatched when clicking anywhere in the tree with no selection
    void _on_tree_nothing_selected();

    /// Dispached when a tree item is collapsed or expanded
    /// @param p_item the item collapsed or expanded
    void _on_tree_item_collapsed(TreeItem* p_item);

    /// Dispatched when the user clicks a button in the tree
    /// @param p_item the current tree item who's button was clicked
    /// @param p_column the column the button is within
    /// @param p_id the button's unique id
    /// @param p_button_index the mouse button used to click the button
    void _on_tree_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_button_index);

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
