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
#ifndef ORCHESTRATOR_NAME_UTILS_H
#define ORCHESTRATOR_NAME_UTILS_H

#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace NameUtils {
    /// Create a unique name based on the prefix and not in the names array
    /// @param p_prefix the prefix
    /// @param p_names the existing names
    /// @return the unique name
    String create_unique_name(const String& p_prefix, const PackedStringArray& p_names = PackedStringArray());
}

#endif // ORCHESTRATOR_NAME_UTILS_H