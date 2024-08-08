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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_H

#include "script/node_pin.h"

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorGraphEdit;
class OrchestratorGraphNode;

/// Resolved type
struct ResolvedType
{
    Variant::Type type{ Variant::NIL };
    StringName class_name;
    Ref<OScriptTargetObject> object;

    _FORCE_INLINE_ bool is_non_object_type() const { return type != Variant::NIL && type != Variant::OBJECT; }
    _FORCE_INLINE_ bool is_class_type() const { return !class_name.is_empty(); }
    _FORCE_INLINE_ bool has_target_object() const { return !object.is_null() && object->has_target(); }
    _FORCE_INLINE_ StringName get_target_class() const { return object->get_target_class(); }
};

/// The base implementation of the OrchestratorGraphNode's pins.
///
/// An orchestration is made of up several resources, that together, are responsible for storing
/// the data associated with a visual-script graph. This class provides all the base functionality
/// for all OScriptNodePin types.
class OrchestratorGraphNodePin : public HBoxContainer
{
    GDCLASS(OrchestratorGraphNodePin, HBoxContainer);
    static void _bind_methods();

protected:
    enum ContextMenuIds
    {
        CM_BREAK_LINKS,
        CM_BREAK_LINK,
        CM_PROMOTE_TO_VARIABLE,
        CM_PROMOTE_TO_LOCAL_VARIABLE,
        CM_RESET_TO_DEFAULT,
        CM_REMOVE,
        CM_SELECT_NODES,
        CM_JUMP_NODE,
        CM_VIEW_DOCUMENTATION,
        CM_MAX,
        CM_CHANGE_PIN_TYPE = CM_MAX,
    };

    OrchestratorGraphNode* _node{ nullptr };    //! The owning node
    TextureRect* _icon{ nullptr };              //! The pin's icon
    Label* _label{ nullptr };                   //! The pin's label
    Control* _default_value{ nullptr };         //! The default value control
    PopupMenu* _context_menu{ nullptr };        //! The context menu
    Ref<OScriptNodePin> _pin;                   //! The script pin reference

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    OrchestratorGraphNodePin() = default;

    /// Get the connection color name to be used for this pin.
    /// @return the connection color name.
    virtual String _get_color_name() const;

    /// Updates the pin's label
    virtual void _update_label();

    /// Return whether to update the label on default value visibility change
    /// @return true if label is updated when default value widget visibility is toggled
    virtual bool _is_label_updated_on_default_value_visibility_change() { return false; }

    /// Creates the UI widgets for this specific pin.
    virtual void _create_widgets();

    /// Create the default value widget control
    /// @return the default value control, default is null
    virtual Control* _get_default_value_widget() { return nullptr; }

    /// Whether the default value should be rendered below the label.
    /// @return true to add default value below the label, defaults to false.
    virtual bool _render_default_value_below_label() const { return false; }

    /// Return whether the pin can be promoted to a variable
    /// @return true if the pin can be promoted, false otherwise
    virtual bool _can_promote_to_variable() const { return true; }

public:
    /// Constructs a new script node pin for the graph and node
    /// @param p_node the owning node, should not be null
    /// @param p_pin the pin reference, should be valid
    OrchestratorGraphNodePin(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);

    /// Handle GUI input events for the pin.
    /// @param p_event the input event
    void _gui_input(const Ref<InputEvent>& p_event) override;

    /// Checks whether the pin is an execution pin
    /// @return true if the pin is an execution pin; false otherwise
    _FORCE_INLINE_ bool is_execution() const { return _pin->is_execution(); }

    /// Get the associated graph
    /// @return the owning graph instance
    OrchestratorGraphEdit* get_graph();

    /// Get the associated graph node
    /// @return the owning graph node instance
    OrchestratorGraphNode* get_graph_node();

    /// Get the pin's color, based on its value type
    /// @return the pin's color
    Color get_color() const;

    /// Returns the unique slot type id
    /// @return slot type id
    virtual int get_slot_type() const;

    /// Returns the name of the slot icon to be used
    /// @return the slot icon name
    virtual String get_slot_icon_name() const;

    /// Examine pin/node paths and guess/resolve final type
    /// @return the type guess
    virtual ResolvedType resolve_type() const;

    /// Returns whether the pin is an input pin
    /// @return true if the pin is an input; false if its an output
    bool is_input() const;

