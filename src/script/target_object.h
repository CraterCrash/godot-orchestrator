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
#ifndef ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H
#define ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

class OScriptTargetObject : public RefCounted {
    GDCLASS(OScriptTargetObject, RefCounted);

    Variant _reference;
    bool _owned = false;

    OScriptTargetObject() = default;

protected:
    static void _bind_methods() {}

public:

    _FORCE_INLINE_ bool has_target() const { return _reference.get_type() != Variant::NIL; }

    Variant get_target() const;
    StringName get_target_class() const;

    TypedArray<Dictionary> get_target_property_list() const;
    TypedArray<Dictionary> get_target_method_list() const;
    TypedArray<Dictionary> get_target_signal_list() const;

    explicit OScriptTargetObject(const Variant& p_reference, bool p_owned);
    ~OScriptTargetObject() override;
};

#endif  // ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H