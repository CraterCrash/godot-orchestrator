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
#ifndef ORCHESTRATOR_SCRIPT_NODE_PIN_H
#define ORCHESTRATOR_SCRIPT_NODE_PIN_H

#include "common/guid.h"
#include "script/target_object.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OScriptNode;

/// Pin direction
///
/// A pin can either represent an input, where data or control flow enters the
/// owning node or an output, where data or control flow exits the node.
enum EPinDirection : int {
    PD_Input,
    PD_Output,
    PD_MAX
};

/// Pin Type
///
/// A pin can either represent an execution or control flow where the execution of
/// the graph is controlled by these connections, or a data pin where the pin is
/// responsible for accepting or passing data between nodes.
enum EPinType : int {
    PT_Execution,
    PT_Data,
    PT_MAX
};

/// Node Pin
///
/// This class represents a connection point on a given script node. We utilize a pin class
/// to centralize behaviors around pins and their connections.
class OScriptNodePin : public Resource {
    GDCLASS(OScriptNodePin, Resource);

    friend class OScriptNode;

public:

    /// Pin flags
    ///
    /// These flags control the state that a pin should be used, both at runtime, and
    /// within the editor's UI. This allows custom nodes to control precisely how a
    /// node and its pined are presented and used.
    enum Flags {
        NONE            = 1 << 0,   //! Typically not used, just a placeholder
        DATA            = 1 << 1,   //! Pin allows connections to/from data pins
        EXECUTION       = 1 << 2,   //! Pin allows connections to/from execution pins
        IGNORE_DEFAULT  = 1 << 3,   //! Don't render default value, even if one exists
        READ_ONLY       = 1 << 4,   //! Not connectable and default value is immutable
        HIDDEN          = 1 << 5,   //! Pin should not be rendered
        ORPHANED        = 1 << 6,   //! Pin was orphaned, can happen in upgrades
        ADVANCED        = 1 << 7,   //! Pin is marked as advanced (no used atm)
        NO_CONNECTION   = 1 << 8,   //! No connection is permissible to the pin
        SHOW_LABEL      = 1 << 9,   //! Label should be shown, always
        HIDE_LABEL      = 1 << 10,  //! Label should be hidden, always
        NO_CAPITALIZE   = 1 << 11,  //! Label is not capitalized
        NO_AUTOWIRE     = 1 << 12,  //! Prevents being autowired
        CONST           = 1 << 20,  //! Represents a "const" data port
        REFERENCE       = 1 << 21,  //! Represents a "reference" data port
        OBJECT          = 1 << 22,  //! Refers to an object type
        FILE            = 1 << 23,  //! Should allow the user to select a file
        MULTILINE       = 1 << 24,  //! Text should be rendered using a TextEdit rather than LineEdit
        ENUM            = 1 << 25,  //! Target class holds the name of the enum
        BITFIELD        = 1 << 26,  //! Target class holds a bitfield
    };

private:
    PropertyInfo _property;                    //! Pin's property details
    String _target_class;                      //! The target class associated with the pin
    Variant _default_value;                    //! The default value
    Variant _generated_default_value;          //! Generated default value
    EPinDirection _direction = PD_Input;       //! The direction
    BitField<Flags> _flags = 0;                //! Pin flags
    String _label;                             //! A custom label name
    OScriptNode* _owning_node = nullptr;       //! The node that owns this pin
    bool _set_type_resets_default = false;     //! Whether changing the type resets the default value
    bool _valid = true;                        //! Indicates if the pin is valid
    int _cached_pin_index = -1;                //! Cached pin index calculated after pins added to node

protected:
    static void _bind_methods();

    /// Creates a pin for the specified node.
    /// @param p_owning_node the owning node
    /// @param p_property the property info for the pin
    /// @return the script pin reference
    static Ref<OScriptNodePin> create(OScriptNode* p_owning_node, const PropertyInfo& p_property);

    /// Clears a specific flag on the pin
    /// @param p_flag the flag to clear
    void _clear_flag(Flags p_flag);

    /// Loads the pin data from the provided dictionary.
    /// @param p_data the pin data
    /// @return true if the pin was loaded successfully, false otherwise
    bool _load(const Dictionary& p_data);

    /// Saves the pin's data into a dictionary.
    /// @return the dictionary of pin data
    Dictionary _save();

