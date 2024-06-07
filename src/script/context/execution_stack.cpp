// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "script/context/execution_stack.h"

#include "common/logger.h"

#include <cassert>
#include <version>
#ifdef __cpp_lib_format
#include <format>
#endif

#include <godot_cpp/variant/utility_functions.hpp>

int OScriptExecutionStackInfo::get_stack_size() const
{
    int stack_size = 0;
    stack_size += max_stack_size * sizeof(Variant);
    stack_size += node_count * sizeof(bool);
    stack_size += (max_inputs + max_outputs) * sizeof(Variant*);
    stack_size += flow_size * sizeof(int);
    stack_size += pass_size * sizeof(int);

    return stack_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptExecutionStack::OScriptExecutionStack(const OScriptExecutionStackInfo& p_stack_info, void* p_stack, bool p_init, bool p_allocated)
    : _info(p_stack_info)
    , _stack(p_stack)
    , _allocated(p_allocated)
{
    assert(_stack != nullptr);

    if (p_allocated)
        memset(_stack, 0, _info.get_stack_size());

    // Reserve pointers into the stack
    _variant_stack = static_cast<Variant*>(_stack);
    _execution_bits = reinterpret_cast<bool*>(_variant_stack + _info.max_stack_size);
    _inputs = reinterpret_cast<Variant**>(_execution_bits + _info.node_count);
    _outputs = reinterpret_cast<Variant**>(_inputs + _info.max_inputs);

    const int max_flow = _info.flow_size;
    _flow = max_flow ? reinterpret_cast<int*>(_outputs + p_stack_info.max_outputs) : nullptr;
    _pass = _flow ? reinterpret_cast<int*>(_flow + max_flow) : nullptr;

    if (p_init)
    {
        for (int i = 0; i < _info.max_stack_size; i++)
            memnew_placement(&_variant_stack[i], Variant);
    }
}

OScriptExecutionStack::~OScriptExecutionStack()
{
    // Deallocate the memory block
    if (_allocated)
        ::free(_stack);
}

void OScriptExecutionStack::cleanup_variant_stack()
{
    cleanup_variant_stack(_info, _variant_stack);
}

void OScriptExecutionStack::cleanup_variant_stack(const OScriptExecutionStackInfo& p_info, Variant* p_stack)
{
    for (int i = 0; i < p_info.max_stack_size; i++)
        p_stack[i].~Variant();
}

void OScriptExecutionStack::push_node_onto_flow_stack(int p_node_id)
{
    if (_flow)
        _flow[0] = p_node_id;
}

void OScriptExecutionStack::push_arguments(const Variant* const* p_args, int p_count)
{
    for (int i = 0; i < p_count; i++)
        _variant_stack[i] = *p_args[i];
}

void OScriptExecutionStack::dump()
{
    #ifdef __cpp_lib_format
    Logger::debug("STACK DETAILS");
    Logger::debug("====================================================");
    Logger::debug(std::format("      Base : {} (max count {})", (void*)_variant_stack, _info.max_stack_size).c_str());
    Logger::debug(std::format(" Exec Bits : {} (max count {})", (void*)_execution_bits, _info.node_count).c_str());
    Logger::debug(std::format("    Inputs : {} (max count {})", (void*)_inputs, _info.max_inputs).c_str());
    Logger::debug(std::format("   Outputs : {} (max count {})", (void*)_outputs, _info.max_outputs).c_str());
    Logger::debug(std::format("      Flow : {} (max count {})", (void*)_flow, _info.flow_size).c_str());
    Logger::debug(std::format("      Pass : {} (max count {})", (void*)_pass, _info.pass_size).c_str());
    #endif
}

void OScriptExecutionStack::dump_variant_stack()
{
    #ifdef __cpp_lib_format
    const int max = Math::min(_info.max_stack_size, _info.max_inputs + _info.max_outputs);

    Logger::debug("STACK:");
    for (int i = 0; i < max; i++)
    {
        const Variant& val = _variant_stack[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_variant_stack[i], i);
        Logger::debug(vformat(text.c_str(), val));
    }
    #endif
}

void OScriptExecutionStack::dump_input_stack()
{
    #ifdef __cpp_lib_format
    Logger::debug("Input stack max size: ", _info.max_inputs);
    for (int i = 0; i < _info.max_inputs; i++)
    {
        Variant* val = _inputs[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_inputs[i], i);
        Logger::debug(vformat(text.c_str(), (val ? *val : "<null>")));
    }
    #endif
}

void OScriptExecutionStack::dump_output_stack()
{
    #ifdef __cpp_lib_format
    Logger::debug("Output stack max size: ", _info.max_outputs);
    for (int i = 0; i < _info.max_outputs; i++)
    {
        Variant* val = _outputs[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_outputs[i], i);
        Logger::debug(vformat(text.c_str(), (val ? *val : "<null>")));
    }
    #endif
}