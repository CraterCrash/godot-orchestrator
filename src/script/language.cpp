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
#include "script/language.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/resource_utils.h"
#include "common/settings.h"
#include "common/resource_utils.h"
#include "common/string_utils.h"
#include "core/godot/core_constants.h"
#include "core/godot/variant/variant.h"
#include "orchestration/serialization/text/variant_parser.h"
#include "script/compiler/analyzer.h"
#include "script/nodes/utilities/print_string.h"
#include "script/parser/parser.h"
#include "script/script.h"
#include "script/script_cache.h"
#include "script/utility_functions.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/mutex_lock.hpp>

#if GODOT_VERSION >= 0x040300
  #include <godot_cpp/classes/os.hpp>
#endif

OScriptLanguage* OScriptLanguage::_singleton = nullptr;

thread_local OScriptLanguage::CallLevel* OScriptLanguage::_call_stack = nullptr;
thread_local uint32_t OScriptLanguage::_call_stack_size = 0;
thread_local StringPtr OScriptLanguage::_debug_parse_err_file = StringPtr();
thread_local int OScriptLanguage::_debug_parse_err_line = 0;
thread_local StringPtr OScriptLanguage::_debug_error = StringPtr();

struct OScriptDepSort {
    //must support sorting so inheritance works properly (parent must be reloaded first)
    bool operator()(const Ref<OScript> &A, const Ref<OScript> &B) const {
        if (A == B) {
            return false; //shouldn't happen but..
        }
        const OScript *I = B->get_base().ptr();
        while (I) {
            if (I == A.ptr()) {
                // A is a base of B
                return true;
            }
            I = I->get_base().ptr();
        }

        return false; //not a base
    }
};

OScriptLanguage::CallLevel* OScriptLanguage::_get_stack_level(uint32_t p_level) {
    ERR_FAIL_UNSIGNED_INDEX_V(p_level, _call_stack_size, nullptr);
    CallLevel* level = _call_stack;
    uint32_t level_index = 0;
    while (p_level > level_index) {
        level_index++;
        level = level->prev;
    }
    return level;
}

void OScriptLanguage::_add_global(const StringName& p_name, const Variant& p_value) {
    if (globals.has(p_name)) {
        // Overwrite existing
        global_array.write[globals[p_name]] = p_value;
        return;
    }

    if (global_array_empty_indexes.size()) {
        const int index = global_array_empty_indexes[global_array_empty_indexes.size() - 1];
        globals[p_name] = index;
        global_array.write[index] = p_value;
        global_array_empty_indexes.resize(global_array_empty_indexes.size() - 1);
    } else {
        globals[p_name] = global_array.size(); // NOLINT
        global_array.push_back(p_value);
        _global_array = global_array.ptrw();
    }
}

void OScriptLanguage::_remove_global(const StringName& p_name) {
    if (!globals.has(p_name)) {
        return;
    }

    global_array_empty_indexes.push_back(globals[p_name]);
    global_array.write[globals[p_name]] = Variant::NIL;
    globals.erase(p_name);
}

String OScriptLanguage::_get_name() const {
    // todo: GDScript uses "GDScript" as type
    // return TYPE;
    return "OScript";
}

void OScriptLanguage::_init() {
    _debug_max_call_stack = ORCHESTRATOR_GET("settings/runtime/max_call_stack", 1024);
    track_call_stack = ORCHESTRATOR_GET("settings/runtime/always_track_call_stacks", false);
    track_locals = ORCHESTRATOR_GET("settings/runtime/always_track_local_variables", false);
    _call_stack = nullptr;

    #if DEBUG_ENABLED
    // GDScript populates this in the ctor, but we can't do that here because singletons
    // are not yet available in the GDExtension when constructors are called.
    track_call_stack = true;
    track_locals = track_locals || EngineDebugger::get_singleton()->is_active();
    #endif

    const String storage_format = ORCHESTRATOR_GET("settings/storage_format", "Text");
    if (storage_format.match("Binary")) {
        _extension = ORCHESTRATOR_SCRIPT_EXTENSION;
    }

    // Populate CoreConstants
    uint32_t core_constants_count = GDE::CoreConstants::get_global_constant_count();
    for (uint32_t i = 0; i < core_constants_count; i++) {
        _add_global(GDE::CoreConstants::get_global_constant_name(i), GDE::CoreConstants::get_global_constant_value(i));
    }

    // Math constants
    for (const String& name : ExtensionDB::get_math_constant_names()) {
        const ConstantInfo& constant = ExtensionDB::get_math_constant(name);
        _add_global(constant.name, constant.value);
    }

    // Native classes
    for (const String& class_name : ClassDB::get_class_list()) {
        if (globals.has(class_name)) {
            continue;
        }
        Ref<OScriptNativeClass> nc = memnew(OScriptNativeClass(class_name));
        _add_global(class_name, nc);
    }

    // Populate singletons
    for (const String& singleton_class : Engine::get_singleton()->get_singleton_list()) {
        _add_global(singleton_class, Engine::get_singleton()->get_singleton(singleton_class));
    }
}

