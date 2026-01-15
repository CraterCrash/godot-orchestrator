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
#include "common/godot_utils.h"
#include "common/macros.h"
#include "editor/actions/introspector.h"
#include "script/nodes/data/arrays.h"
#include "script/script_server.h"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>

OrchestratorEditorActionRegistry* OrchestratorEditorActionRegistry::_singleton = nullptr;

void OrchestratorEditorActionRegistry::_build_actions() {
    // Immutable actions are ones that will never be overwritten
    Vector<Ref<Action>> actions;

    // These are all the immutable actions
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_script_nodes());
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_variant_types());
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_builtin_functions());
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_native_classes());
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_static_script_methods());

    // Deduplicate the actions
    _actions = GodotUtils::deduplicate<Ref<Action>, ActionComparator>(actions);
}

void OrchestratorEditorActionRegistry::_global_script_classes_updated() {
    _global_classes = OrchestratorEditorIntrospector::generate_actions_from_script_global_classes();
}

void OrchestratorEditorActionRegistry::_autoloads_updated() {
    _autoloads = OrchestratorEditorIntrospector::generate_actions_from_autoloads();
}

void OrchestratorEditorActionRegistry::_resources_reloaded(const PackedStringArray& p_file_names) {
    // No-op
}

Vector<Ref<OrchestratorEditorActionRegistry::Action>> OrchestratorEditorActionRegistry::get_actions() {
    // If something calls this method before the background thread finishes, it blocks
    while (_building) {
        OS::get_singleton()->delay_msec(1000);
    }
    return _actions;
}

Vector<Ref<OrchestratorEditorActionRegistry::Action>> OrchestratorEditorActionRegistry::get_actions(
    const Ref<Script>& p_script, const Ref<Script>& p_other) {
    Vector<Ref<Action>> actions;
    actions.append_array(get_actions());

    if (p_script.is_valid()) {
        actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_script(p_script));
    }

    if (p_other.is_valid()) {
        actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_script(p_other));
    }

    actions.append_array(_global_classes);
    actions.append_array(_autoloads);

    return GodotUtils::deduplicate<Ref<Action>, ActionComparator>(actions);
}

Vector<Ref<OrchestratorEditorActionRegistry::Action>> OrchestratorEditorActionRegistry::get_actions(Object* p_target) {
    Vector<Ref<Action>> actions;
    actions.append_array(get_actions());

    if (p_target) {
        actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_object(p_target));
    }

    actions.append_array(_global_classes);
    actions.append_array(_autoloads);

    return GodotUtils::deduplicate<Ref<Action>, ActionComparator>(actions);
}

Vector<Ref<OrchestratorEditorActionRegistry::Action>> OrchestratorEditorActionRegistry::get_actions(const StringName& p_class_name) {
    Vector<Ref<Action>> actions;
    actions.append_array(get_actions());
    actions.append_array(OrchestratorEditorIntrospector::generate_actions_from_class(p_class_name));
    actions.append_array(_global_classes);
    actions.append_array(_autoloads);

    return GodotUtils::deduplicate<Ref<Action>, ActionComparator>(actions);
}

void OrchestratorEditorActionRegistry::_bind_methods() {
}

OrchestratorEditorActionRegistry::OrchestratorEditorActionRegistry()
{
    _global_script_class_update_timer = memnew(Timer);
    _global_script_class_update_timer->set_one_shot(true);
    _global_script_class_update_timer->set_wait_time(.5);
    _global_script_class_update_timer->connect("timeout", callable_mp_this(_global_script_classes_updated));
    add_child(_global_script_class_update_timer);

    _project_settings_update_timer = memnew(Timer);
    _project_settings_update_timer->set_one_shot(true);
    _project_settings_update_timer->set_wait_time(.5);
    _project_settings_update_timer->connect("timeout", callable_mp_this(_autoloads_updated));
    add_child(_project_settings_update_timer);

    _singleton = this;
    _building = true;

    // Performs the building of immutable actions in a background thread
    // This should prevent the UI from blocking during editor loads
    WorkerThreadPool::get_singleton()->add_task(callable_mp_lambda(this, [&] {
        _build_actions();
        _building = false;
        _global_script_classes_updated();
        _autoloads_updated();
    }));

    EI->get_resource_filesystem()->connect("script_classes_updated", callable_mp_lambda(this, [&] {
        // In the event this signal is called multiple times by the file system in quick succession,
        // the plugin uses a timer to debounce the calls so that only one rebuild fires.
        _global_script_class_update_timer->start();
    }));

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_lambda(this, [&] {
        _project_settings_update_timer->start();
    }));

    EI->get_resource_filesystem()->connect("resources_reload", callable_mp_this(_resources_reloaded));
}

OrchestratorEditorActionRegistry::~OrchestratorEditorActionRegistry() {
    OrchestratorEditorIntrospector::free_resources();
    _singleton = nullptr;
}