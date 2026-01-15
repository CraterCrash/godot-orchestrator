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
#include "editor/actions/definition.h"

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::tooltip(const String& p_tooltip) {
    _action->tooltip = p_tooltip;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::icon(const String& p_icon) {
    _action->icon = p_icon;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::type_icon(const String& p_type_icon) {
    _action->type_icon = p_type_icon;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::target_class(const String& p_target_class) {
    _action->target_class = p_target_class;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::keywords(const PackedStringArray& p_keywords) {
    _action->keywords = p_keywords;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::type(OrchestratorEditorActionDefinition::ActionType p_type) {
    _action->type = p_type;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::graph_type(OrchestratorEditorActionDefinition::GraphType p_type) {
    _action->graph_type = p_type;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::selectable(bool p_selectable) {
    _action->selectable = p_selectable;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::no_capitalize(bool p_no_capitalize) {
    _action->no_capitalize = p_no_capitalize;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::node_class(const String& p_node_class) {
    _action->node_class = p_node_class;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::method(const MethodInfo& p_method) {
    _action->method = p_method;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::property(const PropertyInfo& p_property) {
    _action->property = p_property;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::node_path(const NodePath& p_path) {
    _action->node_path = p_path;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::class_name(const String& p_class_name) {
    _action->class_name = p_class_name;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::target_classes(const PackedStringArray& p_target_classes) {
    _action->target_classes = p_target_classes;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::data(const Dictionary& p_data) {
    _action->data = p_data;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::flags(OrchestratorEditorActionDefinition::ActionFlags p_flags) {
    _action->flags = p_flags;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::inputs(const Vector<Variant::Type>& p_inputs) {
    _action->inputs = p_inputs;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::outputs(const Vector<Variant::Type>& p_outputs) {
    _action->outputs = p_outputs;
    return *this;
}

OrchestratorEditorActionBuilder& OrchestratorEditorActionBuilder::executions(bool p_executions) {
    _action->executions = p_executions;
    return *this;
}

Ref<OrchestratorEditorActionDefinition> OrchestratorEditorActionBuilder::build() const {
    return _action;
}

OrchestratorEditorActionBuilder::OrchestratorEditorActionBuilder(const String& p_category) {
    _action.instantiate();
    _action->category = p_category;
    _action->name = "";
}

OrchestratorEditorActionBuilder::OrchestratorEditorActionBuilder(const String& p_category, const String& p_name) {
    _action.instantiate();
    _action->category = p_category;
    _action->name = p_name;
}


