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
#ifndef ORCHESTRATOR_SCRIPT_VIEW_H
#define ORCHESTRATOR_SCRIPT_VIEW_H

#include "script/script.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

/// Forward declarations
class OrchestratorGraphEdit;
class OrchestratorPlugin;

/// Represents a component section
class OrchestratorScriptViewSection : public VBoxContainer
{
    GDCLASS(OrchestratorScriptViewSection, VBoxContainer);

protected:
    String _section_name;                     //! Name of the section
    PanelContainer* _panel{ nullptr };        //! Panel container
    HBoxContainer* _panel_hbox{ nullptr };    //! Panel HBox container
    Tree* _tree{ nullptr };                   //! The tree list
    Button* _collapse_button{ nullptr };      //! The collapse button
    PopupMenu* _context_menu{ nullptr };      //! Context menu
    ConfirmationDialog* _confirm{ nullptr };  //! Confirmation dialog
    AcceptDialog* _notify{ nullptr };         //! Notification dialog
    bool _expanded{ true };                   //! Whether the section is currently expanded
    bool _theme_changing{ false };            //! Whether the theme is being changed

    static void _bind_methods() {}

    /// Clears the tree of all items but the root.
    void _clear_tree();

    /// Updates the state of the collapse button.
    void _update_collapse_button_icon();

    /// Toggles the visbility of the tree control.
    void _toggle();

    /// Notifies the user of a message.
    /// @param p_message the text to notify
    void _show_notification(const String& p_message);

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
    /// @return true if the item is found and activated, false otherwise
    bool _find_child_and_activate(const String& p_name, bool p_edit = true);

    /// Get the panel's horizontal box control
    /// @return the HBoxContainer child of the top panel, never null
    HBoxContainer* _get_panel_hbox();

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

    /// Get the section item name, used for the add button tooltip
    /// @return the name of what items are called in the tree view.
    virtual String _get_section_item_name() const { return "item"; }

    /// Populates the context menu with options
    /// @param p_item the tree item to create a context menu for, never null
    /// @return true if the context menu is to be shown, false to not show the popup
    virtual bool _populate_context_menu(TreeItem* p_item) { return false; }

    /// Handles context menu selections.
    /// @param p_id the menu id selected
    virtual void _handle_context_menu(int p_id) { }

    /// Handles adding new items to the tree
    virtual void _handle_add_new_item() { }

    /// Handles when an item is activated
    /// @param p_item the activated item, never null
    virtual void _handle_item_activated(TreeItem* p_item) { }

    /// Handles when an item is selected
    virtual void _handle_item_selected() { }

    /// Handles when an item is renamed
    /// @param p_old_name the old name
    /// @param p_new_name the new name
    virtual void _handle_item_renamed(const String& p_old_name, const String& p_new_name) { }

    /// Handles the removal of the item
    /// @param p_item the tree item to be removed
    virtual void _handle_remove(TreeItem* p_item) { }

    /// Generates a drag-and-drop dictionary data bucket
    /// @param p_position the position where the drag started
    /// @return data to be used for the draggable item, returning an empty Dictionary cancels the drag
    virtual Dictionary _handle_drag_data(const Vector2& p_position) { return {}; }

    OrchestratorScriptViewSection() = default;

public:
    /// Constructs a view section
    /// @param p_section_name the name of the section
    OrchestratorScriptViewSection(const String& p_section_name);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Handles Godot GUI input
    /// @param p_event the input event
    void _gui_input(const Ref<InputEvent>& p_event) override;

    /// Updates this control, should be called by the script view
    virtual void update();

private:

    /// Updates the control's theme.
    void _update_theme();

    /// Dispatched when the add button is pressed
    void _on_add_pressed();

    /// Dispatched when a tree item is activated
    void _on_item_activated();

    /// Dispatched when a tree item is edited
    void _on_item_edited();

    /// Dispatched when a tree item is selected
    void _on_item_selected();

