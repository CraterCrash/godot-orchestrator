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
#include "core/godot/core_constants.h"

#include "api/extension_db.h"

#include <godot_cpp/variant/utility_functions.hpp>

Vector<GDE::CoreConstants::CoreConstant> GDE::CoreConstants::global_constants;
HashMap<StringName, int> GDE::CoreConstants::global_constants_map;
HashMap<StringName, Vector<GDE::CoreConstants::CoreConstant>> GDE::CoreConstants::global_enums;

#define ENSURE_PRIMED                   \
    if (global_constants.is_empty()) {  \
        _prime();                       \
    }

// todo: efficiency improvement?
//  we prime this lazily on first access; however, perhaps there is a better way to integrate
//  ExtensionDB here by having the ExtensionDB store the values in a similar way so that the
//  information can be stored once?

void GDE::CoreConstants::_prime() {
    for (const String& enumeration_name : ExtensionDB::get_global_enum_names()) {
        const EnumInfo& ei = ExtensionDB::get_global_enum(enumeration_name);
        for (const EnumValue& ev : ei.values) {
            global_constants.push_back(CoreConstant(ei.name, ev.name, ev.value ));
            global_constants_map[ev.name] = global_constants.size() - 1; // NOLINT
            global_enums[ei.name].push_back((global_constants.ptr())[global_constants.size() - 1]);
        }
    }
}

int GDE::CoreConstants::get_global_constant_count() {
    ENSURE_PRIMED
    return global_constants.size(); // NOLINT
}

bool GDE::CoreConstants::is_global_constant(const StringName& p_name) {
    ENSURE_PRIMED
    return global_constants_map.has(p_name);
}

StringName GDE::CoreConstants::get_global_constant_name(int p_index) {
    ENSURE_PRIMED
    return global_constants[p_index].name;
}

int GDE::CoreConstants::get_global_constant_index(const StringName& p_name) {
    ENSURE_PRIMED
    ERR_FAIL_COND_V_MSG(!global_constants_map.has(p_name), -1, "Trynig to getindex of non-existing constant.");
    return global_constants_map[p_name];
}

int64_t GDE::CoreConstants::get_global_constant_value(int p_index) {
    ENSURE_PRIMED
    return global_constants[p_index].value;
}

bool GDE::CoreConstants::is_global_enum(const StringName& p_name) {
    ENSURE_PRIMED
    return global_enums.has(p_name);
}

StringName GDE::CoreConstants::get_global_constant_enum(int p_index) {
    ENSURE_PRIMED
    return global_constants[p_index].enum_name;
}

HashMap<StringName, int64_t> GDE::CoreConstants::get_enum_values(const StringName& p_native_type) {
    ENSURE_PRIMED
    ERR_FAIL_COND_V(!ExtensionDB::get_global_enum_names().has(p_native_type), {});

    HashMap<StringName, int64_t> results;
    const EnumInfo& ei = ExtensionDB::get_global_enum(p_native_type);
    for (const EnumValue& value : ei.values) {
        results[value.name] = value.value;
    }
    return results;
}