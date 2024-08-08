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
#ifndef ORCHESTRATOR_NODE_PROPERTY_H
#define ORCHESTRATOR_NODE_PROPERTY_H

#include "script/script.h"

using namespace godot;

/// An abstract script node for all property operations
///
/// By default, property nodes operate on the script and attached node in a "SELF" based
/// capacity; however, there are other call modes that should be supported, including:
///
///     Node Paths
///     Instance
///
/// In these cases a new input pin is created to set the incoming node path or the object
/// instance that should be used as the source for setting or getting the property from.
///
class OScriptNodeProperty : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeProperty, OScriptNode);
    static void _bind_methods();

public:
    enum CallMode : uint32_t
    {
        CALL_SELF,
        CALL_INSTANCE,
        CALL_NODE_PATH
    };

protected:
    CallMode _call_mode{ CALL_SELF };
    StringName _base_type;
    NodePath _node_path;
    PropertyInfo _property;
    bool _has_property{ false };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Checks whether a property exists with the given name in the array
    /// @param p_properties an array of properties
    /// @return true if the property exists, false otherwise
    bool _property_exists(const TypedArray<Dictionary>& p_properties) const;

    /// Gets the property list for a given class name
    /// @param p_class_name the class name
    /// @return array of properties
    TypedArray<Dictionary> _get_class_property_list(const String& p_class_name) const;

    /// Gets the class property if found
    /// @param p_class_name the class name
    /// @param p_name the property name
    /// @param r_property the returned property structure
    /// @return true if found, false otherwise
    bool _get_class_property(const String& p_class_name, const String& p_name, PropertyInfo& r_property);

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    OScriptNodeProperty();

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    String get_icon() const override;
    String get_node_title_color_name() const override { return "properties"; }
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface
};

VARIANT_ENUM_CAST(OScriptNodeProperty::CallMode)

#endif  // ORCHESTRATOR_NODE_PROPERTY_H