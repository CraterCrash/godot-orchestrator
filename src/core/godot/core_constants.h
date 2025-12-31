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
#ifndef ORCHESTRATOR_CORE_GODOT_CORE_CONSTANTS_H
#define ORCHESTRATOR_CORE_GODOT_CORE_CONSTANTS_H

#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

namespace GDE {
    struct CoreConstants {
    private:
        struct CoreConstant {
            StringName name;
            StringName enum_name;
            int64_t value = 0;

            CoreConstant() = default;
            CoreConstant(const StringName& p_enum_name, const StringName& p_name, int64_t p_value)
                : enum_name(p_enum_name), name(p_name), value(p_value) {}
        };

        static Vector<CoreConstant> global_constants;
        static HashMap<StringName, int> global_constants_map;
        static HashMap<StringName, Vector<CoreConstant>> global_enums;

        static void _prime();

    public:
        CoreConstants() = delete;

        static int get_global_constant_count();
        static bool is_global_constant(const StringName& p_name);
        static StringName get_global_constant_name(int p_index);
        static int get_global_constant_index(const StringName& p_name);
        static int64_t get_global_constant_value(int p_index);

        static bool is_global_enum(const StringName& p_name);
        static StringName get_global_constant_enum(int p_index);

        static HashMap<StringName, int64_t> get_enum_values(const StringName& p_native_type);
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_CORE_CONSTANTS_H