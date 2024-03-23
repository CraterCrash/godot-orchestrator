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
#ifndef ORCHESTRATOR_SCRIPT_NODE_PIN_H
#define ORCHESTRATOR_SCRIPT_NODE_PIN_H

#include "action.h"
#include "common/guid.h"

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
enum EPinDirection : int
{
    PD_Input,
    PD_Output,
    PD_MAX
};

VARIANT_ENUM_CAST(EPinDirection)

/// Node Pin
///
/// This class represents a connection point on a given script node. We utilize a pin class
/// to centralize behaviors around pins and their connections.
class OScriptNodePin : public Resource
{
    GDCLASS(OScriptNodePin, Resource);

public:

    /// Pin flags
    ///
    /// These flags control the state that a pin should be used, both at runtime, and
    /// within the editor's UI. This allows custom nodes to control precisely how a
    /// node and its pined are presented and used.
    enum Flags
    {
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
    };

private:
    friend class OScriptNode;

    StringName _pin_name;                      //! The pin's name, must be unique.
    Variant::Type _type{ Variant::NIL };       //! The type associated with the pin
    String _target_class;                      //! The target class associated with the pin
    Variant _default_value;                    //! The default value
    Variant _generated_default_value;          //! Generated default value
    EPinDirection _direction{ PD_Input };      //! The direction
    BitField<Flags> _flags{ 0 };               //! Pin flags
    String _label;                             //! A custom label name
    String _file_types;                        //! File dialog types
    OScriptNode* _owning_node{ nullptr };      //! The node that owns this pin
    bool _set_type_resets_default{ false };    //! Whether changing the type resets the default value
    int _cached_pin_index{ -1 };               //! Cached pin index calculated after pins added to node

protected:
    static void _bind_methods();

    /// Loads the pin data from the provided dictionary.
    /// @param p_data the pin data
    /// @return true if the pin was loaded successfully, false otherwise
    bool _load(const Dictionary& p_data);

    /// Saves the pin's data into a dictionary.
    /// @return the dictionary of pin data
    Dictionary _save();

    Vector2 _calculate_midpoint_between_nodes(const Ref<OScriptNode>& p_source, const Ref<OScriptNode>& p_target) const;

    /// Creates an intermediate node
    /// @return the constructed intermediate node
    Ref<OScriptNode> _create_intermediate_node(Variant::Type p_source_type, Variant::Type p_target_type) const;

public:

    /// Perform pin post initialization
    virtual void post_initialize();

    /// Helper method to create a pin for the specified node
    /// @param p_owning_node the node that owns this pin
    /// @return the script pin reference
    static Ref<OScriptNodePin> create(OScriptNode* p_owning_node);

    /// Get the owning Orchestrator script node
    /// @return the orchestrator script node
    OScriptNode* get_owning_node() const;

    /// Set the script node that owns this pin
    /// @param p_script the owning script node
    void set_owning_node(OScriptNode* p_owning_node);

    /// Get the pin's slot index
    /// @return the slot index
    int32_t get_pin_index() const;

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

    /// Get the pin flags
    /// @return the flags
    BitField<Flags> get_flags() const;

    /// Set the pin flags
    /// @param p_flags the pin flags
    void set_flags(BitField<Flags> p_flags);

    /// Get the pin's label
    /// @return the pin's label
    String get_label() const;

    /// Set the pin's label
    /// @param p_label the label
    void set_label(const String &p_label);

    /// Set the file types associated with a file pin
    /// @param p_file_types the file types
    void set_file_types(const String &p_file_types) { _file_types = p_file_types; }

    /// Get the file types for a file pin
    String get_file_types() const { return _file_types; }

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

    /// Return whether this pin acts as an execution pin.
    /// @return true if the pin is a control flow, execution pin, false otherwise
    _FORCE_INLINE_ bool is_execution() const { return get_flags().has_flag(OScriptNodePin::Flags::EXECUTION); }

    /// Attempts to resolve the target object of this pin.
    /// @return the target object of the pin or {@code nullptr} if there is no target.
    Object* resolve_target();
};

VARIANT_BITFIELD_CAST(OScriptNodePin::Flags)

#endif  // ORCHESTRATOR_SCRIPT_NODE_PIN_H