    Vector2 _calculate_midpoint_between_nodes(const Ref<OScriptNode>& p_source, const Ref<OScriptNode>& p_target) const;

public:

    /// Perform pin post initialization
    virtual void post_initialize();

    /// Helper method to create a pin for the specified node
    /// @param p_owning_node the node that owns this pin
    /// @return the script pin reference
    static Ref<OScriptNodePin> create(OScriptNode* p_owning_node);

    /// Return whether the pin is valid
    /// @return true if the pin is valid; false otherwise.
    bool is_valid() const { return _valid; }

    /// Get the owning Orchestrator script node
    /// @return the orchestrator script node
    OScriptNode* get_owning_node() const;

    /// Set the script node that owns this pin
    /// @param p_owning_node the owning script node
    void set_owning_node(OScriptNode* p_owning_node);

    /// Get the pin's slot index
    /// @return the slot index
    int32_t get_pin_index() const;

    /// Get the pin's property info
    /// @return an immutable property info that describes the pin
    const PropertyInfo& get_property_info() const { return _property; }

    /// Get the pin's name
    /// @return the pin's name
    StringName get_pin_name() const;

    /// Set the pin's name
    /// @param p_pin_name the pin's name
    void set_pin_name(const StringName &p_pin_name);

    /// Get the pin's type
    Variant::Type get_type() const;

    /// Set the pin type
    /// @param p_type the pin type
    void set_type(Variant::Type p_type);

    /// Get the pin's type name
    /// @return the pin type name
    String get_pin_type_name() const;

    /// Get the target class name
    /// @return the target class name
    StringName get_target_class() const;

    /// Set the target class name
    /// @param p_target_class the target class name
    void set_target_class(const StringName &p_target_class);

    /// Get the default value
    /// @return the default value
    Variant get_default_value() const;

    /// Set the default value
    /// @param p_default_value the default value
    void set_default_value(const Variant &p_default_value);

    /// Resets the default value to the initial, generated value.
    void reset_default_value();

    /// Get the generated default value
    /// @return the generated default value
    Variant get_generated_default_value() const;

    /// Set the generated default value
    /// @param p_default_value the generated default value
    void set_generated_default_value(const Variant &p_default_value);

    /// Get the effective default value. The effective default value is resolved by first
    /// checking whether a default value is set and if so, returning it. If no default is
    /// set but a generated default exists, the generated default is returned instead.
    Variant get_effective_default_value() const;

    /// Get the pin direction.
    /// @return the pin direction
    EPinDirection get_direction() const;

    /// Set the pin direction
    /// @param p_direction the pin direction
    void set_direction(EPinDirection p_direction);

    /// Get the complimentary direction for this pin.
    /// In other words, if this pin is an input, returns output and vice versa.
    EPinDirection get_complimentary_direction() const;

    // Utility methods for checking pin direction
    _FORCE_INLINE_ bool is_input() const { return _direction == PD_Input; }
    _FORCE_INLINE_ bool is_output() const { return _direction == PD_Output; }

    /// Set a specific flag on the pin
    /// @param p_flag the flag to set
    /// @deprecated use specific helper methods instead
    void set_flag(Flags p_flag);

    /// Get the pin's label
    /// @return the pin's label
    String get_label() const;

    /// Set the pin's label
    /// @param p_label the label
    /// @param p_pretty_format whether the label should be formatted using the pretty algorithm
    void set_label(const String &p_label, bool p_pretty_format = true);

    /// Shows the label for the pin
    void show_label();

    /// Marks the label for the pin to be hidden
    void hide_label();

    /// Toggles pretty format of labels off
    void no_pretty_format();

    /// Set the file types associated with a file pin
    /// @param p_file_types the file types
    void set_file_types(const String &p_file_types);

    /// Get the file types for a file pin
    String get_file_types() const;

    /// Checks whether this pin can be connected with the supplied pin.
    /// @param p_pin the other pin
    /// @return true if the two pins can be connected, false otherwise
    bool can_accept(const Ref<OScriptNodePin> &p_pin) const;

    /// Link this (source) pin with the other (target) pin.
    /// @param p_pin the target pin
    void link(const Ref<OScriptNodePin>& p_pin);

