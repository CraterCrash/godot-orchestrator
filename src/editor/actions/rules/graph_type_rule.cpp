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
#include "editor/actions/rules/graph_type_rule.h"

bool OrchestratorEditorActionGraphTypeRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context)
{
    ERR_FAIL_COND_V(!p_action.is_valid(), false);

    // Always accept all
    if (p_action->graph_type == Action::GRAPH_ALL)
        return true;

    return p_action->graph_type == _graph_type;
}