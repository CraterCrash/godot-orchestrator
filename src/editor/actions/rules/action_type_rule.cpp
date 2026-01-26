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
#include "editor/actions/rules/action_type_rule.h"

#include "editor/actions/filter_engine.h"

bool OrchestratorEditorActionTypeRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    ERR_FAIL_COND_V(!p_action.is_valid(), false);

    if (p_context._filter_action_type < 0) {
        return true;
    }

    return p_action->type == p_context._filter_action_type;
}