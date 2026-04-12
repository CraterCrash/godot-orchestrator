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
#include "orchestration/target_object.h"

TypedArray<Dictionary> OScriptTargetObject::_get_object_list(ListMethod p_method) const {
    Object* obj = _get_object();
    return obj ? (obj->*p_method)() : TypedArray<Dictionary>();
}

bool OScriptTargetObject::has_target() const {
    if (_reference.get_type() == Variant::OBJECT) {
        return _get_object() != nullptr;
    }
    return _reference.get_type() != Variant::NIL;
}

Variant OScriptTargetObject::get_target() const {
    return _reference;
}

StringName OScriptTargetObject::get_target_class() const {
    if (!has_target()) {
        return {};
    }

    if (_reference.get_type() == Variant::OBJECT) {
        return _get_object()->get_class();
    }

    return Variant::get_type_name(_reference.get_type());
}

TypedArray<Dictionary> OScriptTargetObject::get_target_property_list() const {
    return _get_object_list(&Object::get_property_list);
}

TypedArray<Dictionary> OScriptTargetObject::get_target_method_list() const {
    return _get_object_list(&Object::get_method_list);
}

TypedArray<Dictionary> OScriptTargetObject::get_target_signal_list() const {
    return _get_object_list(&Object::get_signal_list);
}

OScriptTargetObject::OScriptTargetObject(const Variant& p_reference, bool p_owned) {
    _reference = p_reference;
    _owned = p_owned;
}

OScriptTargetObject::~OScriptTargetObject() {
    if (_owned && _reference.get_type() == Variant::OBJECT) {
        Object* obj = _get_object();
        if (obj && !cast_to<RefCounted>(obj)) {
            memdelete(obj);
        }
    }
}
