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
#ifndef ORCHESTRATOR_NODE_EVENT_H
#define ORCHESTRATOR_NODE_EVENT_H

#include "script/nodes/functions/function_entry.h"

using namespace godot;

/// Script node that represents an event handler
///
/// In Godot, there are numerous built-in events such as '_ready' or '_process'
/// and this node simulates those.
///
class OScriptNodeEvent : public OScriptNodeFunctionEntry
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeEvent, OScriptNodeFunctionEntry);
    static void _bind_methods() { }

protected:

    //~ Begin OScriptNodeFunctionTerminator Interface
    bool _is_inputs_outputs_mutable() const override { return false; }
    //~ End OScriptNodeFunctionTerminator Interface

    //~ Begin OScriptNodeFunctionEntry Interface
    bool _is_user_defined() const override { return false; }
    //~ End OScriptNodeFunctionEntry Interface

public:

    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "events"; }
    String get_help_topic() const override;
    String get_icon() const override { return "PlayStart"; }
    bool can_user_delete_node() const override { return true; }
    bool can_inspect_node_properties() const override { return true; }
    bool can_duplicate() const override { return false; }
    StringName resolve_type_class(const Ref<OScriptNodePin>& p_pin) const override;
    //~ End OScriptNode Interface

    /// Checks whether the supplied method is an event-based method.
    /// @param p_method the method
    /// @return true if the method is for an event callback, false otherwise
    static bool is_event_method(const MethodInfo& p_method);
};

#endif  // ORCHESTRATOR_NODE_EVENT_H