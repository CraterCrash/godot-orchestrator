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
#include "script/target_object.h"

Variant OScriptTargetObject::get_target() const {
    return _reference;
}

StringName OScriptTargetObject::get_target_class() const {
    switch (_reference.get_type()) {
        case Variant::OBJECT:
            return cast_to<Object>(_reference)->get_class();
        default:
            return Variant::get_type_name(_reference.get_type());
    }
}

TypedArray<Dictionary> OScriptTargetObject::get_target_property_list() const {
    if (_reference.get_type() == Variant::OBJECT) {
        return cast_to<Object>(_reference)->get_property_list();
    }
    return {};
}

TypedArray<Dictionary> OScriptTargetObject::get_target_method_list() const {
    if (_reference.get_type() == Variant::OBJECT) {
        return cast_to<Object>(_reference)->get_method_list();
    }
    return {};
}

TypedArray<Dictionary> OScriptTargetObject::get_target_signal_list() const {
    if (_reference.get_type() == Variant::OBJECT) {
        return cast_to<Object>(_reference)->get_signal_list();
    }
    return {};
}

OScriptTargetObject::OScriptTargetObject(const Variant& p_reference, bool p_owned) {
    _reference = p_reference;
    _owned = p_owned;
}

OScriptTargetObject::~OScriptTargetObject() {
    if (_owned && _reference.get_type() == Variant::OBJECT) {
        RefCounted* referenced = cast_to<RefCounted>(_reference);
        if (!referenced) {
            memdelete(cast_to<Object>(_reference));
        }
    }
}
