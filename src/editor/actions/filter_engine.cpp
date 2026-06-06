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
#include "editor/actions/filter_engine.h"

void OrchestratorEditorActionFilterEngine::_remove_rule_by_class(const String& p_class_name) {
    for (const Ref<OrchestratorEditorActionFilterRule>& rule : _rules) {
        if (rule->get_class() == p_class_name) {
            _rules.erase(rule);
            break;
        }
    }
}

void OrchestratorEditorActionFilterEngine::add_rule(const Ref<OrchestratorEditorActionFilterRule>& p_rule) {
    _rules.push_back(p_rule);
}

void OrchestratorEditorActionFilterEngine::clear_rules() {
    _rules.clear();
}

Vector<ScoredAction> OrchestratorEditorActionFilterEngine::filter_actions(
    const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
    const FilterContext& p_context) {

    Vector<ScoredAction> result;
    for (const Ref<OrchestratorEditorActionDefinition>& action : p_actions) {
        float score = 1.f;
        if (filter_action(action, p_context, score)) {
            result.push_back({ action, score });
        }
    }

    return result;
}

bool OrchestratorEditorActionFilterEngine::filter_action(
    const Ref<OrchestratorEditorActionDefinition>& p_action,
    const FilterContext& p_context,
    float& r_score) {

    // Sanity check
    if (!p_action.is_valid()) {
        r_score = 0.f;
        return false;
    }

    const bool use_context = p_context.context_sensitive;

    // Only selectable actions are filtered out; categories always pass so the tree
    // structure can be rebuilt around whatever leaves survive.
    if (p_action->selectable) {
        for (const Ref<OrchestratorEditorActionFilterRule>& rule : _rules) {
            if (!use_context && rule->is_context_sensitive()) {
                continue;
            }

            if (!rule->matches(p_action, p_context)) {
                return false;
            }
        }
    }

    float score = 1.f;
    for (const Ref<OrchestratorEditorActionFilterRule>& rule : _rules) {
        score += rule->score(p_action, p_context);
    }

    r_score = score;
    return true;
}

