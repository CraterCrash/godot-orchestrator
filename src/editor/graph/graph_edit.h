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
#ifndef ORCHESTRATOR_GRAPH_EDIT_H
#define ORCHESTRATOR_GRAPH_EDIT_H

#include "actions/action_menu.h"
#include "common/version.h"
#include "graph_node.h"

#include <functional>

#include <godot_cpp/classes/curve2d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/timer.hpp>

using namespace godot;

// Forward declarations
class OScriptNode;
class OrchestratorScriptAutowireSelections;
class OrchestratorGraphKnot;
class OrchestratorPlugin;

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

    enum ContextMenuIds
    {
        CM_VARIABLE_GET,
        CM_VARIABLE_GET_VALIDATED,
        CM_VARIABLE_SET,
        CM_PROPERTY_GET,
        CM_PROPERTY_SET,
        CM_FILE_GET_PATH,
        CM_FILE_PRELOAD,
        CM_FUNCTION_CALL,
        CM_FUNCTION_CALLABLE
    };

    /// Simple drag state for the graph
    struct DragContext
    {
        StringName node_name;
        int node_port{ -1 };
        bool output_port{ false };
        bool dragging{ false };

        DragContext();
        void reset();

        void start_drag(const StringName& p_from, int p_from_port, bool p_output);
        void end_drag();

        bool should_autowire() const;
        EPinDirection get_direction() const;
    };

    struct Clipboard
    {
        HashMap<int, Ref<OScriptNode>> nodes;
        HashMap<int, Vector2> positions;
        RBSet<OScriptConnection> connections;

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
        }
    };

    static Clipboard* _clipboard;

    #if GODOT_VERSION >= 0x040300
    OptionButton* _grid_pattern{ nullptr };                //! Grid pattern option button
    #endif
    Ref<OScriptGraph> _script_graph;                       //! The underlying orchestration script graph
    OrchestratorGraphActionMenu* _action_menu{ nullptr };  //! Actions menu
    Vector2 _saved_mouse_position;                         //! Mouse position where node/dialog is placed
    DragContext _drag_context;                             //! Drag context details
    int _deferred_tween_node{ -1 };                        //! Node id to tween to upon load
    PopupMenu* _context_menu{ nullptr };                   //! Graph context menu
    OrchestratorPlugin* _plugin{ nullptr };                //! The plugin
    Control* _status{ nullptr };                           //! Displays status in the center of graphs
    Label* _drag_hint{ nullptr };                          //! Displays the drag status at the bottom of the graph
    Timer* _drag_hint_timer{ nullptr };                    //! Timer for drag hint messages
    Timer* _theme_update_timer{ nullptr };
    Button* _base_type_button{ nullptr };
    Dictionary _hovered_connection;                        //! Hovered connection details
    HashMap<uint64_t, Vector<Ref<KnotPoint>>> _knots;      //! Knots for each graph connection
    GDExtensionGodotVersion _version;                      //! Godot version
    bool _is_43p{ false };                                 //! Is Godot 4.3+
    OrchestratorScriptAutowireSelections* _autowire{ nullptr };

    OrchestratorGraphEdit() = default;

protected:
    static void _bind_methods();

