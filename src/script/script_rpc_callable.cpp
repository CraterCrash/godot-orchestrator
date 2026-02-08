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
#include "script/script_rpc_callable.h"

#include "common/resource_utils.h"
#include "core/godot/variant/array.h"

#include <godot_cpp/classes/multiplayer_api.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>

bool OScriptRPCCallable::_compare_equal(const CallableCustom* p_a, const CallableCustom* p_b) {
    return p_a->hash() == p_b->hash();
}

bool OScriptRPCCallable::_compare_less(const CallableCustom* p_a, const CallableCustom* p_b) {
    return p_a->hash() < p_b->hash();
}

uint32_t OScriptRPCCallable::hash() const {
    return _h;
}

String OScriptRPCCallable::get_as_text() const {
    ERR_FAIL_NULL_V(_object, "");

    String class_name = _object->get_class();
    Ref<Script> script = _object->get_script();
    if (script.is_valid()) {
        if (!script->get_global_name().is_empty()) {
            class_name += "(" + script->get_global_name() + ")";
        } else if (ResourceUtils::is_file(script->get_path())) {
            class_name += "(" + script->get_path().get_file() + ")";
        }
    }
    return class_name + "::" + String(_method) + " (rpc)";
}

CallableCustom::CompareEqualFunc OScriptRPCCallable::get_compare_equal_func() const {
    return _compare_equal;
}

CallableCustom::CompareLessFunc OScriptRPCCallable::get_compare_less_func() const {
    return _compare_less;
}

ObjectID OScriptRPCCallable::get_object() const {
    ERR_FAIL_NULL_V(_object, {});
    return ObjectID(_object->get_instance_id());
}

int OScriptRPCCallable::get_argument_count(bool& r_is_valid) const {
    ERR_FAIL_NULL_V(_object, 0);
    if (!_object->has_method(_method)) {
        r_is_valid = false;
        return 0;
    }
    return _object->get_method_argument_count(_method);
}

void OScriptRPCCallable::call(const Variant** p_arguments, int p_arg_count, Variant& r_return_value, GDExtensionCallError& r_call_error) const {
    Variant obj = _object;
    obj.callp(_method, p_arguments, p_arg_count, r_return_value, r_call_error);
}

StringName OScriptRPCCallable::get_method() const {
    return _method;
}

Error OScriptRPCCallable::rpc(int p_peer_id, const Variant** p_arguments, int p_argcount, GDExtensionCallError& r_call_error) const {
    if (unlikely(!_node)) {
        r_call_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        return ERR_UNCONFIGURED;
    }
    r_call_error.error = GDEXTENSION_CALL_OK;

    // Since Node::rpcp is not exposed, we need to go directly to the MultiplayerAPI
    Ref<MultiplayerAPI> api = _node->get_multiplayer();
    if (api.is_null()) {
        return ERR_UNCONFIGURED;
    }

    return api->rpc(p_peer_id, _node, _method, GDE::Array::from_variant_ptrs(p_arguments, p_argcount));
}

OScriptRPCCallable::OScriptRPCCallable(Object *p_object, const StringName &p_method) {
    ERR_FAIL_NULL(p_object);
    _object = p_object;
    _method = p_method;
    _h = _method.hash();
    _h = hash_murmur3_one_64(_object->get_instance_id(), _h);
    _node = Object::cast_to<Node>(_object);
    ERR_FAIL_NULL_MSG(_node, "RPC can only be defined on class that extends Node.");
}


