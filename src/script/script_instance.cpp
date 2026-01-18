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
#include "script/script_instance.h"

#include "common/dictionary_utils.h"
#include "core/godot/scene_string_names.h"
#include "core/godot/variant/variant.h"
#include "script/script.h"
#include "script/script_rpc_callable.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace MemoryUtils {
    template <typename T>
    T* memnew_ptr(const T& p_value) {
        T* ptr = memnew(T);
        *ptr = p_value;
        return ptr;
    }

    _FORCE_INLINE_ StringName* memnew_stringname(const StringName& p_value = StringName()) {
        return memnew_ptr(p_value);
    }

    _FORCE_INLINE_ String* memnew_string(const String& p_value = String()) {
        return memnew_ptr(p_value);
    }

    template<typename T> int
    memnew_ptr_size(const T* p_ptr)
    {
        // Read the size from a pre-allocated pointer from memnew_with_size function.
        return !p_ptr ? 0 : *((int*)p_ptr - 1);
    }

    template <typename T>
    T* memnew_with_size(int p_size) {
        uint64_t size = sizeof(T) * p_size;
        void* ptr = memalloc(size + sizeof(int));
        *static_cast<int*>(ptr) = p_size;
        return reinterpret_cast<T*>(static_cast<int*>(ptr) + 1);
    }

    template<typename T>
    void memdelete_with_size(const T* p_ptr) {
        memfree((int*)p_ptr - 1);
    }

    void free_property_info(const GDExtensionPropertyInfo& p_property) {
        memdelete((StringName*) p_property.name);
        memdelete((StringName*) p_property.class_name);
        memdelete((String*) p_property.hint_string);
    }

    void free_method_info(const GDExtensionMethodInfo& p_method) {
        memdelete((StringName*) p_method.name);
        free_property_info(p_method.return_value);

        if (p_method.argument_count > 0)
        {
            for (uint32_t i = 0; i < p_method.argument_count; i++)
                free_property_info(p_method.arguments[i]);
            memdelete_arr(p_method.arguments);
        }

        if (p_method.default_argument_count > 0)
            memdelete((Variant*) p_method.default_arguments);
    }
}

static void make_gdextension_property_info(const PropertyInfo& p_property, GDExtensionPropertyInfo& p_info) { // NOLINT
    p_info.type = static_cast<GDExtensionVariantType>(p_property.type);
    p_info.name = MemoryUtils::memnew_stringname(p_property.name);
    p_info.class_name = MemoryUtils::memnew_stringname(p_property.class_name);
    p_info.hint_string = MemoryUtils::memnew_string(p_property.hint_string);
    p_info.hint = p_property.hint;
    p_info.usage = p_property.usage;
}

