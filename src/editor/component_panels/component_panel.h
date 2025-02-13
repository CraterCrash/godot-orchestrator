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
#ifndef ORCHESTRATOR_SCRIPT_COMPONENT_PANEL_H
#define ORCHESTRATOR_SCRIPT_COMPONENT_PANEL_H

#include "script/script.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

/// Forward declarations
namespace godot
{
    class AcceptDialog;
    class Button;
    class ConfirmationDialog;
    class HBoxContainer;
    class Label;
    class PanelContainer;
    class PopupMenu;
    class Tree;
    class TreeItem;
}

/// A component panel for the Orchestrator Script editor.
class OrchestratorScriptComponentPanel : public VBoxContainer
{
    GDCLASS(OrchestratorScriptComponentPanel, VBoxContainer);
    static void _bind_methods();

    void _iterate_tree_item(TreeItem* p_item, const Callable& p_callable);

protected:
    String _title;                            //! Title
    Orchestration* _orchestration;            //! The owning orchestration
    PanelContainer* _panel{ nullptr };        //! Panel container
    HBoxContainer* _panel_hbox{ nullptr };    //! Panel HBox container
    Tree* _tree{ nullptr };                   //! The tree list
    Button* _collapse_button{ nullptr };      //! The collapse button
    Button* _add_button{ nullptr };           //! Add button
    PopupMenu* _context_menu{ nullptr };      //! Context menu
    ConfirmationDialog* _confirm{ nullptr };  //! Confirmation dialog
    AcceptDialog* _notify{ nullptr };         //! Notification dialog
    bool _expanded{ true };                   //! Whether the section is currently expanded
    bool _theme_changing{ false };            //! Whether the theme is being changed
    bool _update_blocked{ false };

