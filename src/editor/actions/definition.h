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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_DEFINITION_H
#define ORCHESTRATOR_EDITOR_ACTIONS_DEFINITION_H

#include <optional>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

class OrchestratorEditorActionDefinition : public RefCounted
{
    GDCLASS(OrchestratorEditorActionDefinition, RefCounted);

protected:
    static void _bind_methods() { }

public:
    // Defines different action types
    enum ActionType
    {
        ACTION_NONE,
        ACTION_SPAWN_NODE,
        ACTION_GET_PROPERTY,
        ACTION_SET_PROPERTY,
        ACTION_CALL_MEMBER_FUNCTION,
        ACTION_CALL_SCRIPT_FUNCTION,
        ACTION_EVENT,
        ACTION_EMIT_MEMBER_SIGNAL,
        ACTION_EMIT_SIGNAL,
        ACTION_VARIABLE_GET,
        ACTION_VARIABLE_SET
    };

    enum GraphType
    {
        GRAPH_ALL,
        GRAPH_EVENT,
        GRAPH_FUNCTION,
        GRAPH_MACRO
    };

    enum ActionFlags
    {
        FLAG_NONE,
        FLAG_EXPERIMENTAL
    };

    // View based attributes
    String name;
    String category;
    String tooltip;
    String icon;
    String type_icon;
    String target_class;
    PackedStringArray keywords;
    ActionType type = ACTION_NONE;
    GraphType graph_type = GRAPH_ALL;
    bool selectable = false;
    ActionFlags flags = FLAG_NONE;

    std::optional<String> node_class;                   //! Node to spawn
    std::optional<MethodInfo> method;                   //! Class/Script method/function/signal
    std::optional<PropertyInfo> property;               //! Class/Script properties
    std::optional<NodePath> node_path;                  //! Not used
    std::optional<StringName> class_name;               //! Script method/property/signal or Class method owner
    std::optional<Dictionary> data;                     //! Dictionary data structure to pass to spawner
    std::optional<PackedStringArray> target_classes;    //! Used by properties, but why not use class_name?
    std::optional<Vector<Variant::Type>> inputs;        //! Operators pass their input types
    std::optional<Vector<Variant::Type>> outputs;       //! Operators pass their output types
    bool executions = false;                            //! Whether the action has execution pins
};

struct OrchestratorEditorActionDefinitionComparator
{
    bool operator()(const Ref<OrchestratorEditorActionDefinition>& a, const Ref<OrchestratorEditorActionDefinition>& b) const
    {
        const bool a_valid = a.is_valid();
        const bool b_valid = b.is_valid();

        // Invalid references are first
        if (!a_valid && !b_valid)
            return false;
        if (!a_valid)
            return true;
        if (!b_valid)
            return false;

        if (a->category == b->category)
            return a->name < b->name;

        return a->category < b->category;
    }
};

/// Helper class for creating <code>OrchestratorEditorActionDefinition</code> objects
class OrchestratorEditorActionBuilder
{
    Ref<OrchestratorEditorActionDefinition> _action;

public:

    OrchestratorEditorActionBuilder& tooltip(const String& p_tooltip);
    OrchestratorEditorActionBuilder& icon(const String& p_icon);
    OrchestratorEditorActionBuilder& type_icon(const String& p_type_icon);
    OrchestratorEditorActionBuilder& target_class(const String& p_target_class);
    OrchestratorEditorActionBuilder& keywords(const PackedStringArray& p_keywords);
    OrchestratorEditorActionBuilder& type(OrchestratorEditorActionDefinition::ActionType p_type);
    OrchestratorEditorActionBuilder& graph_type(OrchestratorEditorActionDefinition::GraphType p_type);
    OrchestratorEditorActionBuilder& selectable(bool p_selectable);
    OrchestratorEditorActionBuilder& node_class(const String& p_node_class);
    OrchestratorEditorActionBuilder& method(const MethodInfo& p_method);
    OrchestratorEditorActionBuilder& property(const PropertyInfo& p_property);
    OrchestratorEditorActionBuilder& node_path(const NodePath& p_path);
    OrchestratorEditorActionBuilder& class_name(const String& p_class_name);
    OrchestratorEditorActionBuilder& target_classes(const PackedStringArray& p_target_classes);
    OrchestratorEditorActionBuilder& data(const Dictionary& p_data);
    OrchestratorEditorActionBuilder& flags(OrchestratorEditorActionDefinition::ActionFlags p_flags);
    OrchestratorEditorActionBuilder& inputs(const Vector<Variant::Type>& p_inputs);
    OrchestratorEditorActionBuilder& outputs(const Vector<Variant::Type>& p_outputs);
    OrchestratorEditorActionBuilder& executions(bool p_executions);

    Ref<OrchestratorEditorActionDefinition> build() const;

    OrchestratorEditorActionBuilder(const String& p_category);
    OrchestratorEditorActionBuilder(const String& p_category, const String& p_name);

};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_DEFINITION_H