static void make_gdextension_method_info(const MethodInfo& p_method, GDExtensionMethodInfo& p_info) { // NOLINT
    p_info.name = MemoryUtils::memnew_stringname(p_method.name);
    p_info.flags = p_method.flags;

    make_gdextension_property_info(p_method.return_val, p_info.return_value);

    p_info.argument_count = p_method.arguments.size();
    if (p_info.argument_count > 0) {
        GDExtensionPropertyInfo* args = memnew_arr(GDExtensionPropertyInfo, p_info.argument_count);
        for (size_t i = 0; i < p_info.argument_count; i++) {
            make_gdextension_property_info(p_method.arguments[i], args[i]);
        }
        p_info.arguments = args;
    }

    p_info.default_argument_count = p_method.default_arguments.size();
    if (p_info.default_argument_count > 0) {
        Variant* args = memnew_arr(Variant, p_info.default_argument_count);
        for (size_t i = 0; i < p_info.default_argument_count; i++) {
            args[i] = p_method.default_arguments[i];
        }
        p_info.default_arguments = reinterpret_cast<GDExtensionVariantPtr*>(args);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define INSTANCE static_cast<T*>(p_instance)
#define CAST_STRINGNAME(x) *static_cast<const StringName*>(x)
#define CAST_STRING(x) *static_cast<const String*>(x)
#define CAST_CONST_VARIANT(x) *static_cast<const Variant*>(x)
#define CAST_VARIANT(x) *static_cast<Variant*>(x)
#define CAST_BOOL(x) reinterpret_cast<bool*>(x)

template <typename T>
struct OScriptInstanceCallbacks {
    static GDExtensionBool set_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstVariantPtr p_name, GDExtensionConstVariantPtr p_value) {
        return INSTANCE->set(CAST_STRINGNAME(p_name), CAST_CONST_VARIANT(p_value));
    }

    static GDExtensionBool get_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstVariantPtr p_name, GDExtensionVariantPtr r_value) {
        return INSTANCE->get(CAST_STRINGNAME(p_name), CAST_VARIANT(r_value));
    }

    static const GDExtensionPropertyInfo* get_property_list_func(GDExtensionScriptInstanceDataPtr p_instance, uint32_t* r_size) {
        return INSTANCE->get_property_list(r_size);
    }

    #if GODOT_VERSION >= 0x040300
    static void free_property_list_func(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionPropertyInfo* p_list, uint32_t p_count) {
        INSTANCE->free_property_list(p_list, p_count);
    }
    #else
    static void free_property_list_func(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionPropertyInfo* p_list) {
        INSTANCE->free_property_list(p_list);
    }
    #endif

    static GDExtensionBool property_can_revert_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name) {
        return INSTANCE->property_can_revert(CAST_STRINGNAME(p_name));
    }

    static GDExtensionBool property_get_revert_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_value) {
        return INSTANCE->property_get_revert(CAST_STRINGNAME(p_name), CAST_VARIANT(r_value));
    }

    static GDExtensionObjectPtr get_owner_func(GDExtensionScriptInstanceDataPtr p_instance) {
        return INSTANCE->get_owner()->_owner;
    }

    static void get_property_state_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionScriptInstancePropertyStateAdd p_add_func, void* p_userdata) {
        INSTANCE->get_property_state(p_add_func, p_userdata);
    }

    static const GDExtensionMethodInfo* get_method_list_func(GDExtensionScriptInstanceDataPtr p_instance, uint32_t* r_size) {
        return INSTANCE->get_method_list(r_size);
    }

    #if GODOT_VERSION >= 0x040300
    static void free_method_list_func(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionMethodInfo* p_list, uint32_t p_count) {
        INSTANCE->free_method_list(p_list, p_count);
    }
    #else
    static void free_method_list_func(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionMethodInfo* p_list) {
        INSTANCE->free_method_list(p_list);
    }
    #endif

    static GDExtensionVariantType get_property_type_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionBool* r_valid) {
        return static_cast<GDExtensionVariantType>(INSTANCE->get_property_type(CAST_STRINGNAME(p_name), CAST_BOOL(r_valid)));
    }

    static GDExtensionBool validate_property_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionPropertyInfo* p_property) {
        if (p_property) {
            PropertyInfo property;
            property.name = CAST_STRINGNAME(p_property->name);
            property.type = static_cast<Variant::Type>(p_property->type);
            property.class_name = CAST_STRINGNAME(p_property->class_name);
            property.hint = p_property->hint;
            property.hint_string = CAST_STRING(p_property->hint_string);
            property.usage = p_property->usage;
            INSTANCE->validate_property(property);
            return true;
        }
        return false;
    }

    static GDExtensionBool has_method_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name) {
        return INSTANCE->has_method(CAST_STRINGNAME(p_name));
    }

    static GDExtensionInt get_method_argument_count_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionBool* r_valid) {
        return INSTANCE->get_method_argument_count(CAST_STRINGNAME(p_name), CAST_BOOL(r_valid));
    }

    static void call_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr* p_args, GDExtensionInt p_count, GDExtensionVariantPtr r_value, GDExtensionCallError* r_error) {
        Variant* result = static_cast<Variant*>(r_value);
        *result = INSTANCE->callp(CAST_STRINGNAME(p_method), (const Variant**)p_args, p_count, *r_error);
    }

    static void notification_func(GDExtensionScriptInstanceDataPtr p_instance, int32_t p_what, GDExtensionBool p_reversed) {
        INSTANCE->notification(p_what, p_reversed);
    }

    static void to_string_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionBool* r_valid, GDExtensionStringPtr r_value) {
        if (r_value) {
            *r_valid = true;
            *static_cast<String*>(r_value) = INSTANCE->to_string();
        }
    }

    static GDExtensionBool refcount_decremented_func(GDExtensionScriptInstanceDataPtr p_instance) { // NOLINT
        // Regardless of instance, it can always freed
        return true;
    }

    static GDExtensionObjectPtr get_script_func(GDExtensionScriptInstanceDataPtr p_instance) {
        return INSTANCE->get_script()->_owner;
    }

    static GDExtensionBool is_placeholder_func(GDExtensionScriptInstanceDataPtr p_instance) {
        return INSTANCE->is_placeholder();
    }

    static GDExtensionBool property_set_fallback_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value) {
        bool valid;
        INSTANCE->property_set_fallback(CAST_STRINGNAME(p_name), CAST_CONST_VARIANT(p_value), &valid);
        return valid;
    }

    static GDExtensionBool property_get_fallback_func(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name,  GDExtensionVariantPtr r_value) {
        bool valid;
        Variant result = INSTANCE->property_get_fallback(CAST_STRINGNAME(p_name), &valid);
        if (valid && r_value) {
            CAST_VARIANT(r_value) = result;
        }
        return valid;
    }

    static GDExtensionScriptLanguagePtr get_language_func(GDExtensionScriptInstanceDataPtr p_instance) {
        return INSTANCE->get_language()->_owner;
    }

    static void free_func(GDExtensionScriptInstanceDataPtr p_instance) {
        memdelete(INSTANCE);
    }
};

#undef INSTANCE
#undef CAST_STRINGNAME
#undef CAST_STRING
#undef CAST_CONST_VARIANT
#undef CAST_VARIANT
#undef CAST_BOOL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct OScriptInstanceCallbacksImpl : OScriptInstanceCallbacks<OScriptInstance> {
};