    //~ Begin Signal handlers
    void _toggle();
    void _tree_add_item();
    void _tree_item_activated();
    void _tree_item_edited();
    void _tree_item_mouse_selected(const Vector2& p_position, int p_button);
    void _remove_confirmed();
    void _tree_item_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button);
    Variant _tree_drag_data(const Vector2& p_position);
    //~ End Signal handlers

    void _queue_update();

    /// Iterates all tree items, calling the callable
    /// @param p_callback the callback to call for all tree items
    void _iterate_tree_items(const Callable& p_callback);

    /// Disconnect a slot
    /// @param p_item the tree item
    void _disconnect_slot(TreeItem* p_item);

    /// Creates an item in the tree.
    /// @param p_parent the parent
    /// @param p_text the display text
    /// @param p_item_name the item name
    /// @param p_icon_name the icon
    TreeItem* _create_item(TreeItem* p_parent, const String& p_text, const String& p_item_name, const String& p_icon_name);

    /// Get the item's real name
    /// @param p_item the tree item
    /// @return the item's real name
    String _get_tree_item_name(TreeItem* p_item);

    /// Updates the control's theme.
    void _update_theme();

    /// Clears the tree of all items but the root.
    void _clear_tree();

    /// Edit the selected tree item
    void _edit_selected_tree_item();

    /// Updates the state of the collapse button.
    void _update_collapse_button_icon();

    /// Notifies the user of a message.
    /// @param p_message the text to notify
    void _show_notification(const String& p_message);

    /// Shows a common dialog error about invalid identifier names.
    /// @param p_type the type
    /// @param p_supports_friendly_names true to show detail about friendly names
    void _show_invalid_name(const String& p_type, bool p_supports_friendly_names = true);

    /// Presents the user a dialog, confirming the removal of the tree item.
    /// @param p_item the item to be removed, should not be null
    void _confirm_removal(TreeItem* p_item);

    /// Creates a unique name in the tree with the given prefix.
    /// This is useful to guarantee that all new items have a unique name.
    /// @param p_prefix the prefix to use
    /// @return the new unique name
    String _create_unique_name_with_prefix(const String& p_prefix);

    /// Finds the specified child in the tree with text that matches the given name.
    /// @param p_name the text to find and activate in the tree control
    /// @param p_edit true if the item should be edited after activation, false otherwise
    /// @param p_activate whether to trigger item activation
    /// @return true if the item is found and activated, false otherwise
    bool _find_child_and_activate(const String& p_name, bool p_edit = true, bool p_activate = false);

    /// Get the panel's horizontal box control
    /// @return the HBoxContainer child of the top panel, never null
    HBoxContainer* _get_panel_hbox() const { return _panel_hbox; }

    /// Get the prefix used for creating new elements
    /// @return the new unique name prefix
    virtual String _get_unique_name_prefix() const { return "item"; }

    /// Get all the existing names for a given element.
    /// @return packed string array of all existing object names
    virtual PackedStringArray _get_existing_names() const { return {}; }

    /// Get the tooltip text for the panel header.
    /// @return the tooltip text to be shown.
    virtual String _get_tooltip_text() const { return {}; }

    /// Get the confirmation dialog text to show on item removal.
    /// @param p_item the item to be removed
    /// @return the text to show in the dialog
    virtual String _get_remove_confirm_text(TreeItem* p_item) const { return {}; }

    /// Get the item name, used for the add button tooltip
    /// @return the name of what items are called in the tree view.
    virtual String _get_item_name() const { return "item"; }

    /// Populates the context menu with options
    /// @param p_item the tree item to create a context menu for, never null
    /// @return true if the context menu is to be shown, false to not show the popup
    virtual bool _populate_context_menu(TreeItem* p_item) { return false; }

    /// Handles context menu selections.
    /// @param p_id the menu id selected
    virtual void _handle_context_menu(int p_id) { }

    /// Handles adding new items to the tree
    /// @param p_name the item's name to be added
    /// @return true if the item added, false otherwise
    virtual bool _handle_add_new_item(const String& p_name) { return false; }

    /// Handles when an item is activated
    /// @param p_item the activated item, never null
    virtual void _handle_item_activated(TreeItem* p_item) { }

    /// Handles when an item is selected
    virtual void _handle_item_selected() { }

    /// Handles when an item is renamed
    /// @param p_old_name the old name
    /// @param p_new_name the new name
    virtual bool _handle_item_renamed(const String& p_old_name, const String& p_new_name) { return false; }

    /// Handles the removal of the item
    /// @param p_item the tree item to be removed
    virtual void _handle_remove(TreeItem* p_item) { }

    /// Handles button click in the tree
    /// @param p_item the tree item
    /// @param p_column the tree column
    /// @param p_id the button id
    /// @param p_mouse_button the mouse button pressed
    virtual void _handle_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button) { }

    /// Generates a drag-and-drop dictionary data bucket
    /// @param p_position the position where the drag started
    /// @return data to be used for the draggable item, returning an empty Dictionary cancels the drag
    virtual Dictionary _handle_drag_data(const Vector2& p_position) { return {}; }

    /// Called when the tree receives a <code>gui_input</code> event
    /// @param p_event the event received
    void _tree_gui_input(const Ref<InputEvent>& p_event);

    /// Handles <code>gui_input</code> events in the tree widget
    /// @param p_event the event received
    /// @param p_item the current selected item
    virtual void _handle_tree_gui_input(const Ref<InputEvent>& p_event, TreeItem* p_item) { }

    /// Default constructor
    OrchestratorScriptComponentPanel() = default;

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    /// Updates this control, should be called by the script view
    virtual void update();

    /// Find the item by name and edit it (if it exists)
    /// @param p_item_name
    void find_and_edit(const String& p_item_name);

    /// Return whether the component panel is collapsed
    /// @return true if the panel is collapsed, false otherwise
    bool is_collapsed() const { return !_expanded; }

    /// Set whether the panel is collapsed
    /// @param p_collapsed whether the panel is collapsed
    void set_collapsed(bool p_collapsed);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Constructs a component panel
    /// @param p_title the panel title
    /// @param p_orchestration the owning orchestration
    OrchestratorScriptComponentPanel(const String& p_title, Orchestration* p_orchestration);
};

#endif  // ORCHESTRATOR_SCRIPT_COMPONENT_PANEL_H