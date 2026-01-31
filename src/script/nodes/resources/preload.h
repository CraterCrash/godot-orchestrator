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
#ifndef ORCHESTRATOR_SCRIPT_NODE_PRELOAD_H
#define ORCHESTRATOR_SCRIPT_NODE_PRELOAD_H

#include "script/script.h"

/// Preloads a resource
class OScriptNodePreload : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodePreload, OScriptNode);

    String _resource_path;    //! The resource path

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    StringName _get_resource_class_name() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "resources"; }
    String get_icon() const override;
    StringName resolve_type_class(const Ref<OScriptNodePin>& p_pin) const override;
    Ref<OScriptTargetObject> resolve_target(const Ref<OScriptNodePin>& p_pin) const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface

    String get_resource_path() const { return _resource_path; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_PRELOAD_H
