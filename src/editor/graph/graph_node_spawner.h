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
#ifndef ORCHESTRATOR_GRAPH_NODE_SPAWNER_H
#define ORCHESTRATOR_GRAPH_NODE_SPAWNER_H

#include "actions/action_menu_filter.h"

/// Base class for all OrchestratorGraphNode spawner action handlers
class OrchestratorGraphNodeSpawner : public OrchestratorGraphActionHandler
{
    GDCLASS(OrchestratorGraphNodeSpawner, OrchestratorGraphActionHandler);

    static void _bind_methods() { }

    static bool _has_all_filter_keywords(const Vector<String>& p_filter_keywords, const PackedStringArray& p_values);

    void test() {}

public:
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerProperty : public OrchestratorGraphNodeSpawner
{
    GDCLASS(OrchestratorGraphNodeSpawnerProperty, OrchestratorGraphNodeSpawner);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerProperty() = default;

    PropertyInfo _property;
    NodePath _node_path;
    Vector<StringName> _target_classes;

public:
    OrchestratorGraphNodeSpawnerProperty(const PropertyInfo& p_property, const NodePath& p_node_path)
        : _property(p_property)
        , _node_path(p_node_path)
    {
    }
    OrchestratorGraphNodeSpawnerProperty(const PropertyInfo& p_property, const Vector<StringName>& p_target_classes)
        : _property(p_property)
        , _target_classes(p_target_classes)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerPropertyGet : public OrchestratorGraphNodeSpawnerProperty
{
    GDCLASS(OrchestratorGraphNodeSpawnerPropertyGet, OrchestratorGraphNodeSpawnerProperty);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerPropertyGet() = default;

public:
    OrchestratorGraphNodeSpawnerPropertyGet(const PropertyInfo& p_property, const NodePath p_node_path = NodePath())
        : OrchestratorGraphNodeSpawnerProperty(p_property, p_node_path)
    {
    }
    OrchestratorGraphNodeSpawnerPropertyGet(const PropertyInfo& p_property, const Vector<StringName>& p_target_classes)
        : OrchestratorGraphNodeSpawnerProperty(p_property, p_target_classes)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerPropertySet : public OrchestratorGraphNodeSpawnerProperty
{
    GDCLASS(OrchestratorGraphNodeSpawnerPropertySet, OrchestratorGraphNodeSpawnerProperty);
    static void _bind_methods() { }

    Variant _default_value;

protected:
    OrchestratorGraphNodeSpawnerPropertySet() = default;

public:
    OrchestratorGraphNodeSpawnerPropertySet(const PropertyInfo& p_property, const NodePath& p_node_path = NodePath(),
                                      const Variant& p_default_value = Variant())
        : OrchestratorGraphNodeSpawnerProperty(p_property, p_node_path), _default_value(p_default_value)
    {
    }
    OrchestratorGraphNodeSpawnerPropertySet(const PropertyInfo& p_property, const Vector<StringName>& p_target_classes)
        : OrchestratorGraphNodeSpawnerProperty(p_property, p_target_classes)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerCallMemberFunction : public OrchestratorGraphNodeSpawner
{
    GDCLASS(OrchestratorGraphNodeSpawnerCallMemberFunction, OrchestratorGraphNodeSpawner);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerCallMemberFunction() = default;

    MethodInfo _method;

public:
    OrchestratorGraphNodeSpawnerCallMemberFunction(const MethodInfo& p_method)
        : _method(p_method)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerCallScriptFunction : public OrchestratorGraphNodeSpawnerCallMemberFunction
{
    GDCLASS(OrchestratorGraphNodeSpawnerCallScriptFunction, OrchestratorGraphNodeSpawnerCallMemberFunction);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerCallScriptFunction() = default;

    MethodInfo _method;

public:
    OrchestratorGraphNodeSpawnerCallScriptFunction(const MethodInfo& p_method)
        : _method(p_method)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerEvent : public OrchestratorGraphNodeSpawnerCallMemberFunction
{
    GDCLASS(OrchestratorGraphNodeSpawnerEvent, OrchestratorGraphNodeSpawnerCallMemberFunction);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerEvent() = default;

public:
    OrchestratorGraphNodeSpawnerEvent(const MethodInfo& p_method)
        : OrchestratorGraphNodeSpawnerCallMemberFunction(p_method)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerEmitSignal : public OrchestratorGraphNodeSpawnerCallMemberFunction
{
    GDCLASS(OrchestratorGraphNodeSpawnerEmitSignal, OrchestratorGraphNodeSpawnerCallMemberFunction);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerEmitSignal() = default;

    MethodInfo _method;

public:
    OrchestratorGraphNodeSpawnerEmitSignal(const MethodInfo& p_method)
        : _method(p_method)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerVariable : public OrchestratorGraphNodeSpawner
{
    GDCLASS(OrchestratorGraphNodeSpawnerVariable, OrchestratorGraphNodeSpawner);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerVariable() = default;

    StringName _variable_name;

public:
    OrchestratorGraphNodeSpawnerVariable(const StringName& p_variable_name)
        : _variable_name(p_variable_name)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerVariableGet : public OrchestratorGraphNodeSpawnerVariable
{
    GDCLASS(OrchestratorGraphNodeSpawnerVariableGet, OrchestratorGraphNodeSpawnerVariable);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerVariableGet() = default;

public:
    OrchestratorGraphNodeSpawnerVariableGet(const StringName& p_variable_name)
        : OrchestratorGraphNodeSpawnerVariable(p_variable_name)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerVariableSet : public OrchestratorGraphNodeSpawnerVariable
{
    GDCLASS(OrchestratorGraphNodeSpawnerVariableSet, OrchestratorGraphNodeSpawnerVariable);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerVariableSet() = default;

public:
    OrchestratorGraphNodeSpawnerVariableSet(const StringName& p_variable_name)
        : OrchestratorGraphNodeSpawnerVariable(p_variable_name)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OrchestratorGraphNodeSpawnerScriptNode : public OrchestratorGraphNodeSpawner
{
    GDCLASS(OrchestratorGraphNodeSpawnerScriptNode, OrchestratorGraphNodeSpawner);
    static void _bind_methods() { }

protected:
    OrchestratorGraphNodeSpawnerScriptNode() = default;

    StringName _node_name;
    Dictionary _data;

public:
    OrchestratorGraphNodeSpawnerScriptNode(const StringName& p_node_name, const Dictionary& p_data)
        : _node_name(p_node_name)
        , _data(p_data)
    {
    }

    //~ Begin OrchestratorGraphNodeSpawner Interface
    void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) override;
    bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) override;
    //~ End OrchestratorGraphNodeSpawner Interface
};

#endif  // ORCHESTRATOR_GRAPH_NODE_SPAWNER_H
