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
#include "script/script.h"

#include "common/dictionary_utils.h"
#include "common/resource_utils.h"
#include "core/godot/editor/file_system/editor_paths.h"
#include "core/godot/variant/variant.h"
#include "orchestration/serialization/text/text_parser.h"
#include "script/compiler/analyzer.h"
#include "script/compiler/compiler.h"
#include "script/nodes/script_nodes.h"
#include "script/parser/parser.h"
#include "script/script_instance.h"
#include "script/script_cache.h"
#include "script/script_server.h"

#ifdef TOOLS_ENABLED
#include "script/script_docgen.h"
#endif

#include "common/macros.h"
#include "core/godot/error_macros.h"
#include "core/godot/gdextension_compat.h"
#include "editor/debugger/script_debugger_plugin.h"
#include "orchestration/serialization/binary/binary_parser.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/mutex_lock.hpp>

OScript::UpdatableFuncPtr::UpdatableFuncPtr(OScriptCompiledFunction* p_function) {
    if (p_function == nullptr) {
        return;
    }

    ptr = p_function;
    script = ptr->get_script();
    ERR_FAIL_NULL(script);

    MutexLock script_lock(*script->func_ptrs_to_update_mutex.ptr());
    list_element = script->func_ptrs_to_update.push_back(this);
}

OScript::UpdatableFuncPtr::~UpdatableFuncPtr() {
    ERR_FAIL_NULL(script);

    if (list_element) {
        MutexLock scriptlock(*script->func_ptrs_to_update_mutex.ptr());
        list_element->erase();
        list_element = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScript

void OScript::_recurse_replace_function_ptrs(const HashMap<OScriptCompiledFunction*, OScriptCompiledFunction*>& p_replacements) const {
    MutexLock lock(*func_ptrs_to_update_mutex.ptr());
    for (UpdatableFuncPtr* updatable : func_ptrs_to_update) {
        HashMap<OScriptCompiledFunction*, OScriptCompiledFunction*>::ConstIterator replacement = p_replacements.find(updatable->ptr);
        if (replacement) {
            updatable->ptr = replacement->value;
        } else {
            // Probably a lambda from another reload, ignore.
            updatable->ptr = nullptr;
        }
    }

    for (HashMap<StringName, Ref<OScript>>::ConstIterator subscript = subclasses.begin(); subscript; ++subscript) {
        subscript->value->_recurse_replace_function_ptrs(p_replacements);
    }
}

#ifdef TOOLS_ENABLED
void OScript::_save_old_static_data() {
    old_static_variables_indices = static_variables_indices;
    old_static_variables = static_variables;
    for (KeyValue<StringName, Ref<OScript>>& inner : subclasses) {
        inner.value->_save_old_static_data();
    }
}

void OScript::_restore_old_static_data() {
    for (KeyValue<StringName, MemberInfo>& E : old_static_variables_indices) {
        if (static_variables_indices.has(E.key)) {
            static_variables.write[static_variables_indices[E.key].index] = old_static_variables[E.value.index];
        }
    }
    old_static_variables_indices.clear();
    old_static_variables.clear();
    for (KeyValue<StringName, Ref<OScript>>& inner : subclasses) {
        inner.value->_restore_old_static_data();
    }
}

void OScript::_add_doc(const DocData::ClassDoc& p_doc) { // NOLINT
    doc_class_name = p_doc.name;
    if (subclass_owner) {
        // Only the top-level class stores doc info.
        subclass_owner->_add_doc(p_doc);
    } else { // Remove old docs, add new.
        for (int i = 0; i < docs.size(); i++) {
            if (docs[i].name == p_doc.name) {
                docs.remove_at(i);
                break;
            }
        }
        docs.append(p_doc);
    }
}

void OScript::_clear_doc() {
    doc_class_name = StringName();
    doc = DocData::ClassDoc();
    docs.clear();
}
#endif

Error OScript::_static_init() { // NOLINT
    if (likely(_valid) && static_initializer) {
        GDExtensionCallError error;
        static_initializer->call(nullptr, nullptr, 0, error);
        if (error.error != GDEXTENSION_CALL_OK) {
            return ERR_CANT_CREATE;
        }
    }
    Error err = OK;
    for (const KeyValue<StringName, Ref<OScript>>& inner : subclasses) {
        err = inner.value->_static_init();
        if (err) {
            break;
        }
    }
    return err;
}

void OScript::_static_default_init() {
    for (const KeyValue<StringName, MemberInfo>& E : static_variables_indices) {
        const OScriptDataType& type = E.value.data_type;
        if (type.kind != OScriptDataType::BUILTIN) {
            continue;
        }
        if (type.builtin_type == Variant::ARRAY && type.has_container_element_type(0)) {
            const OScriptDataType element_type = type.get_container_element_type(0);
            Array default_value;
            default_value.set_typed(element_type.builtin_type, element_type.native_type, element_type.script_type);
            static_variables.write[E.value.index] = default_value;
        } else if (type.builtin_type == Variant::DICTIONARY && type.has_container_element_types()) {
            const OScriptDataType key_type = type.get_container_element_type_or_variant(0);
            const OScriptDataType value_type = type.get_container_element_type_or_variant(1);
            Dictionary default_value;
            default_value.set_typed(key_type.builtin_type, key_type.native_type, key_type.script_type,
                value_type.builtin_type, value_type.native_type, value_type.script_type);
            static_variables.write[E.value.index] = default_value;
        } else {
            Variant default_value;
            GDExtensionCallError error;
            GDExtensionVariantType vtype = static_cast<GDExtensionVariantType>(type.builtin_type);
            GDE_INTERFACE(variant_construct)(vtype, &default_value, nullptr, 0, &error);
            static_variables.write[E.value.index] = default_value;
        }
    }
}

Variant OScript::callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
    OScript* top = this;
    while (top) {
        if (likely(top->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = top->member_functions.find(p_method);
            if (E) {
                ERR_FAIL_COND_V_MSG(!E->value->is_static(), Variant(), "Can't call non-static function '" + String(p_method) + "' in script.");
                return E->value->call(nullptr, p_args, p_arg_count, r_error);
            }
        }
        top = top->base.ptr();
    }

    Variant result;
    Script* parent_this = this;
    GDE_INTERFACE(variant_call)(
        parent_this,
        &p_method,
        reinterpret_cast<GDExtensionConstVariantPtr*>(p_args),
        p_arg_count,
        &result,
        &r_error);

    return result;
}

OScriptCompiledFunction* OScript::_super_constructor(OScript* p_script) { // NOLINT
    if (likely(p_script->_valid) && p_script->initializer) {
        return p_script->initializer;
    } else {
        OScript* base_scr = p_script->base.ptr();
        if (base_scr != nullptr) {
            return _super_constructor(base_scr);
        } else {
            return nullptr;
        }
    }
}

void OScript::_super_implicit_constructor(OScript* p_script, OScriptInstance* p_instance, GDExtensionCallError& r_error) { // NOLINT
    OScript* base_scr = p_script->base.ptr();
    if (base_scr != nullptr) {
        _super_implicit_constructor(base_scr, p_instance, r_error);
        if (r_error.error != GDEXTENSION_CALL_OK) {
            return;
        }
    }

    ERR_FAIL_NULL(p_script->implicit_initializer);
    if (likely(p_script->_valid)) {
        p_script->implicit_initializer->call(p_instance, nullptr, 0, r_error);
    } else {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    }
}

OScriptInstance* OScript::_create_instance(const Variant** p_args, int p_arg_count, Object* p_owner, GDExtensionCallError& r_error) const {
    OScriptInstance* si = memnew(OScriptInstance(Ref<OScript>(this), p_owner));
    si->_members.resize(member_indices.size());
    si->_script = Ref<OScript>(this);
    si->_owner = p_owner;
    si->_owner_id = p_owner->get_instance_id();

    #ifdef DEBUG_ENABLED
    // Needed for hot reloading
    for (const KeyValue<StringName, MemberInfo>& E : member_indices) {
        si->_member_indices_cache[E.key] = E.value.index;
    }
    #endif

    si->set_instance_info(GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptInstance::INSTANCE_INFO, si));
    {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        instances.insert(p_owner);
        instance_script_instances[p_owner] = si;
    }

    // Hack to work around const
    OScript* self = const_cast<OScript*>(this);
    self->_super_implicit_constructor(self, si, r_error);

    if (r_error.error != GDEXTENSION_CALL_OK) {
        String error_text = GDE::Variant::get_call_error_text(si->get_owner(), "@implicit_new", nullptr, 0, r_error);
        si->_script = Ref<OScript>();
        si->_owner->set_script(Variant());
        {
            MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
            instances.erase(p_owner);
            instance_script_instances.erase(p_owner);
        }
        ERR_FAIL_V_MSG(nullptr, "Error constructing a OScriptInstance: " + error_text);
    }

    if (p_arg_count < 0) {
        return si;
    }

    OScriptCompiledFunction* initializer = self->_super_constructor(self);
    if (initializer != nullptr) {
        initializer->call(si, p_args, p_arg_count, r_error);
        if (r_error.error != GDEXTENSION_CALL_OK) {
            String error_text = GDE::Variant::get_call_error_text(si->get_owner(), "_init", p_args, p_arg_count, r_error);
            si->_script = Ref<OScript>();
            si->_owner->set_script(Variant());
            {
                MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
                instances.erase(p_owner);
                instance_script_instances.erase(p_owner);
            }
            ERR_FAIL_V_MSG(nullptr, "Error constructing a OScriptInstance: " + error_text);
        }
    }

    return si;
}

String OScript::_get_debug_path() const {
    if (is_built_in() && !get_name().is_empty()) {
        return vformat("%s(%s)", get_name(), get_script_path());
    }
    return get_script_path();
}

void OScript::_update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties) { // NOLINT
    #ifdef TOOLS_ENABLED
    for (const KeyValue<StringName, Variant>& E : member_default_values_cache) {
        r_values[E.key] = E.value;
    }

    for (const PropertyInfo& E : members_cache) {
        r_properties.push_back(E);
    }

    if (base_cache.is_valid()) {
        base_cache->_update_export_values(r_values, r_properties);
    }
    #endif
}