    /// Dispatched when a mouse selection is made
    /// @param p_position the mouse position
    /// @param p_button the mouse button used
    void _on_item_mouse_selected(const Vector2& p_position, int p_button);

    /// Dispatched when the item's collapse state changes.
    /// @param p_item the tree item, should not be null
    void _on_item_collapsed(TreeItem* p_item);

    /// Dispatched when a context menu item is pressed
    /// @param p_id the menu item's id that was pressed
    void _on_menu_id_pressed(int p_id);

    /// Dispatched when the user accepts the removal confirmation dialog.
    void _on_remove_confirmed();

    /// Dispatched when drag data is requested for the tree control.
    /// @param p_position the position where the drag started
    Variant _on_tree_drag_data(const Vector2& p_position);
};

/// Represents a component section for event graphs
class OrchestratorScriptViewGraphsSection : public OrchestratorScriptViewSection
{
    GDCLASS(OrchestratorScriptViewGraphsSection, OrchestratorScriptViewSection);

    enum ContextMenuIds
    {
        CM_OPEN_GRAPH,
        CM_RENAME_GRAPH,
        CM_REMOVE_GRAPH,
        CM_FOCUS_FUNCTION,
        CM_REMOVE_FUNCTION
    };

protected:
    Ref<OScript> _script;

    static void _bind_methods();

    /// Notifies the script view to show the specified graph associated with the tree item
    /// @param p_item the graph tree item, should not be null
    void _show_graph_item(TreeItem* p_item);

    /// Notifies the script view to focus a specific event node in the graph.
    /// @param p_item the graph event item, should not be null
    void _focus_graph_function(TreeItem* p_item);

    /// Notifies the script view that a graph has been removed.
    /// This is useful to close any views that pertain to the graph.
    /// @param p_item the graph item, should not be null
    void _remove_graph(TreeItem* p_item);

    /// Notifies the script view that a graph function was removed.
    /// @param p_item the graph function item, should not be null
    void _remove_graph_function(TreeItem* p_item);

    //~ Begin OrchestratorScriptViewSection Interface
    PackedStringArray _get_existing_names() const override;
    String _get_tooltip_text() const override;
    String _get_remove_confirm_text(TreeItem* p_item) const override;
    String _get_section_item_name() const override { return "EventGraph"; }
    bool _populate_context_menu(TreeItem* p_item) override;
    void _handle_context_menu(int p_id) override;
    void _handle_add_new_item() override;
    void _handle_item_activated(TreeItem* p_item) override;
    void _handle_item_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_remove(TreeItem* p_item) override;
    //~ End OrchestratorScriptViewSection Interface

    OrchestratorScriptViewGraphsSection() = default;

public:
    OrchestratorScriptViewGraphsSection(const Ref<OScript>& p_script);

    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface
};

/// Represents a component section for functions
class OrchestratorScriptViewFunctionsSection : public OrchestratorScriptViewSection
{
    GDCLASS(OrchestratorScriptViewFunctionsSection, OrchestratorScriptViewSection);

    enum ContextMenuIds
    {
        CM_OPEN_FUNCTION_GRAPH,
        CM_RENAME_FUNCTION,
        CM_REMOVE_FUNCTION
    };

protected:
    Ref<OScript> _script;

    static void _bind_methods();

    /// Dispatched when the user requests to override a virtual function
    void _on_override_virtual_function();

    /// Notifies the script view to show the function graph and focus the entry node.
    /// @param p_item the function graph tree item, should not be null
    void _show_function_graph(TreeItem* p_item);

    //~ Begin OrchestratorScriptViewSection Interface
    PackedStringArray _get_existing_names() const override;
    String _get_tooltip_text() const override;
    String _get_remove_confirm_text(TreeItem* p_item) const override;
    String _get_section_item_name() const override { return "Function"; }
    bool _populate_context_menu(TreeItem* p_item) override;
    void _handle_context_menu(int p_id) override;
    void _handle_add_new_item() override;
    void _handle_item_activated(TreeItem* p_item) override;
    void _handle_item_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_remove(TreeItem* p_item) override;
    Dictionary _handle_drag_data(const Vector2& p_position) override;
    //~ End OrchestratorScriptViewSection Interface

