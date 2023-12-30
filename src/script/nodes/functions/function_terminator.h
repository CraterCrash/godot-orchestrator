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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FUNCTION_TERMINATOR_H
#define ORCHESTRATOR_SCRIPT_NODE_FUNCTION_TERMINATOR_H

#include "script/script.h"

/// A terminal node for an event or function call
class OScriptNodeFunctionTerminator : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeFunctionTerminator, OScriptNode);

protected:
    Guid _guid;                      //! Function guid
    Ref<OScriptFunction> _function;  //! Function reference
    bool _return_value{ false };     //! Whether the function has a return value

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    void _validate_property(PropertyInfo& p_property) const;
    //~ End Wrapped Interface

    /// Returns whether the terminator node supports return values.
    /// @return true by default, but derived node classes can override this behavior.
    virtual bool _supports_return_values() const { return true; }

    /// Callback when the underlying function is changed.
    void _on_function_changed();

    /// Creates the pins for the function entry/exit node
    /// @param p_function the function
    /// @param p_function_entry true if creating pins for entry, false for exit
    bool create_pins_for_function_entry_exit(const Ref<OScriptFunction>& p_function, bool p_function_entry);

public:
    //~ Begin OScriptNode overrides
    void post_initialize() override;
    void post_placed_new_node() override;
    String get_node_title_color_name() const override { return "function_terminator"; }
    //~ End OScriptNode overrides

    /// Get the function reference
    Ref<OScriptFunction> get_function() const { return _function; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_FUNCTION_TERMINATOR_H
