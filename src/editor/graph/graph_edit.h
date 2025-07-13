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
#ifndef ORCHESTRATOR_GRAPH_EDIT_H
#define ORCHESTRATOR_GRAPH_EDIT_H

#include "common/godot_version.h"
#include "common/version.h"
#include "common/weak_ref.h"
#include "editor/actions/definition.h"
#include "editor/graph/graph_node.h"
#include "script/function.h"
#include "script/signals.h"
#include "script/variable.h"

#include <functional>

#include <godot_cpp/classes/curve2d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/timer.hpp>

using namespace godot;

// Forward declarations
class OScriptNode;
class OrchestratorGraphKnot;

/// Helper class for storing a reference to a position for the knot in the graph
class OrchestratorKnotPoint : public RefCounted
{
    GDCLASS(OrchestratorKnotPoint, RefCounted);
    static void _bind_methods() {}
public:
    Vector2 point; //! The knot position
};

/// A custom implementation of the Godot GraphEdit class that provides a node-based
/// workspace for editing Orchestrations.
///
/// GraphEdit provides tools for creation, manipulation, and displaying of various types of graphs.
/// Its main purpose in the engine is to power the visual programming systems, such as visual shaders.
///
/// Out of the box, GraphEdit provides an infinite grid where GraphNode instances are placed.
/// In this context, the user places OrchestratorGraphNode instances, which represent a node in the
/// graph, which represents a single step within the Orchestration.
///
class OrchestratorGraphEdit : public GraphEdit
{
    GDCLASS(OrchestratorGraphEdit, GraphEdit);

    typedef OrchestratorKnotPoint KnotPoint;

public:

    struct NodeSpawnOptions {
        StringName node_class;
        OrchestratorGraphNodePin* drag_pin = nullptr;
        OScriptNodeInitContext context;
        Vector2 position;
        bool select_on_spawn = false;
        bool center_on_spawn = false;
    };

private:

    struct PinHandle
    {
        uint64_t node_id = 0;
        int32_t pin_port = 0;
    };

    // Defines as a weak reference so that in the event the graph is redrawn or if the pin is
    // no longer valid, any future use will return null if the pin object no longer exists.
    WeakRef<OrchestratorGraphNodePin> _drag_from_pin;

    struct Clipboard
    {
        HashMap<int, Ref<OScriptNode>> nodes;
        HashMap<int, Vector2> positions;
        RBSet<OScriptConnection> connections;
        RBSet<Ref<OScriptFunction>> functions;
        RBSet<Ref<OScriptVariable>> variables;
        RBSet<Ref<OScriptSignal>> signals;

        /// Returns whether the clipboard is empty
        bool is_empty() const
        {
            return nodes.is_empty();
        }

        /// Reset the clipboard
        void reset()
        {
            nodes.clear();
            positions.clear();
            connections.clear();
            signals.clear();
            variables.clear();
            functions.clear();
        }
    };

    static Clipboard* _clipboard;

    #if GODOT_VERSION >= 0x040300
    OptionButton* _grid_pattern{ nullptr };                //! Grid pattern option button
    #endif
    Ref<OScriptGraph> _script_graph;                       //! The underlying orchestration script graph
    Vector2 _saved_mouse_position;                         //! Mouse position where node/dialog is placed
    int _deferred_tween_node{ -1 };                        //! Node id to tween to upon load
    Control* _status{ nullptr };                           //! Displays status in the center of graphs
    Label* _drag_hint{ nullptr };                          //! Displays the drag status at the bottom of the graph
    Timer* _drag_hint_timer{ nullptr };                    //! Timer for drag hint messages
    Timer* _theme_update_timer{ nullptr };
    Button* _base_type_button{ nullptr };
    Dictionary _hovered_connection;                        //! Hovered connection details
    HashMap<uint64_t, Vector<Ref<KnotPoint>>> _knots;      //! Knots for each graph connection
    GodotVersionInfo _version;                             //! Godot version
    bool _is_43p{ false };                                 //! Is Godot 4.3+
    bool _box_selection{ false };                          //! Is graph doing box selection?
    bool _disable_delete_confirmation{ false };            //! Allows temporarily disabling delete confirmation
    Vector2 _box_selection_from;                           //! Mouse position box selection started from

    OrchestratorGraphEdit() = default;

protected:
    static void _bind_methods();

    /// Move the selected nodes by the delta
    /// @param p_delta the delta to move selected nodes by
    void _move_selected(const Vector2& p_delta);

    /// Sorts child nodes after a node is added as a child.
    /// @param p_node the node that was added
    void _resort_child_nodes_on_add(Node* p_node);

