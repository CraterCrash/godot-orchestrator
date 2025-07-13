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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_CLASS_HIERARCHY_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_CLASS_HIERARCHY_RULE_H

#include "editor/actions/rules/rule.h"

using namespace godot;

/// This rule is designed to filter all actions that have a <code>target_class</code> attribute and to
/// make sure that class matches the script's class name, the script's native class name, or any one
/// of the parent class types of the script.
///
class OrchestratorEditorActionClassHierarchyScopeRule : public OrchestratorEditorActionFilterRule
{
    GDCLASS(OrchestratorEditorActionClassHierarchyScopeRule, OrchestratorEditorActionFilterRule);

protected:
    static void _bind_methods() { }

public:
    //~ Begin ActionFilterRule Interface
    bool is_context_sensitive() const override { return true; }
    bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) override;
    //~ End ActionFilterRule Interface
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_CLASS_HIERARCHY_RULE_H