const OScriptInstanceInfo OScriptInstance::INSTANCE_INFO = [] {
    OScriptInstanceInfo result = {};
    result.set_func = OScriptInstanceCallbacksImpl::set_func;
    result.get_func = OScriptInstanceCallbacksImpl::get_func;
    result.get_property_list_func = OScriptInstanceCallbacksImpl::get_property_list_func;
    result.free_property_list_func = OScriptInstanceCallbacksImpl::free_property_list_func;
    result.property_can_revert_func = OScriptInstanceCallbacksImpl::property_can_revert_func;
    result.property_get_revert_func = OScriptInstanceCallbacksImpl::property_get_revert_func;
    result.get_owner_func = OScriptInstanceCallbacksImpl::get_owner_func;
    result.get_property_state_func = OScriptInstanceCallbacksImpl::get_property_state_func;
    result.get_method_list_func = OScriptInstanceCallbacksImpl::get_method_list_func;
    result.free_method_list_func = OScriptInstanceCallbacksImpl::free_method_list_func;
    result.get_property_type_func = OScriptInstanceCallbacksImpl::get_property_type_func;
    result.validate_property_func = OScriptInstanceCallbacksImpl::validate_property_func;
    result.has_method_func = OScriptInstanceCallbacksImpl::has_method_func;
    result.get_method_argument_count_func = OScriptInstanceCallbacksImpl::get_method_argument_count_func;
    result.call_func = OScriptInstanceCallbacksImpl::call_func;
    result.notification_func = OScriptInstanceCallbacksImpl::notification_func;
    result.to_string_func = OScriptInstanceCallbacksImpl::to_string_func;
    result.get_script_func = OScriptInstanceCallbacksImpl::get_script_func;
    result.is_placeholder_func = OScriptInstanceCallbacksImpl::is_placeholder_func;
    result.set_fallback_func = OScriptInstanceCallbacksImpl::property_set_fallback_func;
    result.get_fallback_func = OScriptInstanceCallbacksImpl::property_get_fallback_func;
    result.get_language_func = OScriptInstanceCallbacksImpl::get_language_func;
    result.free_func = OScriptInstanceCallbacksImpl::free_func;
    return result;
}();

#if GODOT_VERSION >= 0x040500
bool OScriptInstanceBase::_is_same_script_instance() const {
    return _owner && internal::gdextension_interface_object_get_script_instance(_owner->_owner, OScriptLanguage::get_singleton()) == this;
}
#endif

void OScriptInstanceBase::property_set_fallback(const StringName& p_name, const Variant& p_value, bool* r_valid) {
    if (r_valid) {
        *r_valid = false;
    }
}

Variant OScriptInstanceBase::property_get_fallback(const StringName& p_name, bool* r_valid) {
    if (r_valid) {
        *r_valid = false;
    }
    return {};
}

void OScriptInstanceBase::get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void* p_user_data) {
    uint32_t count = 0;
    GDExtensionPropertyInfo* props = get_property_list(&count);
    if (props) {
        for (uint32_t i = 0; i < count; i++) {
            GDExtensionStringNamePtr name = props[i].name;
            if (props[i].usage & PROPERTY_USAGE_STORAGE) {
                Variant value;
                bool is_valid = get(reinterpret_cast<StringName&>(name), value);
                if (is_valid) {
                    p_add_func(name, &value, p_user_data);
                }
            }
        }
        free_property_list(props, count);
    }
}

static void add_to_state(GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value, void* p_userdata)
{
    List<Pair<StringName, Variant>>* list = static_cast<List<Pair<StringName, Variant>>*>(p_userdata);
    list->push_back({ *static_cast<const StringName*>(p_name), *static_cast<const Variant*>(p_value) });
}

void OScriptInstanceBase::get_property_state(List<Pair<StringName, Variant>>& p_list) {
    get_property_state(add_to_state, &p_list);
}

GDExtensionPropertyInfo* OScriptInstanceBase::get_property_list(uint32_t* r_size) {
    const LocalVector<PropertyInfo> properties = _get_property_list();

    uint32_t categories = 0;
    for (const PropertyInfo& property : properties) {
        if (property.usage & PROPERTY_USAGE_CATEGORY) {
            categories++;
        }
    }

    // If the properties are empty or if all there are is properties, we should treat this as an empty list
    if (properties.size() == 0 || categories == properties.size()) {
        if (r_size) {
            *r_size = 0;
        }
        return nullptr;
    }

    if (r_size) {
        *r_size = properties.size();
    }

    LocalVector<GDExtensionPropertyInfo> extension_properties;
    for (const PropertyInfo& property : properties) {
        GDExtensionPropertyInfo extension_property;
        make_gdextension_property_info(property, extension_property);
        extension_properties.push_back(extension_property);
    }

    GDExtensionPropertyInfo* result = MemoryUtils::memnew_with_size<GDExtensionPropertyInfo>(extension_properties.size());
    memcpy(result, extension_properties.ptr(), sizeof(GDExtensionPropertyInfo) * extension_properties.size());
    return result;
}

void OScriptInstanceBase::free_property_list(const GDExtensionPropertyInfo* p_list, uint32_t p_size) {
    if (p_list) {
        for (uint32_t i = 0; i < p_size; i++) {
            MemoryUtils::free_property_info(p_list[i]);
        }
        MemoryUtils::memdelete_with_size<GDExtensionPropertyInfo>(p_list);
    }
}

void OScriptInstanceBase::free_property_list(const GDExtensionPropertyInfo* p_list) {
    if (p_list) {
        int size = MemoryUtils::memnew_ptr_size<GDExtensionPropertyInfo>(p_list);
        free_property_list(p_list, size);
    }
}

