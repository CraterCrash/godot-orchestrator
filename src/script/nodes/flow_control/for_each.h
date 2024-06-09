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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FOR_EACH_H
#define ORCHESTRATOR_SCRIPT_NODE_FOR_EACH_H

#include "script/script.h"

/// Provides a basic for-each loop over an array where the start/end indices
/// are based on the collection's size.
class OScriptNodeForEach : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeForEach, OScriptNode);

protected:
    bool _with_break{ false };  //! Whether break is enabled

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
    void allocate_default_pins() override;
    void post_initialize() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override;
    void get_actions(List<Ref<OScriptAction>>& p_action_list) override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_FOR_EACH_H
