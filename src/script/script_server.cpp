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
#include "script/script_server.h"

#include "common/version.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

bool ScriptServer::_reload_scripts_on_save = false;

Ref<Script> ScriptServer::GlobalClass::_load_script(const String& path) {
    if (ResourceLoader::get_singleton()->has_cached(path)) {
        return ResourceLoader::get_singleton()->load(path);
    }
    return ResourceLoader::get_singleton()->load(path, "", ResourceLoader::CACHE_MODE_IGNORE);
}

TypedArray<Dictionary> ScriptServer::GlobalClass::get_property_list() const {
    const Ref<Script> script = _load_script(path);
    return script.is_valid() ? script->get_script_property_list() : TypedArray<Dictionary>();
}

TypedArray<Dictionary> ScriptServer::GlobalClass::get_method_list() const {
    const Ref<Script> script = _load_script(path);
    return script.is_valid() ? script->get_script_method_list() : TypedArray<Dictionary>();
}

TypedArray<Dictionary> ScriptServer::GlobalClass::get_signal_list() const {
    const Ref<Script> script = _load_script(path);
    return script.is_valid() ? script->get_script_signal_list() : TypedArray<Dictionary>();
}

Dictionary ScriptServer::GlobalClass::get_constants_list() const {
    const Ref<Script> script = _load_script(path);
    return script.is_valid() ? script->get_script_constant_map() : Dictionary();
}

StringName ScriptServer::GlobalClass::get_integer_constant_enum(const StringName& p_enum_constant_name) const {
    const Dictionary constants_map = get_constants_list();
    if (!constants_map.is_empty()) {
        const Array& keys = constants_map.keys();
        for (int i = 0; i < keys.size(); i++) {
            const Variant& value = constants_map[keys[i]];
            if (value.get_type() == Variant::DICTIONARY) {
                const Dictionary& enum_dict = value;
                if (enum_dict.has(p_enum_constant_name)) {
                    return keys[i];
                }
            }
        }
    }
    return "";
}

PackedStringArray ScriptServer::GlobalClass::get_integer_constant_list() const {
    PackedStringArray names;

    const Dictionary constants_map = get_constants_list();
    if (!constants_map.is_empty()) {
        const Array& keys = constants_map.keys();
        for (int i= 0; i < keys.size(); i++) {
            // Check and skip enums
            const Variant& value = constants_map[keys[i]];
            if (value.get_type() == Variant::DICTIONARY) {
                const Dictionary& enum_dict = value;
                names.append_array(enum_dict.keys());
            } else {
                names.push_back(keys[i]);
            }
        }
    }
    return names;
}

int64_t ScriptServer::GlobalClass::get_integer_constant(const StringName& p_constant_name) const {
    const Dictionary constants_map = get_constants_list();
    if (!constants_map.is_empty()) {
        const Array& keys = constants_map.keys();
        for (int i= 0; i < keys.size(); i++) {
            if (keys[i] == p_constant_name) {
                return constants_map[keys[i]];
            }

            // Check and skip enums
            const Variant& value = constants_map[keys[i]];
            if (value.get_type() == Variant::DICTIONARY) {
                const Dictionary& enum_dict = value;
                if (enum_dict.has(p_constant_name)) {
                    return enum_dict[p_constant_name];
                }
            }
        }
    }
    return 0;
}

bool ScriptServer::GlobalClass::has_method(const StringName& p_method_name) const {
    if (!name.is_empty() && !path.is_empty()) {
        const TypedArray<Dictionary> method_list = get_method_list();
        for (uint32_t i = 0; i < method_list.size(); i++) {
            const Dictionary& dict = method_list[i];
            if (dict.has("name") && p_method_name.match(dict["name"])) {
                return true;
            }
        }
    }
    return false;
}

bool ScriptServer::GlobalClass::has_property(const StringName& p_property_name) const {
    if (!name.is_empty() && !path.is_empty()) {
        const TypedArray<Dictionary> property_list = get_property_list();
        for (uint32_t i = 0; i < property_list.size(); i++) {
            const Dictionary& dict = property_list[i];
            if (dict.has("name") && p_property_name.match(dict["name"])) {
                return true;
            }
        }
    }
    return false;
}

bool ScriptServer::GlobalClass::has_signal(const StringName& p_signal_name) const {
    if (!name.is_empty() && !path.is_empty()) {
        const TypedArray<Dictionary> signal_list = get_signal_list();
        for (uint32_t i = 0; i < signal_list.size(); i++) {
            const Dictionary& dict = signal_list[i];
            if (dict.has("name") && p_signal_name.match(dict["name"])) {
                return true;
            }
        }
    }
    return false;
}