GDExtensionMethodInfo* OScriptInstanceBase::get_method_list(uint32_t* r_size) const {
    const LocalVector<MethodInfo> method_list = _get_method_list();

    LocalVector<GDExtensionMethodInfo> methods;
    for (const MethodInfo& method : method_list) {
        GDExtensionMethodInfo info;
        make_gdextension_method_info(method, info);
        methods.push_back(info);
    }

    if (r_size) {
        *r_size = methods.size();
    }

    if (methods.is_empty()) {
        return nullptr;
    }

    GDExtensionMethodInfo* result = MemoryUtils::memnew_with_size<GDExtensionMethodInfo>(methods.size());
    memcpy(result, methods.ptr(), sizeof(GDExtensionMethodInfo) * methods.size());
    return result;
}

void OScriptInstanceBase::free_method_list(const GDExtensionMethodInfo* p_list, uint32_t p_size) const {
    if (p_list) {
        for (uint32_t i = 0; i < p_size; i++) {
            MemoryUtils::free_method_info(p_list[i]);
        }
        MemoryUtils::memdelete_with_size<GDExtensionMethodInfo>(p_list);
    }
}

void OScriptInstanceBase::free_method_list(const GDExtensionMethodInfo* p_list) const {
    if (p_list) {
        int size = MemoryUtils::memnew_ptr_size<GDExtensionMethodInfo>(p_list);
        free_method_list(p_list, size);
    }
}

Ref<OScript> OScriptInstanceBase::get_script() const {
    return _script;
}

ScriptLanguage* OScriptInstanceBase::get_language() const {
    return OScriptLanguage::get_singleton();
}

Variant OScriptInstanceBase::get_rpc_config() const {
    return _script->get_rpc_config();
}

String OScriptInstanceBase::to_string() {
    String prefix = "";
    if (Node* node = Object::cast_to<Node>(_owner)) {
        if (!node->get_name().is_empty()) {
            prefix = vformat("%s:", node->get_name());
        }
    }
    return vformat("%s<%s#%d>", prefix, _owner->get_class(), _owner->get_instance_id());
}

void OScriptInstanceBase::set_instance_info(const GDExtensionScriptInstancePtr& p_info) {
    _script_instance = p_info;
}

OScriptInstanceBase::OScriptInstanceBase(const Ref<OScript>& p_script, Object* p_owner) {
    _script = p_script;
    _owner = p_owner;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptInstance

void OScriptInstance::_call_implicit_ready_recursively(const OScript* p_script) { // NOLINT
    if (p_script->base.ptr()) {
        _call_implicit_ready_recursively(p_script->base.ptr());
    }
    if (likely(p_script->_valid) && p_script->implicit_ready) {
        GDExtensionCallError err;
        p_script->implicit_ready->call(this, nullptr, 0, err);
    }
}

LocalVector<PropertyInfo> OScriptInstance::_get_property_list() {
    // Exported members not done yet
    const OScript* sptr = _script.ptr();
    LocalVector<PropertyInfo> props;
    LocalVector<PropertyInfo> properties;

    while (sptr) {
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._get_property_list);
            if (E) {
                GDExtensionCallError err;
                Variant ret = E->value->call(this, nullptr, 0, err);
                if (err.error == GDEXTENSION_CALL_OK) {
                    ERR_FAIL_COND_V_MSG(ret.get_type() != Variant::ARRAY, {}, "Wrong type for _get_property_list, must be an array of dictionaries.");

                    Array arr = ret;
                    for (int i = 0; i < arr.size(); i++) {
                        const Dictionary d = arr[i];
                        ERR_CONTINUE(!d.has("name"));
                        ERR_CONTINUE(!d.has("type"));

                        PropertyInfo info;
                        info.name = d["name"];
                        info.type = static_cast<Variant::Type>(d["type"].operator int());
                        info.class_name = d.get("class_name", info.class_name);
                        info.hint = static_cast<PropertyHint>(d.get("hint", info.hint).operator int());
                        info.hint_string = d.get("hint_string", info.hint_string);
                        info.usage = d.get("usage", info.usage);

                        ERR_CONTINUE(info.name.is_empty() && info.usage & PROPERTY_USAGE_STORAGE);
                        ERR_CONTINUE(info.type < 0  || info.type >= Variant::VARIANT_MAX);
                        props.push_back(info);
                    }
                }
            }
        }

        Vector<OScript::OScriptMemberSort> msort;
        for (const KeyValue<StringName, OScript::MemberInfo>& F : sptr->member_indices) {
            if (!sptr->members.has(F.key)) {
                continue; // skip class base members
            }
            OScript::OScriptMemberSort ms;
            ms.index = F.value.index;
            ms.name = F.key;
            msort.push_back(ms);
        }

        msort.sort();
        msort.reverse();

        for (const OScript::OScriptMemberSort& item : msort) {
            props.insert(0, sptr->member_indices[item.name].property_info);
        }

        #ifdef TOOLS_ENABLED
        properties.insert(0, sptr->get_class_category());
        #endif

        for (PropertyInfo& property : props) {
            validate_property(property);
            properties.push_back(property);
        }

        props.clear();
        sptr = sptr->base.ptr();
    }

    return properties;
}

LocalVector<MethodInfo> OScriptInstance::_get_method_list() const {
    LocalVector<MethodInfo> result;
    const OScript *sptr = _script.ptr();
    while (sptr) {
        for (const KeyValue<StringName, OScriptCompiledFunction*> &E : sptr->member_functions) {
            result.push_back(E.value->get_method_info());
        }
        sptr = sptr->base.ptr();
    }
    return result;
}

