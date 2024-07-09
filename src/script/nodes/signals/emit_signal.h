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
#ifndef ORCHESTRATOR_SCRIPT_NODE_EMIT_SIGNAL_H
#define ORCHESTRATOR_SCRIPT_NODE_EMIT_SIGNAL_H

#include "script/script.h"

/// Emits one of the script-defined signals.
///
/// Like GDScript, a user can defined a custom signal within the Orchestration and this
/// node raises that signal in the same way that "emit_signal" does in code.
///
/// NOTE: This node does not serialize the arguments for the signal, those are saved by
/// the main Orchestration script. Instead, this node maintains a reference to the
/// signal name and dynamically looks up the signal arguments, mimics the same behavior
/// as OScriptNodeCallFunction.
///
class OScriptNodeEmitSignal : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeEmitSignal, OScriptNode);
    static void _bind_methods() { }

protected:
    Ref<OScriptSignal> _signal;     //! Signal reference
    String _signal_name;            //! Signal name reference

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName &p_name, Variant &r_value) const;
    bool _set(const StringName &p_name, const Variant &p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Called when the underlying signal resource is modified.
    void _on_signal_changed();

public:

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "signals"; }
    bool can_inspect_node_properties() const override;
    Ref<Resource> get_inspect_object() override { return _signal; }
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    /// Get the associated signal object
    /// @return the signal object
    Ref<OScriptSignal> get_signal() const { return _signal; }

};

#endif  // ORCHESTRATOR_SCRIPT_NODE_EMIT_SIGNAL_H