TypedArray<Dictionary> ScriptServer::GlobalClass::get_static_method_list() const {
    TypedArray<Dictionary> results;
    const TypedArray<Dictionary> method_list = get_method_list();
    for (uint32_t i = 0; i < method_list.size(); i++) {
        const Dictionary& dict = method_list[i];
        const uint32_t flags = dict.get("flags", METHOD_FLAGS_DEFAULT);
        if (flags & METHOD_FLAG_STATIC) {
            results.append(dict);
        }
    }
    return results;
}

ScriptServer::GlobalClass::GlobalClass(const Dictionary& p_dict) {
    name = p_dict["class"];
    base_type = p_dict["base"];
    path = p_dict["path"];
    language = p_dict["language"];
    icon_path = p_dict.get("icon", "");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ScriptServer

TypedArray<Dictionary> ScriptServer::_get_global_class_list() {
    // ProjectSettings automatically caches the global class list, so it's safe to recall it.
    return ProjectSettings::get_singleton()->get_global_class_list();
}

Dictionary ScriptServer::_get_global_class(const StringName& p_class_name) {
    const TypedArray<Dictionary> list = _get_global_class_list();
    for (uint32_t i = 0; i < list.size(); i++) {
        const Dictionary& entry = list[i];
        if (entry.has("class") && p_class_name.match(entry["class"])) {
            return entry;
        }
    }
    return {};
}

bool ScriptServer::is_global_class(const StringName& p_class_name) {
    return !_get_global_class(p_class_name).is_empty();
}

bool ScriptServer::is_parent_class(const StringName& p_source_class_name, const StringName& p_target_class_name) {
    return get_class_hierarchy(p_source_class_name, true).has(p_target_class_name);
}

PackedStringArray ScriptServer::get_global_class_list() {
    PackedStringArray global_class_names;
    const TypedArray<Dictionary> class_list = _get_global_class_list();
    for (uint32_t i = 0; i < class_list.size(); i++) {
        const Dictionary& entry = class_list[i];
        if (entry.has("class")) {
            global_class_names.push_back(entry["class"]);
        }
    }
    return global_class_names;
}

ScriptServer::GlobalClass ScriptServer::get_global_class(const StringName& p_class_name) {
    const Dictionary entry = _get_global_class(p_class_name);
    if (!entry.is_empty()) {
        return GlobalClass(entry);
    }
    return {};
}

ScriptServer::GlobalClass ScriptServer::get_global_class_by_path(const String& p_path) {
    const TypedArray<Dictionary> classes = _get_global_class_list();
    for (uint32_t i = 0; i < classes.size(); i++) {
        const Dictionary& data = classes[i];
        if (data.get("path", "") == p_path) {
            return GlobalClass(data);
        }
    }
    return {};
}

String ScriptServer::get_global_class_path(const StringName& p_class_name) {
    if (!is_global_class(p_class_name)) {
        return "";
    }
    return get_global_class(p_class_name).path;
}

StringName ScriptServer::get_global_class_native_base(const StringName& p_class_name) {
    PackedStringArray hierarchy = get_class_hierarchy(p_class_name, true);
    for (const String& class_name : hierarchy) {
        if (!is_global_class(class_name)) {
            return class_name;
        }
    }
    return Object::get_class_static();
}

PackedStringArray ScriptServer::get_class_hierarchy(const StringName& p_class_name, bool p_include_native_classes) {
    PackedStringArray hierarchy;
    StringName class_name = p_class_name;
    while (!class_name.is_empty()) {
        if (is_global_class(class_name)) {
            hierarchy.push_back(class_name);
            class_name = get_global_class(class_name).base_type;
        } else if (p_include_native_classes) {
            hierarchy.push_back(class_name);
            class_name = ClassDB::get_parent_class(class_name);
        } else {
            break;
        }
    }
    return hierarchy;
}

String ScriptServer::get_global_name(const Ref<Script>& p_script) {
    if (p_script.is_valid()) {
        #if GODOT_VERSION >= 0x040300
        return p_script->get_global_name();
        #else
        for (const Variant& global_class : _get_global_class_list()) {
            const Dictionary& entry = global_class;
            if (entry.has("path") && p_script->get_path().match(entry["path"]) && entry.has("class")) {
                return entry["class"];
            }
        }
        #endif
    }
    return "";
}

bool ScriptServer::is_scripting_enabled() {
    #ifdef TOOLS_ENABLED
    // Other than '@tool' scripts, the editor does not enable scripting
    if (Engine::get_singleton()->is_editor_hint()) {
        return false;
    }
    #endif
    return _scripting_enabled;
}

void ScriptServer::set_scripting_enabled(bool p_enabled) {
    _scripting_enabled = p_enabled;
}