bool OScriptInstance::set(const StringName& p_name, const Variant& p_value) {
    {
        HashMap<StringName, OScript::MemberInfo>::Iterator E = _script->member_indices.find(p_name);
        if (E) {
            const OScript::MemberInfo* member = &E->value;
            Variant value = p_value;
            if (!member->data_type.is_type(value)) {
                const Variant* args = &p_value;
                GDExtensionCallError err = GDE::Variant::construct(member->data_type.builtin_type, value, &args, 1);
                if (err.error != GDEXTENSION_CALL_OK || !member->data_type.is_type(value)) {
                    return false;
                }
            }
            if (likely(_script->_valid) && !member->setter.is_empty()) {
                const Variant* args = &value;
                GDExtensionCallError err;
                Variant ret = callp(member->setter, &args, 1, err);
                return err.error == GDEXTENSION_CALL_OK;
            }

            _members.write[member->index] = value;
            return true;
        }
    }

    OScript* sptr = _script.ptr();
    while (sptr) {
        {
            HashMap<StringName, OScript::MemberInfo>::ConstIterator E = sptr->static_variables_indices.find(p_name);
            if (E) {
                const OScript::MemberInfo* member = &E->value;
                Variant value = p_value;
                if (!member->data_type.is_type(value)) {
                    const Variant* args = &p_value;
                    GDExtensionCallError err = GDE::Variant::construct(member->data_type.builtin_type, value, &args, 1);
                    if (err.error != GDEXTENSION_CALL_OK || !member->data_type.is_type(value)) {
                        return false;
                    }
                }

                if (likely(sptr->_valid) && !member->setter.is_empty()) {
                    const Variant* args = &value;
                    GDExtensionCallError err;
                    Variant ret = callp(member->setter, &args, 1, err);
                    return err.error == GDEXTENSION_CALL_OK;
                }

                sptr->static_variables.write[member->index] = value;
                return true;
            }
        }

        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::Iterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._set);
            if (E) {
                Variant name = p_name;
                const Variant* args[2] = { &name, &p_value };

                GDExtensionCallError err;
                Variant ret = E->value->call(this, args, 2, err);
                if (err.error == GDEXTENSION_CALL_OK && ret.get_type() == Variant::BOOL && ret.operator bool()) {
                    return true;
                }
            }
        }

        sptr = sptr->base.ptr();
    }
    return false;
}

bool OScriptInstance::get(const StringName& p_name, Variant& r_value) {
    {
        HashMap<StringName, OScript::MemberInfo>::ConstIterator E = _script->member_indices.find(p_name);
        if (E) {
            if (likely(_script->_valid) && !E->value.getter.is_empty()) {
                GDExtensionCallError err;
                const Variant ret = callp(E->value.getter, nullptr, 0, err);
                r_value = err.error == GDEXTENSION_CALL_OK ? ret : Variant();
                return true;
            }
            r_value = _members[E->value.index];
            return true;
        }
    }

    const OScript* sptr = _script.ptr();
    while (sptr) {
        {
            HashMap<StringName, Variant>::ConstIterator E = sptr->constants.find(p_name);
            if (E) {
                r_value = E->value;
                return true;
            }
        }
        {
            HashMap<StringName, OScript::MemberInfo>::ConstIterator E = sptr->static_variables_indices.find(p_name);
            if (E) {
                if (likely(sptr->_valid) && !E->value.getter.is_empty()) {
                    GDExtensionCallError err;
                    const Variant ret = const_cast<OScript*>(sptr)->callp(E->value.getter, nullptr, 0, err);
                    r_value = err.error == GDEXTENSION_CALL_OK ? ret : Variant();
                    return true;
                }
                r_value = sptr->static_variables[E->value.index];
                return true;
            }
        }
        {
            HashMap<StringName, MethodInfo>::ConstIterator E = sptr->signals.find(p_name);
            if (E) {
                r_value = Signal(_owner, E->key);
                return true;
            }
        }
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(p_name);
            if (E) {
                if (sptr->rpc_config.has(p_name)) {
                    r_value = Callable(memnew(OScriptRPCCallable(_owner, E->key)));
                } else {
                    r_value = Callable(_owner, E->key);
                }
                return true;
            }
        }
        {
            HashMap<StringName, Ref<OScript>>::ConstIterator E = sptr->subclasses.find(p_name);
            if (E) {
                r_value = E->value;
                return true;
            }
        }
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._get);
            if (E) {
                Variant name = p_name;
                const Variant* args[1] = { &name };

                GDExtensionCallError err;
                Variant ret = E->value->call(this, args, 1, err);
                if (err.error == GDEXTENSION_CALL_OK && ret.get_type() != Variant::NIL) {
                    r_value = ret;
                    return true;
                }
            }
        }
        sptr = sptr->base.ptr();
    }
    return false;
}

void OScriptInstance::validate_property(PropertyInfo& p_property) const {
    const OScript* sptr = _script.ptr();
    while (sptr) {
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._validate_property);
            if (E) {
                Variant property = static_cast<Dictionary>(p_property);
                const Variant* args[1] = { &property };

                GDExtensionCallError err;
                Variant ret = E->value->call(const_cast<OScriptInstance*>(this), args, 1, err);
                if (err.error == GDEXTENSION_CALL_OK) {
                    p_property = PropertyInfo::from_dict(property);
                    return;
                }
            }
        }
        sptr = sptr->base.ptr();
    }
}

