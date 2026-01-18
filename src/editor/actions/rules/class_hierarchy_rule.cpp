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

bool OrchestratorEditorActionClassHierarchyScopeRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    // If the action is not scoped to a class, allow
    const String& target_class = p_action->target_class;
    if (target_class.is_empty()) {
        return true;
    }

    // If there is no class to compare to, allow
    if (_script_hierarchy_classes.is_empty()) {
        return true;
    }

    if (target_class == _global_class_name) {
        return true;
    }

    return _script_hierarchy_classes.has(target_class);
}

void OrchestratorEditorActionClassHierarchyScopeRule::set_script_classes(const Ref<Script>& p_script) {
    if (p_script.is_valid()) {
        _global_class_name = ScriptServer::get_global_name(p_script);

        String class_name = p_script->get_instance_base_type();
        if (const Ref<OScript> oscript = p_script; oscript.is_valid()) {
            class_name = oscript->get_orchestration()->get_base_type();
        }

        while (!class_name.is_empty()) {
            _script_hierarchy_classes.push_back(class_name);
            class_name = ClassDB::get_parent_class(class_name);
        }
    }
}