public:
    // The OrchestratorGraphEdit maintains a static clipboard so that data can be shared across different graph
    // instances easily in the tab view, and so these methods are called by the MainView during the
    // ENTER_TREE and EXIT_TREE notifications.
    static void initialize_clipboard();
    static void free_clipboard();

    /// Creates the Orchestration OrchestratorGraphEdit instance.
    /// @param p_plugin the plugin instance, should never be null
    /// @param p_graph the orchestration graph, should never be invalid
    OrchestratorGraphEdit(OrchestratorPlugin* p_plugin, const Ref<OScriptGraph>& p_graph);

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Get the owning orchestration script graph.
    /// @return the script graph reference, should always be valid
    Ref<OScriptGraph> get_owning_graph() { return _script_graph; }

    /// Get the owning orchestration
    /// @return the owning orchestration, should always be valid
    Orchestration* get_orchestration() { return _script_graph->get_orchestration(); }

    /// Get the editor graph action menu.
    OrchestratorGraphActionMenu* get_action_menu() { return _action_menu; }

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

    /// Helper method for spawning nodes
    /// @param p_context the node initialization context
    /// @param p_position the position to spawn the node, if provided.
    /// @param p_callback the callback to call when the node is spawned successfully.
    template <typename T>
    void spawn_node(const OScriptNodeInitContext& p_context, const Vector2& p_position = Vector2(), const Callable& p_callback = Callable())
    {
        spawn_node(T::get_class_static(), p_context, p_position, p_callback);
    }

    /// Helper method to spawn a node by it's type
    /// @param p_type the node type to spawn
    /// @param p_context the node initialization context
    /// @param p_position the position to spawn the node
    /// @param p_callback the callback to call when the node is spawned successfully.
    void spawn_node(const StringName& p_type, const OScriptNodeInitContext& p_context, const Vector2& p_position = Vector2(), const Callable& p_callback = Callable());

    void sync();

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

    /// Checks whether the specified position is within any node rect.
    /// @param p_position the position to check
    /// @return true if the position is within any node rect, false otherwise
    bool _is_position_within_node_rect(const Vector2& p_position) const;

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

    /// Synchronizes the child order
    void _synchronize_child_order();

    /// Perform any post-steps after spawning a node
    /// @param p_spawned the spawned node
    /// @param p_callback a callback that is called after spawning the node
    void _complete_spawn(const Ref<OScriptNode>& p_spawned, const Callable& p_callback);

    /// Queue the autowire action for the spawned node
    /// @param p_source the source pin
    /// @param p_spawned the spawned node
    /// @param p_callback optional callback to be fired when the node was spawned
    void _queue_autowire(const Ref<OScriptNodePin>& p_source, const Ref<OScriptNode>& p_spawned, const Callable& p_callback = Callable());

    /// Completes the autowire action
    /// @param p_spawned the spawned node
    /// @param p_callback the callback to fire when the node is spawned
    /// @param p_confirmed whether the user confirmed any choices
    void _complete_autowire(const Ref<OScriptNode>& p_spawned, const Callable& p_callback, bool p_confirmed);

    /// Shows the actions menu
    /// @param p_position the position to show the dialog
    /// @param p_filter the filters to be applied
    void _show_action_menu(const Vector2& p_position, const OrchestratorGraphActionFilter& p_filter);

    /// Update the saved mouse position
    /// @param p_position the position
    void _update_saved_mouse_position(const Vector2& p_position);

    /// Displays the drag status hint
    /// @param p_message the hint message
    void _show_drag_hint(const String& p_message) const;

    /// Hides the drag status hint
    void _hide_drag_hint();

    /// Creates a callable mapping for the script function
    /// @param p_function_name the name of the function to call
    void _create_script_function_callable(const StringName& p_function_name);

    /// Connection drag started
    /// @param p_from the source node
    /// @param p_from_port source node port
    /// @param p_output is the drag from an output port
    void _on_connection_drag_started(const StringName& p_from, int p_from_port, bool p_output);

    /// Connection drag ended
    void _on_connection_drag_ended();

    /// Dispatched when the action menu is closed without a selection.
    void _on_action_menu_cancelled();

    /// Dispatched when an action menu item is selected in the action menu
    /// @param p_handler the action handler to be executed
    void _on_action_menu_action_selected(OrchestratorGraphActionHandler* p_handler);

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
    void _on_attempt_connection_from_empty(const StringName& p_to_node, int p_to_port, const Vector2& p_position);

    /// Dispatched when user drags a connection line from a node output to an empty area of the graph
    /// @param p_from_node source node
    /// @param p_from_port source node port
    /// @param p_position mouse position where released
    void _on_attempt_connection_to_empty(const StringName& p_from_node, int p_from_port, const Vector2& p_position);

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

    /// Dispatched when the context menu option is selected
    /// @param p_id the menu option id
    void _on_context_menu_selection(int p_id);

    /// Dispatched when project settings are changed
    void _on_project_settings_changed();

    /// Shows the script details in the inspector
    void _on_inspect_script();

    /// Validates and builds the script
    void _on_validate_and_build();

    /// Dispatched when the user presses {@code Ctrl+C} to copy selected nodes to the clipboard.
    void _on_copy_nodes_request();

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