bool OScriptInstance::property_can_revert(const StringName& p_name) const {
    Variant name = p_name;
    const Variant* args[1] = { &name };

    const OScript* sptr = _script.ptr();
    while (sptr) {
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._property_can_revert);
            if (E) {
                GDExtensionCallError err;
                Variant ret = E->value->call(const_cast<OScriptInstance*>(this), args, 1, err);
                if (err.error == GDEXTENSION_CALL_OK && ret.get_type() == Variant::BOOL && ret.operator bool()) {
                    return true;
                }
            }
        }
        sptr = sptr->base.ptr();
    }

    return false;
}

bool OScriptInstance::property_get_revert(const StringName& p_name, Variant& r_value) const {
    Variant name = p_name;
    const Variant *args[1] = { &name };

    const OScript *sptr = _script.ptr();
    while (sptr) {
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(OScriptLanguage::get_singleton()->strings._property_get_revert);
            if (E) {
                GDExtensionCallError err;
                Variant ret = E->value->call(const_cast<OScriptInstance*>(this), args, 1, err);
                if (err.error == GDEXTENSION_CALL_OK && ret.get_type() != Variant::NIL) {
                    r_value = ret;
                    return true;
                }
            }
        }
        sptr = sptr->base.ptr();
    }
    return false;
}

Variant::Type OScriptInstance::get_property_type(const StringName& p_name, bool* r_valid) {
    if (_script->member_indices.has(p_name)) {
        if (r_valid) {
            *r_valid = true;
        }
        return _script->member_indices[p_name].property_info.type;
    }

    if (r_valid) {
        *r_valid = false;
    }

    return Variant::NIL;
}

bool OScriptInstance::has_method(const StringName& p_name) const {
    const OScript* sptr = _script.ptr();
    while (sptr) {
        HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(p_name);
        if (E) {
            return true;
        }
        sptr = sptr->base.ptr();
    }
    return false;
}

int OScriptInstance::get_method_argument_count(const StringName& p_name, bool* r_valid) const {
    const OScript* sptr = _script.ptr();
    while (sptr) {
        HashMap<StringName, OScriptCompiledFunction*>::ConstIterator E = sptr->member_functions.find(p_name);
        if (E) {
            if (r_valid) {
                *r_valid = true;
            }
            return E->value->get_argument_count();
        }
        sptr = sptr->base.ptr();
    }
    if (r_valid) {
        *r_valid = false;
    }
    return 0;
}

void OScriptInstance::notification(int p_notification, bool p_reversed) {
    if (unlikely(!_script->_valid)) {
        return;
    }

    // Notification is not virtual, it gets called at ALL levels just like in C.
    Variant value = p_notification;
    const Variant* args[1] = { &value };
    const StringName& notification_str = OScriptLanguage::get_singleton()->strings._notification;

    LocalVector<OScript*> script_stack;
    int32_t script_count = 0;
    for (OScript* sptr = _script.ptr(); sptr; sptr = sptr->base.ptr(), ++script_count) {
        script_stack.push_back(sptr);
    }

    const int start = p_reversed ? 0 : script_count - 1;
    const int end = p_reversed ? script_count : -1;
    const int step = p_reversed ? 1 : -1;

    for (int index = start; index != end; index += step) {
        OScript* sc = script_stack[index];
        if (likely(sc->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::Iterator E = sc->member_functions.find(notification_str);
            if (E) {

                GDExtensionCallError err;
                E->value->call(this, args, 1, err);
                if (err.error != GDEXTENSION_CALL_OK) {
                    // print error about notification call
                }
            }
        }
    }
}

Variant OScriptInstance::callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
    OScript* sptr = _script.ptr();

    if (unlikely(p_method == SceneStringName(_ready))) {
        // Call implicit ready first, including the super classes recursively.
        _call_implicit_ready_recursively(sptr);
    }

    while (sptr) {
        if (likely(sptr->_valid)) {
            HashMap<StringName, OScriptCompiledFunction*>::Iterator E = sptr->member_functions.find(p_method);
            if (E) {
                return E->value->call(this, p_args, p_arg_count, r_error);
            }
        }
        sptr = sptr->base.ptr();
    }

    r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    return {};

    // // When methods are called in tight loops, this allows for higher cache hits and we can
    // // minimize the lookup into the HashMap.
    // static StringName cached_method;
    // static OScriptCompiledFunction* cached_function = nullptr;
    //
    // OScriptCompiledFunction* func = nullptr;
    // if (cached_method == p_method) {
    //     func = cached_function;
    // } else {
    //     while (sptr) {
    //         if (likely(sptr->_valid)) {
    //             HashMap<StringName, OScriptCompiledFunction*>::Iterator E = sptr->member_functions.find(p_method);
    //             if (E) {
    //                 func = E->value;
    //                 cached_method = p_method;
    //                 cached_function = func;
    //                 break;
    //             }
    //         }
    //         sptr = sptr->base.ptr();
    //     }
    // }
    //
    // if (func) {
    //     func->call(this, p_args, p_arg_count, r_error, r_value);
    //     return;
    // }
    //
    // r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    // // return {};
}

