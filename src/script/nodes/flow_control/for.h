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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FOR_LOOP_H
#define ORCHESTRATOR_SCRIPT_NODE_FOR_LOOP_H

#include "script/script.h"

/// Provides a basic for-loop construct based on start/end index values.
/// These start/end index values can be supplied by a connecting node.
///
class OScriptNodeForLoop : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeForLoop, OScriptNode);
    static void _bind_methods() { }

protected:
    bool _with_break{ false };  //! Whether break is enabled
    int _start_index{ 0 };      //! Starting index
    int _end_index{ 10000 };    //! Ending index

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Set whether the break pin is used.
    /// @param p_break_status true if the break pin is visible, false otherwise
    void _set_with_break(bool p_break_status);

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override;
    PackedStringArray get_keywords() const override { return Array::make("for", "loop"); }
    bool is_loop_port(int p_port) const override;
    void get_actions(List<Ref<OScriptAction>>& p_action_list) override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_FOR_LOOP_H
