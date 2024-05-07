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
#include "common/name_utils.h"

#include <limits>

#include <godot_cpp/variant/variant.hpp>

namespace NameUtils
{
    String create_unique_name(const String& p_prefix, const PackedStringArray& p_names)
    {
        if (!p_names.has(p_prefix))
            return p_prefix;

        for (int i = 0; i < std::numeric_limits<int>::max(); i++)
        {
            const String name = vformat("%s_%s", p_prefix, i);
            if (!p_names.has(name))
                return name;
        }

        WARN_PRINT("Failed to create a unique name for prefix: " + p_prefix);
        return {};
    }
}