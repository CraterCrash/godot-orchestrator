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
#ifndef ORCHESTRATOR_EDITOR_VIEWPORT_H
#define ORCHESTRATOR_EDITOR_VIEWPORT_H

#include "common/version.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OrchestratorEditorPanel;
class OrchestratorGraphEdit;
class OScriptNode;

struct OScriptConnection;

/// Base class for various editor viewport types
class OrchestratorEditorViewport : public HSplitContainer
{
    GDCLASS(OrchestratorEditorViewport, HSplitContainer);
    static void _bind_methods();

protected:
    const String EVENT_GRAPH_NAME{ "EventGraph" };

    // Represents all different types of active connections for a Vector<Ref<OScriptNode>> set.
    struct NodeSetConnections
    {
        RBSet<OScriptConnection> connections;  //! Connections between the node set
        RBSet<OScriptConnection> inputs;       //! Input connections from outside the node set
        RBSet<OScriptConnection> outputs;      //! Output connections to output the node set
        int input_executions{ 0 };             //! Number of input execution connections
        int output_executions{ 0 };            //! Number of output execution connections
        int input_data{ 0 };                   //! Number of input data connections
        int output_data{ 0 };                  //! Number of output data connections
    };

    Ref<Resource> _resource;                         //! The edited resource
    Orchestration* _orchestration{ nullptr };        //! The orchestration instance
    TabContainer* _tabs{ nullptr };                  //! The graph editor tab container
    ScrollContainer* _scroll_container{ nullptr };   //! The right component container
    ConfirmationDialog* _confirm_dialog{ nullptr };  //! Build confirmation dialog
    VBoxContainer* _component_container{ nullptr };  //! VBoxContainer

    //~ Begin Godot Interface
    void _notification(int p_what);
    //~ End Godot Interface

    /// Gets the Rect2 that contains all the specified nodes.
    /// @param p_nodes the nodes to calculate the rect about
    /// @return a Rect2 that is large enough to contain all the given nodes
    static Rect2 _get_node_set_rect(const Vector<Ref<OScriptNode>>& p_nodes);

    /// Update components, by default nothing is updated
    virtual void _update_components() { }

    /// Focuses the given object
    /// @param p_object the object to focus
    virtual void _focus_object(Object* p_object) { }

    /// Allows for performing actions on a graph that was just created/opened.
    /// @param p_graph the graph edit that was opened
    virtual void _graph_opened(OrchestratorGraphEdit* p_graph);

    /// Allows for performing actions when a graph that is opened is selected
    /// @param p_graph the graph edit that was selected
    virtual void _graph_selected(OrchestratorGraphEdit* p_graph);

    /// Allows a viewport to control whether a graph can be closed
    /// @param p_graph the graph to inspect
    /// @return true if the graph can be closed, false otherwise
    virtual bool _can_graph_be_closed(OrchestratorGraphEdit* p_graph) { return true; }

    /// Resolve the connection details for the given set of nodes
    /// @param p_nodes the set of nodes to inspect and resolve connection details about
    /// @param r_connections the resolved connection details
    void _resolve_node_set_connections(const Vector<Ref<OScriptNode>>& p_nodes, NodeSetConnections& r_connections);

    /// Closes the specified graph tab by index
    /// @param p_tab_index the tab index to close
    void _close_tab(int p_tab_index);

    /// Called when requesting the tab to be closed
    /// @param p_tab_index the tab requesting closure
    void _close_tab_requested(int p_tab_index);

    /// Called when a tab is changed
    /// @param p_tab_index the new tab index
    void _tab_changed(int p_tab_index);

    /// Called when the graph's node set has changed
    void _graph_nodes_changed();

    /// Called when the graph wants to focus a specific object
    /// @param p_object the object to focus
    void _graph_focus_requested(Object* p_object);

    /// Retreive the tab index by tab name
    /// @param p_tab_name the name of the tab
    /// @return the tab index, or -1 if no tab found
    int _get_tab_index_by_name(const String& p_tab_name) const;

    /// Gets, optionally creating a new graph edit for the tab
    /// @param p_name the name of the tab
    /// @param p_focus whether to focus the tab
    /// @param p_create whether to create the graph
    /// @return the graph edit, or null if not found or created
    OrchestratorGraphEdit* _get_or_create_tab(const StringName& p_name, bool p_focus = true, bool p_create = true);

    /// Gets the current tab
    /// @return the current graph edit tab, or null if no tab is open.
    OrchestratorGraphEdit* _get_current_tab();

    /// Renames a tab from the old name to the new name, if opened
    /// @param p_old_name the old name
    /// @param p_new_name the new name
    void _rename_tab(const String& p_old_name, const String& p_new_name);

    /// Default constructor, intentionally protected
    OrchestratorEditorViewport() = default;

public:
    /// Requests any pending changes to be flushed
    virtual void apply_changes();

    /// Reload the view from disk
    void reload_from_disk();

    /// Renames the underlying script with the new file name
    /// @param p_new_file_name the new file name
    void rename(const String& p_new_file_name);

    /// Saves the view to disk with the new file name
    /// @param p_new_file_name the new file name
    /// @return true if successful, false otherwise
    bool save_as(const String& p_new_file_name);

    /// Return whether this viewport is for the same script
    /// @return true if the same script, false otherwise
    bool is_same_script(const Ref<Script>& p_script) const;

    /// Return whether the edited object has been modified
    /// @return true if the object has been modified, false otherwise
    bool is_modified() const;

    /// Performs the build step
    /// @param p_show_success whether to show the validation results upon success
    /// @return true if the build is successful, false otherwise
    bool build(bool p_show_success = false);

    #if GODOT_VERSION >= 0x040300
    /// Clear all breakpoints in the script view
    void clear_breakpoints();

    /// Sets the breakpoint status on the specified node
    /// @param p_node_id the graph node id
    /// @param p_enabled whether the breakpoint is enabled
    void set_breakpoint(int p_node_id, bool p_enabled);

    /// Get a list of breakpoints
    /// @return the list of breakpoints for this script editor
    PackedStringArray get_breakpoints() const;
    #endif

    /// Focuses on the specified node
    /// @param p_node_id the node ID
    void goto_node(int p_node_id);

    /// Add a script function
    /// @param p_object the object
    /// @param p_function_name the function name
    /// @param p_args the arguments for the function
    virtual void add_script_function(Object* p_object, const String& p_function_name, const PackedStringArray& p_args) { }

    /// Notifies this viewport that the scene tab has changed
    void notify_scene_tab_changed();

    /// Notifies this viewport that the component panel visibility has changed
    /// @param p_visible whether the component panel should be visible
    void notify_component_panel_visibility_changed(bool p_visible);

    /// Constructs the editor viewport.
    /// @param p_resource the resource being edited
    explicit OrchestratorEditorViewport(const Ref<Resource>& p_resource);

    /// Default destructor
    ~OrchestratorEditorViewport() override = default;
};

#endif  // ORCHESTRATOR_EDITOR_VIEWPORT_H