String OScriptLanguage::_get_type() const {
    // todo: GDScript uses "GDScript" as type
    return OScript::get_class_static();
}

String OScriptLanguage::_get_extension() const {
    return _extension;
}

void OScriptLanguage::_finish() {
    if (finishing) {
        return;
    }

    finishing = true;

    OScriptCache::clear();

    SelfList<OScript> *s = _scripts.first();
    while (s) {
        Ref<OScript> scr = s->self();
        if (scr.is_valid()) {
            for (KeyValue<StringName, OScriptCompiledFunction*>& E : scr->member_functions) {
                OScriptCompiledFunction* func = E.value;
                for (int i = 0; i < func->argument_types.size(); i++) {
                    func->argument_types.write[i].script_type_ref = Ref<Script>();
                }
                func->return_type.script_type_ref = Ref<Script>();
            }
            for (KeyValue<StringName, OScript::MemberInfo>& E : scr->member_indices) {
                E.value.data_type.script_type_ref = Ref<Script>();
            }
            scr->clear();
        }
        s = s->next();
    }

    #if GODOT_VERSION >= 0x040500
    _scripts.clear();
    function_list.clear();
    #else
    while (_scripts.first()) {
        _scripts.remove(_scripts.first());
    }
    while (function_list.first()) {
        function_list.remove(function_list.first());
    }
    #endif

    finishing = false;
}

Ref<Script> OScriptLanguage::_make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const {
    // NOTE:
    // The p_template argument is the content of the template, set in _get_built_in_templates.
    // Even if the user deselects the template option in the script dialog, this method is called.
    //
    // The p_class_name is derived from the file name.
    // The p_base_class_name is the actor/class type the script inherits from.
    //
    Ref<OScript> script;
    script.instantiate();

    Ref<Orchestration> orchestration;
    orchestration.instantiate();
    orchestration->set_base_type(p_base_class_name);
    orchestration->create_graph("EventGraph", OScriptGraph::GF_EVENT);
    orchestration->_self = script.ptr();
    script->orchestration = orchestration;

    orchestration->post_initialize();

    return script;
}

TypedArray<Dictionary> OScriptLanguage::_get_built_in_templates(const StringName& p_object) const {
    Dictionary data;
    data["inherit"] = p_object;
    data["name"] = "Orchestration";
    data["description"] = "Basic Orchestration";
    data["content"] = "";
    data["id"] = 0;
    data["origin"] = 0; //built-in

    return Array::make(data);
}

Dictionary OScriptLanguage::_validate(const String& p_script, const String& p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const {
    // Called by ScriptTextEditor::_validate_script, ScriptTextEditor::_validate
    // These cases do not apply to us since we don't use the ScriptTextEditor, so just return valid.
    return DictionaryUtils::of({{"valid", true}});
}

Object* OScriptLanguage::_create_script() const {
    // todo: this does not appear to be called in Godot.

    OScript* script = memnew(OScript);

    Ref<Orchestration> orchestration;
    orchestration.instantiate();
    orchestration->set_base_type(ORCHESTRATOR_GET("settings/default_type", "Node"));
    orchestration->create_graph("EventGraph", OScriptGraph::GF_EVENT);

    orchestration->_self = script;
    script->orchestration = orchestration;

    orchestration->post_initialize();

    return script;
}

int32_t OScriptLanguage::_find_function(const String& p_function_name, const String& p_code) const {
    // Locates the function name in the specified code.
    // For visual scripts, can we use this somehow?
    return -1;
}

String OScriptLanguage::_make_function(const String& p_class_name, const String& p_function_name, const PackedStringArray& p_function_args) const {
    // Creates a function stub for the given name.
    // This is called by the ScriptTextEditor::add_callback
    // Since we don't use the ScriptTextEditor, this doesn't apply.
    return {};
}

void OScriptLanguage::_add_global_constant(const StringName& p_name, const Variant& p_value) {
    _add_global(p_name, p_value);
}

void OScriptLanguage::_add_named_global_constant(const StringName& p_name, const Variant& p_value) {
    named_globals[p_name] = p_value;
}

void OScriptLanguage::_remove_named_global_constant(const StringName& p_name) {
    ERR_FAIL_COND(!named_globals.has(p_name));
    named_globals.erase(p_name);
}

String OScriptLanguage::_debug_get_error() const {
    return _debug_error.get();
}

int32_t OScriptLanguage::_debug_get_stack_level_count() const {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0)
        return 1;
    return _call_stack_size;
    #else
    return 0;
    #endif
}

