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
#ifndef ORCHESTRATOR_SCRIPT_RPC_CALLABLE_H
#define ORCHESTRATOR_SCRIPT_RPC_CALLABLE_H

#include <godot_cpp/variant/callable_custom.hpp>

using namespace godot;

/// Forward declarations
namespace godot {
    class Node;
}

class OScriptRPCCallable : public CallableCustom {
    Object* _object = nullptr;
    Node* _node = nullptr;
    StringName _method;
    uint32_t _h = 0;

    static bool _compare_equal(const CallableCustom* p_a, const CallableCustom* p_b);
    static bool _compare_less(const CallableCustom* p_a, const CallableCustom* p_b);

public:
    //~ Begin CallableCustom Interface
    uint32_t hash() const override;
    String get_as_text() const override;
    CompareEqualFunc get_compare_equal_func() const override;
    CompareLessFunc get_compare_less_func() const override;
    ObjectID get_object() const override;
    int get_argument_count(bool& r_is_valid) const override;
    void call(const Variant** p_arguments, int p_arg_count, Variant& r_return_value, GDExtensionCallError& r_call_error) const override;
    //~ End CallableCustom Interface

    StringName get_method() const;
    Error rpc(int p_peer_id, const Variant** p_arguments, int p_arg_count, GDExtensionCallError& r_call_error) const;

    OScriptRPCCallable(Object* p_object, const StringName& p_method);
    ~OScriptRPCCallable() override = default;
};

#endif // ORCHESTRATOR_SCRIPT_RPC_CALLABLE_H