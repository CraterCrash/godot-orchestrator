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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_STATIC_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_STATIC_FUNCTION_H

#include "script/node.h"

class OScriptNodeCallStaticFunction : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallStaticFunction, OScriptNode);
    static void _bind_methods() { }

protected:
    StringName _class_name;
    StringName _method_name;
    MethodInfo _method;
    GDExtensionMethodBindPtr _method_bind{ nullptr };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    void _resolve_method_info();

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override { return "MemberMethod"; }
    void validate_node_during_build(BuildLog& p_log) const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

#endif // ORCHESTRATOR_SCRIPT_NODE_CALL_STATIC_FUNCTION_H