    /// Returns whether the pin is an output pin
    /// @return true if the pin is an output; false if its an input
    bool is_output() const;

    /// Returns whether the pin can be connected or not
    /// @return whether the pin is connectable
    bool is_connectable() const;

    /// Returns whether this pin has at least one connection
    /// @return true if the pin is connected, false otherwise
    bool is_connected() const;

    /// Returns whether the pin is hidden
    /// @return true if the pin is hidden, false otherwise
    bool is_hidden() const;

    /// Checks whether this pin accepts connections from the associated pin
    /// @param p_pin the other pin
    /// @return true if a connection is possible, false otherwise
    bool can_accept(OrchestratorGraphNodePin* p_pin);

    /// Attempt to connect this pin with another (target) pin
    /// @param p_pin the other pin
    /// @return true if the connection was successful; false otherwise
    void link(OrchestratorGraphNodePin* p_pin);

    /// Disconnects this pin from the provided (target) pin
    /// @param p_pin the other pin
    void unlink(OrchestratorGraphNodePin* p_pin);

    /// Disconnects all pins with this pin
    void unlink_all();

    /// Return whether a coercion node is required to connect the two pins.
    /// @param p_other the other pin
    /// @return true if a coercion node is necessary; false otherwise
    bool is_coercion_required(OrchestratorGraphNodePin* p_other) const;

    /// Get the pin's underlying value type
    /// @return the value type the pin represents
    Variant::Type get_value_type() const;

    /// Get the pin's default value
    /// @return the default value
    Variant get_default_value() const;

    /// Sets the pin's new default value
    /// @param p_default_value the new default value
    void set_default_value(const Variant& p_default_value);

    /// Sets the visibility for the default value control
    /// @param p_visible whether the control is visible
    void set_default_value_control_visibility(bool p_visible);

    /// Whether the icons are shown
    /// @param p_visible whether to show the icon
    void show_icon(bool p_visible);

private:

    void _select_nodes_for_pin(const Ref<OScriptNodePin>& p_pin);
    void _select_nodes_for_pin(const Ref<OScriptNodePin>& p_pin, OrchestratorGraphNode* p_node);

    /// Removes the editable pin
    void _remove_editable_pin();

    /// Promote this pin to a variable
    void _promote_as_variable();

    /// Creates a new variable name
    /// @return the variable name
    String _create_promoted_variable_name();

    /// Creates the pin's rendered icon
    /// @param p_visible whether the icon is visible
    /// @return the icon texture rect
    TextureRect* _create_type_icon(bool p_visible);

    /// Updates the pin's tooltip text
    void _update_tooltip();

    /// Populates the context menu sub-menu for connection pin-based options
    /// @param p_id the sub-menu menu id
    /// @param p_prefix the sub-menu option prefix text
    /// @param p_menu the sub menu, never null
    /// @param p_pins vector of connections, can be empty
    void _populate_graph_node_in_sub_menu(int p_id, const String& p_prefix, PopupMenu* p_menu,
                                          const Vector<Ref<OScriptNodePin>>& p_pins);

    /// Show the pin's context menu
    /// @param p_position the position
    void _show_context_menu(const Vector2& p_position);

    /// Get a context menu's sub-menu item metadata
    /// @param p_menu_id sub-menu id
    /// @param p_id sub-menu selected id
    /// @return the item's metadata
    Variant _get_context_sub_menu_item_metadata(int p_menu_id, int p_id);

    /// Get the associated pin connection from a sub-menu choice.
    /// @param p_menu_id sub-menu id
    /// @param p_id sub-menu selected id
    /// @return the connection pin, could be invalid if the connection lookup fails
    Ref<OScriptNodePin> _get_connected_pin_by_sub_menu_metadata(int p_menu_id, int p_id);

    /// Handles the selection of a context menu item
    /// @param p_id the selected item id
    void _handle_context_menu(int p_id);

    /// Cleans up the context menu after it has closed
    void _cleanup_context_menu();

    /// Changes the pin's type to the selected type
    /// @param p_id the menu id of the new pin type
    void _change_pin_type(int p_id);

    /// Breaks or unlinks a pin's connection
    /// @param p_id the menu id of the pin to break
    void _link_pin(int p_id);

    /// Jumps to an adjacent node
    /// @param p_id the menu id of the node to jump to
    void _jump_to_adjacent_node(int p_id);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_H