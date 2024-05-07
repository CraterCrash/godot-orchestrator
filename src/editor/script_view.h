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
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/tree.hpp>

/// Forward declarations
class OrchestratorGraphEdit;
class OrchestratorPlugin;
class OrchestratorMainView;
class OrchestratorScriptComponentPanel;
class OrchestratorScriptFunctionsComponentPanel;
class OrchestratorScriptGraphsComponentPanel;
class OrchestratorScriptMacrosComponentPanel;
class OrchestratorScriptSignalsComponentPanel;
class OrchestratorScriptVariablesComponentPanel;

/// Main Orchestrator Script View
class OrchestratorScriptView : public HSplitContainer
{
    friend class OrchestratorScriptFunctionsComponentPanel;

    GDCLASS(OrchestratorScriptView, HSplitContainer);
    static void _bind_methods() { }

    // Represents all different types of active connections for a Vector<Ref<OScriptNode>> set.
    struct NodeSetConnections
    {
        RBSet<OScriptConnection> connections;  //! Connections between the node set.
        RBSet<OScriptConnection> inputs;       //! Connections that are inputs from outside the node set.
        RBSet<OScriptConnection> outputs;      //! Connections that are outputs from the node set.
        int input_executions{ 0 };             //! Number of node set execution inputs connected
        int input_data{ 0 };                   //! Number of node set data inputs connected
        int output_executions{ 0 };            //! Number of node set execution outputs connected
        int output_data{ 0 };                  //! Number of node set data outputs connected
    };

protected:
    Ref<OScript> _script;                                   //! The orchestrator script
    TabContainer* _tabs{ nullptr };                         //! The tab container
    ScrollContainer* _scroll_container{ nullptr };          //! The right component container
    OrchestratorGraphEdit* _event_graph{ nullptr };         //! The standard event graph that cannot be removed
    OrchestratorPlugin* _plugin{ nullptr };                 //! Reference to the plug-in
    OrchestratorMainView* _main_view{ nullptr };            //! The owning main view
    OrchestratorScriptGraphsComponentPanel* _graphs;        //! Graphs section
    OrchestratorScriptFunctionsComponentPanel* _functions;  //! Functions section
    OrchestratorScriptMacrosComponentPanel* _macros;        //! Macros section
    OrchestratorScriptVariablesComponentPanel* _variables;  //! Variables section
    OrchestratorScriptSignalsComponentPanel* _signals;      //! Signals section

    /// Creates a new user-defined function
    /// @param p_name the new function name
    /// @param p_add_return_node whether to add a return node
    /// @return the newly created function, or an invalid reference if the create failed
    Ref<OScriptFunction> _create_new_function(const String& p_name, bool p_add_return_node = false);

    /// Resolves the node set connections based on the provided set of nodes
    /// @param p_nodes the nodes to resolve connections for
    /// @param r_connections the popuplated connections structure
    void _resolve_node_set_connections(const Vector<Ref<OScriptNode>>& p_nodes, NodeSetConnections& r_connections);

    /// Calculates the Rect2 that bounds the provided set of nodes
    /// @param p_nodes the nodes to calculate the bounding rect
    /// @return the rect area that bounds the nodes
    Rect2 _get_node_set_rect(const Vector<Ref<OScriptNode>>& p_nodes) const;

    /// Moves nodes between two graphs
    /// @param p_nodes the nodes to move
    /// @param p_source the source graph to remove the node from
    /// @param p_target the target graph to move the node to
    void _move_nodes(const Vector<Ref<OScriptNode>>& p_nodes, const Ref<OScriptGraph>& p_source, const Ref<OScriptGraph>& p_target);

    /// Collapse all selected nodes in the graph to a function
    /// @param p_graph the graph to source the selected nodes from
    void _collapse_selected_to_function(OrchestratorGraphEdit* p_graph);

    /// Expands the seleced node in the graph, replacing the call node with the function's contents
    /// @param p_node_id the node id to expand
    /// @param p_graph the graph that contains this node
    void _expand_node(int p_node_id, OrchestratorGraphEdit* p_graph);

    OrchestratorScriptView() = default;

public:
    OrchestratorScriptView(OrchestratorPlugin* p_plugin, OrchestratorMainView* p_main_view, const Ref<OScript>& p_script);

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Return whether the given script is what the editor view represents.
    /// @param p_script the script to check
    /// @return true if the scripts are the same, false otherwise
    bool is_same_script(const Ref<OScript>& p_script) const { return p_script == _script; }

    /// Locates the node in the Orchestration and navigates to it, opening any graph that is
    /// necessary to navigate to the node.
    /// @param p_node_id the node to locate and focus
    void goto_node(int p_node_id);

    /// Notifies the script view that the current scene tab has changed
    void scene_tab_changed();

    /// Return whether the underlying script has been modified or has unsaved changes.
    /// @return true if the editor has pending changes, false otherwise
    bool is_modified() const;

    /// Requests the editor to reload the script contents from disk.
    void reload_from_disk();

    /// Requests that any pending changes be flushed to the script.
    void apply_changes();

    /// Renames the script resource.
    /// @param p_new_file new file name
    void rename(const String& p_new_file);

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

    //~ Begin Signal handlers
    void _on_close_tab_requested(int p_tab_index);
    void _on_graph_nodes_changed();
    void _on_graph_focus_requested(Object* p_object);
    void _on_show_graph(const String& p_graph_name);
    void _on_close_graph(const String& p_graph_name);
    void _on_graph_renamed(const String& p_old_name, const String& p_new_name);
    void _on_focus_node(const String& p_graph_name, int p_node_id);
    void _on_override_function();
    void _on_toggle_component_panel(bool p_visible);
    void _on_scroll_to_item(TreeItem* p_item);
    //~ End Signal handlers

    /// Dispatched when a user creates a signal connection via the Editor UI
    /// @param p_object the object to whom is being connected
    /// @param p_function_name the signal function to create
    /// @param p_args array of function arguments
    void _add_callback(Object* p_object, const String& p_function_name, const PackedStringArray& p_args);
};

#endif  // ORCHESTRATOR_SCRIPT_VIEW_H