int32_t OScriptLanguage::_debug_get_stack_level_line(int32_t p_level) const {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return _debug_parse_err_line;
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, -1);
    return *(_get_stack_level(p_level)->node);
    #else
    return -1;
    #endif
}

String OScriptLanguage::_debug_get_stack_level_function(int32_t p_level) const {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return {};
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, {});
    OScriptCompiledFunction* func = _get_stack_level(p_level)->function;
    return func ? String(func->get_name()) : "";
    #else
    return {};
    #endif
}

String OScriptLanguage::_debug_get_stack_level_source(int32_t p_level) const {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return _debug_parse_err_file.get();
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, {});
    return _get_stack_level(p_level)->function->get_source();
    #else
    return {};
    #endif
}

Dictionary OScriptLanguage::_debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return {};
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, {});

    CallLevel* cl = _get_stack_level(p_level);
    OScriptCompiledFunction* func = cl->function;

    List<Pair<StringName, int>> locals;
    func->debug_get_stack_member_state(*cl->node, &locals);

    PackedStringArray local_names;
    Array local_values;

    local_names.push_back("Script Node ID");
    local_values.push_back(*cl->node);

    for (const Pair<StringName, int>& E : locals) {
        local_names.push_back(E.first);
        local_values.push_back(cl->stack[E.second]);
    }

    Dictionary result;
    result["locals"] = local_names;
    result["values"] = local_values;
    return result;
    #else
    return {};
    #endif
}

Dictionary OScriptLanguage::_debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return {};
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, {});

    CallLevel* cl = _get_stack_level(p_level);
    OScriptInstance* instance = cl->instance;
    if (!instance) {
        return {};
    }

    Ref<OScript> script = instance->get_script();
    ERR_FAIL_COND_V(script.is_null(), {});

    PackedStringArray member_names;
    Array member_values;

    const HashMap<StringName, OScript::MemberInfo>& mi = script->debug_get_member_indices();
    for (const KeyValue<StringName, OScript::MemberInfo>& E : mi) {
        member_names.push_back(E.key);
        member_values.push_back(instance->debug_get_member_by_index(E.value.index));
    }

    Dictionary members;
    members["members"] = member_names;
    members["values"] = member_values;

    return members;
    #else
    return {};
    #endif
}

void* OScriptLanguage::_debug_get_stack_level_instance(int32_t p_level) {
    #if GODOT_VERSION >= 0x040300
    if (_debug_parse_err_line >= 0) {
        return nullptr;
    }

    ERR_FAIL_INDEX_V(p_level, _call_stack_size, nullptr);
    return _get_stack_level(p_level)->instance->_script_instance;
    #else
    return nullptr;
    #endif
}

