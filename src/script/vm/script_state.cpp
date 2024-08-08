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
#include "script/vm/script_state.h"

#include "script/instances/node_instance.h"

void OScriptState::_signal_callback(const Variant** p_args, GDExtensionInt p_argcount, GDExtensionCallError& r_err)
{
    // If no function is set, we cannot resume execution.
    ERR_FAIL_COND(!is_valid());

    r_err.error = GDEXTENSION_CALL_OK;

    // NOTE:
    // This callback should always be called with a minimum of 1 argument, which is a reference to
    // the state object. If there isn't at least 1 argument, resume should fail.
    if (p_argcount <=  0)
    {
        r_err.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
        r_err.argument = 1;
        return;
    }

    const Ref<OScriptState> state = *p_args[p_argcount - 1];
    if (state.is_null())
    {
        r_err.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        r_err.argument = p_argcount - 1;
        r_err.expected = Variant::OBJECT;
        return;
    }

    r_err.error = GDEXTENSION_CALL_OK;
    _call_method(r_err);
}

Variant OScriptState::_call_method(GDExtensionCallError& r_error)
{
    void* stack = const_cast<unsigned char*>(_stack.ptr());
    OScriptExecutionContext context(_stack_info, stack, _flow_stack_pos, _pass);
    context._script_instance = _script_instance;

    Variant result;
    _instance->_call_method_internal(_function, &context, true, _node, _func_ptr, result, r_error);

    _function = StringName();

    return result;
}

void OScriptState::connect_to_signal(Object* p_object, const String& p_signal, const Array& p_bindings)
{
    // Cannot bind if the provided object instance is null
    ERR_FAIL_NULL(p_object);
    p_object->connect(p_signal, Callable(this, "_signal_callback").bind(Ref<OScriptState>(this)), CONNECT_ONE_SHOT);
}

bool OScriptState::is_valid() const
{
    return _function != StringName();
}

Variant OScriptState::resume(const Array& p_args)
{
    ERR_FAIL_COND_V(!is_valid(), Variant());

    GDExtensionCallError err;
    err.error = GDEXTENSION_CALL_OK;

    return _call_method(err);
}

void OScriptState::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("connect_to_signal", "object", "signals", "args"), &OScriptState::connect_to_signal);
    ClassDB::bind_method(D_METHOD("resume", "args"), &OScriptState::resume, DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("is_valid"), &OScriptState::is_valid);

    MethodInfo mi;
    mi.name = "_signal_callback";
    mi.arguments.push_back(PropertyInfo(Variant::OBJECT, "data"));
    ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "_signal_callback", &OScriptState::_signal_callback, mi); //, std::vector<Variant>(), true);
}

OScriptState::~OScriptState()
{
    if (_function != StringName())
    {
        Variant* ptr = reinterpret_cast<Variant*>(const_cast<unsigned char*>(_stack.ptr()));
        OScriptExecutionContext::_cleanup_stack(_stack_info, ptr);
    }
}
