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
    for (int i  = 0; i < p_actions.size(); i++) {
        const Ref<OrchestratorEditorActionDefinition>& action = p_actions[i];
        const bool use_context = p_context.context_sensitive;

        bool passed = true;
        if (action->selectable) {
            for (const Ref<OrchestratorEditorActionFilterRule>& rule : _rules) {
                if (!use_context && rule->is_context_sensitive()) {
                    continue;
                }

                if (!rule->matches(action, p_context)) {
                    passed = false;
                    break;
                }
            }
        }

        if (passed) {
            // add other filters
            result.push_back({ action, 1.f });
        }
    }

    for (int i = 0; i < result.size(); i++) {
        for (const Ref<OrchestratorEditorActionFilterRule>& rule : _rules) {
            result.write[i].score += rule->score(result[i].action, p_context);
        }
    }

    return result;
}