Dictionary OScriptLanguage::_debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth) {
    #if GODOT_VERSION >= 0x040300
    const HashMap<StringName, int>& name_index = get_global_map();
    const Variant* gl_array = get_global_array();

    PackedStringArray global_names;
    Array global_values;

    const Dictionary cinfo = _get_public_constants();
    for (const KeyValue<StringName, int>& E : name_index) {
        if (OScriptAnalyzer::class_exists(E.key) || Engine::get_singleton()->has_singleton(E.key)) {
            continue;
        }

        const bool is_script_constant = cinfo.keys().has(E.key);
        if (is_script_constant) {
            continue;
        }

        const Variant& var = gl_array[E.value];
        bool freed = false;
        const Object* obj = GDE::Variant::get_validated_object_with_check(var, freed);
        if (obj && !freed) {
            if (Object::cast_to<OScriptNativeClass>(obj)) {
                continue;
            }
        }

        bool skip = false;
        for (int i = 0; i < GDE::CoreConstants::get_global_constant_count(); i++) {
            if (E.key == GDE::CoreConstants::get_global_constant_name(i)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }

        global_names.push_back(E.key);
        global_values.push_back(var);
    }

    Dictionary results;
    results["globals"] = global_names;
    results["values"] = global_values;
    return results;
    #else
    return {};
    #endif
}

String OScriptLanguage::_debug_parse_stack_level_expression(int32_t p_level, const String& p_expression, int32_t p_max_subitems, int32_t p_max_depth) {
    List<String> names;
    List<Variant> values;

    const Dictionary data = _debug_get_stack_level_locals(p_level, p_max_subitems, p_max_depth);

    Ref<Expression> expression;
    expression.instantiate();
    if (expression->parse(p_expression, data["names"]) == OK) {
        OScriptInstanceBase* inst = static_cast<OScriptInstanceBase*>(_debug_get_stack_level_instance(p_level));
        if (inst) {
            Variant ret = expression->execute(data["values"], inst->get_owner());

            String value;
            OScriptVariantWriter::write_to_string(ret, value);
            return value;
        }
    }

    return {};
}

TypedArray<Dictionary> OScriptLanguage::_debug_get_current_stack_info() {
    TypedArray<Dictionary> array;
    #if GODOT_VERSION >= 0x040300
    CallLevel* cl = _call_stack;
    while (cl) {
        Dictionary data;
        data["file"] = cl->function->get_script()->get_script_path();
        data["func"] = cl->function->get_name();
        data["line"] = cl->node;
        array.append(data);
        cl = cl->prev;
    }
    #endif
    return array;
}

void OScriptLanguage::_reload_all_scripts()
{
    #ifdef DEBUG_ENABLED
    print_verbose("OScript: Reloading all scripts");
    Array scripts;
    {
        MutexLock script_lock(*lock.ptr());
        SelfList<OScript>* elem = _scripts.first();
        while (elem) {
            if (ResourceUtils::is_file(elem->self()->get_path())) {
                print_verbose("OScript: Found: " + elem->self()->get_path());
                scripts.push_back(Ref<OScript>(elem->self())); // cast to avoid erasure by accident
            }
            elem = elem->next();
        }
        #if TOOLS_ENABLED
        if (Engine::get_singleton()->is_editor_hint()) {
            // Reload all pointers to existing singletons so that tool scripts can work with reloaded extensions.
            for (const String& singleton_name : Engine::get_singleton()->get_singleton_list()) {
                if (globals.has(singleton_name)) {
                    Object* singleton = Engine::get_singleton()->get_singleton(singleton_name);
                    _add_global(singleton_name, singleton);
                }
            }
        }
        #endif
    }

    _reload_scripts(scripts, true);
    #endif // DEBUG_ENABLED
}

void OScriptLanguage::_reload_scripts(const Array& p_scripts, bool p_soft_reload) {
    #ifdef DEBUG_ENABLED
    List<Ref<OScript>> scripts;
    {
        MutexLock script_lock(*lock.ptr());
        SelfList<OScript>* elem = _scripts.first();
        while (elem) {
            if (elem->self()->is_root_script() && !elem->self()->get_path().is_empty()) {
                scripts.push_back(Ref<OScript>(elem->self()));
            }
            elem = elem->next();
        }
    }

    HashMap<Ref<OScript>, HashMap<ObjectID, List<Pair<StringName, Variant>>>> to_reload;

    scripts.sort_custom<OScriptDepSort>(); // update in inheritance dependency order
    for (const Ref<OScript>& script : scripts) {
        bool reload = p_scripts.has(script) || to_reload.has(script->get_base());
        if (!reload) {
            continue;
        }

        to_reload.insert(script, HashMap<ObjectID, List<Pair<StringName, Variant>>>());
        if (!p_soft_reload) {
            // Save state and remove script from instance
            HashMap<ObjectID, List<Pair<StringName, Variant>>>& map = to_reload[script];
            while (script->instances.front()) {
                Object* object = script->instances.front()->get();
                List<Pair<StringName, Variant>> state;
                Ref<Script> object_script = object->get_script();
                if (object_script.is_valid()) {
                    OScriptInstanceBase* instance = script->instance_script_instances[object];
                    instance->get_property_state(state);
                    map[ObjectID(object->get_instance_id())] = state;
                    object->set_script(Variant());
                }
            }

            #ifdef DEBUG_ENABLED
            // Same for placeholders
            while (script->placeholders.size()) {
                OScriptPlaceHolderInstance* placeholder = *script->placeholders.begin();
                Object* object = placeholder->get_owner();
                Ref<Script> object_script = object->get_script();
                if (object_script.is_valid()) {
                    // Save instance information
                    map.insert(ObjectID(object->get_instance_id()), List<Pair<StringName, Variant>>());
                    List<Pair<StringName, Variant>>& state = map[ObjectID(object->get_instance_id())];
                    placeholder->get_property_state(state);
                    object->set_script(Variant());
                } else {
                    // No instance found, remove it so we don't loop forever
                    script->placeholders.erase(placeholder);
                }
            }
            #endif

            for (const KeyValue<ObjectID, List<Pair<StringName, Variant>>>& F : script->pending_reload_state) {
                map[F.key] = F.value; // pending to reload, use this one instead
            }
        }
    }

    for (KeyValue<Ref<OScript>, HashMap<ObjectID, List<Pair<StringName, Variant>>>>& E : to_reload) {
        Ref<OScript> script = E.key;
        print_verbose("OScript: Reloading " + script->get_path());
        if (script->is_built_in()) {
            // TODO: It would be nice to do it more efficiently than loading the whole scene again
            Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(script->get_path().get_slice("::", 0), "", ResourceLoader::CACHE_MODE_IGNORE_DEEP);
            ERR_CONTINUE(scene.is_null());

            // todo: Requires https://github.com/CraterCrash/godot-orchestrator/issues/1140
            ERR_CONTINUE_MSG(true, "Cannot reload built-in script " + script->get_path() +
                ". See https://github.com/CraterCrash/godot-orchestrator/issues/1140");

            // Ref<SceneState> state = scene->get_state();
            // Ref<OScript> fresh = state->get_sub_resource(script->get_path());
            // ERR_CONTINUE(fresh.is_null());
            //
            // script->set_source_code(fresh->get_source_code());
        } else {
            script->load_source_code(script->get_path());
        }

        // Restore state if saved
        for (KeyValue<ObjectID, List<Pair<StringName, Variant>>>& F : E.value) {
            List<Pair<StringName, Variant>>& saved_state = F.value;

            Object *obj = ObjectDB::get_instance(F.key);
            if (!obj) {
                continue;
            }

            if (!p_soft_reload) {
                //clear it just in case (may be a pending reload state)
                obj->set_script(Variant());
            }
            obj->set_script(script);

            OScriptInstanceBase* inst = script->instance_script_instances[obj];
            if (!inst) {
                if (!script->pending_reload_state.has(ObjectID(obj->get_instance_id()))) {
                    script->pending_reload_state[ObjectID(obj->get_instance_id())] = saved_state;
                }
                continue;
            }

            if (inst->is_placeholder() && script->_is_placeholder_fallback_enabled()) {
                OScriptPlaceHolderInstance* placeholder = reinterpret_cast<OScriptPlaceHolderInstance*>(inst);
                for (List<Pair<StringName, Variant>>::Element *G = saved_state.front(); G; G = G->next()) {
                    bool valid;
                    placeholder->property_set_fallback(G->get().first, G->get().second, &valid);
                }
            } else {
                for (List<Pair<StringName, Variant>>::Element *G = saved_state.front(); G; G = G->next()) {
                    inst->set(G->get().first, G->get().second);
                }
            }

            script->pending_reload_state.erase(ObjectID(obj->get_instance_id())); // reloaded, remove pending state
        }
    }
    #endif
}

void OScriptLanguage::_reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) {
    #if GODOT_VERSION >= 0x040500
    Array scripts = { p_script };
    #else
    Array scripts;
    scripts.push_back(p_script);
    #endif
    _reload_scripts(scripts, p_soft_reload);
}

