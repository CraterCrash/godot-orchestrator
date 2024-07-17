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
#include "instance_base.h"

#include "common/memory_utils.h"
#include "script/script.h"

#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>

static void add_to_state(GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value, void* p_userdata)
{
    List<Pair<StringName, Variant>>* list = reinterpret_cast<List<Pair<StringName, Variant>>*>(p_userdata);
    list->push_back({ *(const StringName*)p_name, *(const Variant*)p_value });
}

OScriptInstanceBase::OScriptInstanceBase()
{
}

OScriptInstanceBase::~OScriptInstanceBase()
{
}

void OScriptInstanceBase::init_instance(OScriptInstanceInfo& p_info)
{
    // Not all compilers do this automatically
    // If they're not overwritten here or by the derived implementations, null is used
    memset(&p_info, 0, sizeof(GDExtensionScriptInstanceInfo2));

    p_info.get_property_list_func = [](void* p_self, uint32_t* r_count) -> const GDExtensionPropertyInfo* {
        return ((OScriptInstanceBase*)p_self)->get_property_list(r_count);
    };

    #if GODOT_VERSION >= 0x040300
    p_info.free_property_list_func = [](void* p_self, const GDExtensionPropertyInfo* p_list, uint32_t p_count) {
        ((OScriptInstanceBase*)p_self)->free_property_list(p_list, p_count);
    };
    #else
    p_info.free_property_list_func = [](void* p_self, const GDExtensionPropertyInfo* p_list) {
        ((OScriptInstanceBase*)p_self)->free_property_list(p_list);
    };
    #endif

    p_info.get_owner_func = [](void* p_self) {
        return ((OScriptInstanceBase*)p_self)->get_owner()->_owner;
    };

    p_info.get_property_state_func = [](void* p_self, GDExtensionScriptInstancePropertyStateAdd p_add_func,
                                        void* p_userdata) {
        return ((OScriptInstanceBase*)p_self)->get_property_state(p_add_func, p_userdata);
    };

    p_info.get_method_list_func = [](void* p_self, uint32_t* r_count) -> const GDExtensionMethodInfo* {
        return ((OScriptInstanceBase*)p_self)->get_method_list(r_count);
    };

    #if GODOT_VERSION >= 0x040300
    p_info.free_method_list_func = [](void* p_self, const GDExtensionMethodInfo* p_list, uint32_t p_count) {
        ((OScriptInstanceBase*)p_self)->free_method_list(p_list, p_count);
    };
    #else
    p_info.free_method_list_func = [](void* p_self, const GDExtensionMethodInfo* p_list) {
        ((OScriptInstanceBase*)p_self)->free_method_list(p_list);
    };
    #endif

    p_info.get_property_type_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                                       GDExtensionBool* r_is_valid) -> GDExtensionVariantType {
        return (GDExtensionVariantType)((OScriptInstanceBase*)p_self)
            ->get_property_type(*(const StringName*)p_name, (bool*)r_is_valid);
    };

    p_info.get_script_func = [](void* p_self) {
        return ((OScriptInstanceBase*)p_self)->get_script().ptr()->_owner;
    };

    p_info.get_language_func = [](void* p_self) {
        return ((OScriptInstanceBase*)p_self)->get_language()->_owner;
    };

    p_info.is_placeholder_func = [](void* p_self) -> GDExtensionBool {
        return ((OScriptInstanceBase*) p_self)->is_placeholder();
    };

}

void OScriptInstanceBase::get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void* p_userdata)
{
    uint32_t count = 0;
    GDExtensionPropertyInfo* props = get_property_list(&count);
    for (uint32_t i = 0; i < count; i++)
    {
        StringName* name = (StringName*)props[i].name;
        if (props[i].usage & PROPERTY_USAGE_STORAGE)
        {
            Variant value;
            bool is_valid = get(*name, value);
            if (is_valid)
                p_add_func(name, &value, p_userdata);
        }
    }
    #if GODOT_VERSION >= 0x040300
    free_property_list(props, count);
    #else
    free_property_list(props);
    #endif
}