bool OScript::_update_exports_placeholder(bool* r_err, bool p_recursive_call, OScriptPlaceHolderInstance* p_instance_to_update, bool p_base_exports_changed) { // NOLINT
    #ifdef TOOLS_ENABLED
    static Vector<OScript*> base_caches;
    if (!p_recursive_call) {
        base_caches.clear();
    }
    base_caches.append(this);

    bool changed = p_base_exports_changed;
    if (source_changed_cache) {
        source_changed_cache = false;
        changed = true;

        String basedir = path;
        if (basedir.is_empty()) {
            basedir = get_path();
        }
        if (!basedir.is_empty()) {
            basedir = basedir.get_base_dir();
        }

        OScriptParser parser;
        OScriptAnalyzer analyzer(&parser);

        Error err = parser.parse(orchestration.ptr(), path);
        if (err == OK && analyzer.analyze() == OK) {
            const OScriptParser::ClassNode* c = parser.get_tree();
            if (base_cache.is_valid()) {
                base_cache->inheritors_cache.erase(ObjectID(get_instance_id()));
                base_cache = Ref<OScript>();
            }

            OScriptParser::DataType base_type = parser.get_tree()->base_type;
            if (base_type.kind == OScriptParser::DataType::CLASS) {
                Ref<OScript> bf = OScriptCache::get_full_script(base_type.script_path, err, path);
                if (err == OK) {
                    bf = Ref<OScript>(bf->find_class(base_type.class_type->fqcn));
                    if (bf.is_valid()) {
                        base_cache = bf;
                        bf->inheritors_cache.insert(ObjectID(get_instance_id()));
                    }
                }
            }

            members_cache.clear();
            member_default_values_cache.clear();
            signals.clear();

            members_cache.push_back(get_class_category());

            for (const OScriptParser::ClassNode::Member& member : c->members) {
                switch (member.type) {
                    case OScriptParser::ClassNode::Member::VARIABLE: {
                        if (!member.variable->exported) {
                            continue;
                        }
                        members_cache.push_back(member.variable->export_info);
                        Variant default_value = analyzer.make_variable_default_value(member.variable);
                        member_default_values_cache[member.variable->identifier->name] = default_value;
                    } break;
                    case OScriptParser::ClassNode::Member::SIGNAL: {
                        signals[member.signal->identifier->name] = member.signal->method;
                    } break;
                    case OScriptParser::ClassNode::Member::GROUP: {
                        members_cache.push_back(member.annotation->export_info);
                    } break;
                    default: {
                        // nothing
                    } break;
                }
            }
        } else {
            _placeholder_fallback_enabled = true;
            return false;
        }
    } else if (_placeholder_fallback_enabled) {
        return false;
    }

    _placeholder_fallback_enabled = false;

    if (base_cache.is_valid() && base_cache->_is_valid()) {
        for (OScript* base_cache_item : base_caches) {
            if (base_cache_item == base_cache.ptr()) {
                if (r_err) {
                    *r_err = true;
                }
                _valid = false;
                base_cache->_valid = false;
                base_cache->inheritors_cache.clear();
                base_cache.unref();
                base.unref();
                ERR_FAIL_V_MSG(false, "Cyclic inheritance in script class.");
            }
        }
        if (base_cache->_update_exports_placeholder(r_err, true)) {
            if (r_err && *r_err) {
                return false;
            }
            changed = true;
        }
    }

    if ((changed || p_instance_to_update) && placeholders.size()) {
        HashMap<StringName, Variant> values;
        List<PropertyInfo> property_names;
        _update_export_values(values, property_names);

        if (changed) {
            for (OScriptPlaceHolderInstance* E : placeholders) {
                E->update(property_names, values);
            }
        } else if (p_instance_to_update) {
            p_instance_to_update->update(property_names, values);
        }
    }

    return changed;
    #else
    return false;
    #endif
}

