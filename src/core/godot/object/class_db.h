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
#ifndef ORCHESTRATOR_CORE_GODOT_OBJECT_CLASSDB_H
#define ORCHESTRATOR_CORE_GODOT_OBJECT_CLASSDB_H

#include <godot_cpp/core/object.hpp>

namespace GDE {

    struct ClassDB {
        ClassDB() = delete;

        static bool is_abstract(const godot::StringName& p_class_name);
        static bool is_class_exposed(const godot::StringName& p_class_name);
        static godot::StringName get_parent_class_nocheck(const godot::StringName& p_class_name);

        static bool has_enum(const godot::StringName& p_class_name, const godot::String& p_enum_name, bool p_no_inheritance = false);

        static int64_t get_integer_constant(const godot::StringName& p_class_name, const godot::String& p_constant_name, bool& r_valid);
        static godot::StringName get_integer_constant_enum(const godot::StringName& p_class_name, const godot::String& p_enum_name);
        static bool has_integer_constant(const godot::StringName& p_class_name, const godot::String& p_constant_name, bool p_no_inheritance = false);

        static bool get_method_info(const godot::StringName& p_class_name, const godot::StringName& p_method_name, godot::MethodInfo& r_info, bool p_no_inheritance = false, bool p_exclude_from_properties = false);

        static bool has_property(const godot::StringName& p_class_name, const godot::StringName& p_property_name, bool p_no_inheritance = false);
        static godot::StringName get_property_setter(const godot::StringName& p_class_name, const godot::StringName& p_property_name);
        static godot::StringName get_property_getter(const godot::StringName& p_class_name, const godot::StringName& p_property_name);

        static godot::Variant get_property_default_value(const godot::StringName& p_class_name, const godot::StringName& p_property_name);

        static bool has_signal(const godot::StringName& p_class_name, const godot::StringName& p_signal_name, bool p_no_inheritance = false);
        static bool get_signal(const godot::StringName& p_class_name, const godot::StringName& p_signal_name, godot::MethodInfo& r_info);
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_OBJECT_CLASSDB_H