PackedStringArray OScriptLanguage::_get_recognized_extensions() const {
    return { Array::make(ORCHESTRATOR_SCRIPT_EXTENSION, ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) };
}

TypedArray<Dictionary> OScriptLanguage::_get_public_functions() const {
    TypedArray<Dictionary> results;
    for (const StringName& function : OScriptUtilityFunctions::get_function_list()) {
        const MethodInfo method = OScriptUtilityFunctions::get_function_info(function);
        results.push_back(DictionaryUtils::from_method(method));
    }
    return results;
}

Dictionary OScriptLanguage::_get_public_constants() const {
    return {};
}

void OScriptLanguage::_profiling_start() {
    #ifdef DEBUG_ENABLED
    MutexLock function_lock(*lock.ptr());

    SelfList<OScriptCompiledFunction> *elem = function_list.first();
    while (elem) {
        elem->self()->profile.call_count.set(0);
        elem->self()->profile.self_time.set(0);
        elem->self()->profile.total_time.set(0);
        elem->self()->profile.frame_call_count.set(0);
        elem->self()->profile.frame_self_time.set(0);
        elem->self()->profile.frame_total_time.set(0);
        elem->self()->profile.last_frame_call_count = 0;
        elem->self()->profile.last_frame_self_time = 0;
        elem->self()->profile.last_frame_total_time = 0;
        elem->self()->profile.native_calls.clear();
        elem->self()->profile.last_native_calls.clear();
        elem = elem->next();
    }
    profiling = true;
    #endif
}

void OScriptLanguage::_profiling_stop() {
    #ifdef DEBUG_ENABLED
    MutexLock function_lock(*lock.ptr());
    profiling = false;
    #endif
}

void OScriptLanguage::_profiling_set_save_native_calls(bool p_enable) {
    #ifdef DEBUG_ENABLED
    MutexLock function_lock(*lock.ptr());
    profile_native_calls = p_enable;
    #endif
}