#ifdef TOOLS_ENABLED
void OScript::_update_exports_down(bool p_base_exports_changed) { // NOLINT
    bool cyclic_error = false;
    bool changed = _update_exports_placeholder(&cyclic_error, false, nullptr, p_base_exports_changed);
    if (cyclic_error) {
        return;
    }

    HashSet<ObjectID> copy = inheritors_cache;
    for (const ObjectID& E : copy) {
        Object* instance = ObjectDB::get_instance(E);
        OScript* script = cast_to<OScript>(instance);
        if (!script) {
            continue;
        }
        script->_update_exports_down(p_base_exports_changed || changed);
    }
}
#endif

TypedArray<Dictionary> OScript::_get_script_properties(bool p_include_base) const {
    const OScript* sptr = this;

    TypedArray<Dictionary> results;

    List<PropertyInfo> properties;
    while (sptr) {
        Vector<OScriptMemberSort> msort;
        for (const KeyValue<StringName, MemberInfo>& E : sptr->member_indices) {
            if (!sptr->members.has(E.key)) {
                continue; // Skip base class members.
            }
            OScriptMemberSort ms;
            ms.index = E.value.index;
            ms.name = E.key;
            msort.push_back(ms);
        }

        msort.sort();
        msort.reverse();

        for (const OScriptMemberSort& item : msort) {
            properties.push_front(sptr->member_indices[item.name].property_info);
        }

        #ifdef TOOLS_ENABLED
        results.push_back(DictionaryUtils::from_property(sptr->get_class_category()));
        #endif

        for (const PropertyInfo& E : properties) {
            results.push_back(DictionaryUtils::from_property(E));
        }

        if (!p_include_base) {
            break;
        }

        properties.clear();
        sptr = sptr->base.ptr();
    }

    return results;
}

TypedArray<Dictionary> OScript::_get_script_methods(bool p_include_base) const {
    TypedArray<Dictionary> results;

    const OScript* sptr = this;
    while (sptr) {
        for (const KeyValue<StringName, OScriptCompiledFunction*>& E : sptr->member_functions) {
            results.push_back(DictionaryUtils::from_method(E.value->method_info));
        }

        if (!p_include_base) {
            break;
        }

        sptr = sptr->base.ptr();
    }

    return results;
}

TypedArray<Dictionary> OScript::_get_script_signals(bool p_include_base) const { // NOLINT
    TypedArray<Dictionary> list;

    for (const KeyValue<StringName, MethodInfo>& E : signals) {
        list.push_back(DictionaryUtils::from_method(E.value));
    }

    if (p_include_base) {
        if (base.is_valid()) {
            list.append_array(base->_get_script_signals(p_include_base));
        }
        #ifdef TOOLS_ENABLED
        else if (base_cache.is_valid()) {
            list.append_array(base_cache->_get_script_signals(p_include_base));
        }
        #endif
    }

    return list;
}

OScript* OScript::_get_from_variant(const Variant& p_value) { // NOLINT
    Object* obj = p_value;
    if (obj == nullptr || ObjectID(obj->get_instance_id()).is_null()) {
        return nullptr;
    }
    return cast_to<OScript>(obj);
}

void OScript::_collect_function_dependencies(OScriptCompiledFunction* p_function, RBSet<OScript*>& p_dependencies, const OScript* p_except) { // NOLINT
    if (p_function == nullptr) {
        return;
    }
    for (OScriptCompiledFunction* lambda : p_function->lambdas) {
        _collect_function_dependencies(lambda, p_dependencies, p_except);
    }
    for (const Variant& value : p_function->constants) {
        OScript* script = _get_from_variant(value);
        if (script != nullptr && script != p_except) {
            script->_collect_dependencies(p_dependencies, p_except);
        }
    }
}

void OScript::_collect_dependencies(RBSet<OScript*>& p_dependencies, const OScript* p_except) { // NOLINT
    if (p_dependencies.has(this)) {
        return;
    }
    if (this != p_except) {
        p_dependencies.insert(this);
    }
    for (const KeyValue<StringName, OScriptCompiledFunction*>& E : member_functions) {
        _collect_function_dependencies(E.value, p_dependencies, p_except);
    }
    if (implicit_initializer) {
        _collect_function_dependencies(implicit_initializer, p_dependencies, p_except);
    }
    if (implicit_ready) {
        _collect_function_dependencies(implicit_ready, p_dependencies, p_except);
    }
    if (static_initializer) {
        _collect_function_dependencies(static_initializer, p_dependencies, p_except);
    }
    for (const KeyValue<StringName, Ref<OScript>>& E : subclasses) {
        if (E.value != p_except) {
            E.value->_collect_dependencies(p_dependencies, p_except);
        }
    }
    for (const KeyValue<StringName, Variant>& E : constants) {
        OScript* script = _get_from_variant(E.value);
        if (script != nullptr && script != p_except) {
            script->_collect_dependencies(p_dependencies, p_except);
        }
    }
}

bool OScript::_editor_can_reload_from_file() {
    return false;
}

void OScript::_placeholder_erased(void* p_placeholder) {
    #ifdef TOOLS_ENABLED
    OScriptPlaceHolderInstance* psi = static_cast<OScriptPlaceHolderInstance*>(p_placeholder);
    if (psi) {
        placeholders.erase(psi);
        instance_script_instances.erase(psi->get_owner());
    }
    #endif
}

bool OScript::_can_instantiate() const {
    #ifdef TOOLS_ENABLED
    // Normally in the Engine codebase, when recovery mode hint is toggled, scripting languages always would
    // return false to this method because recovery mode prevents it; however, because OScript is defined in
    // the context of GDExtension, extensions are not loaded in recovery, so we can ignore that requirement
    // as the OScript language won't be enabled.
    return _valid && (_is_tool() || ScriptServer::is_scripting_enabled());
    #else
    return _valid;
    #endif
}