    /// Gets the child index for the GraphEdit's <code>_connection_layer</code> control.
    /// @return the child index of the connection layer control
    int _get_connection_layer_index() const;

    /// Checks whether the specified node is a comment node
    /// @param p_node the node to check
    /// @return true if it's a comment node, false otherwise
    bool _is_comment_node(Node* p_node) const;

    /// Resolves the graph node pin from handle
    /// @param p_handle the pin handle
    /// @param p_input whether the pin is an input
    /// @return the resolved graph pin or null if the pin could not be resolved
    OrchestratorGraphNodePin* _resolve_pin_from_handle(const PinHandle& p_handle, bool p_input);

    // drop_data helpers
    void _drop_data_files(const String& p_node_type, const Array& p_files, const Vector2& p_at_position);
    void _drop_data_property(const Dictionary& p_property, const Vector2& p_at_position, const String& p_path, bool p_setter);
    void _drop_data_function(const Dictionary& p_function, const Vector2& p_at_position, bool p_as_callable);
    void _drop_data_variable(const String& p_name, const Vector2& p_at_position, bool p_validated, bool p_setter);

public:
    // The OrchestratorGraphEdit maintains a static clipboard so that data can be shared across different graph
    // instances easily in the tab view, and so these methods are called by the MainView during the
    // ENTER_TREE and EXIT_TREE notifications.
    static void initialize_clipboard();
    static void free_clipboard();

    /// Creates the Orchestration OrchestratorGraphEdit instance.
    /// @param p_graph the orchestration graph, should never be invalid
    OrchestratorGraphEdit(const Ref<OScriptGraph>& p_graph);

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Get the owning orchestration script graph.
    /// @return the script graph reference, should always be valid
    Ref<OScriptGraph> get_owning_graph() { return _script_graph; }

    /// Get the owning orchestration
    /// @return the owning orchestration, should always be valid
    Orchestration* get_orchestration() { return _script_graph->get_orchestration(); }

    /// Return whether this graph represents an event-graph.
    bool is_event_graph() const { return _script_graph->get_flags().has_flag(OScriptGraph::GF_EVENT); }

    /// Return whether this graph represents a user-derived function graph.
    bool is_function() const { return _script_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION); }

    /// Clear all selected nodes
    void clear_selection();

    Vector<OrchestratorGraphNode*> get_selected_nodes();
    Vector<Ref<OScriptNode>> get_selected_script_nodes();

    /// Causes the graph to tween focus the specified node in the graph.
    /// @param p_node_id the node's unique id
    /// @param p_animated whether to animate the movement, enabled by default
    void focus_node(int p_node_id);

    /// Request focus on the desired object
    /// @param p_object
    void request_focus(Object* p_object);

    /// Saves any changes to the underlying script
    void apply_changes();

    /// A post save operation that is called after apply_changes.
    void post_apply_changes();

    /// Sets the spawn position the center of the graph edit view
    void set_spawn_position_center_view();

    /// Goto class help
    /// @param p_class_name the class name to show help for
    void goto_class_help(const String& p_class_name);

    /// Perform an action for each graph node
    /// @param p_func the lambda to be applied
    void for_each_graph_node(std::function<void(OrchestratorGraphNode*)> p_func);

    /// Perform an action for each <code>GraphNode</code> object type
    /// @param p_func the function to call for each graph element
    /// @param p_nodes whether <code>OrchestratorGraphNode</code> objects are included, defaults to <code>true</code>
    /// @param p_knots whether <code>OrchestratorGraphKnot</code> objects are included, defaults to <code>true</code>
    void for_each_graph_element(const std::function<void(GraphElement*)>& p_func, bool p_nodes = true, bool p_knots = true);

    /// Execute the specified action
    /// @param p_action_name the action to execute
    void execute_action(const String& p_action_name);

    #if GODOT_VERSION < 0x040300
    /// Backport of Godot 4.3's get_closest_connection_at_point
    /// @param p_position the mouse position
    /// @param p_max_distance the max distance to calculate against
    /// @return the connection closest to the point
    Dictionary get_closest_connection_at_point(const Vector2& p_position, float p_max_distance = 4.0f);
    #endif

    //~ GraphEdit overrides
    void _gui_input(const Ref<InputEvent>& p_event) override;
    bool _can_drop_data(const Vector2& p_position, const Variant& p_data) const override;
    void _drop_data(const Vector2& p_position, const Variant& p_data) override;
    bool _is_node_hover_valid(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port) override;
    PackedVector2Array _get_connection_line(const Vector2& p_from_position, const Vector2& p_to_position) const override;
    //~ End GraphEdit overrides

    /// Spawn a node in the graph
    /// @param p_options the node spawn options
    /// @return the spawned node or <code>nullptr</code> if the spawn failed
    OrchestratorGraphNode* spawn_node(const NodeSpawnOptions& p_options);

    void sync();

    /// Shows the override function action menu
    void show_override_function_action_menu();

    /// Centers the view on the given node
    /// @param p_node the node to center into the view
    void center_node(OrchestratorGraphNode* p_node);

    /// Scrolls the view to the desired position
    /// @param p_position the position to scroll toward
    /// @param p_time the maximum time the scroll should take
    void scroll_to_position(const Vector2& p_position, float p_time = 0.2f);

