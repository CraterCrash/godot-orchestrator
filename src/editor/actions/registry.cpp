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
#include "orchestration/nodes/arrays.h"
#include "script/script_server.h"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>

OrchestratorEditorActionRegistry* OrchestratorEditorActionRegistry::_singleton = nullptr;

void OrchestratorEditorActionRegistry::_rebuild_base_actions() {
    _building = true;

    // Performs the building of immutable actions in a background thread
    // This should prevent the UI from blocking during editor loads
    WorkerThreadPool::get_singleton()->add_task(callable_mp_lambda(this, [&] {
        if (_immutable_actions.is_empty()) {
            _build_actions();
        }
        _global_script_classes_updated();
        _autoloads_updated();

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
        _building = false;
    }));
}

void OrchestratorEditorActionRegistry::_build_actions() {
    // Immutable actions are ones that will never be overwritten
    _immutable_actions.clear();

    // These are all the immutable actions
    OrchestratorEditorIntrospector::generate_actions_from_script_nodes(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_variant_types(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_builtin_functions(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_native_classes(_immutable_actions);
    OrchestratorEditorIntrospector::generate_actions_from_static_script_methods(_immutable_actions);
}

void OrchestratorEditorActionRegistry::_global_script_classes_updated() {
    _global_class_actions.clear();
    OrchestratorEditorIntrospector::generate_actions_from_script_global_classes(_global_class_actions);
}

void OrchestratorEditorActionRegistry::_autoloads_updated() {
    _autoload_actions.clear();
    OrchestratorEditorIntrospector::generate_actions_from_autoloads(_autoload_actions);
}

void OrchestratorEditorActionRegistry::_resources_reloaded(const PackedStringArray& p_file_names) {
    // No-op
}

OrchestratorEditorActionSet OrchestratorEditorActionRegistry::get_actions() {
    // If something calls this method before the background thread finishes, it blocks
    while (_building) {
        OS::get_singleton()->delay_msec(1000);
    }
    return _base_actions;
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

OrchestratorEditorActionRegistry::OrchestratorEditorActionRegistry()
{
    _global_script_class_update_timer = memnew(Timer);
    _global_script_class_update_timer->set_one_shot(true);
    _global_script_class_update_timer->set_wait_time(.5);
    _global_script_class_update_timer->connect("timeout", callable_mp_this(_rebuild_base_actions));
    add_child(_global_script_class_update_timer);

    _project_settings_update_timer = memnew(Timer);
    _project_settings_update_timer->set_one_shot(true);
    _project_settings_update_timer->set_wait_time(.5);
    _project_settings_update_timer->connect("timeout", callable_mp_this(_rebuild_base_actions));
    add_child(_project_settings_update_timer);

    _singleton = this;

    _rebuild_base_actions();

    EI->get_resource_filesystem()->connect("script_classes_updated", callable_mp_lambda(this, [&] {
        // In the event this signal is called multiple times by the file system in quick succession,
        // the plugin uses a timer to debounce the calls so that only one rebuild fires.
        _global_script_class_update_timer->start();
    }));

    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_lambda(this, [&] {
        _project_settings_update_timer->start();
    }));

    EI->get_resource_filesystem()->connect("resources_reload", callable_mp_this(_resources_reloaded));
}

OrchestratorEditorActionRegistry::~OrchestratorEditorActionRegistry() {
    OrchestratorEditorIntrospector::free_resources();
    _singleton = nullptr;
}