Ref<Script> OScript::_get_base_script() const {
    return base;
}

StringName OScript::_get_global_name() const {
    return global_name;
}

bool OScript::_inherits_script(const Ref<Script>& p_script) const {
    Ref<OScript> scr = p_script;
    if (scr.is_null()) {
        return false;
    }

    const OScript* sptr = this;
    while (sptr) {
        if (sptr == p_script.ptr()) {
            return true;
        }
        sptr = sptr->base.ptr();
    }

    return false;
}

StringName OScript::_get_instance_base_type() const { // NOLINT
    if (native.is_valid()) {
        return native->get_name();
    }
    if (base.is_valid() && base->_is_valid()) {
        return base->_get_instance_base_type();
    }
    return {};
}

void* OScript::_instance_create(Object* p_object) const {
    ERR_FAIL_COND_V_MSG(!_valid, nullptr, "Script is invalid!");

    const OScript* scr = this;
    while (scr->base.ptr()) {
        scr = scr->base.ptr();
    }

    if (scr->native.is_valid()) {
        if (!ClassDB::is_parent_class(p_object->get_class(), scr->native->get_name())) {
            const String message = vformat(
                "Orchestration inherits from native type '%s', so it can't be assigned to an object of type: '%s'",
                scr->native->get_name(), p_object->get_class());
            if (EngineDebugger::get_singleton()->is_active()) {
                OScriptLanguage::get_singleton()->debug_break_parse(_get_debug_path(), 1, message);
            }
            ERR_FAIL_V_MSG(nullptr, message);
        }
    }

    GDExtensionCallError err;
    OScriptInstance* instance = _create_instance(nullptr, 0, p_object, err);
    return instance ? instance->get_instance_info() : nullptr;
}

void* OScript::_placeholder_instance_create(Object* p_object) const {
    #ifdef TOOLS_ENABLED
    const String name = cast_to<Node>(p_object) ? cast_to<Node>(p_object)->get_name() : "<unnamed>";

    OScriptPlaceHolderInstance* psi = memnew(OScriptPlaceHolderInstance(Ref<OScript>(this), p_object));
    psi->set_instance_info(GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptPlaceHolderInstance::INSTANCE_INFO, psi));
    {
        MutexLock lock(*_language->lock.ptr());
        instance_script_instances[p_object] = psi;
        placeholders.insert(psi);
    }

    // Hack because this method is exposed as const via ScriptExtension
    OScript* self = const_cast<OScript*>(this);
    self->_update_exports_placeholder(nullptr, false, psi);

    return psi->get_instance_info();
    #else
    return nullptr;
    #endif
}

bool OScript::_instance_has(Object* p_object) const {
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    return instances.has(p_object);
}

bool OScript::_has_source_code() const {
    return false;
}

String OScript::_get_source_code() const {
    return {};
}

void OScript::_set_source_code(const String& p_code) {
    // See https://github.com/godotengine/godot/pull/115157
    //
    // When a script language supports documentation, and a script should be reloaded, the EditorFileSystem
    // will call the Script::reload_from_file. This method reloads the script off disk and then calls
    // set_source_code(reloaded_script->get_source_code()).
    //
    // To address this difference with OScript, in which the source may not be represented as a "String"
    // but could be a PackedByteArray for binary resources, the virtual "Resource::reload_from_file()"
    // method should be overridable for custom resources.
    //
}

