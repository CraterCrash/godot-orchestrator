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
#ifndef ORCHESTRATOR_SCRIPT_NATIVE_CLASS_H
#define ORCHESTRATOR_SCRIPT_NATIVE_CLASS_H

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

class OScriptNativeClass : public RefCounted {
    GDCLASS(OScriptNativeClass, RefCounted);
    StringName _name;

protected:
    static void _bind_methods();
    bool _get(const StringName& p_name, Variant& r_value) const; // NOLINT

    OScriptNativeClass() = default;

public:
    _FORCE_INLINE_ const StringName& get_name() const { return _name; }
    Variant _new();
    Object* instantiate();
    virtual Variant callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error);

    String _to_string() const { return vformat("<OScriptNativeClass:%s:%s>", _name, get_instance_id()); }

    explicit OScriptNativeClass(const StringName& p_name);
};

#endif // ORCHESTRATOR_SCRIPT_NATIVE_CLASS_H
