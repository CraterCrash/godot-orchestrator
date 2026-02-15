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
#include "editor/actions/rules/search_text_rule.h"

#include "common/string_utils.h"
#include "editor/actions/filter_engine.h"

bool OrchestratorEditorActionSearchTextRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    ERR_FAIL_COND_V(!p_action.is_valid(), false);

    // If the search text is empty, match all
    if (p_context.query.is_empty()) {
        return true;
    }

    // Tokenize the user input
    const PackedStringArray query_tokens = p_context.query.to_lower().split(" ", false);

    // Generate the necessary text for each action
    const String combined = p_action->name + " " + p_action->tooltip + " " + StringUtils::join(" ", p_action->keywords);

    // Iterate tokens
    for (const String& token : query_tokens) {
        if (combined.findn(token) == -1) {
            return false;
        }
    }
    return true;
}

float OrchestratorEditorActionSearchTextRule::score(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    const PackedStringArray tokens = p_context.query.to_lower().split(" ", false);
    const String name = p_action->name.to_lower();
    const PackedStringArray keywords = p_action->keywords;
    const String tooltip = p_action->tooltip.to_lower();

    float score = 0.0f;
    float name_boost = 1.0f;
    float keyword_boost = 0.7f;
    float tooltip_boost = 0.5f;

    for (const String& token : tokens) {
        if (name.findn(token) != -1) {
            score += name_boost;
        } else if (keywords.has(token)) {
            score += keyword_boost;
        } else if (tooltip.findn(token) != -1) {
            score += tooltip_boost;
        } else {
            score -= 0.3f; // Penalty for unmatched token
        }
    }

    // Normalize score by number of tokens
    score /= tokens.size();

    // Favor shorter names for equal match
    score *= 1.0f - 0.1f * (float(name.length()) / 100.0f);

    if (!p_action->selectable) {
        score *= 0.1f;
    }

    return CLAMP(score, 0.0f, 1.0f);
}
