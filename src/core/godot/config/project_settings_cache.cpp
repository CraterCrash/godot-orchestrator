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
#include "core/godot/config/project_settings_cache.h"

#include "common/macros.h"
#include "core/godot/io/resource_uid.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

OrchestratorProjectSettingsCache* OrchestratorProjectSettingsCache::_singleton = nullptr;

void OrchestratorProjectSettingsCache::_settings_changed() {

    HashMap<StringName, AutoloadInfo> autoloads;
    PackedStringArray input_action_names;

    const TypedArray<Dictionary> properties = ProjectSettings::get_singleton()->get_property_list();
    for (uint32_t i = 0; i < properties.size(); i++) {
        const Dictionary& dict = properties[i];
        if (!dict.has("name")) {
            continue;
        }

        const String& name = dict["name"];
        if (name.begins_with("autoload/")) {
            AutoloadInfo ai;
            ai.name = name.get_slice("/", 1);

            const String path = ProjectSettings::get_singleton()->get_setting(name);
            ai.singleton = path.begins_with("*");
            ai.path = ai.singleton ? path.substr(1) : path;
            if (ai.path.begins_with("uid://")) {
                ai.uid = ai.path;
                ai.path = GDE::ResourceUID::uid_to_path(ai.path);
            }
            autoloads[ai.name] = ai;
        }
    }

    _autoloads = autoloads;

    emit_signal("settings_changed");
}

void OrchestratorProjectSettingsCache::create() {
    _singleton = memnew(OrchestratorProjectSettingsCache);
}

void OrchestratorProjectSettingsCache::destroy() {
    if (_singleton) {
        memdelete(_singleton);
        _singleton = nullptr;
    }
}

bool OrchestratorProjectSettingsCache::has_autoload(const StringName& p_name) {
    return _autoloads.has(p_name);
}

bool OrchestratorProjectSettingsCache::has_singleton_autoload(const StringName& p_name) {
    const AutoloadInfo* ptr = _autoloads.getptr(p_name);
    return ptr ? ptr->singleton : false;
}

const OrchestratorProjectSettingsCache::AutoloadInfo& OrchestratorProjectSettingsCache::get_autoload(const StringName& p_name) const {
    const AutoloadInfo* ptr = _autoloads.getptr(p_name);
    return ptr ? *ptr : EMPTY;
}

PackedStringArray OrchestratorProjectSettingsCache::get_autoload_names() const {
    PackedStringArray names;
    for (const KeyValue<StringName, AutoloadInfo>& E : _autoloads) {
        names.push_back(E.key);
    }
    return names;
}

const HashMap<StringName, OrchestratorProjectSettingsCache::AutoloadInfo>& OrchestratorProjectSettingsCache::get_autoloads() const {
    return _autoloads;
}

void OrchestratorProjectSettingsCache::_bind_methods() {
    ADD_SIGNAL(MethodInfo("settings_changed"));
}

OrchestratorProjectSettingsCache::OrchestratorProjectSettingsCache() {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    ps->connect("settings_changed", callable_mp_this(_settings_changed));
}
