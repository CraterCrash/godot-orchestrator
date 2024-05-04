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
#include "common/method_utils.h"

#include <godot_cpp/core/class_db.hpp>

namespace MethodUtils
{
    bool has_return_value(const MethodInfo& p_method)
    {
        // When the method specifies a non-NIL type, this means it isn't Variant, but
        // instead returns an explicit type.
        if (p_method.return_val.type != Variant::NIL)
            return true;

        // When the usage flag PROPERTY_USAGE_NIL_IS_VARIANT is set, the return value is Variant
        if (p_method.return_val.usage & PROPERTY_USAGE_NIL_IS_VARIANT)
            return true;

        // No return value
        return false;
    }

    void set_no_return_value(MethodInfo& p_method)
    {
        p_method.return_val.type = Variant::NIL;
        p_method.return_val.usage &= ~PROPERTY_USAGE_NIL_IS_VARIANT;
    }

    void set_return_value(MethodInfo& p_method)
    {
        if (p_method.return_val.type == Variant::NIL)
            p_method.return_val.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
        else
            p_method.return_val.usage &= ~PROPERTY_USAGE_NIL_IS_VARIANT;
    }

    void set_return_value_type(MethodInfo& p_method, Variant::Type p_type)
    {
        p_method.return_val.type = p_type;
        set_return_value(p_method);
    }

    String get_method_class(const String& p_class_name, const String& p_method_name)
    {
        String class_name = p_class_name;
        while (!class_name.is_empty())
        {
            TypedArray<Dictionary> methods = ClassDB::class_get_method_list(class_name, true);
            for (int i = 0; i < methods.size(); i++)
            {
                const Dictionary& method = methods[i];
                if (p_method_name.match(method["name"]))
                    return class_name;
            }
            class_name = ClassDB::get_parent_class(class_name);
        }
        return {};
    }
}