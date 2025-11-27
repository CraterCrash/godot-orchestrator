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
#include "orchestration/io/orchestration_parser_binary.h"
#include "script/instances/script_instance.h"
#include "script/instances/script_instance_placeholder.h"
#include "script/nodes/script_nodes.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/mutex_lock.hpp>

OScript::OScript()
    : _valid(true)
    , _language(OScriptLanguage::get_singleton())
{
}

void OScript::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("connections_changed", PropertyInfo(Variant::STRING, "caller")));
    ADD_SIGNAL(MethodInfo("functions_changed"));
    ADD_SIGNAL(MethodInfo("variables_changed"));
    ADD_SIGNAL(MethodInfo("signals_changed"));
}

/// ScriptExtension ////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::_editor_can_reload_from_file()
{
    return true;
}

void* OScript::_placeholder_instance_create(Object* p_object) const
{
    #ifdef TOOLS_ENABLED
    Ref<Script> script(this);
    OScriptPlaceHolderInstance* psi = memnew(OScriptPlaceHolderInstance(script, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _placeholders[p_object->get_instance_id()] = psi;
    }
    psi->_script_instance = GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptPlaceHolderInstance::INSTANCE_INFO, psi);
    _update_exports_placeholder(nullptr, false, psi);
    return psi->_script_instance;
    #else
    return nullptr;
    #endif
}

void OScript::_placeholder_erased(void* p_placeholder)
{
    for (const KeyValue<uint64_t, OScriptPlaceHolderInstance*>& E : _placeholders)
    {
        if (E.value == p_placeholder)
        {
            _placeholders.erase(E.key);
            break;
        }
    }
}

bool OScript::_is_placeholder_fallback_enabled() const
{
    return _placeholder_fallback_enabled;
}

bool OScript::placeholder_has(Object* p_object) const
{
    return _placeholders.has(p_object->get_instance_id());
}

void* OScript::_instance_create(Object* p_object) const
{
    ERR_FAIL_COND_V_MSG(!_orchestration.is_valid(), nullptr, "Cannot create instance with no orchestration");

    const String base_type = _orchestration->get_base_type();
    if (!ClassDB::is_parent_class(p_object->get_class(), base_type))
    {
        const String message = vformat("Orchestration inherits from native type '%s', so it can't be assigned to an object of type: '%s'", base_type, p_object->get_class());
        if (EngineDebugger::get_singleton()->is_active())
            OScriptLanguage::get_singleton()->debug_break_parse(get_path(), -1, message);
        ERR_FAIL_V_MSG(nullptr, message);
    }

    OScriptInstance* si = memnew(OScriptInstance(Ref<Script>(this), _language, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _instances[p_object] = si;
    }

    si->_script_instance = GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptInstance::INSTANCE_INFO, si);

    // Dispatch the "Init Event" if its wired
    if (_orchestration->has_function("_init"))
    {
        Variant result;
        GDExtensionCallError err;
        si->call("_init", nullptr, 0, &result, &err);
    }

    return si->_script_instance;
}

bool OScript::_instance_has(Object* p_object) const
{
    return _instances.has(p_object);
}

bool OScript::_can_instantiate() const
{
    bool editor = Engine::get_singleton()->is_editor_hint();
    // Built-in script languages check if scripting is enabled OR if this is a tool script
    // Scripting is disabled by default in the editor
    return _valid && (_is_tool() || !editor);
}

Ref<Script> OScript::_get_base_script() const
{
    // No inheritance

    // Base in this case infers that a script inherits from another script, not that your script
    // inherits from a super type, such as Node.
    return {};
}

bool OScript::_inherits_script(const Ref<Script>& p_script) const
{
    // No inheritance
    return false;
}

StringName OScript::_get_global_name() const
{
    return "";
}

StringName OScript::_get_instance_base_type() const
{
    ERR_FAIL_COND_V_MSG(!_orchestration.is_valid(), "", "Cannot get instance base type without an orchestration");
    return _orchestration->get_base_type();
}

bool OScript::_has_source_code() const
{
    // No source
    return false;
}

String OScript::_get_source_code() const
{
    return {};
}

void OScript::_set_source_code(const String& p_code)
{
}

Error OScript::_reload(bool p_keep_state)
{
    // todo: need to find a way to reload the script when requested
    _valid = true;
    return OK;
}

TypedArray<Dictionary> OScript::_get_documentation() const
{
    // todo:    see how to generate it from the script/node contents
    //          see doc_data & script_language_extension
    return {};
}

bool OScript::_has_static_method(const StringName& p_method) const
{
    // Currently we don't support static methods
    return false;
}

bool OScript::_has_method(const StringName& p_method) const
{
    if (!_orchestration.is_valid())
        return false;

    return _orchestration->has_function(p_method);
}

Dictionary OScript::_get_method_info(const StringName& p_method) const
{
    return {};
}

TypedArray<Dictionary> OScript::_get_script_method_list() const
{
    TypedArray<Dictionary> results;
    if (_orchestration.is_valid())
    {
        for (const Ref<OScriptFunction>& function : _orchestration->get_functions())
            results.push_back(function->to_dict());
    }
    return results;
}

