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
#include "script/instances/script_instance.h"

#include "common/dictionary_utils.h"
#include "common/memory_utils.h"
#include "script/nodes/script_nodes.h"
#include "script/script.h"

#include <sstream>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/local_vector.hpp>

static OScriptInstanceInfo init_script_instance_info()
{
    OScriptInstanceInfo info;
    OScriptInstanceBase::init_instance(info);

    info.set_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionConstVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptInstance*)p_self)->set(*((StringName*)p_name), *(const Variant*)p_value, &r_error);
    };

    info.get_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptInstance*)p_self)->get(*((StringName*)p_name), *(reinterpret_cast<Variant*>(p_value)), &r_error);
    };

    info.has_method_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->has_method(*((StringName*)p_name));
    };

    info.property_can_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->property_can_revert(*((StringName*)p_name));
    };

    info.property_get_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                                       GDExtensionVariantPtr r_ret) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->property_get_revert(*((StringName*)p_name), (Variant*)r_ret);
    };

    info.call_func = [](void* p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr* p_args,
                        GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError* r_error) {
        return ((OScriptInstance*)p_self)
            ->call(*((StringName*)p_method), (const Variant**)p_args, p_argument_count, (Variant*)r_return, r_error);
    };

    info.notification_func = [](void* p_self, int32_t p_what, GDExtensionBool p_reversed) {
        ((OScriptInstance*)p_self)->notification(p_what, p_reversed);
    };

    info.free_func = [](void* p_self) {
        memdelete(((OScriptInstance*)p_self));
    };

    info.refcount_decremented_func = [](void*) -> GDExtensionBool {
        // If false (default), object cannot die
        return true;
    };

    return info;
}

const OScriptInstanceInfo OScriptInstance::INSTANCE_INFO = init_script_instance_info();

OScriptInstance::OScriptInstance(const Ref<OScript>& p_script, OScriptLanguage* p_language, Object* p_owner)
    : _script(p_script)
    , _owner(p_owner)
    , _language(p_language)
{
    _vm.set_owner(p_owner);
    _vm.set_script(p_script);

    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : p_script->_variables)
        _vm.register_variable(E.value);

    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : p_script->_functions)
        _vm.register_function(E.value);
}

OScriptInstance::~OScriptInstance()
{
    MutexLock lock(*_language->lock.ptr());
    _script->_instances.erase(_owner);
}

bool OScriptInstance::set(const StringName& p_name, const Variant& p_value, PropertyError* r_err)
{
    const String variable_name = _get_variable_name_from_path(p_name);

    OScriptVirtualMachine::Variable* variable = _vm.get_variable(variable_name);
    if (!variable)
    {
        if (r_err)
            *r_err = PROP_NOT_FOUND;
        return false;
    }

    if (r_err)
        *r_err = PROP_OK;

    variable->value = p_value;

    return true;
}

bool OScriptInstance::get(const StringName& p_name, Variant& p_value, PropertyError* r_err)
{
    // First check if we have a member variable
    const String variable_name = _get_variable_name_from_path(p_name);
    if (_vm.has_variable(variable_name))
    {
        OScriptVirtualMachine::Variable* variable = _vm.get_variable(variable_name);
        if (!variable)
        {
            if (r_err)
                *r_err = PROP_NOT_FOUND;
            return false;
        }

        if (r_err)
            *r_err = PROP_OK;

        p_value = variable->value;
        return true;
    }

    // Next check signals - for named access, i.e. "await obj.signal"
    if (_vm.has_signal(p_name))
    {
        p_value = _vm.get_signal(p_name);
        return true;
    }

    return false;
}

GDExtensionPropertyInfo* OScriptInstance::get_property_list(uint32_t* r_count)
{
    LocalVector<GDExtensionPropertyInfo> infos;
    for (const Ref<OScriptVariable>& variable : _script->get_variables())
    {
        // Only exported
        if (!variable->is_exported())
            continue;

        PropertyInfo info = variable->get_info();

        if (variable->is_grouped_by_category())
            info.name = vformat("%s/%s", variable->get_category(), info.name);

        const Dictionary property = DictionaryUtils::from_property(info);
        GDExtensionPropertyInfo pi = DictionaryUtils::to_extension_property(property);
        infos.push_back(pi);
    }

    *r_count = infos.size();
    if (infos.size() == 0)
        return nullptr;

    GDExtensionPropertyInfo* list = MemoryUtils::memnew_with_size<GDExtensionPropertyInfo>(infos.size());
    memcpy(list, infos.ptr(), sizeof(GDExtensionPropertyInfo) * infos.size());
    return list;
}

Variant::Type OScriptInstance::get_property_type(const StringName& p_name, bool* r_is_valid) const
{
    OScriptVirtualMachine::Variable* variable = _vm.get_variable(_get_variable_name_from_path(p_name));
    if (!variable)
    {
        if (r_is_valid)
            *r_is_valid = false;
        ERR_FAIL_V(Variant::NIL);
    }

    if (r_is_valid)
        *r_is_valid = true;

    return variable->type;
}

bool OScriptInstance::has_method(const StringName& p_name) const
{
    return _script->has_function(p_name);
}

Object* OScriptInstance::get_owner() const
{
    return _owner;
}

Ref<OScript> OScriptInstance::get_script() const
{
    return _script;
}

ScriptLanguage* OScriptInstance::get_language() const
{
    return _language;
}

bool OScriptInstance::is_placeholder() const
{
    return false;
}

String OScriptInstance::get_base_type() const
{
    return _script->get_base_type();
}

void OScriptInstance::set_base_type(const String& p_base_type)
{
    _script->set_base_type(p_base_type);
}

bool OScriptInstance::property_can_revert(const StringName& p_name)
{
    // Only applicable for Editor
    return false;
}

bool OScriptInstance::property_get_revert(const StringName& p_name, Variant* r_ret)
{
    // Only applicable for Editor
    return false;
}

void OScriptInstance::notification(int32_t p_what, bool p_reversed)
{
    const Array args = Array::make(p_what, p_reversed);
    const Variant** argptrs = (const Variant**)alloca(sizeof(Variant*) * args.size());
    for (int i = 0; i < args.size(); i++)
        argptrs[i] = &args[i];

    GDExtensionCallError error;
    Variant ret;
    call("_notification", argptrs, args.size(), &ret, &error);
}

void OScriptInstance::to_string(GDExtensionBool* r_is_valid, String* r_out)
{
    *r_is_valid = true;

    if (r_out)
    {
        std::stringstream ss;
        ss << "OrchestratorScriptInstance[" << _script->get_path().utf8().get_data() << "]:" << std::hex << this;
        *r_out = ss.str().c_str();
    }
}

bool OScriptInstance::get_variable(const StringName& p_name, Variant& r_value) const
{
    return _vm.get_variable(_get_variable_name_from_path(p_name), r_value);
}

bool OScriptInstance::set_variable(const StringName& p_name, const Variant& p_value)
{
    return _vm.set_variable(_get_variable_name_from_path(p_name), p_value);
}

OScriptInstance* OScriptInstance::from_object(GDExtensionObjectPtr p_object)
{
    return nullptr;
}

void OScriptInstance::call(const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count,
                           Variant* r_return, GDExtensionCallError* r_err)
{
    _vm.call_method(this, p_method, p_args, p_arg_count, r_return, r_err);
}
