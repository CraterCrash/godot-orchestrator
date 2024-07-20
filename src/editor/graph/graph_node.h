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
#ifndef ORCHESTRATOR_GRAPH_NODE_H
#define ORCHESTRATOR_GRAPH_NODE_H

#include "common/version.h"
#include "script/node.h"

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/popup_menu.hpp>

using namespace godot;

/// Forward declarations
class OScript;
class OrchestratorGraphEdit;
class OrchestratorGraphNodePin;

/// Specialized implementation of the Godot's GraphNode for Orchestrations.
///
/// When creating an Orchestration in the editor, the user interacts with a specialized
/// GraphEdit interface. This class is meant to provide custom functionality which is
/// part of GraphNode, a component of a node graph.
///
class OrchestratorGraphNode : public GraphNode
{
    GDCLASS(OrchestratorGraphNode, GraphNode);

    enum ContextMenuId
    {
        CM_NONE,
        CM_SELECT_GROUP,
        CM_DESELECT_GROUP,
        CM_DELETE,
        CM_CUT,
        CM_COPY,
        CM_PASTE,
        CM_DUPLICATE,
        CM_REFRESH,
        CM_BREAK_LINKS,
        CM_ADD_OPTION_PIN,
        CM_RENAME,
        CM_TOGGLE_BREAKPOINT,
        CM_ADD_BREAKPOINT,
        CM_ENABLE_BREAKPOINT,
        CM_REMOVE_BREAKPOINT,
        CM_DISABLE_BREAKPOINT,
        CM_VIEW_DOCUMENTATION,
        CM_COLLAPSE_FUNCTION,
        CM_EXPAND_NODE,
        CM_RESIZABLE,
        CM_NODE_ACTION = 1000
    };

    OrchestratorGraphEdit* _graph{ nullptr };   //! The editor graph that owns this node
    Ref<OScriptNode> _node;                     //! The script node instance
    List<Ref<OScriptAction>> _context_actions;  //! Context menu actions
    PopupMenu* _context_menu{ nullptr };        //! The node's context menu
    HBoxContainer* _indicators{ nullptr };      //! Container for indicators

protected:
    OrchestratorGraphNode() = default;
    static void _bind_methods();

public:

    /// Creates an editor graph node
    /// @param p_graph the owning graph, should not be null
    /// @param p_node the script node this editor node represents, should not be null
    OrchestratorGraphNode(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node);

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Process and accept UI element inputs
    /// @param p_event the event
    void _gui_input(const Ref<InputEvent>& p_event) override;

    /// Return the owning graph
    /// @return the graph
    OrchestratorGraphEdit* get_graph();

    /// Get the node's script unique id
    /// @return node's script unique id
    int get_script_node_id() const;

    /// Get the script node
    /// @return the script node reference
    Ref<OScriptNode> get_script_node() { return _node; }

    /// Get the graph node input pin at a given port
    /// @param p_port the port or slot index
    /// @return the editor graph node pin, or null if not found
    virtual OrchestratorGraphNodePin* get_input_pin(int p_port) { return nullptr; }

    /// Get the graph node output pin at the given port
    /// @param p_port the port or slot index
    /// @return the editor graph node pin, or null if not found
    virtual OrchestratorGraphNodePin* get_output_pin(int p_port) { return nullptr; }

    /// Sets the input port opacity if it cannot accept connection with pin.
    /// @param p_opacity the opacity to set
    /// @param p_other the pin
    void set_inputs_for_accept_opacity(float p_opacity, OrchestratorGraphNodePin* p_other);

    /// Sets the output port opacity if it cannot accept connection with pin.
    /// @param p_opacity the opacity to set
    /// @param p_other the pin
    void set_outputs_for_accept_opacity(float p_opacity, OrchestratorGraphNodePin* p_other);

    /// Sets all input ports opacity to the specified value
    /// @param p_opacity the opacity to set
    void set_all_inputs_opacity(float p_opacity = 1.f);

    /// Sets all output ports opacity to the specified value
    /// @param p_opacity the opacity to set
    void set_all_outputs_opacity(float p_opacity = 1.f);

    /// Get the count of input ports with the specified opacity
    /// @param p_opacity the opacity, defaults to 1.f
    /// @return the number of ports found
    int get_inputs_with_opacity(float p_opacity = 1.f);

    /// Get the count of output ports with the specified opacity
    /// @param p_opacity the opacity, defaults to 1.f
    /// @return the number of ports found
    int get_outputs_with_opacity(float p_opacity = 1.f);

    /// Unlinks all connections to all pins on this node
    void unlink_all();

    /// Set whether node icons are shown
    virtual void show_icons(bool p_show_icons) { }

    /// Get a list of nodes within this node's global rect.
    List<OrchestratorGraphNode*> get_nodes_within_global_rect();

    /// Get the specified point index at the given position and direction
    /// @param p_position the position
    /// @param p_direction the direction
    /// @return the point index
    int32_t get_port_at_position(const Vector2& p_position, EPinDirection p_direction);

    // Group API

    virtual bool is_groupable() const { return false; }
    virtual bool is_group_selected() { return false; }
    virtual void select_group() {}
    virtual void deselect_group() {}

protected:
    /// Update pins for this graph node
    virtual void _update_pins();

    /// Updates node indicators
    virtual void _update_indicators();

    /// Should the node resize on updates, by default is true.
    virtual bool _resize_on_update() const { return true; }

    /// Update the nodes titlebar details
    void _update_titlebar();

    /// Update the node's styles
    void _update_styles();

    /// Get the selection color
    /// @return the border color for when nodes are selected
    Color _get_selection_color() const;

    /// Called by various callbacks to update node attributes
    void _update_node_attributes();

    /// Updates the node's tooltip
    void _update_tooltip();

    /// Display the node's context menu
    void _show_context_menu(const Vector2& p_position);

    /// Returns whether the node is considered editable
    bool _is_editable() const;

    /// Is the "add-pin" button visible
    /// @return true if the add-pin is visible, otherwise false
    bool _is_add_pin_button_visible() const;

    /// Adds a new option pin to the node
    void _add_option_pin();

    /// Simulates the action being pressed
    /// @param p_action_name the action to simulate
    void _simulate_action_pressed(const String& p_action_name);

    #if GODOT_VERSION >= 0x040300
    /// Initializes the node's breakpoint state
    void _initialize_node_beakpoint_state();

    /// Set the breakpoint state
    /// @param p_flag the breakpoint flag state
    void _set_breakpoint_state(OScriptNode::BreakpointFlags p_flag);
    #endif

private:
    /// Called when the graph node is moved
    /// @param p_old_pos old position
    /// @param p_new_pos new position
    void _on_node_moved(const Vector2 p_old_pos, const Vector2 p_new_pos);

    /// Called when the graph node is resized
    void _on_node_resized();

    /// Called when any pin detail has changed for this node
    void _on_pins_changed();

    /// Called when a pin is connected
    /// @param p_type pin type
    /// @param p_index the pin index
    void _on_pin_connected(int p_type, int p_index);

    /// Called when a pin is disconnected
    /// @param p_type pin type
    /// @param p_index the pin index
    void _on_pin_disconnected(int p_type, int p_index);

    /// Called when the underlying script node calls "emit_changed()"
    void _on_changed();

    /// Called when the "add-pin" button pressed
    void _on_add_pin_pressed();

    /// Handles the selection of a context menu item
    /// @param p_id the selected context-menu option
    void _handle_context_menu(int p_id);

    /// Cleans up the context menu after it has closed
    void _cleanup_context_menu();
};

#endif  // ORCHESTRATOR_GRAPH_NODE_H