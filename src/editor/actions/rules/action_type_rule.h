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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ACTION_TYPE_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ACTION_TYPE_RULE_H

#include "editor/actions/rules/rule.h"

using namespace godot;

/// This rule is designed to inspect each <code>OrchestrationEditorActionDefinition</code> specifically
/// looking at the action's type and matching only those actions that have the same type. The action
/// type to match is taken from the drop-down type filter field on the dialog, if set and available.
///
class OrchestratorEditorActionTypeRule : public OrchestratorEditorActionFilterRule
{
    GDCLASS(OrchestratorEditorActionTypeRule, OrchestratorEditorActionFilterRule);

protected:
    static void _bind_methods() { }

public:
    //~ Begin ActionFilterRule Interface
    bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) override;
    //~ End ActionFilterRule Interface
};


#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ACTION_TYPE_RULE_H