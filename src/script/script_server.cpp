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

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>

struct ScriptServer::GlobalClassCache {
    TypedArray<Dictionary> source;            //! The ProjectSettings list the tables were built from
    HashMap<StringName, GlobalClass> classes; //! Global classes keyed by class name
    HashMap<String, StringName> paths;        //! Global class names keyed by script path
};

bool ScriptServer::_reload_scripts_on_save = false;
ScriptServer::GlobalClassCache* ScriptServer::_global_class_cache = nullptr;
Mutex ScriptServer::_global_class_cache_mutex;

Ref<Script> ScriptServer::GlobalClass::_load_script(const String& path) {
    ResourceLoader* loader = ResourceLoader::get_singleton();

    if (loader->has_cached(path)) {
        return loader->load(path);
    }

    // When called from the main thread, the script should be loaded synchronously.
    //
    // The threaded loader below leaks objects per `preload` call, which shows up as leaked RefCounted
    // instances at editor exit, whenever a preload-heavy global class is introspected.
    //
    // The threaded path below exists only to avoid a ResourceLoader deadlock when called from
    // a non-main thread, so this limits that to that only case.
    OS* os = OS::get_singleton();
    if (os->get_thread_caller_id() != os->get_main_thread_id()) {
        WARN_PRINT_ONCE(vformat(
            R"(ScriptServer::GlobalClass::_load_script() reached its off-main-thread fallback for "%s"; )"
            R"(the threaded loader leaks preload dependencies, move this caller onto the main thread.)",
            path));

        // When called from a background thread, delegate the load to the main thread
        // via load_threaded_request and wait for it to complete.
        if (loader->load_threaded_request(path, "", ResourceLoader::CACHE_MODE_IGNORE) == OK) {
            ResourceLoader::ThreadLoadStatus status;
            do {
                status = loader->load_threaded_get_status(path);
            } while (status == ResourceLoader::THREAD_LOAD_IN_PROGRESS);

            // Always call load_threaded_get(), even on failure: the threaded loader only
            // releases the completed task (and any dependencies it already loaded) once the
            // result is retrieved. Returning without it orphans the task. On failure this
            // yields an invalid resource, which we normalize to an empty Ref.
            const Ref<Script> script = loader->load_threaded_get(path);
            return status == ResourceLoader::THREAD_LOAD_LOADED ? script : Ref<Script>();
        }
    }

    return loader->load(path, "", ResourceLoader::CACHE_MODE_IGNORE);
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

PackedStringArray ScriptServer::GlobalClass::get_enum_list() const {
    PackedStringArray names;
    const Dictionary constants_map = get_constants_list();
    if (!constants_map.is_empty()) {
        const Array& keys = constants_map.keys();
        for (int i= 0; i < keys.size(); i++) {
            // Check and skip enums
            const Variant& value = constants_map[keys[i]];
            if (value.get_type() == Variant::DICTIONARY) {
                names.append(keys[i]);
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
    is_abstract = p_dict.get("is_abstract", false);
    is_tool = p_dict.get("is_tool", false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ScriptServer

ScriptServer::GlobalClassCache& ScriptServer::_sync_global_classes() {
    GlobalClassCache& cache = *_global_class_cache;

    // The engine replaces its stored Array on every write, so an unchanged list is pointer-equal
    // inside the engine and the comparison costs a single builtin call. A replaced list is deep
    // compared engine-side without marshalling, and the tables are rebuilt only when the content
    // differs. This never relies on signals, so the cache is never staler than ProjectSettings.
    const TypedArray<Dictionary> list = ProjectSettings::get_singleton()->get_global_class_list();
    if (cache.source != list) {
        cache.classes.clear();
        cache.paths.clear();

        for (uint32_t i = 0; i < list.size(); i++) {
            const GlobalClass global_class(list[i]);
            if (global_class.name.is_empty()) {
                continue;
            }
            cache.classes[global_class.name] = global_class;
            cache.paths[global_class.path] = global_class.name;
        }
    }

    // Adopt the engine's Array so the next comparison short-circuits on identity.
    cache.source = list;
    return cache;
}

bool ScriptServer::is_global_class(const StringName& p_class_name) {
    if (p_class_name.is_empty() || ClassDB::class_exists(p_class_name)) {
        return false;
    }

    MutexLock lock(_global_class_cache_mutex);
    return _sync_global_classes().classes.has(p_class_name);
}

bool ScriptServer::is_parent_class(const StringName& p_source_class_name, const StringName& p_target_class_name) {
    return get_class_hierarchy(p_source_class_name, true).has(p_target_class_name);
}

PackedStringArray ScriptServer::get_global_class_list() {
    MutexLock lock(_global_class_cache_mutex);

    PackedStringArray global_class_names;
    for (const KeyValue<StringName, GlobalClass>& E : _sync_global_classes().classes) {
        global_class_names.push_back(E.key);
    }
    return global_class_names;
}

ScriptServer::GlobalClass ScriptServer::get_global_class(const StringName& p_class_name) {
    if (p_class_name.is_empty()) {
        return {};
    }

    MutexLock lock(_global_class_cache_mutex);
    const GlobalClass* global_class = _sync_global_classes().classes.getptr(p_class_name);
    return global_class ? *global_class : GlobalClass();
}

ScriptServer::GlobalClass ScriptServer::get_global_class_by_path(const String& p_path) {
    MutexLock lock(_global_class_cache_mutex);
    const GlobalClassCache& cache = _sync_global_classes();

    const StringName* class_name = cache.paths.getptr(p_path);
    if (!class_name) {
        return {};
    }

    const GlobalClass* global_class = cache.classes.getptr(*class_name);
    return global_class ? *global_class : GlobalClass();
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
        return p_script->get_global_name();
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
    return true;
}

void ScriptServer::get_static_method_list(const StringName& p_class, TypedArray<Dictionary>* r_methods, bool p_no_inheritance) {

    String class_name = p_class;

    GlobalClass global_class = get_global_class(class_name);
    while (!global_class.name.is_empty()) {
        const TypedArray<Dictionary> methods = global_class.get_method_list();
        for (int i = 0; i < methods.size(); i++) {
            const Dictionary& data = methods[i];
            const int32_t flags = data["flags"];
            if (flags & METHOD_FLAG_STATIC) {
                r_methods->push_back(methods[i]);
            }
        }

        if (p_no_inheritance) {
            return;
        }

        class_name = global_class.base_type;
        global_class = get_global_class(class_name);
    }

    const TypedArray<Dictionary> methods = ClassDB::class_get_method_list(class_name, p_no_inheritance);
    for (int i = 0; i < methods.size(); i++) {
        const Dictionary& data = methods[i];
        const int32_t flags = data["flags"];
        if (flags & METHOD_FLAG_STATIC) {
            r_methods->push_back(methods[i]);
        }
    }

}

void ScriptServer::create() {
    _global_class_cache = memnew(GlobalClassCache);
}

void ScriptServer::free() {
    MutexLock lock(_global_class_cache_mutex);
    if (_global_class_cache) {
        memdelete(_global_class_cache);
        _global_class_cache = nullptr;
    }
}