void OScriptInstance::reload_members() {
    #ifdef DEBUG_ENABLED
    Vector<Variant> new_members;
    new_members.resize(_script->member_indices.size());

    // Pass values to the new indices
    for (KeyValue<StringName, OScript::MemberInfo>& E : _script->member_indices) {
        if (_member_indices_cache.has(E.key)) {
            Variant value = _members[_member_indices_cache[E.key]];
            new_members.write[E.value.index] = value;
        }
    }

    _members.resize(new_members.size());
    _members = new_members;

    // Pass values to new indices
    _member_indices_cache.clear();
    for (const KeyValue<StringName, OScript::MemberInfo>& E : _script->member_indices) {
        _member_indices_cache[E.key] = E.value.index;
    }
    #endif
}

OScriptInstance::OScriptInstance(const Ref<OScript>& p_script, Object* p_owner)
    : OScriptInstanceBase(p_script, p_owner) {
}

OScriptInstance::~OScriptInstance() {
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    while (SelfList<OScriptFunctionState>* E = _pending_func_states.first()) {
        // Order matters since clearing the stack may already cause the OScriptFunctionState to
        // be destroyed and thus removed from the list.
        _pending_func_states.remove(E);

        OScriptFunctionState* state = E->self();
        ObjectID state_id = ObjectID(state->get_instance_id());

        state->_clear_connections();
        if (ObjectDB::get_instance(state_id)) {
            state->_clear_stack();
        }
    }

    if (_script.is_valid() && _owner) {
        _script->instances.erase(_owner);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptPlaceHolderInstance

struct OScriptPlaceHolderInstanceCallbacks : OScriptInstanceCallbacks<OScriptPlaceHolderInstance> {
};

const OScriptInstanceInfo OScriptPlaceHolderInstance::INSTANCE_INFO = [] {
    OScriptInstanceInfo result = {};
    result.set_func = OScriptPlaceHolderInstanceCallbacks::set_func;
    result.get_func = OScriptPlaceHolderInstanceCallbacks::get_func;
    result.get_property_list_func = OScriptPlaceHolderInstanceCallbacks::get_property_list_func;
    result.free_property_list_func = OScriptPlaceHolderInstanceCallbacks::free_property_list_func;
    result.property_can_revert_func = OScriptPlaceHolderInstanceCallbacks::property_can_revert_func;
    result.property_get_revert_func = OScriptPlaceHolderInstanceCallbacks::property_get_revert_func;
    result.get_owner_func = OScriptPlaceHolderInstanceCallbacks::get_owner_func;
    result.get_property_state_func = OScriptPlaceHolderInstanceCallbacks::get_property_state_func;
    result.get_method_list_func = OScriptPlaceHolderInstanceCallbacks::get_method_list_func;
    result.free_method_list_func = OScriptPlaceHolderInstanceCallbacks::free_method_list_func;
    result.get_property_type_func = OScriptPlaceHolderInstanceCallbacks::get_property_type_func;
    result.validate_property_func = OScriptPlaceHolderInstanceCallbacks::validate_property_func;
    result.has_method_func = OScriptPlaceHolderInstanceCallbacks::has_method_func;
    result.get_method_argument_count_func = OScriptPlaceHolderInstanceCallbacks::get_method_argument_count_func;
    result.call_func = OScriptPlaceHolderInstanceCallbacks::call_func;
    result.notification_func = OScriptPlaceHolderInstanceCallbacks::notification_func;
    result.to_string_func = OScriptPlaceHolderInstanceCallbacks::to_string_func;
    result.get_script_func = OScriptPlaceHolderInstanceCallbacks::get_script_func;
    result.is_placeholder_func = OScriptPlaceHolderInstanceCallbacks::is_placeholder_func;
    result.set_fallback_func = OScriptPlaceHolderInstanceCallbacks::property_set_fallback_func;
    result.get_fallback_func = OScriptPlaceHolderInstanceCallbacks::property_get_fallback_func;
    result.get_language_func = OScriptPlaceHolderInstanceCallbacks::get_language_func;
    result.free_func = OScriptPlaceHolderInstanceCallbacks::free_func;
    return result;
}();

LocalVector<PropertyInfo> OScriptPlaceHolderInstance::_get_property_list() {
    LocalVector<PropertyInfo> result;
    for (const PropertyInfo& E : _properties) {
        result.push_back(E);
    }
    return result;
}

LocalVector<MethodInfo> OScriptPlaceHolderInstance::_get_method_list() const {
    LocalVector<MethodInfo> result;

    if (_script->_is_placeholder_fallback_enabled()) {
        return result;
    }

    const TypedArray<Dictionary> methods = _script->get_script_method_list();
    for (uint32_t i = 0; i < methods.size(); i++) {
        result.push_back(DictionaryUtils::to_method(methods[i]));
    }
    return result;
}

bool OScriptPlaceHolderInstance::set(const StringName& p_name, const Variant& p_value) {
    if (_script->_is_placeholder_fallback_enabled()) {
        return false;
    }

    if (_values.has(p_name)) {
        Variant default_value;
        if (_script->get_property_default_value(p_name, default_value)) {
            if (GDE::Variant::evaluate(Variant::OP_EQUAL, default_value, p_value)) {
                _values.erase(p_name);
                return true;
            }
        }
        _values[p_name] = p_value;
        return true;
    }

    Variant default_value;
    if (_script->get_property_default_value(p_name, default_value)) {
        if (GDE::Variant::evaluate(Variant::OP_NOT_EQUAL, default_value, p_value)) {
            _values[p_name] = p_value;
        }
        return true;
    }
    return false;
}

bool OScriptPlaceHolderInstance::get(const StringName& p_name, Variant& r_value) {
    if (_values.has(p_name)) {
        r_value = _values[p_name];
        return true;
    }
    if (_constants.has(p_name)) {
        r_value = _constants[p_name];
        return true;
    }
    if (!_script->_is_placeholder_fallback_enabled()) {
        Variant default_value;
        if (_script->get_property_default_value(p_name, default_value)) {
            r_value = default_value;
            return true;
        }
    }
    return false;
}

void OScriptPlaceHolderInstance::property_set_fallback(const StringName& p_name, const Variant& p_value, bool* r_valid) {
    if (_script->_is_placeholder_fallback_enabled()) {
        HashMap<StringName, Variant>::Iterator E = _values.find(p_name);
        if (E) {
            E->value = p_value;
        } else {
            _values.insert(p_name, p_value);
        }

        bool found = false;
        for (const PropertyInfo& property : _properties) {
            if (property.name == p_name) {
                found = true;
                break;
            }
        }

        if (!found) {
            PropertyHint hint = PROPERTY_HINT_NONE;
            const Object* object = p_value.get_validated_object();
            if (object && object->is_class(Node::get_class_static())) {
                hint = PROPERTY_HINT_NODE_TYPE;
            }
            _properties.push_back(PropertyInfo(p_value.get_type(), p_name, hint, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE));
        }
    }

    if (r_valid) {
        *r_valid = false; // cannot change the value in either case
    }
}

Variant OScriptPlaceHolderInstance::property_get_fallback(const StringName& p_name, bool* r_valid) {
    if (_script->_is_placeholder_fallback_enabled()) {
        HashMap<StringName, Variant>::ConstIterator E = _values.find(p_name);
        if (E) {
            if (r_valid) {
                *r_valid = true;
            }
            return E->value;
        }

        E = _constants.find(p_name);
        if (E) {
            if (r_valid) {
                *r_valid = true;
            }
            return E->value;
        }
    }

    if (r_valid) {
        *r_valid = false;
    }

    return {};
}

Variant::Type OScriptPlaceHolderInstance::get_property_type(const StringName& p_name, bool* r_valid) {
    if (_values.has(p_name)) {
        if (r_valid) {
            *r_valid = true;
        }
        return _values[p_name].get_type();
    }

    if (_constants.has(p_name)) {
        if (r_valid) {
            *r_valid = true;
        }
        return _constants[p_name].get_type();
    }

    if (r_valid) {
        *r_valid = false;
    }

    return Variant::NIL;
}

bool OScriptPlaceHolderInstance::has_method(const StringName& p_name) const {
    if (_script->_is_placeholder_fallback_enabled()) {
        return false;
    }

    Ref<Script> scr = _script;
    while (scr.is_valid()) {
        if (scr->has_method(p_name)) {
            return true;
        }
        scr = scr->get_base_script();
    }

    return false;
}

Variant OScriptPlaceHolderInstance::callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
    r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    #if TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        return String("Attempt to call a method on a placeholder instance. Check if the script is in tool mode.");
    }
    return String("Attempt to call a method on a placeholder instance. Probably a bug, please report.");
    #else
    return Variant();
    #endif // TOOLS_ENABLED
}