Error OScript::_reload(bool p_keep_state) {
    if (reloading) {
        return OK;
    }

    reloading = true;

    bool has_instances;
    {
        MutexLock lock(*_language->lock.ptr());
        has_instances = instances.size();
    }

    // Check condition but reset flag before early return
    if (!p_keep_state && has_instances) {
        reloading = false;
        ERR_FAIL_V_MSG(ERR_ALREADY_IN_USE, "Cannot reload script while instances exist.");
    }

    String basedir = path;
    if (basedir.is_empty()) {
        basedir = get_path();
    }
    if (!basedir.is_empty()) {
        basedir = basedir.get_base_dir();
    }

    #ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint() && basedir.begins_with(GDE::EditorPaths::get_project_script_templates_dir())) {
        reloading = false;
        return OK;
    }
    #endif

    {
        String source_path = path;
        if (source_path.is_empty()) {
            source_path = get_path();
        }
        if (!source_path.is_empty()) {
            if (OScriptCache::get_cached_script(source_path).is_null()) {
                MutexLock lock(OScriptCache::get_cache_mutex());
                OScriptCache::_singleton->_shallow_cache[source_path] = Ref<OScript>(this);
            }
            if (OScriptCache::has_parser(source_path)) {
                Error err = OK;
                Ref<OScriptParserRef> parser_ref = OScriptCache::get_parser(source_path, OScriptParserRef::EMPTY, err);
                if (parser_ref.is_valid()) {
                    uint32_t source_hash = source.hash();
                    if (parser_ref->get_source_hash() != source_hash) {
                        OScriptCache::remove_parser(source_path);
                    }
                }
            }
        }
    }

    bool can_run = ScriptServer::is_scripting_enabled() || _is_tool();

    #ifdef TOOLS_ENABLED
    if (p_keep_state && can_run && _is_valid()) {
        _save_old_static_data();
    }
    #endif

    _valid = false;

    const int64_t modified_time = FileAccess::get_modified_time(path);
    switch (source.get_type()) {
        case OScriptSource::BINARY: {
            if (!orchestration.is_valid()) {
                OrchestrationBinaryParser binary_parser;
                orchestration = binary_parser.load(path);
                orchestration->set_self(this);
                #if TOOLS_ENABLED
                source_last_modified_time = modified_time;
            } else if (modified_time != source_last_modified_time) {
                OrchestrationBinaryParser binary_parser;
                Ref<Orchestration> temp = binary_parser.load(path);
                if (temp.is_valid()) {
                    orchestration->copy_state(temp);
                }
                source_last_modified_time = modified_time;
                #endif
            }
            break;
        }
        default: {
            if (!orchestration.is_valid()) {
                OrchestrationTextParser text_parser;
                orchestration = text_parser.load(path);
                orchestration->set_self(this);
                #if TOOLS_ENABLED
                source_last_modified_time = modified_time;
            } else if (modified_time != source_last_modified_time) {
                OrchestrationTextParser text_parser;
                Ref<Orchestration> temp = text_parser.load(path);
                if (temp.is_valid()) {
                    orchestration->copy_state(temp);
                }
                source_last_modified_time = modified_time;
                #endif
            }
            break;
        }
    }

    OScriptParser parser;
    Error err = parser.parse(orchestration.ptr(), path);
    if (err) {
        const List<OScriptParser::ParserError>& errors = parser.get_errors();
        if (!errors.is_empty()) {
            if (EngineDebugger::get_singleton()->is_active()) {
                OScriptLanguage::get_singleton()->debug_break_parse(
                    _get_debug_path(),
                    errors.front()->get().node_id,
                    "Parser Error: " + errors.front()->get().message);
            }
            _err_print_error(
                "OScript::reload",
                path.is_empty() ? "built-in" : path.utf8().get_data(),
                errors.front()->get().node_id,
                "Parser Error: " + errors.front()->get().message);
        }
        reloading = false;
        return ERR_PARSE_ERROR;
    }

    OScriptAnalyzer analyzer(&parser);
    err = analyzer.analyze();
    if (err) {
        const List<OScriptParser::ParserError>& errors = parser.get_errors();
        if (EngineDebugger::get_singleton()->is_active() && !errors.is_empty()) {
            OScriptLanguage::get_singleton()->debug_break_parse(
                _get_debug_path(),
                errors.front()->get().node_id,
                "Parser Error: " + errors.front()->get().message);
        }

        const List<OScriptParser::ParserError>::Element *e = errors.front();
        while (e != nullptr) {
            _err_print_error(
                "OScript::reload",
                path.is_empty() ? "built-in" : path.utf8().get_data(),
                parser.get_errors().front()->get().node_id,
                "Parser Error: " + parser.get_errors().front()->get().message);
            e = e->next();
        }

        reloading = false;
        return ERR_PARSE_ERROR;
    }

    can_run = ScriptServer::is_scripting_enabled() || parser.is_tool();

    OScriptCompiler compiler;
    err = compiler.compile(&parser, this, p_keep_state);
    if (err) {
        const List<OScriptParser::ParserError>& errors = parser.get_errors();
        if (!errors.is_empty()) {
            _err_print_error(
                "OScript::reload",
                path.is_empty() ? "built-in" : path.utf8().get_data(),
                errors.front()->get().node_id,
                "Compile Error: " + errors.front()->get().message);
        }

        if (can_run) {
            if (EngineDebugger::get_singleton()->is_active()) {
                OScriptLanguage::get_singleton()->debug_break_parse(
                    _get_debug_path(),
                    compiler.get_error_node_id(),
                    "Compiler Error: " + compiler.get_error());
            }

            reloading = false;
            return ERR_COMPILATION_FAILED;
        } else {
            ERR_PRINT(compiler.get_error());
            reloading = false;
            return err;
        }
    }

    #ifdef TOOLS_ENABLED
    // Done after compilation because it needs the OScript object's inner class OScript objects,
    // which are made by calling make_scripts() within compiler.compile() call above.
    OScriptDocGen::generate_docs(this, parser.get_tree());
    #endif

    #ifdef DEBUG_ENABLED
    for (const OScriptWarning& warning : parser.get_warnings()) {
        if (EngineDebugger::get_singleton()->is_active()) {
            _err_print_error(
                String("OScript::reload").utf8().get_data(),
                get_script_path().utf8().get_data(),
                warning.node,
                warning.get_name(),
                warning.get_message(),
                false,
                ERR_HANDLER_WARNING);
        }
    }
    #endif

    if (can_run) {
        err = _static_init();
        if (err) {
            return err;
        }
    }

    #ifdef TOOLS_ENABLED
    if (can_run && p_keep_state) {
        _restore_old_static_data();
    }

    if (p_keep_state) {
        // Update properties in the inspector
        _update_exports();
    }
    #endif

    reloading = false;
    return OK;
}

#ifdef TOOLS_ENABLED
#if GODOT_VERSION >= 0x040400
StringName OScript::_get_doc_class_name() const {
    return doc_class_name;
}
#endif

TypedArray<Dictionary> OScript::_get_documentation() const {
    TypedArray<Dictionary> result;
    #ifdef TOOLS_ENABLED
    for (const DocData::ClassDoc& class_doc : docs) {
        result.push_back(DocData::ClassDoc::to_dict(class_doc));
    }
    #endif
    return result;
}

String OScript::_get_class_icon_path() const {
    return simplified_icon_path;
}
#endif

bool OScript::_has_method(const StringName& p_method) const {
    return member_functions.has(p_method);
}

bool OScript::_has_static_method(const StringName& p_method) const {
    return member_functions.has(p_method) && member_functions[p_method]->is_static();
}

Variant OScript::_get_script_method_argument_count(const StringName& p_method) const {
    HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = member_functions.find(p_method);
    return !E ? 0 : E->value->get_argument_count();
}

Dictionary OScript::_get_method_info(const StringName& p_method) const {
    HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = member_functions.find(p_method);
    return DictionaryUtils::from_method(E ? E->value->get_method_info() : MethodInfo());
}

bool OScript::_is_tool() const {
    return _tool;
}

bool OScript::_is_valid() const {
    return _valid;
}

bool OScript::_is_abstract() const {
    return is_abstract;
}

ScriptLanguage* OScript::_get_language() const {
    return _language;
}

bool OScript::_has_script_signal(const StringName& p_signal) const {
    if (signals.has(p_signal)) {
        return true;
    }
    if (base.is_valid()) {
        return base->has_script_signal(p_signal);
    }
    #ifdef TOOLS_ENABLED
    else if (base_cache.is_valid()) {
        return base_cache->has_script_signal(p_signal);
    }
    #endif
    return false;
}

TypedArray<Dictionary> OScript::_get_script_signal_list() const {
    return _get_script_signals(true);
}

bool OScript::_has_property_default_value(const StringName& p_property) const {
    Variant result;
    return get_property_default_value(p_property, result);
}

Variant OScript::_get_property_default_value(const StringName& p_property) const {
    Variant result;
    if (!get_property_default_value(p_property, result)) {
        return {};
    }
    return result;
}

void OScript::_update_exports() {
    #ifdef TOOLS_ENABLED
    _update_exports_down(false);
    #endif
}

TypedArray<Dictionary> OScript::_get_script_method_list() const {
    return _get_script_methods(true);
}

TypedArray<Dictionary> OScript::_get_script_property_list() const {
    return _get_script_properties(true);
}

int32_t OScript::_get_member_line(const StringName& p_member) const {
    #ifdef TOOLS_ENABLED
    if (member_node_ids.has(p_member)) {
        return member_node_ids[p_member];
    }
    #endif
    return -1;
}

Dictionary OScript::_get_constants() const {
    Dictionary result;
    for (const KeyValue<StringName, Variant>& E : constants) {
        result[E.key] = E.value;
    }
    return result;
}

