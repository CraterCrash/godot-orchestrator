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
#ifndef ORCHESTRATOR_SCRIPT_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_FUNCTION_H

#include "common/guid.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OScriptGraph;
class OScriptNode;

/// An OrchestratorScript function resource
///
/// An Orchestration manages a collection of function definitions, of which can either refer
/// to a Godot function or a user-defined function. Each node that refers to the function
/// will hold a reference to this object.
///
/// This object should not be managed directly in the InspectorDock but rather the various nodes
/// that hold a reference should act as a delegate for managing the function's state.
///
class OScriptFunction : public Resource
{
    friend class Orchestration;

    GDCLASS(OScriptFunction, Resource);
    static void _bind_methods() { }

    Orchestration* _orchestration{ nullptr };  //! Owning orchestration
    Guid _guid;                                //! Unique function id
    MethodInfo _method;                        //! The function definition
    bool _user_defined{ false };               //! Whether function is user-defined
    int _owning_node_id{ -1 };                 //! Owning node id
    bool _returns_value{ false };              //! Whether the function returns a value
    String _description;                       //! Function description

protected:
    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value);
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Constructor
    /// Intentionally protected, functions created via an Orchestration
    OScriptFunction() = default;

public:
    /// Get the function's name
    /// @return the function name
    const StringName& get_function_name() const;

    /// Check whether the function name can be renamed.
    /// @return true if the function name can be renamed, false otherwise
    bool can_be_renamed() const;

    /// Rename the function
    /// @param p_new_name the new function name
    void rename(const StringName& p_new_name);

    /// Get the function's globally unique id
    /// @return the GUID for the function
    const Guid& get_guid() const;

    /// Get the function's method information structure
    /// @return a Godot MethodInfo struct that describes the function
    const MethodInfo& get_method_info() const;

    /// Get whether the function is user-defined or a built-in Godot function
    /// @return true if the function is user-defined, false if its built-in
    bool is_user_defined() const;

    /// Get a reference to the orchestration that owns this function.
    /// @return the owning orchestration reference, should always be valid
    Orchestration* get_orchestration() const;

    /// Get script node id that owns this function.
    /// @return the owning script node id
    int get_owning_node_id() const;

    /// Get the script node that owns this function.
    ///
    /// Using the get_owning_node_id will be more efficient if only the id is needed and the node
    /// script node instance is not required. If the node instance is required, this method will
    /// typically be better than other alternatives.
    ///
    /// @return the owning script node reference, should always be valid.
    Ref<OScriptNode> get_owning_node() const;

    /// Get the function's first return node, if any exist.
    /// @return the first return node, or an invalid reference if none exist
    Ref<OScriptNode> get_return_node() const;

    /// Get the function's return nodes, if any exist
    /// @return a vector of return nodes, may be empty
    Vector<Ref<OScriptNode>> get_return_nodes() const;

    /// Get the function graph
    /// @return the graph this function is associated with
    Ref<OScriptGraph> get_function_graph() const;

    /// Return the function definition as a Dictionary that contains a MethodInfo definition.
    /// In addition, the dictionary will include two custom properties,  "_oscript_guid" and
    /// the "_oscript_owning_node_id" values as well.
    /// @return a dictionary
    Dictionary to_dict() const;

    /// Get the number of function arguments
    /// @return the number of arguments
    size_t get_argument_count() const;

    /// Resizes the argument list to the specified size.
    ///
    /// When resizing the argument list so that it grows, new arguments will be added with a
    /// type of Variant::NIL and a name of "arg#" where the hash is the index in the list of
    /// arguments. When shrinking the list, arguments at the end of the list are removed.
    ///
    /// NOTE: Built-in functions do not allow argument resizing, so this method will return
    /// false if the method is not user-defined. Additionally, if the new size matches the
    /// existing size of the argument list, the method also will return false too.
    ///
    /// @param p_new_size the new argument list size
    /// @return true if the argument list was resized, false if it was not.
    bool resize_argument_list(size_t p_new_size);

    /// Allows changing the argument name based on the supplied argument list index.
    /// @param p_index the argument list index to change
    /// @param p_name the new argument name
    void set_argument_name(size_t p_index, const StringName& p_name);

    /// Allows changing the argument type based on the supplied argument list index.
    /// @param p_index the argument list index to change
    /// @param p_type the new argument type
    void set_argument_type(size_t p_index, Variant::Type p_type);

    /// Utility function to check whether the function returns a value. This is determined by
    /// looking at the Variant::Type in the method's return definition and if it is NIL, then
    /// it's considered to have a void (or no) return value.
    /// @return true if there is a return value, false otherwise
    bool has_return_type() const;

    /// Get the return value type
    /// @return the return value type
    Variant::Type get_return_type() const;

    /// Set the return value type for user-defined functions. Built-in functions act as a no-op.
    /// @param p_type the new return value type
    void set_return_type(Variant::Type p_type);

    /// Sets whether the function has a return value
    /// @param p_has_return_value value true if the function has a return value, false otherwise
    void set_has_return_value(bool p_has_return_value);

    /// Get the description
    /// @return the description
    String get_description() const { return _description; }

    /// Sets the description
    /// @param p_description the description
    void set_description(const String& p_description);
};

#endif  // ORCHESTRATOR_SCRIPT_FUNCTION_H