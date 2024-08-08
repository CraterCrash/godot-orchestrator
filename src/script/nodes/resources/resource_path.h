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
#ifndef ORCHESTRATOR_SCRIPT_NODE_RESOURCE_PATH_H
#define ORCHESTRATOR_SCRIPT_NODE_RESOURCE_PATH_H

#include "script/script.h"

/// A simple node that provides a reference to a resource path
class OScriptNodeResourcePath : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeResourcePath, OScriptNode);
    static void _bind_methods() { }

protected:
    String _path;  //! The resource path

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "resources"; }
    bool should_draw_as_bead() const override { return true; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_RESOURCE_PATH_H
