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
#include "editor/actions/registry.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/config/project_settings_cache.h"
#include "editor/actions/introspector.h"
#include "script/script_server.h"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

OrchestratorEditorActionRegistry* OrchestratorEditorActionRegistry::_singleton = nullptr;

void OrchestratorEditorActionRegistry::_build_immutable_actions() {
    // Immutable actions are ones that will never be overwritten
    _immutable_actions.clear();

    // These are all the immutable actions; none of them load project resources.
    OrchestratorEditorIntrospector::generate_actions_from_script_nodes(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_variant_types(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_builtin_functions(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_native_classes(_immutable_actions);
}

void OrchestratorEditorActionRegistry::_ensure_built() {
    if (!_global_classes_dirty && !_autoloads_dirty) {
        return;
    }

    // Runs on the main thread: safe to load scripts via ResourceLoader.
    if (_global_classes_dirty) {
        _global_class_actions.clear();
        OrchestratorEditorIntrospector::generate_actions_from_script_global_classes(_global_class_actions);
        _global_classes_dirty = false;
    }

    if (_autoloads_dirty) {
        _autoload_actions.clear();
        OrchestratorEditorIntrospector::generate_actions_from_autoloads(_autoload_actions);
        _autoloads_dirty = false;
    }

    OrchestratorEditorActionSet combined;
    for (const Ref<Action>& action : _immutable_actions) {
        combined.insert(action);
    }
    for (const Ref<Action>& action : _global_class_actions) {
        combined.insert(action);
    }
    for (const Ref<Action>& action : _autoload_actions) {
        combined.insert(action);
    }

    _base_actions = combined;
}

void OrchestratorEditorActionRegistry::_settings_changed() {
    _autoloads_dirty = true;
}

void OrchestratorEditorActionRegistry::_script_classes_updated() {
    // The file system may report this several times in quick succession; the flag coalesces them
    // and the rebuild happens once, on the next request.
    _global_classes_dirty = true;
}

void OrchestratorEditorActionRegistry::prime() {
    _ensure_built();
}

OrchestratorEditorActionSet OrchestratorEditorActionRegistry::get_actions() {
    _ensure_built();
    return OrchestratorEditorActionSet(_base_actions);
}

OrchestratorEditorActionSet OrchestratorEditorActionRegistry::get_actions(const Ref<Script>& p_script, const Ref<Script>& p_other) {
    OrchestratorEditorActionSet actions = get_actions();

    if (p_script.is_valid()) {
        OrchestratorEditorIntrospector::generate_actions_from_script(p_script, actions);
    }

    if (p_other.is_valid()) {
        OrchestratorEditorIntrospector::generate_actions_from_script(p_other, actions);
    }

    return actions;
}

OrchestratorEditorActionSet OrchestratorEditorActionRegistry::get_actions(Object* p_target) {
    if (!p_target) {
        return get_actions();
    }

    const Ref<Script> script = p_target->get_script();
    if (!script.is_valid() && ClassDB::class_exists(p_target->get_class())) {
        return get_actions();
    }

    if (!ScriptServer::get_global_name(script).is_empty()) {
        return get_actions();
    }

    OrchestratorEditorActionSet actions = get_actions();
    OrchestratorEditorIntrospector::generate_actions_from_object(p_target, actions);
    return actions;
}

OrchestratorEditorActionSet OrchestratorEditorActionRegistry::get_actions(const StringName& p_class_name) {
    if (ClassDB::class_exists(p_class_name) || ScriptServer::is_global_class(p_class_name)) {
        return get_actions();
    }

    OrchestratorEditorActionSet actions = get_actions();
    OrchestratorEditorIntrospector::generate_actions_from_class(p_class_name, actions);
    return actions;
}

void OrchestratorEditorActionRegistry::_bind_methods() {
}

OrchestratorEditorActionRegistry::OrchestratorEditorActionRegistry() {
    _singleton = this;

    _build_immutable_actions();

    EI->get_resource_filesystem()->connect("script_classes_updated", callable_mp_this(_script_classes_updated));
    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_this(_settings_changed));
}

OrchestratorEditorActionRegistry::~OrchestratorEditorActionRegistry() {
    OrchestratorEditorIntrospector::free_resources();
    _singleton = nullptr;
}