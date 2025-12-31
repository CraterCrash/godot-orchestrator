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
#include "editor/actions/rules/class_hierarchy_rule.h"

#include "editor/actions/filter_engine.h"
#include "script/script.h"
#include "script/script_server.h"

bool OrchestratorEditorActionClassHierarchyScopeRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context)
{
    // If the action is not scoped to a class, allow
    const String& target_class = p_action->target_class;
    if (target_class.is_empty())
        return true;

    // If there is no class to compare to, allow
    if (!p_context.graph_context.script.is_valid())
        return true;

    const StringName global_name = ScriptServer::get_global_name(p_context.graph_context.script);
    if (target_class == global_name)
        return true;

    String class_to_check = p_context.graph_context.script->get_instance_base_type();

    Ref<OScript> oscript = p_context.graph_context.script;
    if (oscript.is_valid()) {
        class_to_check = oscript->get_orchestration()->get_base_type();
    }

    while (!class_to_check.is_empty())
    {
        if (class_to_check == target_class)
            return true;

        class_to_check = ClassDB::get_parent_class(class_to_check);
    }

    return false;
}

