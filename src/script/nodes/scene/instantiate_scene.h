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
#ifndef ORCHESTRATOR_SCRIPT_NODE_INSTANTIATE_SCENE_H
#define ORCHESTRATOR_SCRIPT_NODE_INSTANTIATE_SCENE_H

#include "script/script.h"

/// Instantiates the specified scene
class OScriptNodeInstantiateScene : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeInstantiateScene, OScriptNode);
    static void _bind_methods() { }

protected:
    String _scene;

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Instantiates the scene
    /// @return the root scene node, or null if the scene cannot be instantiated
    Node* _instantiate_scene() const;

public:

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "scene"; }
    String get_icon() const override;
    void pin_default_value_changed(const Ref<OScriptNodePin>& p_pin) override;
    StringName resolve_type_class(const Ref<OScriptNodePin>& p_pin) const override;
    Ref<OScriptTargetObject> resolve_target(const Ref<OScriptNodePin>& p_pin) const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface
};

#endif // ORCHESTRATOR_SCRIPT_NODE_INSTANTIATE_SCENE_H
