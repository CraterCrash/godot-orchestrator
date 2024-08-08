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
#ifndef ORCHESTRATOR_SCRIPT_NODE_MEMORY_H
#define ORCHESTRATOR_SCRIPT_NODE_MEMORY_H

#include "script/script.h"

/// Creates a new instance of a Godot class
class OScriptNodeNew : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeNew, OScriptNode);
    static void _bind_methods();

protected:
    String _class_name;

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "memory"; }
    String get_help_topic() const override;
    String get_icon() const override;
    StringName resolve_type_class(const Ref<OScriptNodePin>& p_pin) const override { return _class_name; }
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    OScriptNodeNew();
};

/// Destroys an instance of a Godot class
class OScriptNodeFree : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeFree, OScriptNode);
    static void _bind_methods();

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "memory"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    OScriptNodeFree();
};

#endif // ORCHESTRATOR_SCRIPT_NODE_MEMORY_H