private:
    /// Displays a yes/no confirmation dialog to the user.
    /// @param p_text the text to be shown.
    /// @param p_title the confirmation window title text
    /// @param p_confirm_callback the callback if the user presses 'yes'
    void _confirm_yes_no(const String& p_text, const String& p_title, Callable p_confirm_callback);

    /// Displays a notification to the user
    /// @param p_text the text to be shown.
    /// @param p_title the notification window title text
    void _notify(const String& p_text, const String& p_title);

    /// Checks whether the specified position is valid for knot operations
    /// @param p_position the position to check
    /// @return true if the position is valid for knot operations, false otherwise
    bool _is_position_valid_for_knot(const Vector2& p_position) const;

    /// Caches the graph knots for use.
    /// Copies the knot data from the OScriptGraph to this GraphEdit instance.
    void _cache_connection_knots();

    /// Stores the cached graph knots data from this GraphEdit to the OScriptGraph.
    void _store_connection_knots();

    /// Get the connection for the specified points
    /// @param p_from_position the from position
    /// @param p_to_position the to position
    /// @param r_connection the connection
    /// @return true if a connection was resolved, false otherwise
    bool _get_connection_for_points(const Vector2& p_from_position, const Vector2& p_to_position, OScriptConnection& r_connection) const;

    /// Calculate the connection curves
    /// @param p_points the points
    /// @return vector of Curve2D resources
    Vector<Ref<Curve2D>> _get_connection_curves(const PackedVector2Array& p_points) const;

    /// Get all knot points for the specified connection.
    /// @param p_connection the connection
    /// @param p_apply_zoom mutate the knot points by the current zoom factor, defaults to false
    /// @return array of connection knot points, may be empty if no knots are defined
    PackedVector2Array _get_connection_knot_points(const OScriptConnection& p_connection, bool p_apply_zoom = false) const;

    /// Creates a connection wire knot
    /// @param p_connection the connection
    /// @param p_position the position to created the knot
    void _create_connection_knot(const Dictionary& p_connection, const Vector2& p_position);

    /// Updates the GraphEdit theme
    void _update_theme();

    /// Moves the center of the graph to the node, optionally animating the movement.
    /// @param p_node_id the node's unique id, should be greater or equal-to 0.
    /// @param p_animated whether to animate the movement, enabled by default.
    void _focus_node(int p_node_id, bool p_animated = true);

    /// Get a specific script graph node by its id
    /// @param p_id the node id
    /// @return the graph node or nullptr if not found
    OrchestratorGraphNode* _get_node_by_id(int p_id);

    /// Get a specific child node by its name.
    /// @param p_name the name of the child
    /// @return the child instance if found and if the child with the name is of type T; otherwise null.
    template<typename T>
    T* _get_by_name(const StringName& p_name) { return Object::cast_to<T>(get_node_or_null(NodePath(p_name))); }

    /// Helper method that removes all nodes from the graph.
    /// This can be useful when a full resync of the graph is needed.
    void _remove_all_nodes();

    /// Synchronizes the graph with the script's state.
    /// This ideally should be used as little as possible as it completely removes
    /// all graph elements and redraws everything, invalidating any signals.
    /// @param p_apply_position repositions the graph based on the stored state
    void _synchronize_graph_with_script(bool p_apply_position = false);

    /// Updates the graph connections.
    /// This removes all connections in the graph and redraws them based
    /// on the state in the associated script, leaving the nodes as-is.
    void _synchronize_graph_connections_with_script();

    /// Synchronizes the graph knots
    void _synchronize_graph_knots();

    /// Remove all knots related to the specific connection id
    /// @param p_connection_id
    void _remove_connection_knots(uint64_t p_connection_id);

    /// Updates only the specific graph node
    /// @param p_node the node to update.
    void _synchronize_graph_node(Ref<OScriptNode> p_node);

    /// Queue the autowire action for the spawned node
    /// @param p_spawned_node the spawned node
    /// @param p_origin_pin the pin that was dragged from
    void _queue_autowire(OrchestratorGraphNode* p_spawned_node, OrchestratorGraphNodePin* p_origin_pin);

    /// Update the saved mouse position
    /// @param p_position the position
    void _update_saved_mouse_position(const Vector2& p_position);

    /// Displays the drag status hint
    /// @param p_message the hint message
    void _show_drag_hint(const String& p_message) const;

    /// Hides the drag status hint
    void _hide_drag_hint();

    /// Show action menu to connect a new node with the dragged node port
    /// @param p_handle the drag pin handle
    /// @param p_position the position drag ended
    /// @param p_input whether the drag pin is an output
    void _connect_with_menu(PinHandle p_handle, const Vector2& p_position, bool p_input);

    /// Handles adding selected action item
    /// @param p_action the action definition
    void _on_action_menu_selection(const Ref<OrchestratorEditorActionDefinition>& p_action);

    /// Connection drag started
    /// @param p_from the source node
    /// @param p_from_port source node port
    /// @param p_output is the drag from an output port
    void _on_connection_drag_started(const StringName& p_from, int p_from_port, bool p_output);

    /// Connection drag ended
    void _on_connection_drag_ended();

    /// Dispatched when a connection is requested between two nodes
    /// @param p_from_node source node
    /// @param p_from_port source node port
    /// @param p_to_node target node
    /// @param p_to_port target node port
    void _on_connection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node, int p_to_port);

    /// Dispatched when a connection disconnect is requested between two nodes
    /// @param p_from_node source node
    /// @param p_from_port source node port
    /// @param p_to_node target node
    /// @param p_to_port target node port
    void _on_disconnection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node, int p_to_port);

    /// Dispatched when user drags a connection line from a node input to an empty area of the graph
    /// @param p_to_node target node
    /// @param p_to_port target node port
    /// @param p_position mouse position where released
    void _on_connection_from_empty(const StringName& p_to_node, int p_to_port, const Vector2& p_position);

    /// Dispatched when user drags a connection line from a node output to an empty area of the graph
    /// @param p_from_node source node
    /// @param p_from_port source node port
    /// @param p_position mouse position where released
    void _on_connection_to_empty(const StringName& p_from_node, int p_from_port, const Vector2& p_position);

    /// Dispatched when a user selects a node in the graph
    /// @param p_node the selected node
    void _on_node_selected(Node* p_node);

    /// Dispatched when a user deselects the node in the graph
    /// @param p_node the deselected node
    void _on_node_deselected(Node* p_node);

    /// Dispatched when a user has selected one or more nodes and uses the delete key
    /// @param p_node_names the nodes to be deleted
    void _on_delete_nodes_requested(const PackedStringArray& p_node_names);

    /// Deletes nodes with the given names
    /// @param p_node_names the nodes to be deleted
    void _delete_nodes(const PackedStringArray& p_node_names);

    /// Dispatched when the user right clicks the graph edit
    /// @param p_position position where the click event occurred
    void _on_right_mouse_clicked(const Vector2& p_position);

    /// Dispatched when a node is added to the graph
    /// @param p_node_id the node id
    void _on_graph_node_added(int p_node_id);

    /// Dispatched when a node is removed from the graph
    /// @param p_node_id the node id
    void _on_graph_node_removed(int p_node_id);

    /// Dispatched when the underlying script's connection list is changed
    void _on_graph_connections_changed(const String& p_caller);

    /// Dispatched when project settings are changed
    void _on_project_settings_changed();

    /// Shows the script details in the inspector
    void _on_inspect_script();

    /// Validates and builds the script
    void _on_validate_and_build();

    /// Dispatched when the user presses {@code Ctrl+C} to copy selected nodes to the clipboard.
    void _on_copy_nodes_request();

    /// Dispatched when the user pressed {@code Ctrl+X} to cut selected nodes to the clipboard.
    void _on_cut_nodes_request();

    /// Dispatched when the user wants to duplicate a graph node.
    void _on_duplicate_nodes_request();

    /// Dispatched when the user pressed {@code Ctrl+V} to paste nodes onto the graph.
    void _on_paste_nodes_request();

    /// Dispatched when the script is changed
    void _on_script_changed();

    #if GODOT_VERSION >= 0x040300
    /// Dispatched when the grid state is changed
    /// @param p_current_state the current state of the grid
    void _on_show_grid(bool p_current_state);

    /// Dispatched when a grid style option is selected
    /// @param p_index the selected item index
    void _on_grid_style_selected(int p_index);
    #endif
};

#endif  // ORCHESTRATOR_GRAPH_EDIT_H