TypedArray<StringName> OScript::_get_members() const {
    TypedArray<StringName> result;
    for (const StringName& E : members) {
        result.push_back(E);
    }
    return result;
}

bool OScript::_is_placeholder_fallback_enabled() const {
    return _placeholder_fallback_enabled;
}

Variant OScript::_get_rpc_config() const {
    return rpc_config;
}

#ifdef DEBUG_ENABLED
String OScript::debug_get_script_name(const Ref<Script>& p_script) {
    if (p_script.is_valid()) {
        Ref<OScript> oscript = p_script;
        if (oscript.is_valid()) {
            if (oscript->get_local_name() != StringName()) {
                return oscript->get_local_name();
            }
            return oscript->get_fully_qualified_class_name().get_file();
        }
        if (p_script->get_global_name() != StringName()) {
            return p_script->get_global_name();
        } else if (!p_script->get_path().is_empty()) {
            return p_script->get_path().get_file();
        } else if (!p_script->get_name().is_empty()) {
            return p_script->get_name(); // Resource name
        }
    }
    return "<unknown script>";
}
#endif

String OScript::canonicalize_path(const String& p_path) {
    if (p_path.get_extension() == "orch") {
        return p_path.get_basename() + ".torch";
    }
    return p_path;
}

void OScript::reload_from_file() {
    constexpr ResourceLoader::CacheMode CACHE_MODE_IGNORE = ResourceLoader::CACHE_MODE_IGNORE;
    const String script_path = get_path();

    // This logic was taken directly from Script::reload_from_file
    #ifdef TOOLS_ENABLED
    // Setting this to 0 forces a reload off disk when _reload is called
    source_last_modified_time = 0;

    // Only reload scripts that have no compilation errors
    if (_is_valid()) {
        if (Engine::get_singleton()->is_editor_hint() && is_tool()) {
            ScriptLanguageExtension* language = cast_to<ScriptLanguageExtension>(get_language());
            if (language) {
                language->_reload_tool_script(this, true);
            }
        }
        else {
            // It is important to keep p_keep_state to true to manage reload scripts that are
            // instantiated currently.
            _reload(true);
        }
    }
    #else
    if (ResourceUtils::is_file(script_path)) {
        Ref<Script> reload = ResourceLoader::get_singleton()->load(script_path, get_class(), CACHE_MODE_IGNORE);
        if (!reload.is_valid()) {
            return;
        }

        set_block_signals(true);

        reset_state();

        const TypedArray<Dictionary> properties = get_property_list();
        for (int i = 0; i < properties.size(); i++) {
            const PropertyInfo& property = DictionaryUtils::to_property(properties[i]);
            if (!(property.usage & PROPERTY_USAGE_STORAGE)) {
                continue;
            }
            if (property.name.match("resource_path")) {
                continue;
            }
            set(property.name, reload->get(property.name));
        }

        set_block_signals(false);
    }
    #endif
}

#ifdef TOOLS_ENABLED
PropertyInfo OScript::get_class_category() const {
    String path = get_path();

    String script_name;
    if (is_built_in()) {
        if (get_name().is_empty()) {
            script_name = "Built-in Script";
        } else {
            script_name = vformat("%s (%s)", get_name(), "Built-in");
        }
    } else {
        if (get_name().is_empty()) {
            script_name = path.get_file();
        } else {
            script_name = get_name();
        }
    }

    return { Variant::NIL, script_name, PROPERTY_HINT_NONE, path, PROPERTY_USAGE_CATEGORY };
}
#endif // TOOLS_ENABLED

