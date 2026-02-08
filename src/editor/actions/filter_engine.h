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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ENGINE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ENGINE_H

#include "editor/actions/definition.h"
#include "editor/actions/rules/rules.h"

#include <godot_cpp/classes/script.hpp>

/// Parameter object for passing filter context to rules
struct FilterContext
{
    String query;
    bool context_sensitive;
    int _filter_action_type;
};

/// Wrapper that provides scoring aspects for filtered actions
struct ScoredAction
{
    Ref<OrchestratorEditorActionDefinition> action;
    float score = 1.f;
};

/// Filter engine
class OrchestratorEditorActionFilterEngine : public RefCounted
{
    GDCLASS(OrchestratorEditorActionFilterEngine, RefCounted);

    Vector<Ref<OrchestratorEditorActionFilterRule>> _rules;

protected:
    static void _bind_methods() { }

    void _remove_rule_by_class(const String& p_class_name);

public:

    void add_rule(const Ref<OrchestratorEditorActionFilterRule>& p_rule);
    template <typename T> void remove_rule() { _remove_rule_by_class(T::get_class_static()); }
    void clear_rules();

    Vector<ScoredAction> filter_actions(
        const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
        const FilterContext& p_context);
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_ENGINE_H