// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "script/instances/script_instance_placeholder.h"

#include "common/dictionary_utils.h"
#include "common/memory_utils.h"
#include "script/script.h"

#include <godot_cpp/templates/local_vector.hpp>

static OScriptInstanceInfo init_placeholder_instance_info()
{
    OScriptInstanceInfo info;
    OScriptInstanceBase::init_instance(info);

    // not set
    // get_class_category_func
    // validate_property_func
    // refcount_incremented_func
    // is_placeholder_func
    // set_fallback_func
    // get_fallback_func

    info.set_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionConstVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptPlaceHolderInstance*)p_self)->set(*((StringName*)p_name), *(const Variant*)p_value, &r_error);
    };

    info.get_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptPlaceHolderInstance*)p_self)
            ->get(*((StringName*)p_name), *(reinterpret_cast<Variant*>(p_value)), &r_error);
    };

    info.has_method_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptPlaceHolderInstance*)p_self)->has_method(*((StringName*)p_name));
    };

    info.property_can_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptPlaceHolderInstance*)p_self)->property_can_revert(*((StringName*)p_name));
    };

    info.property_get_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                                       GDExtensionVariantPtr r_ret) -> GDExtensionBool {
        return ((OScriptPlaceHolderInstance*)p_self)->property_get_revert(*((StringName*)p_name), (Variant*)r_ret);
    };

    info.call_func = [](void* p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr* p_args,
                        GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError* r_error) {
        return ((OScriptPlaceHolderInstance*)p_self)
            ->call(*((StringName*)p_method), (const Variant**)p_args, p_argument_count, (Variant*)r_return, r_error);
    };

    info.notification_func = [](void* p_self, int32_t p_what, GDExtensionBool p_reversed) {
        ((OScriptPlaceHolderInstance*)p_self)->notification(p_what, p_reversed);
    };

    info.free_func = [](void* p_self) {
        memdelete(((OScriptPlaceHolderInstance*)p_self));
    };

    info.refcount_decremented_func = [](void*) -> GDExtensionBool {
        // If false (default), object cannot die
        return true;
    };

    return info;
}

const OScriptInstanceInfo OScriptPlaceHolderInstance::INSTANCE_INFO = init_placeholder_instance_info();

OScriptPlaceHolderInstance::OScriptPlaceHolderInstance(Ref<OScript> p_script, Object* p_owner)
{
    _script = p_script;
    _owner = p_owner;
}

bool OScriptPlaceHolderInstance::set(const StringName& p_name, const Variant& p_value, PropertyError* r_err)
{
    if (_script->_is_placeholder_fallback_enabled())
        return false;

    if (_variables.has(p_name))
    {
        Variant def_value = _script->get_property_default_value(p_name);

        bool valid = true;
        Variant result;
        Variant::evaluate(Variant::OP_EQUAL, def_value, p_value, result, valid);
        if (valid && bool(result))
            _variables.erase(p_name);
        else
            _variables[p_name] = p_value;

        if (r_err)
            *r_err = PROP_OK;

        return true;
    }
    else
    {
        Variant def_value = _script->get_property_default_value(p_name);

        bool valid = true;
        Variant result;
        Variant::evaluate(Variant::OP_NOT_EQUAL, def_value, p_value, result, valid);
        if (valid && bool(result))
        {
            _variables[p_name] = p_value;
            if (r_err)
                *r_err = PROP_OK;
            return true;
        }
    }

    if (r_err)
        *r_err = PROP_NOT_FOUND;

    return false;
}

bool OScriptPlaceHolderInstance::get(const StringName& p_name, Variant& r_value, PropertyError* r_err)
{
    if (_variables.has(p_name))
    {
        r_value = _variables[p_name];
        if (r_err)
            *r_err = PROP_OK;

        return true;
    }

    if (!_script->_is_placeholder_fallback_enabled())
    {
        Variant def_value = _script->get_property_default_value(p_name);
        if (def_value.get_type() != Variant::NIL)
        {
            r_value = def_value;
            if (r_err)
                *r_err = PROP_OK;

            return true;
        }
    }

    if (r_err)
        *r_err = PROP_NOT_FOUND;

    return false;
}

GDExtensionPropertyInfo* OScriptPlaceHolderInstance::get_property_list(uint32_t* r_count)
{
    LocalVector<GDExtensionPropertyInfo> infos;
    for (const Ref<OScriptVariable>& variable : _script->get_variables())
    {
        // Only exported
        if (!variable->is_exported())
            continue;

        PropertyInfo info = variable->get_info();
        info.usage |= PROPERTY_USAGE_SCRIPT_VARIABLE;

        if (variable->is_grouped_by_category())
            info.name = vformat("%s/%s", variable->get_category(), info.name);

        const Dictionary property = DictionaryUtils::from_property(info);

        GDExtensionPropertyInfo  pi = DictionaryUtils::to_extension_property(property);
        infos.push_back(pi);
    }

    *r_count = infos.size();

    if (infos.size() == 0)
        return nullptr;

    GDExtensionPropertyInfo* list = MemoryUtils::memnew_with_size<GDExtensionPropertyInfo>(infos.size());
    memcpy(list, infos.ptr(), sizeof(GDExtensionPropertyInfo) * infos.size());

    return list;
}

Variant::Type OScriptPlaceHolderInstance::get_property_type(const StringName& p_name, bool* r_is_valid) const
{
    Ref<OScriptVariable> variable = _script->get_variable(_get_variable_name_from_path(p_name));
    if (!variable.is_valid() || !variable->is_exported())
    {
        if(r_is_valid)
            *r_is_valid = false;
        ERR_FAIL_V(Variant::NIL);
    }

    if (r_is_valid)
        *r_is_valid = true;

    return variable->get_variable_type();
}

bool OScriptPlaceHolderInstance::has_method(const StringName& p_name) const
{
    return _script->has_function(p_name);
}

Ref<OScript> OScriptPlaceHolderInstance::get_script() const
{
    return _script;
}

Object* OScriptPlaceHolderInstance::get_owner() const
{
    return _owner;
}

ScriptLanguage* OScriptPlaceHolderInstance::get_language() const
{
    return OScriptLanguage::get_singleton();
}

bool OScriptPlaceHolderInstance::is_placeholder() const
{
    return true;
}

bool OScriptPlaceHolderInstance::property_can_revert(const StringName& p_name)
{
    Ref<OScriptVariable> variable = _script->get_variable(_get_variable_name_from_path(p_name));
    if (!variable.is_valid() || !variable->is_exported())
        return false;

    return true;
}

bool OScriptPlaceHolderInstance::property_get_revert(const StringName& p_name, Variant* r_ret)
{
    Ref<OScriptVariable> variable = _script->get_variable(_get_variable_name_from_path(p_name));
    if (!variable.is_valid() || !variable->is_exported())
        return false;

    if (r_ret)
        *r_ret = variable->get_default_value();

    return true;
}

void OScriptPlaceHolderInstance::call(const StringName& p_method, const Variant* const* p_args,
                                      GDExtensionInt p_arg_count, Variant* r_return, GDExtensionCallError* r_err)
{
    r_err->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    *r_return = Variant();
}

void OScriptPlaceHolderInstance::notification(int32_t p_what, bool p_reversed)
{
}

void OScriptPlaceHolderInstance::to_string(GDExtensionBool* r_is_valid, String* r_out)
{
    *r_is_valid = true;
    *r_out = "OrchestratorPlaceHolderScriptInstance[" + _script->get_name() + "]";
}