void OScriptPlaceHolderInstance::update(const List<PropertyInfo>& p_properties, const HashMap<StringName, Variant>& p_values) {
    HashSet<StringName> new_values;
    for (const PropertyInfo& E : p_properties) {
        if (E.usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP | PROPERTY_USAGE_CATEGORY)) {
            continue;
        }

        StringName n = E.name;
        new_values.insert(n);

        if (!_values.has(n) || (E.type != Variant::NIL && _values[n].get_type() != E.type)) {
            if (p_values.has(n)) {
                _values[n] = p_values[n];
            }
        }
    }

    _properties = p_properties;

    List<StringName> to_remove;
    for (KeyValue<StringName, Variant>& E : _values) {
        if (!new_values.has(E.key)) {
            to_remove.push_back(E.key);
        }

        Variant default_value;
        if (_script->get_property_default_value(E.key, default_value)) {
            // Remove because it's the same as the default
            if (default_value == E.value) {
                to_remove.push_back(E.key);
            }
        }
    }

    while (to_remove.size()) {
        _values.erase(to_remove.front()->get());
        to_remove.pop_front();
    }

    #if GODOT_VERSION >= 0x040500
    if (_owner && _is_same_script_instance()) {
        _owner->notify_property_list_changed();
    }
    #else
    // This may be less efficient on older versions
    if (_owner) {
        _owner->notify_property_list_changed();
    }
    #endif

    _constants.clear();
    _script->get_constants(&_constants);
}

OScriptPlaceHolderInstance::OScriptPlaceHolderInstance(const Ref<OScript>& p_script, Object* p_owner)
    : OScriptInstanceBase(p_script, p_owner) {
}

OScriptPlaceHolderInstance::~OScriptPlaceHolderInstance() {
    if (_script.is_valid()) {
        _script->_placeholder_erased(this);
    }
}