int32_t OScriptLanguage::_profiling_get_accumulated_data(ScriptLanguageExtensionProfilingInfo* p_info_array, int32_t p_info_max) {
    int current = 0;
    #ifdef DEBUG_ENABLED

    MutexLock profile_lock(*lock.ptr());

    profiling_collate_native_call_data(true);
    SelfList<OScriptCompiledFunction>* elem = function_list.first();
    while (elem) {
        if (current >= p_info_max) {
            break;
        }
        int last_non_internal = current;
        p_info_array[current].call_count = elem->self()->profile.call_count.get();
        p_info_array[current].self_time = elem->self()->profile.self_time.get();
        p_info_array[current].total_time = elem->self()->profile.total_time.get();
        p_info_array[current].signature = elem->self()->profile.signature;
        current++;

        int nat_time = 0;
        HashMap<String, OScriptCompiledFunction::Profile::NativeProfile>::ConstIterator nat_calls = elem->self()->profile.native_calls.begin();
        while (nat_calls) {
            p_info_array[current].call_count = nat_calls->value.call_count;
            p_info_array[current].total_time = nat_calls->value.total_time;
            p_info_array[current].self_time = nat_calls->value.total_time;
            p_info_array[current].signature = nat_calls->value.signature;
            nat_time += nat_calls->value.total_time;
            current++;
            ++nat_calls;
        }
        // todo: this does not yet appear exposed
        // p_info_array[last_non_internal].internal_time = nat_time;
        elem = elem->next();
    }
    #endif

    return current;
}

int32_t OScriptLanguage::_profiling_get_frame_data(ScriptLanguageExtensionProfilingInfo* p_info_array, int32_t p_info_max) {
    int current = 0;

    #ifdef DEBUG_ENABLED
    MutexLock profile_lock(*lock.ptr());

    profiling_collate_native_call_data(false);
    SelfList<OScriptCompiledFunction>* elem = function_list.first();
    while (elem) {
        if (current >= p_info_max) {
            break;
        }
        if (elem->self()->profile.last_frame_call_count > 0) {
            int last_non_internal = current;
            p_info_array[current].call_count = elem->self()->profile.last_frame_call_count;
            p_info_array[current].self_time = elem->self()->profile.last_frame_self_time;
            p_info_array[current].total_time = elem->self()->profile.last_frame_total_time;
            p_info_array[current].signature = elem->self()->profile.signature;
            current++;

            int nat_time = 0;
            HashMap<String, OScriptCompiledFunction::Profile::NativeProfile>::ConstIterator nat_calls = elem->self()->profile.last_native_calls.begin();
            while (nat_calls) {
                p_info_array[current].call_count = nat_calls->value.call_count;
                p_info_array[current].total_time = nat_calls->value.total_time;
                p_info_array[current].self_time = nat_calls->value.total_time;
                // todo: not yet exposed
                // p_info_array[current].internal_time = nat_calls->value.total_time;
                p_info_array[current].signature = nat_calls->value.signature;
                nat_time += nat_calls->value.total_time;
                current++;
                ++nat_calls;
            }
            // todo: not yet exposed
            // p_info_array[last_non_internal].internal_time = nat_time;
        }
        elem = elem->next();
    }
    #endif

    return current;
}

void OScriptLanguage::_frame() {
    #ifdef DEBUG_ENABLED
    if (profiling) {
        MutexLock function_lock(*lock.ptr());
        SelfList<OScriptCompiledFunction>* elem = function_list.first();
        while (elem) {
            elem->self()->profile.last_frame_call_count = elem->self()->profile.frame_call_count.get();
            elem->self()->profile.last_frame_self_time = elem->self()->profile.frame_self_time.get();
            elem->self()->profile.last_frame_total_time = elem->self()->profile.frame_total_time.get();
            elem->self()->profile.last_native_calls = elem->self()->profile.native_calls;
            elem->self()->profile.frame_call_count.set(0);
            elem->self()->profile.frame_self_time.set(0);
            elem->self()->profile.frame_total_time.set(0);
            elem->self()->profile.native_calls.clear();
            elem = elem->next();
        }
    }
    #endif
}

bool OScriptLanguage::_handles_global_class_type(const String& p_type) const {
    // todo: GDScript uses "GDScript" as type
    return p_type == _get_type();
}

Dictionary OScriptLanguage::_get_global_class_name(const String& p_path) const {
    // OrchestratorScripts do not have global class names
    return {};
}

void OScriptLanguage::add_orphan_subclass(const String& p_qualified_name, const ObjectID& p_subclass) {
    orphan_subclasses[p_qualified_name] = p_subclass;
}

Ref<OScript> OScriptLanguage::get_orphan_subclass(const String& p_qualified_name) {
    HashMap<String, ObjectID>::Iterator E = orphan_subclasses.find(p_qualified_name);
    if (!E) {
        return {};
    }

    Object* object = ObjectDB::get_instance(E->value);
    orphan_subclasses.remove(E);

    return object ? Ref<OScript>(cast_to<OScript>(object)) : Ref<OScript>();
}

