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
#ifndef ORCHESTRATOR_SCRIPT_NODE_ENGINE_SINGLETON_H
#define ORCHESTRATOR_SCRIPT_NODE_ENGINE_SINGLETON_H

#include "script/script.h"

/// Provides a reference to a specific engine singleton object.
class OScriptNodeEngineSingleton : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeEngineSingleton, OScriptNode);

    String _singleton{ "Engine" };  //! Name of the singleton

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo> *r_list) const;
    bool _get(const StringName &p_name, Variant &r_value) const;
    bool _set(const StringName &p_name, const Variant &p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "variable"; }
    String get_help_topic() const override;
    String get_icon() const override;
    PackedStringArray get_keywords() const override;
    StringName resolve_type_class(const Ref<OScriptNodePin>& p_pin) const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface

    String get_singleton_name() const { return _singleton; }
};

#endif // ORCHESTRATOR_SCRIPT_NODE_ENGINE_SINGLETON_H