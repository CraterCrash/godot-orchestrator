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

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "core/godot/core_string_names.h"
#include "core/godot/gdextension_compat.h"
#include "script/compiler/compiled_function.h"

MethodBind* OScriptNativeClass::_resolve_static_method_bind(const StringName& p_method) {
    MethodBind** binding = _static_bindings.getptr(p_method);
    if (binding) {
        return *binding;
    }

    MethodInfo mi;
    MethodBind* bind = ExtensionDB::get_method(_name, p_method, &mi);
    if (bind && mi.flags & METHOD_FLAG_STATIC) {
        _static_bindings[p_method] = bind;
        return bind;
    }

    return nullptr;
}

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
    Variant value = instantiate();
    Object* object = cast_to<Object>(value);
    ERR_FAIL_NULL_V_MSG(object, Variant(), "Class type: " + String(_name) + " is not instantiable.");

    RefCounted* rc = cast_to<RefCounted>(object);
    if (rc) {
        return Ref<RefCounted>(rc);
    }
    return object;
}

Variant OScriptNativeClass::instantiate() {
    return ClassDB::instantiate(_name);
}

Variant OScriptNativeClass::callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
    if (p_method != CoreStringName(new_)) {
        const MethodBind* bind = _resolve_static_method_bind(p_method);
        if (bind) {
            Variant ret;
            GDE_INTERFACE(object_method_bind_call)(
                    bind,
                    nullptr,
                    reinterpret_cast<GDExtensionConstVariantPtr*>(p_args),
                    p_arg_count,
                    &ret,
                    &r_error);

            return ret;
        }
    }

    r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    return {};
}

void OScriptNativeClass::_bind_methods() {
    ClassDB::bind_method(D_METHOD("new"), &OScriptNativeClass::_new);
}

OScriptNativeClass::OScriptNativeClass(const StringName& p_name) {
    _name = p_name;
}