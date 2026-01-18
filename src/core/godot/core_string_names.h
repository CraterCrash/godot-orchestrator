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
#ifndef ORCHESTRATOR_CORE_GODOT_CORE_STRING_NAMES_H
#define ORCHESTRATOR_CORE_GODOT_CORE_STRING_NAMES_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class CoreStringNames {
    inline static CoreStringNames* singleton = nullptr;

public:
    static void create() { singleton = memnew(CoreStringNames); }
    static void free() {
        memdelete(singleton);
        singleton = nullptr;
    }

    _FORCE_INLINE_ static CoreStringNames* get_singleton() { return singleton; }

    const StringName free_ = "free";
    const StringName changed = "changed";
    const StringName script = "script";
    const StringName script_changed = "script_changed";
    const StringName _iter_init = "_iter_init";
    const StringName _iter_next = "_iter_next";
    const StringName _iter_get = "_iter_get";
    const StringName get_rid = "get_rid";
    const StringName _to_string = "_to_string";
    const StringName _custom_features = "_custom_features";

    const StringName x = "x";
    const StringName y = "y";
    const StringName z = "z";
    const StringName w = "w";
    const StringName r = "r";
    const StringName g = "g";
    const StringName b = "b";
    const StringName a = "a";
    const StringName position = "position";
    const StringName size = "size";
    const StringName end = "end";
    const StringName basis = "basis";
    const StringName origin = "origin";
    const StringName normal = "normal";
    const StringName d = "d";
    const StringName h = "h";
    const StringName s = "s";
    const StringName v = "v";
    const StringName r8 = "r8";
    const StringName g8 = "g8";
    const StringName b8 = "b8";
    const StringName a8 = "a8";

    const StringName call = "call";
    const StringName call_deferred = "call_deferred";
    const StringName bind = "bind";
    const StringName notification = "notification";
    const StringName property_list_changed = "property_list_changed";
};

#define CoreStringName(m_name) CoreStringNames::get_singleton()->m_name

#endif // ORCHESTRATOR_CORE_GODOT_CORE_STRING_NAMES_H