TypedArray<Dictionary> OScript::_get_script_property_list() const
{
    TypedArray<Dictionary> results;
    if (_orchestration.is_valid())
    {
        for (const Ref<OScriptVariable>& E : _orchestration->get_variables())
            if (E->is_exported())
                results.push_back(DictionaryUtils::from_property(E->get_info()));
    }
    return results;
}

bool OScript::_is_tool() const
{
    if (_orchestration.is_valid())
        return _orchestration->get_tool();

    return false;
}

bool OScript::_is_valid() const
{
    return _valid;
}

ScriptLanguage* OScript::_get_language() const
{
    return _language;
}

bool OScript::_has_script_signal(const StringName& p_signal) const
{
    if (!_orchestration.is_valid())
        return false;

    return _orchestration->has_custom_signal(p_signal);
}

TypedArray<Dictionary> OScript::_get_script_signal_list() const
{
    TypedArray<Dictionary> list;
    if (_orchestration.is_valid())
    {
        for (const Ref<OScriptSignal>& E : _orchestration->get_custom_signals())
            list.push_back(DictionaryUtils::from_method(E->get_method_info()));
    }
    return list;
}

bool OScript::_has_property_default_value(const StringName& p_property) const
{
    if (_orchestration.is_valid())
    {
        const Ref<OScriptVariable>& E = _orchestration->get_variable(p_property);
        if (E.is_valid())
            return true;
    }
    return false;
}

Variant OScript::_get_property_default_value(const StringName& p_property) const
{
    if (_orchestration.is_valid())
    {
        const Ref<OScriptVariable>& E = _orchestration->get_variable(p_property);
        if (E.is_valid())
            return E->get_default_value();
    }
    return {};
}

void OScript::_update_exports()
{
    #ifdef TOOLS_ENABLED
    _update_exports_down(false);
    #endif
}

int32_t OScript::_get_member_line(const StringName& p_member) const
{
    return -1;
}

Dictionary OScript::_get_constants() const
{
    return {};
}

TypedArray<StringName> OScript::_get_members() const
{
    return {};
}

Variant OScript::_get_rpc_config() const
{
    // Gather a dictionary of all RPC calls defined
    return Dictionary();
}

String OScript::_get_class_icon_path() const
{
    return {};
}

#if GODOT_VERSION >= 0x040400
StringName OScript::_get_doc_class_name() const
{
    // todo: requires adding documentation support
    return {};
}
#endif

void OScript::_update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties) const
{
    if (!_orchestration.is_valid())
        return;

    for (const Ref<OScriptVariable>& variable : _orchestration->get_variables())
    {
        PropertyInfo property = variable->get_info();
        if (variable->is_grouped_by_category())
            property.name = vformat("%s/%s", variable->get_category(), variable->get_variable_name());

        r_values[property.name] = variable->get_default_value();
        r_properties.push_back(property);
    }
}

bool OScript::_update_exports_placeholder(bool* r_err, bool p_recursive_call, OScriptPlaceHolderInstance* p_instance, bool p_base_exports_changed) const
{
#ifdef TOOLS_ENABLED
    HashMap<StringName, Variant> values;
    List<PropertyInfo> properties;
    _update_export_values(values, properties);

    for (const KeyValue<uint64_t, OScriptPlaceHolderInstance*>& E : _placeholders)
        E.value->update(properties, values);

    return true;
#else
    return false;
#endif
}

void OScript::_update_exports_down(bool p_base_exports_changed)
{
    bool cyclic_error = false;
    _update_exports_placeholder(&cyclic_error, false, nullptr, p_base_exports_changed);
    // todo: add inheriters_cache
}

void OScript::set_orchestration(const Ref<Orchestration>& p_orchestration)
{
    _orchestration = p_orchestration;
    _orchestration->set_self(this);
    emit_changed();
}

void OScript::reload_from_file()
{
    constexpr ResourceLoader::CacheMode CACHE_MODE_IGNORE = ResourceLoader::CACHE_MODE_IGNORE;
    const String path = get_path();

    // This logic was taken directly from Script::reload_from_file
    #ifdef TOOLS_ENABLED
    Ref<OScript> other = ResourceLoader::get_singleton()->load(path, get_class(), CACHE_MODE_IGNORE);
    if (other.is_valid())
    {
        const Ref<Orchestration> orchestration = other->get_orchestration();
        other.unref();

        set_orchestration(orchestration);

        if (_is_valid())
        {
            if (Engine::get_singleton()->is_editor_hint() && is_tool())
            {
                ScriptLanguageExtension* language = cast_to<ScriptLanguageExtension>(get_language());
                if (language)
                    language->_reload_tool_script(this, true);
            }
            else
                _reload(true);
        }
    }
    #else
    if (ResourceUtils::is_file(path))
    {
        Ref<Script> reload = ResourceLoader::get_singleton()->load(path, get_class(), CACHE_MODE_IGNORE);
        if (reload.is_valid())
        {
            reset_state();

            const TypedArray<Dictionary> properties = get_property_list();
            for (int i = 0; i < properties.size(); i++)
            {
                const PropertyInfo& property = DictionaryUtils::to_property(properties[i]);
                if (!(property.usage & PROPERTY_USAGE_STORAGE))
                    continue;

                if (property.name.match("resource_path"))
                    continue;

                set(property.name, reload->get(property.name));
            }
        }
    }
    #endif
}