    OrchestratorScriptViewFunctionsSection() = default;

public:
    OrchestratorScriptViewFunctionsSection(const Ref<OScript>& p_script);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface
};

/// Represents a component section for macros
class OrchestratorScriptViewMacrosSection : public OrchestratorScriptViewSection
{
    GDCLASS(OrchestratorScriptViewMacrosSection, OrchestratorScriptViewSection);

protected:
    Ref<OScript> _script;

    static void _bind_methods() { }

    //~ Begin OrchestratorScriptViewSection Interface
    String _get_tooltip_text() const override;
    String _get_section_item_name() const override { return "Macro"; }
    //~ End OrchestratorScriptViewSection Interface

    OrchestratorScriptViewMacrosSection() = default;

public:
    OrchestratorScriptViewMacrosSection(const Ref<OScript>& p_script);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface
};

/// Represents a component section for variables
class OrchestratorScriptViewVariablesSection : public OrchestratorScriptViewSection
{
    GDCLASS(OrchestratorScriptViewVariablesSection, OrchestratorScriptViewSection);

    enum ContextMenuIds
    {
        CM_RENAME_VARIABLE,
        CM_REMOVE_VARIABLE
    };

protected:
    Ref<OScript> _script;

    static void _bind_methods() { }

    /// Dispatched when the variable resource is changed
    void _on_variable_changed();

    void _create_item(TreeItem* p_parent, const Ref<OScriptVariable>& p_variable);

    //~ Begin OrchestratorScriptViewSection Interface
    PackedStringArray _get_existing_names() const override;
    String _get_tooltip_text() const override;
    String _get_remove_confirm_text(TreeItem* p_item) const override;
    String _get_section_item_name() const override { return "Variable"; }
    bool _populate_context_menu(TreeItem* p_item) override;
    void _handle_context_menu(int p_id) override;
    void _handle_add_new_item() override;
    void _handle_item_selected() override;
    void _handle_item_activated(TreeItem* p_item) override;
    void _handle_item_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_remove(TreeItem* p_item) override;
    Dictionary _handle_drag_data(const Vector2& p_position) override;
    //~ End OrchestratorScriptViewSection Interface

    OrchestratorScriptViewVariablesSection() = default;

public:
    OrchestratorScriptViewVariablesSection(const Ref<OScript>& p_script);

    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface
};

/// Represents a component section for signals
class OrchestratorScriptViewSignalsSection : public OrchestratorScriptViewSection
{
    GDCLASS(OrchestratorScriptViewSignalsSection, OrchestratorScriptViewSection);

    enum ContextMenuIds
    {
        CM_RENAME_SIGNAL,
        CM_REMOVE_SIGNAL
    };

protected:
    Ref<OScript> _script;

    static void _bind_methods() { }

    //~ Begin OrchestratorScriptViewSection Interface
    PackedStringArray _get_existing_names() const override;
    String _get_tooltip_text() const override;
    String _get_remove_confirm_text(TreeItem* p_item) const override;
    String _get_section_item_name() const override { return "Signal"; }
    bool _populate_context_menu(TreeItem* p_item) override;
    void _handle_context_menu(int p_id) override;
    void _handle_add_new_item() override;
    void _handle_item_selected() override;
    void _handle_item_activated(TreeItem* p_item) override;
    void _handle_item_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_remove(TreeItem* p_item) override;
    Dictionary _handle_drag_data(const Vector2& p_position) override;
    //~ End OrchestratorScriptViewSection Interface

    OrchestratorScriptViewSignalsSection() = default;

public:
    OrchestratorScriptViewSignalsSection(const Ref<OScript>& p_script);

    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface
};