void OScriptInstanceBase::get_property_state(List<Pair<StringName, Variant>>& p_list)
{
    get_property_state(add_to_state, &p_list);
}

#if GODOT_VERSION >= 0x040300
void OScriptInstanceBase::free_property_list(const GDExtensionPropertyInfo* p_list, uint32_t p_count) const
#else
void OScriptInstanceBase::free_property_list(const GDExtensionPropertyInfo* p_list) const
#endif
{
    if (p_list)
    {
        int size = MemoryUtils::memnew_ptr_size<GDExtensionPropertyInfo>(p_list);
        for (int i = 0; i < size; i++)
            MemoryUtils::free_property_info(p_list[i]);

        MemoryUtils::memdelete_with_size<GDExtensionPropertyInfo>(p_list);
    }
}

GDExtensionMethodInfo* OScriptInstanceBase::get_method_list(uint32_t* r_count) const
{
    LocalVector<GDExtensionMethodInfo> methods;
    HashSet<StringName> defined;

    const Ref<OScript> script = get_script();
    if (script.is_valid())
    {
        PackedStringArray function_names = script->get_function_names();
        for (const String& function_name : function_names)
        {
            const Ref<OScriptFunction>& function = script->find_function(StringName(function_name));
            defined.insert(function->get_function_name());

            const MethodInfo& mi = function->get_method_info();

            GDExtensionMethodInfo dst;
            dst.name = MemoryUtils::memnew_stringname(function->get_function_name());
            dst.flags = mi.flags;
            copy_property(mi.return_val, dst.return_value);

            dst.argument_count = mi.arguments.size();
            if (dst.argument_count > 0)
            {
                GDExtensionPropertyInfo *list = memnew_arr(GDExtensionPropertyInfo, dst.argument_count);
                for (size_t j = 0; j < dst.argument_count; j++)
                    copy_property(mi.arguments[j], list[j]);

                dst.arguments = list;
            }

            dst.default_argument_count = mi.default_arguments.size();
            if (dst.default_argument_count > 0)
            {
                Variant* args = memnew_arr(Variant, dst.default_argument_count);
                for (size_t j = 0; j < dst.default_argument_count; j++)
                    args[j] = mi.default_arguments[j];

                dst.default_arguments = (GDExtensionVariantPtr*) args;
            }

            methods.push_back(dst);
        }
    }

    int size = methods.size();
    *r_count = size;

    GDExtensionMethodInfo* list = MemoryUtils::memnew_with_size<GDExtensionMethodInfo>(size);
    memcpy(list, methods.ptr(), sizeof(GDExtensionMethodInfo) * size);

    return list;
}

#if GODOT_VERSION >= 0x040300
void OScriptInstanceBase::free_method_list(const GDExtensionMethodInfo* p_list, uint32_t p_count) const
#else
void OScriptInstanceBase::free_method_list(const GDExtensionMethodInfo* p_list) const
#endif
{
    if (p_list)
    {
        int size = MemoryUtils::memnew_ptr_size<GDExtensionMethodInfo>(p_list);
        for (int i = 0; i < size; i++)
            MemoryUtils::free_method_info(p_list[i]);
        MemoryUtils::memdelete_with_size<GDExtensionMethodInfo>(p_list);
    }
}

void OScriptInstanceBase::copy_property(const PropertyInfo& p_property, GDExtensionPropertyInfo& p_dst) const
{
    p_dst.type = GDExtensionVariantType(p_property.type);
    p_dst.name = MemoryUtils::memnew_stringname(p_property.name);
    p_dst.class_name = MemoryUtils::memnew_stringname(p_property.class_name);
    p_dst.hint = p_property.hint;
    p_dst.hint_string = MemoryUtils::memnew_string(p_property.hint_string);
    p_dst.usage = p_property.usage;
}

StringName OScriptInstanceBase::_get_variable_name_from_path(const StringName& p_property_path) const
{
    const PackedStringArray parts = p_property_path.split("/");
    if (parts.size() == 0)
        return {};

    return parts[parts.size() - 1];
}