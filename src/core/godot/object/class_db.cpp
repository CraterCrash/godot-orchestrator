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
#include "core/godot/object/class_db.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;
using GClassDB = godot::ClassDB;

bool GDE::ClassDB::is_abstract(const StringName& p_class_name) {
    // todo: this is just a workaround for now - this needs to be exposed
    return !GClassDB::can_instantiate(p_class_name);
}

bool GDE::ClassDB::is_class_exposed(const StringName& p_class_name) {
    // todo: is there a better way to check this?
    return GClassDB::get_class_list().has(p_class_name);
}

StringName GDE::ClassDB::get_parent_class_nocheck(const StringName& p_class_name) {
    // Explicitly check whether the class exists to avoid Godot logged errors
    if (!GClassDB::class_exists(p_class_name)) {
        return {};
    }
    return GClassDB::get_parent_class(p_class_name);
}

bool GDE::ClassDB::has_enum(const StringName& p_class_name, const String& p_enum_name, bool p_no_inheritance) {
    return GClassDB::class_get_enum_list(p_class_name, p_no_inheritance).has(p_enum_name);
}

int64_t GDE::ClassDB::get_integer_constant(const StringName& p_class_name, const String& p_constant_name, bool& r_valid) {
    if (GClassDB::class_has_integer_constant(p_class_name, p_constant_name)) {
        r_valid = true;
        return GClassDB::class_get_integer_constant(p_class_name, p_constant_name);
    }
    r_valid = false;
    return 0;
}

StringName GDE::ClassDB::get_integer_constant_enum(const StringName& p_class_name, const String& p_enum_name) {
    if (GClassDB::class_has_enum(p_class_name, p_enum_name)) {
        return GClassDB::class_get_integer_constant_enum(p_class_name, p_enum_name);
    }
    return {};
}

bool GDE::ClassDB::has_integer_constant(const StringName& p_class_name, const String& p_constant_name, bool p_no_inheritance) {
    return GClassDB::class_get_integer_constant_list(p_class_name, p_no_inheritance).has(p_constant_name);
}

bool GDE::ClassDB::get_method_info(const StringName& p_class_name, const StringName& p_method_name, MethodInfo& r_info, bool p_no_inheritance, bool p_exclude_from_properties) {
    return ExtensionDB::get_class_method_info(p_class_name, p_method_name, r_info, p_no_inheritance);
}

bool GDE::ClassDB::has_property(const StringName& p_class_name, const StringName& p_property_name, bool p_no_inheritance) {
    const TypedArray<Dictionary> properties = GClassDB::class_get_property_list(p_class_name, p_no_inheritance);
    for (uint32_t i = 0; i < properties.size(); i++) {
        const Dictionary& dict = properties[i];
        if (StringName(dict.get("name", "")) == p_property_name) {
            return true;
        }
    }
    return false;
}

StringName GDE::ClassDB::get_property_setter(const StringName& p_class_name, const StringName& p_property_name) {
    return GClassDB::class_get_property_setter(p_class_name, p_property_name);
}

StringName GDE::ClassDB::get_property_getter(const StringName& p_class_name, const StringName& p_property_name) {
    return GClassDB::class_get_property_getter(p_class_name, p_property_name);
}

bool GDE::ClassDB::has_signal(const StringName& p_class_name, const StringName& p_signal_name, bool p_no_inheritance) {
    const TypedArray<Dictionary> signals = GClassDB::class_get_signal_list(p_class_name, p_no_inheritance);
    for (uint32_t i = 0; i < signals.size(); i++) {
        const Dictionary& dict = signals[i];
        if (StringName(dict.get("name", "")) == p_signal_name) {
            return true;
        }
    }
    return false;
}

bool GDE::ClassDB::get_signal(const StringName& p_class_name, const StringName& p_signal_name, MethodInfo& r_info) {
    if (GClassDB::class_has_signal(p_class_name, p_signal_name)) {
        const Dictionary signal = GClassDB::class_get_signal(p_class_name, p_signal_name);
        r_info = DictionaryUtils::to_method(signal);
        return true;
    }
    return false;
}