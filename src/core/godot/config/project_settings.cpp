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
#include "core/godot/config/project_settings.h"

#include <godot_cpp/classes/project_settings.hpp>

HashMap<StringName, GDE::ProjectSettings::AutoloadInfo> GDE::ProjectSettings::get_autoload_list() {
    HashMap<StringName, AutoloadInfo> results;

    const TypedArray<Dictionary> properties = godot::ProjectSettings::get_singleton()->get_property_list();
    for (const Variant& entry : properties) {
        const Dictionary& dict = entry;
        if (!dict.has("name")) {
            continue;
        }

        const String& name = dict["name"];
        if (name.begins_with("autoload/")) {
            AutoloadInfo ai;
            ai.name = name.get_slice("/", 1);

            const String path = godot::ProjectSettings::get_singleton()->get_setting(name);
            ai.is_singleton = path.begins_with("*");
            ai.path = ai.is_singleton ? path.substr(1) : path;
            results[ai.name] = ai;
        }
    }

    return results;
}

bool GDE::ProjectSettings::has_autoload(const StringName& p_name) {
    return get_autoload_list().has(p_name);
}

bool GDE::ProjectSettings::has_singleton_autoload(const StringName& p_name) {
    const HashMap<StringName, AutoloadInfo> results = get_autoload_list();
    if (results.has(p_name)) {
        return results[p_name].is_singleton;
    }
    return false;
}

GDE::ProjectSettings::AutoloadInfo GDE::ProjectSettings::get_autoload(const StringName& p_name) {
    const HashMap<StringName, AutoloadInfo> results = get_autoload_list();
    if (results.has(p_name)) {
        return results[p_name];
    }
    return {};
}