Variant OScriptLanguage::get_any_global_constant(const StringName& p_name) {
    if (named_globals.has(p_name)) {
        return named_globals[p_name];
    }

    if (globals.has(p_name)) {
        return _global_array[globals[p_name]];
    }

    ERR_FAIL_V_MSG(Variant(), vformat("Could not find any global constant with name: %s.", p_name));
}

PackedStringArray OScriptLanguage::get_global_constant_names() const {
    PackedStringArray keys;
    keys.append_array(get_global_named_constant_names());

    for (const KeyValue<StringName, int>& E : globals) {
        if (!keys.has(E.key)) {
            keys.push_back(E.key);
        }
    }

    return keys;
}

PackedStringArray OScriptLanguage::get_global_named_constant_names() const {
    PackedStringArray keys;
    for (const KeyValue<StringName, Variant>& E : named_globals) {
        if (!keys.has(E.key)) {
            keys.push_back(E.key);
        }
    }
    return keys;
}

bool OScriptLanguage::debug_break(const String& p_error, bool p_allow_continue) {
    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active()) {
        _debug_parse_err_line = -1;
        _debug_parse_err_file = "";
        _debug_error = p_error;

        bool is_error_breakpoint = p_error != "Breakpoint";
        EngineDebugger::get_singleton()->script_debug(this, p_allow_continue, is_error_breakpoint);

        // Because this is a thread local, clear the memory afterward
        _debug_parse_err_file = String();
        _debug_error = String();
        return true;
    }
    #endif
    return false;
}

bool OScriptLanguage::debug_break_parse(const String& p_file, int p_node, const String& p_error) {
    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
    {
        if (OS::get_singleton()->get_thread_caller_id() == OS::get_singleton()->get_main_thread_id())
        {
            _debug_parse_err_line = p_node;
            _debug_parse_err_file = p_file;
            _debug_error = p_error;

            EngineDebugger::get_singleton()->script_debug(this, false, true);

            // Because this is a thread local, clear the memory afterward
            _debug_parse_err_file = String();
            _debug_error = String();

            return true;
        }
    }
    #endif
    return false;
}

void OScriptLanguage::enter_function(CallLevel* p_level, OScriptInstance* p_instance, OScriptCompiledFunction* p_function, Variant* p_stack, int* p_ip, int* p_node) {
    if (!track_call_stack) {
        return;
    }

    #ifdef DEBUG_ENABLED
    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->is_active() && debugger->get_lines_left() > 0 && debugger->get_depth() >= 0) {
        debugger->set_depth(debugger->get_depth() + 1);
    }
    #endif

    if (unlikely(_call_stack_size) >= _debug_max_call_stack) {
        _debug_error = vformat("Stack overflow detected (stack size: %s)", _debug_max_call_stack);
        #ifdef DEBUG_ENABLED
        if (debugger) {
            debugger->script_debug(this, false, false);
        }
        #endif
        return;
    }

    p_level->prev = _call_stack;
    _call_stack = p_level;

    p_level->stack = p_stack;
    p_level->instance = p_instance;
    p_level->function = p_function;
    p_level->ip = p_ip;
    p_level->node = p_node;

    _call_stack_size++;
}

void OScriptLanguage::exit_function() {
    if (!track_call_stack) {
        return;
    }

    #ifdef DEBUG_ENABLED
    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->is_active() && debugger->get_lines_left() > 0 && debugger->get_depth() >= 0) {
        debugger->set_depth(debugger->get_depth() - 1);
    }
    #endif

    if (unlikely(_call_stack_size == 0)) {
        #ifdef DEBUG_ENABLED
        _debug_error = "Stack Underflow (Engine Bug)";
        if (debugger) {
            debugger->script_debug(this, false, false);
        }
        #else
        ERR_PRINT("Stack Underflow! (Engine Bug)");
        #endif
        return;
    }

    _call_stack_size--;
    _call_stack = _call_stack->prev;
}

void OScriptLanguage::profiling_collate_native_call_data(bool p_accumulated) {
    #ifdef DEBUG_ENABLED
    // The same native call can be called from multiple functions, so join them together here.
    // Only use the name of the function (ie signature.split[2]).
    HashMap<String, OScriptCompiledFunction::Profile::NativeProfile*> seen_nat_calls;
    SelfList<OScriptCompiledFunction>* elem = function_list.first();
    while (elem) {
        HashMap<String, OScriptCompiledFunction::Profile::NativeProfile>* nat_calls = p_accumulated
            ? &elem->self()->profile.native_calls
            : &elem->self()->profile.last_native_calls;
        HashMap<String, OScriptCompiledFunction::Profile::NativeProfile>::Iterator it = nat_calls->begin();

        while (it != nat_calls->end()) {
            PackedStringArray sig = it->value.signature.split("::");
            HashMap<String, OScriptCompiledFunction::Profile::NativeProfile*>::ConstIterator already_found = seen_nat_calls.find(sig[2]);
            if (already_found) {
                already_found->value->total_time += it->value.total_time;
                already_found->value->call_count += it->value.call_count;
                elem->self()->profile.last_native_calls.remove(it);
            } else {
                seen_nat_calls.insert(sig[2], &it->value);
            }
            ++it;
        }
        elem = elem->next();
    }
    #endif
}