/// Main Orchestrator Script View
class OrchestratorScriptView : public HSplitContainer
{
    GDCLASS(OrchestratorScriptView, HSplitContainer);

private:
    Ref<OScript> _script;                                //! The orchestrator script
    TabContainer* _tabs{ nullptr };                      //! The tab container
    OrchestratorGraphEdit* _event_graph{ nullptr };      //! The standard event graph that cannot be removed
    OrchestratorPlugin* _plugin{ nullptr };              //! Reference to the plug-in
    OrchestratorScriptViewGraphsSection* _graphs;        //! Graphs section
    OrchestratorScriptViewFunctionsSection* _functions;  //! Functions section
    OrchestratorScriptViewMacrosSection* _macros;        //! Macros section
    OrchestratorScriptViewVariablesSection* _variables;  //! Variables section
    OrchestratorScriptViewSignalsSection* _signals;      //! Signals section

protected:
    static void _bind_methods() { }

    OrchestratorScriptView() = default;

public:
    OrchestratorScriptView(OrchestratorPlugin* p_plugin, const Ref<OScript>& p_script);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Locates the node in the Orchestration and navigates to it, opening any graph that is
    /// necessary to navigate to the node.
    /// @param p_node_id the node to locate and focus
    void goto_node(int p_node_id);

    /// Return whether the underlying script has been modified or has unsaved changes.
    /// @return true if the editor has pending changes, false otherwise
    bool is_modified() const;

    /// Requests the editor to reload the script contents from disk.
    void reload_from_disk();

    /// Requests that any pending changes be flushed to the script.
    void apply_changes();

    /// Save the script in the editor with the new file name.
    /// @param p_new_file the new file name
    /// @return true if successful, false otherwise.
    bool save_as(const String& p_new_file);

    /// Performs the build step
    /// @return true if the build is successful, false otherwise
    bool build();

private:
    /// Updates the components tree
    void _update_components();

    /// Returns the tab's index by name
    /// @param p_name the tab name
    /// @return the tab index or -1 if no tab is found
    int _get_tab_index_by_name(const String& p_name) const;

    /// Gets the graph tab if it exists, or creates a new graph tab if it doesn't
    /// @param p_graph_nme the name of the tab
    /// @param p_focus automatically focuses the tab
    /// @param p_create controls whether the function creates the graph if it doesn't exist
    /// @return the existing or newly constructed graph, with all signals connected
    OrchestratorGraphEdit* _get_or_create_tab(const StringName& p_tab_name, bool p_focus = true, bool p_create = true);

    /// Displays the search dialog with only available function overrides
    void _show_available_function_overrides();

    /// Closes the specified tab
    /// @param p_tab_index the tab index
    void _close_tab(int p_tab_index);

    /// Dispatched when a user requests a tab to be closed.
    /// @param p_tab_index the tab's index
    void _on_close_tab_requested(int p_tab_index);

    /// Dispatched when a graph has changed
    void _on_graph_nodes_changed();

    /// Dispatched when a node requests a specific object focus
    void _on_graph_focus_requested(Object* p_object);

    /// Dispatched when a graph is requested to be opened and focused.
    void _on_show_graph(const String& p_graph_name);

    /// Dispatched when a graph is requested to be closed, if its open.
    void _on_close_graph(const String& p_graph_name);

    /// Dispatched when a graph is renamed
    void _on_graph_renamed(const String& p_old_name, const String& p_new_name);

    /// Dispatched when a graph node is requested for focus.
    void _on_focus_node(const String& p_graph_name, int p_node_id);

    /// Dispatched when function override is requested
    void _on_override_function();

    /// Dispatched when a user creates a signal connection via the Editor UI
    /// @param p_object the object to whom is being connected
    /// @param p_function_name the signal function to create
    /// @param p_args array of function arguments
    void _add_callback(Object* p_object, const String& p_function_name, const PackedStringArray& p_args);
};

#endif  // ORCHESTRATOR_SCRIPT_VIEW_H