    /// Unlink this pin with another pin.
    /// @param p_pin the other pin.
    void unlink(const Ref<OScriptNodePin>& p_pin);

    /// Unlink this pin from all connections
    /// @param p_notify_nodes whether to notify nodes of changes, defauls to false.
    void unlink_all(bool p_notify_nodes = false);

    /// Check whether this pin has any connections
    /// @return true if there are connections, false otherwise
    bool has_any_connections() const;

    /// Get the connections to this pin.
    /// @return the connected pins
    Vector<Ref<OScriptNodePin>> get_connections() const;

    /// Get the target connection for this pin.
    ///
    /// For control flow, there is always one output pin, while for data flow pins there is always one input.
    /// This utility method checks the pin's direction and returns the single connection if one exists, or it
    /// will throw an error if the direction and use case mismatch, with an invalid reference.
    /// @return the singular connected pin
    Ref<OScriptNodePin> get_connection() const;

    /// Return whether this pin is hidden.
    /// @return true if the pin is hidden, false otherwise
    _FORCE_INLINE_ bool is_hidden() const { return _flags.has_flag(HIDDEN); }

    /// Return whether this pin acts as an execution pin.
    /// @return true if the pin is a control flow, execution pin, false otherwise
    _FORCE_INLINE_ bool is_execution() const { return _flags.has_flag(EXECUTION); }

    /// Return whether this pin acts as a file selection pin.
    /// @return true if the pin should be rendered as a file selector
    _FORCE_INLINE_ bool is_file() const { return _flags.has_flag(FILE); }

    /// Return whether this pin acts as an enumeration
    /// @return true if this pin is an enumeration, false otherwise
    _FORCE_INLINE_ bool is_enum() const { return _flags.has_flag(ENUM); }

    /// Return whether this pin acts as a bitfield
    /// @return true if this pin is an enumeration, false otherwise
    _FORCE_INLINE_ bool is_bitfield() const { return _flags.has_flag(BITFIELD); }

    /// Return whether this pin is rendered as multi-lined text.
    /// @return true if this is a multi-lined text pin, false otherwise
    _FORCE_INLINE_ bool is_multiline_text() const { return _flags.has_flag(MULTILINE); }

    /// Return whether to default field is ignored and unused (not rendered)
    /// @return true if the default field is ignored/unused, false otherwise
    _FORCE_INLINE_ bool is_default_ignored() const { return _flags.has_flag(IGNORE_DEFAULT); }

    /// Returns whether the pin permits connections.
    /// @return true if the pin is connectable, false otherwise
    _FORCE_INLINE_ bool is_connectable() const { return !_flags.has_flag(NO_CONNECTION); }

    /// Returns whether to render pin labels with pretty formatting
    /// @return true to use pretty formatting, false to render labels/names as-is.
    _FORCE_INLINE_ bool use_pretty_labels() const { return !_flags.has_flag(NO_CAPITALIZE); }

    /// Return whether the pin can be autowired
    /// @return true if the pin can be autowired, false otherwise
    _FORCE_INLINE_ bool can_autowire() const { return !_flags.has_flag(NO_AUTOWIRE); }

    /// Return whether the label is visible for this pin
    /// @return true if the label is visible, false otherwise
    bool is_label_visible() const;

    /// Attempts to resolve the target object of this pin.
    /// @return the target object of the pin or {@code nullptr} if there is no target.
    Ref<OScriptTargetObject> resolve_target();

    /// Resolves signal names for pin's connected object. Only applicable for input pins.
    /// @param p_self_fallback whether to get signal names from script's node as a fallback
    /// @return signal names list
    PackedStringArray resolve_signal_names(bool p_self_fallback = false) const;

    /// Get UI suggestions for this pin
    /// @return suggestions to show, or an empty list if there are no suggestions
    PackedStringArray get_suggestions();

    /// Return whether the target of this pin is self
    /// @return true if target is self, false otherwise
    bool is_target_self() const;
};

VARIANT_ENUM_CAST(EPinDirection);
VARIANT_ENUM_CAST(EPinType);
VARIANT_BITFIELD_CAST(OScriptNodePin::Flags)

typedef OScriptNodePin OrchestrationGraphPin;

#endif  // ORCHESTRATOR_SCRIPT_NODE_PIN_H