Ref<OScript> OScriptLanguage::get_script_by_fully_qualified_name(const String& p_name) {
    {
        MutexLock script_lock(*lock.ptr());
        SelfList<OScript>* elem = _scripts.first();
        while (elem) {
            OScript* scr = elem->self();
            if (scr->fully_qualified_name == p_name) {
                return scr;
            }
            elem = elem->next();
        }
    }

    Ref<OScript> scr;
    scr.instantiate();
    scr->fully_qualified_name = p_name;
    return scr;
}

String OScriptLanguage::get_script_extension_filter() const {
    PackedStringArray results;
    for (const String& extension : _get_recognized_extensions())
        results.push_back(vformat("*.%s", extension));

    return StringUtils::join(",", results);
}

#ifdef TOOLS_ENABLED
List<Ref<OScript>> OScriptLanguage::get_scripts() const {
    List<Ref<OScript>> scripts;
    {
        const PackedStringArray extensions = _get_recognized_extensions();

        MutexLock mutex_lock(*this->lock.ptr());
        const SelfList<OScript>* iterator = _scripts.first();
        while (iterator) {
            String path = iterator->self()->get_path();
            if (extensions.has(path.get_extension().to_lower())) {
                scripts.push_back(Ref<OScript>(iterator->self()));
            }
            iterator = iterator->next();
        }
    }
    return scripts;
}
#endif

bool OScriptLanguage::validate(const Ref<OScript>& p_script, const String& p_path, List<String>* r_functions, List<Warning>* r_warnings, List<ScriptError>* r_errors) {
    if (!p_script.is_valid()) {
        return false;
    }

    BuildLog build_log;
    for (const Ref<OScriptNode>& node : p_script->get_orchestration()->get_nodes()) {
        node->validate_node_during_build(build_log);
    }

    bool valid = true;
    for (const BuildLog::Failure& failure : build_log.get_failures()) {
        valid = false;

        switch (failure.type) {
            case BuildLog::FT_Warning: {
                if (r_warnings) {
                    Warning warning;
                    warning.node = failure.node->get_id();
                    warning.name = failure.node->get_node_title() + " / " + failure.node->get_class().replace("OScriptNode", "").capitalize();
                    warning.message = failure.message;
                    r_warnings->push_back(warning);
                }
                break;
            }
            case BuildLog::FT_Error: {
                if (r_errors) {
                    ScriptError error;
                    error.node = failure.node->get_id();
                    error.name = failure.node->get_node_title() + " / " + failure.node->get_class().replace("OScriptNode", "").capitalize();
                    error.message = failure.message;
                    r_errors->push_back(error);
                }
                break;
            }
        }
    }

    return valid;
}

OScriptLanguage* OScriptLanguage::get_singleton() {
    return _singleton;
}

OScriptNodePrintStringOverlay* OScriptLanguage::get_or_create_overlay() {
    MutexLock guard(*lock.ptr());
    return OScriptNodePrintStringOverlay::get_or_create_overlay();
}

void OScriptLanguage::_bind_methods() {
}

OScriptLanguage::OScriptLanguage() {
    ERR_FAIL_COND(_singleton);
    _singleton = this;
    lock.instantiate();

    strings._init = StringName("init");
    strings._static_init = StringName("_static_init");
    strings._notification = StringName("_notification");
    strings._set = StringName("_set");
    strings._get = StringName("_get");
    strings._get_property_list = StringName("_get_property_list");
    strings._validate_property = StringName("_validate_property");
    strings._property_can_revert = StringName("_property_can_revert");
    strings._property_get_revert = StringName("_property_get_revert");
    strings._script_source = StringName("script/source");
    _debug_parse_err_line = -1;
    _debug_parse_err_file = "";

    #ifdef DEBUG_ENABLED
    profiling = false;
    profile_native_calls = false;
    script_frame_time = 0;
    #endif // DEBUG_ENABLED

    // These are initialized in the "init" function
    _debug_max_call_stack = 0;
    track_call_stack = false;
    track_locals = false;
}

OScriptLanguage::~OScriptLanguage() {
    _singleton = nullptr;

    global_array.clear();
    globals.clear();
    named_globals.clear();
    global_array_empty_indexes.clear();
    _global_array = nullptr;
}