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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ANY_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ANY_RULE_H

#include "editor/actions/rules/rule.h"

using namespace godot;

/// The <code>OrchestratorEditorFilterEngine</code> is designed to apply a conjunction across all rules,
/// meaning that all rules must match for an action to be made available.
///
/// There are situations when  it may make sense to provide a disjunction, meaning where an action is
/// made available when only one rule matches. This class is designed to evaluate all provided rule's
/// <code>matches</code> function result, returning <code>true</code> when at least one rule matches.
///
class OrchestratorEditorActionAnyFilterRule : public OrchestratorEditorActionFilterRule
{
    GDCLASS(OrchestratorEditorActionAnyFilterRule, OrchestratorEditorActionFilterRule);

    Vector<Ref<OrchestratorEditorActionFilterRule>> _rules;

protected:
    static void _bind_methods() { }

public:
    //~ Begin ActionFilterRule Interface
    bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) override;
    //~ End ActionFilterRule Interface

    void add_rule(const Ref<OrchestratorEditorActionFilterRule>& p_rule) { _rules.push_back(p_rule); }
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ANY_RULE_H