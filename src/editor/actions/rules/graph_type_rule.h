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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_GRAPH_TYPE_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_GRAPH_TYPE_RULE_H

#include "editor/actions/rules/rule.h"

using namespace godot;

/// This rule is designed to be configured with a specific <code>GraphType</code> and filters actions
/// so that they match the given type or have no graph type association.
///
class OrchestratorEditorActionGraphTypeRule : public OrchestratorEditorActionFilterRule {
    GDCLASS(OrchestratorEditorActionGraphTypeRule, OrchestratorEditorActionFilterRule);

    using Action = OrchestratorEditorActionDefinition;

    Action::GraphType _graph_type = Action::GRAPH_ALL;

protected:
    static void _bind_methods() { }

public:
    //~ Begin ActionFilterRule Interface
    bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) override;
    //~ End ActionFilterRule Interface

    void set_graph_type(Action::GraphType p_graph_type) { _graph_type = p_graph_type;}
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_GRAPH_TYPE_RULE_H