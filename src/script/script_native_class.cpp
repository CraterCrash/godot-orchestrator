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
#include "script/script_native_class.h"

#include "common/dictionary_utils.h"

bool OScriptNativeClass::_get(const StringName& p_name, Variant& r_value) const {
    if (ClassDB::class_has_integer_constant(_name, p_name)) {
        r_value = ClassDB::class_get_integer_constant(_name, p_name);
        return true;
    }

    const TypedArray<Dictionary> methods = ClassDB::class_get_method_list(_name);
    for (uint32_t i = 0; i < methods.size(); i++) {
        const Dictionary& method = methods[i];
        if (method.get("name", "") == p_name) {
            const MethodInfo info = DictionaryUtils::to_method(method);
            if (info.flags & METHOD_FLAG_STATIC) {
                Object* object = const_cast<Object*>(cast_to<const Object>(this));
                r_value = Callable(object, p_name);
                return true;
            }
        }
    }

    return false;
}

Variant OScriptNativeClass::_new() {
    Object* object = instantiate();
    ERR_FAIL_NULL_V_MSG(object, Variant(), "Class type: " + String(_name) + " is not instantiable.");

    RefCounted* rc = cast_to<RefCounted>(object);
    if (rc) {
        return Ref<RefCounted>(rc);
    }
    return object;
}

Object* OScriptNativeClass::instantiate() {
    return ClassDB::instantiate(_name);
}

Variant OScriptNativeClass::callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
    r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    return {};
}

void OScriptNativeClass::_bind_methods() {
    ClassDB::bind_method(D_METHOD("new"), &OScriptNativeClass::_new);
}

OScriptNativeClass::OScriptNativeClass(const StringName& p_name) {
    _name = p_name;
}