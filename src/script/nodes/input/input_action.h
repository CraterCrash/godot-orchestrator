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
#ifndef ORCHESTRATOR_SCRIPT_NODE_INPUT_ACTION_H
#define ORCHESTRATOR_SCRIPT_NODE_INPUT_ACTION_H

#include "script/script.h"

/// Allows checking whether an input action is pressed, released, or recently pressed or released.
class OScriptNodeInputAction : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeInputAction, OScriptNode);
    static void _bind_methods();

public:
    // Various action modes
    enum ActionMode {
        AM_PRESSED,
        AM_RELEASED,
        AM_JUST_PRESSED,
        AM_JUST_RELEASED
    };

protected:
    String _action_name;
    int _mode{ AM_PRESSED };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo> *r_list) const;
    bool _get(const StringName &p_name, Variant &r_value) const;
    bool _set(const StringName &p_name, const Variant &p_value);
    //~ End Wrapped Interface

    /// Called when the project settings are modified.
    void _settings_changed();

    PackedStringArray _get_action_names() const;
    String _get_mode() const;

public:

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

};

VARIANT_ENUM_CAST(OScriptNodeInputAction::ActionMode)

#endif // ORCHESTRATOR_SCRIPT_NODE_INPUT_ACTION_H