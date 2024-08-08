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
#include "memory_utils.h"

namespace MemoryUtils
{
    void free_method_info(const GDExtensionMethodInfo& p_method)
    {
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

    void free_property_info(const GDExtensionPropertyInfo& p_property)
    {
        memdelete((StringName*) p_property.name);
        memdelete((StringName*) p_property.class_name);
        memdelete((String*) p_property.hint_string);
    }
}