Variant OScript::_new(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error) {
    if (!_valid) {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    r_error.error = GDEXTENSION_CALL_OK;
    Ref<RefCounted> ref;
    Object *owner = nullptr;

    OScript* baseptr = this;
    while (baseptr->base.ptr()) {
        baseptr = baseptr->base.ptr();
    }

    ERR_FAIL_COND_V(baseptr->native.is_null(), Variant());
    if (baseptr->native.ptr()) {
        owner = baseptr->native->instantiate();
    } else {
        owner = memnew(RefCounted); //by default, no base means use reference
    }
    ERR_FAIL_NULL_V_MSG(owner, Variant(), "Can't inherit from a virtual class.");

    RefCounted *r = cast_to<RefCounted>(owner);
    if (r) {
        ref = Ref<RefCounted>(r);
    }

    //
    // We need to use `set_script` here. This forces `Object` to call `script->instance_create` which
    // delegates to `_instance_create` in the script extension, calling `_create_instance`. This is a
    // fast way to make sure the script instance is set on the object.
    //
    // We tried creating the script instance with `_create_instance` and then using the GDE_INTERFACE
    // `object_set_script_instance` API, but it was unreliable and crashed, but using `set_script`
    // always seemed to work as expected.
    if (ref.is_valid()) {
        ref->set_script(this);
        return ref;
    } else {
        owner->set_script(this);
        return owner;
    }
}

String OScript::get_script_path() const {
    if (!path_valid && !get_path().is_empty()) {
        return get_path();
    }
    return path;
}

void OScript::clear(ClearData* p_clear_data) {
    if (clearing) {
        return;
    }

    clearing = true;

    ClearData data;
    ClearData* clear_data = p_clear_data;
    bool is_root = false;

    // When 'clear_data' is null, this is the root script.
    // The root is in charge to clear functions and scripts of itself and dependencies
    if (clear_data == nullptr) {
        clear_data = &data;
        is_root = true;
    }

    {
        MutexLock lock(*func_ptrs_to_update_mutex.ptr());
        for (UpdatableFuncPtr* updatable : func_ptrs_to_update) {
            updatable->ptr = nullptr;
        }
    }

    // If we are in the process of shutting down then every single script will be cleared
    // So we can safely skip this costly step
    // todo: implement this when class inheritance/dependencies
    if (!OScriptLanguage::get_singleton()->finishing) {
    //     RBSet<OScript*> must_clear_dependencies = get_must_clear_dependencies();
    //     for (OScript* E : must_clear_dependencies) {
    //         clear_data->scripts.insert(E);
    //         E->clear(clear_data);
    //     }
    }

    for (const KeyValue<StringName, OScriptCompiledFunction*>& E : member_functions) {
        clear_data->functions.insert(E.value);
    }
    member_functions.clear();

    for (KeyValue<StringName, MemberInfo>& E : member_indices) {
        clear_data->scripts.insert(E.value.data_type.script_type_ref);
        E.value.data_type.script_type_ref = Ref<Script>();
    }

    for (KeyValue<StringName, MemberInfo>& E : static_variables_indices) {
        clear_data->scripts.insert(E.value.data_type.script_type_ref);
        E.value.data_type.script_type_ref = Ref<Script>();
    }
    static_variables.clear();
    static_variables_indices.clear();

    if (implicit_initializer) {
        clear_data->functions.insert(implicit_initializer);
        implicit_initializer = nullptr;
    }

    if (implicit_ready) {
        clear_data->functions.insert(implicit_ready);
        implicit_ready = nullptr;
    }

    if (static_initializer) {
        clear_data->functions.insert(static_initializer);
        static_initializer = nullptr;
    }

    // todo: add this if we decide to support subclasses
    // _save_orphan_subclasses(clear_data);

    #ifdef TOOLS_ENABLED
    if (_owner) {
        _clear_doc();
    }
    #endif

    if (is_root) {
        for (OScriptCompiledFunction* E : clear_data->functions) {
            memdelete(E);
        }

        for (Ref<Script>& E : clear_data->scripts) {
            Ref<OScript> scr = E;
            if (scr.is_valid()) {
                OScriptCache::remove_script(scr->get_path());
            }
        }

        clear_data->clear();
    }
}

void OScript::cancel_pending_functions(bool p_warn) {
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    while (SelfList<OScriptFunctionState>* E = pending_func_states.first()) {
        // Order matters since clearing the stack may already cause the OScriptFunctionState
        // to be destroyed and thus removed from the list.
        pending_func_states.remove(E);
        OScriptFunctionState* state = E->self();
        #ifdef DEBUG_ENABLED
        if (p_warn) {
            WARN_PRINT("Canceling suspended execution of \"" + state->get_readable_function() + "\" due to a script reload.");
        }
        #endif

        const uint64_t id = state->get_instance_id();
        state->_clear_connections();

        if (ObjectDB::get_instance(id)) {
            state->_clear_stack();
        }
    }
}

OScript* OScript::find_class(const String& p_qualified_name) { // NOLINT
    String first = p_qualified_name.get_slice("::", 0);

    PackedStringArray class_names;
    OScript *result = nullptr;
    // Empty initial name means start here.
    if (first.is_empty() || first == global_name) {
        class_names = p_qualified_name.split("::");
        result = this;
    } else if (p_qualified_name.begins_with(get_root_script()->path)) {
        // Script path could have a class path separator("::") in it.
        class_names = p_qualified_name.trim_prefix(get_root_script()->path).split("::");
        result = get_root_script();
    } else if (HashMap<StringName, Ref<OScript>>::Iterator E = subclasses.find(first)) {
        class_names = p_qualified_name.split("::");
        result = E->value.ptr();
    } else if (subclass_owner != nullptr) {
        // Check parent scope.
        return subclass_owner->find_class(p_qualified_name);
    }

    // Starts at index 1 because index 0 was handled above.
    for (int i = 1; result != nullptr && i < class_names.size(); i++) {
        if (HashMap<StringName, Ref<OScript>>::Iterator E = result->subclasses.find(class_names[i])) {
            result = E->value.ptr();
        } else {
            // Couldn't find inner class.
            return nullptr;
        }
    }

    return result;
}

bool OScript::has_class(const OScript* p_script) {
    String fqn = p_script->fully_qualified_name;
    if (fully_qualified_name.is_empty() && fqn.get_slice("::", 0).is_empty()) {
        return p_script == this;
    } else if (fqn.begins_with(fully_qualified_name)) {
        return p_script == find_class(fqn.trim_prefix(fully_qualified_name));
    }
    return false;
}

OScript* OScript::get_root_script() {
    OScript *result = this;
    while (result->subclass_owner) {
        result = result->subclass_owner;
    }
    return result;
}

RBSet<OScript*> OScript::get_dependencies() {
    RBSet<OScript*> dependencies;
    _collect_dependencies(dependencies, this);
    dependencies.erase(this);
    return dependencies;
}

HashMap<OScript*, RBSet<OScript*>> OScript::get_all_dependencies() {
    HashMap<OScript*, RBSet<OScript*>> all_dependencies;

    List<OScript*> scripts;
    {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        SelfList<OScript>* elem = OScriptLanguage::get_singleton()->_scripts.first();
        while (elem) {
            scripts.push_back(elem->self());
            elem = elem->next();
        }
    }

    for (OScript* scr : scripts) {
        if (scr == nullptr || scr->destructing) {
            continue;
        }
        all_dependencies.insert(scr, scr->get_dependencies());
    }

    return all_dependencies;
}

RBSet<OScript*> OScript::get_must_clear_dependencies() {
    RBSet<OScript*> must_clear_dependencies;
    RBSet<OScript*> dependencies = get_dependencies();
    HashMap<OScript*, RBSet<OScript*>> all_dependencies = get_all_dependencies();

    RBSet<OScript*> cant_clear;
    for (KeyValue<OScript*, RBSet<OScript*>>& E : all_dependencies) {
        if (dependencies.has(E.key)) {
            continue;
        }
        for (OScript* F : E.value) {
            if (dependencies.has(F)) {
                cant_clear.insert(F);
            }
        }
    }

    for (OScript* E : dependencies) {
        if (cant_clear.has(E) || ScriptServer::is_global_class(E->get_fully_qualified_class_name())) {
            continue;
        }
        must_clear_dependencies.insert(E);
    }

    cant_clear.clear();
    dependencies.clear();
    all_dependencies.clear();

    return must_clear_dependencies;
}

StringName OScript::debug_get_member_by_index(int p_index) const {
    for (const KeyValue<StringName, MemberInfo>& E : member_indices) {
        if (E.value.index == p_index) {
            return E.key;
        }
    }
    return "<error>";
}

StringName OScript::debug_get_static_var_by_index(int p_index) const {
    for (const KeyValue<StringName, MemberInfo>& E : static_variables_indices) {
        if (E.value.index == p_index) {
            return E.key;
        }
    }
    return "<error>";
}

bool OScript::get_property_default_value(const StringName& p_property, Variant& r_value) const { // NOLINT
    #ifdef TOOLS_ENABLED
    HashMap<StringName, Variant>::ConstIterator E = member_default_values_cache.find(p_property);
    if (E) {
        r_value = E->value;
        return true;
    }
    if (base_cache.is_valid()) {
        return base_cache->get_property_default_value(p_property, r_value);
    }
    #endif
    return false;
}

void OScript::get_constants(HashMap<StringName, Variant>* r_constants) {
    if (r_constants) {
        for (const KeyValue<StringName, Variant>& E : constants) {
            (*r_constants)[E.key] = E.value;
        }
    }
}

void OScript::unload_static() const {
    OScriptCache::remove_script(fully_qualified_name);
}

Ref<Orchestration> OScript::get_orchestration() {
    return orchestration;
}

void OScript::set_edited(bool p_edited) {
    if (orchestration.is_valid()) {
        orchestration->set_edited(p_edited);
    }
}

void OScript::set_source(const OScriptSource& p_source) {
    if (source == p_source) {
        return;
    }
    source = p_source;
    #ifdef TOOLS_ENABLED
    source_changed_cache = true;
    set_edited(false);
    source_last_modified_time = FileAccess::get_modified_time(path);
    #endif
}

Error OScript::load_source_code(const String& p_path) {
    if (p_path.is_empty()) {
        return OK;
    }

    const OScriptSource new_source = OScriptSource::load(p_path);
    if (!new_source.is_valid()) {
        return ERR_FILE_CANT_OPEN;
    }

    set_source(new_source);
    path = p_path;
    path_valid = true;

    return OK;
}

#ifdef DEV_TOOLS
String OScript::dump_compiled_state() {
    String result;

    result += "========================= Compilation Report===========================\n";
    result += vformat("Script File Path : %s\n", path);
    result += vformat("Script File Size : %s bytes\n", FileAccess::get_size(path));
    result += vformat("Script File Time : %s\n", Time::get_singleton()->get_datetime_string_from_unix_time(FileAccess::get_modified_time(path)));
    result += vformat("OScript Version  : %s\n", VERSION_FULL_BUILD);
    result += vformat("Compiled At      : %s\n", Time::get_singleton()->get_datetime_string_from_system());
    result += vformat("Godot Version    : %d.%d.%d.%s\n", GODOT_VERSION_MAJOR, GODOT_VERSION_MINOR, GODOT_VERSION_PATCH, GODOT_VERSION_STATUS);
    result += "=======================================================================\n";
    result += "\n";

    if (!static_variables.is_empty()) {
        result += vformat("Static Variables: %d\n", static_variables.size());
        for (const KeyValue<StringName, MemberInfo>& E : static_variables_indices) {
            result += vformat("  - Index   : %d\n", E.value.index);
            result += vformat("    Getter  : %s\n", E.value.getter);
            result += vformat("    Setter  : %s\n", E.value.setter);
            result += vformat("    Type    : %s\n", E.value.data_type.builtin_type);
            result += vformat("    Property: %s\n", DictionaryUtils::from_property(E.value.property_info));
            result += vformat("    Value   : %s\n", static_variables[E.value.index]);
        }
        result += "\n";
    }

    if (!signals.is_empty()) {
        result += vformat("Signals Count   : %d\n", signals.size());
        for (const KeyValue<StringName, MethodInfo>& E : signals) {
            result += vformat("  - Name    : %s\n", E.key);
            result += vformat("    Method  : %s\n", DictionaryUtils::from_method(E.value));
        }
        result += "\n";
    }

    if (!members.is_empty()) {
        result += vformat("Member Count    : %d\n", members.size());
        for (const KeyValue<StringName, MemberInfo>& E : member_indices) {
            result += vformat("  - Index   : %d\n", E.value.index);
            result += vformat("    Getter  : %s\n", E.value.getter);
            result += vformat("    Setter  : %s\n", E.value.setter);
            result += vformat("    Type    : %s\n", E.value.data_type.builtin_type);
            result += vformat("    Property: %s\n", DictionaryUtils::from_property(E.value.property_info));
        }
        result += "\n";
    }

    if (!constants.is_empty()) {
        result += vformat("Constants Count : %d\n", constants.size());
        for (const KeyValue<StringName, Variant>& E : constants) {
            result += vformat("  - Name    : %s\n", E.key);
            result += vformat("    Value   : %s\n", E.value);
        }
        result += "\n";
    }

    if (!rpc_config.is_empty()) {
        result += vformat("RPC             : %s\n", rpc_config);
        result += vformat("\n");
    }

    if (!member_functions.is_empty()) {
        for (const KeyValue<StringName, OScriptCompiledFunction*>& E : member_functions) {
            result += vformat("Function Name   : %s\n", E.key);
            result += vformat("Logical Name    : %s.%s\n", E.value->source, E.value->name);
            result += vformat("Is Static       : %s\n", E.value->_static ? "Yes" : "No");
            result += vformat("MethodInfo      : %s\n", DictionaryUtils::from_method(E.value->method_info));
            result += vformat("RPC             : %s\n", E.value->rpc_config);
            result += vformat("Arg. Count      : %d\n", E.value->argument_count);
            result += vformat("Is VarArg       : %s\n", E.value->is_vararg() ? "Yes" : "No");
            result += vformat("VarArg Index    : %d\n", E.value->vararg_index);
            result += vformat("Stack Size      : %d\n", E.value->stack_size);
            result += vformat("Instr Arg Size  : %d\n", E.value->instruction_arg_size);

            result += vformat("Temporary Slots : %d\n", E.value->temporary_slots.size());
            for (const KeyValue<int, Variant::Type>& T : E.value->temporary_slots) {
                result += vformat("\t[%d]: %s\n", T.key, Variant::get_type_name(T.value));
            }

            result += vformat("Code Size       : %d\n\n", E.value->code_size);
            result += vformat("Code:\n-----------------------------------------------------\n");
            for (int i = 0; i < E.value->code_size; i++) {
                result += vformat("%d ", E.value->code[i]);
            }
            result += "\n\n";

            #ifdef DEBUG_ENABLED
            Vector<String> lines;
            result += vformat("Disassembly:\n-----------------------------------------------------\n");
            E.value->disassemble(Vector<String>(), lines);
            for (const String& line : lines) {
                result += vformat("%s\n", line);
            }
            result += "\n";
            #endif
        }
    }

    return result;
}
#endif

void OScript::_bind_methods() {
    ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "new", &OScript::_new, MethodInfo("new"));
}

OScript::OScript()
    : _language(OScriptLanguage::get_singleton())
    , script_list(this) {

    func_ptrs_to_update_mutex.instantiate();

    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    OScriptLanguage::get_singleton()->_scripts.add(&script_list);
}

OScript::~OScript() {
    if (destructing) {
        return;
    }
    if (is_print_verbose_enabled()) {
        MutexLock lock(*func_ptrs_to_update_mutex.ptr());
        if (!func_ptrs_to_update.is_empty()) {
            print_line(vformat(
                "OScript: %d orphaned lambdas becoming invalid at destruction of script '%s'.",
                    func_ptrs_to_update.size(), fully_qualified_name));
        }
    }
    clear();
    cancel_pending_functions(false);
    {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        script_list.remove_from_list();
    }
}