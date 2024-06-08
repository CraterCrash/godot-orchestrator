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
#ifndef ORCHESTRATOR_SCRIPT_SIGNALS_H
#define ORCHESTRATOR_SCRIPT_SIGNALS_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;

/// Defines a script signal.
///
/// An orchestrator script can define any number of signals and we utilize a resource to be
/// able to easily allow the user to edit it within the Editor InspectorDock but to also
/// serialize the data to and from disk.
///
class OScriptSignal : public Resource
{
    friend class Orchestration;

    GDCLASS(OScriptSignal, Resource);
    static void _bind_methods() { }

    Orchestration* _orchestration{ nullptr };  //! Owning Orchestration
    MethodInfo _method;                        //! The signal definition
    String _description;                       //! Signal description

protected:
    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value);
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Constructor
    /// Intentionally protected, signals created via an Orchestration
    OScriptSignal() = default;

public:
    /// Get a reference to the orchestration that owns this signal.
    /// @return the owning orchestration reference, should always be valid
    Orchestration* get_orchestration() const;

    /// Get the signal name
    /// @return the signal name
    const StringName& get_signal_name() const;

    /// Rename the signal
    /// @param p_new_name the new signal name
    void rename(const StringName& p_new_name);

    /// Get the signal's method information structure
    /// @return a Godot MethodInfo struct that describes the signal
    const MethodInfo& get_method_info() const;

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

    /// Get the description
    /// @return the description
    String get_description() const { return _description; }

    /// Sets the description
    /// @param p_description the description
    void set_description(const String& p_description);
};

#endif  // ORCHESTRATOR_SCRIPT_SIGNALS_H
