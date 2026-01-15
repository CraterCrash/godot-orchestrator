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
#include "editor/actions/rules/virtual_function_rule.h"

#include "editor/actions/filter_engine.h"
#include "script/function.h"
#include "script/script.h"

bool OrchestratorEditorActionVirtualFunctionRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    ERR_FAIL_COND_V(!p_action.is_valid(), false);

    if (p_action->method.has_value()) {
        const MethodInfo& method = p_action->method.value();
        if (method.flags & METHOD_FLAG_VIRTUAL) {
            // GH-282
            // Do not add virtual overrides for "_get" or "_set" methods
            // todo: do we want to add these back but with a config toggle?
            if (method.name.match("_get") || method.name.match("_set")) {
                return false;
            }

            return !_method_exclusion_names.has(method.name);
        }
    }
    return false;
}