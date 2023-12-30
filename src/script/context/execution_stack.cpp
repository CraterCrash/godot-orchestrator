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
#include "execution_stack.h"

#include "common/logger.h"

#include <format>

#include <godot_cpp/variant/utility_functions.hpp>

OScriptExecutionStack::OScriptExecutionStack(const OScriptExecutionStackInfo& p_stack_info)
{
    _info = p_stack_info;

    int max_flow = p_stack_info.flow_size;

    // Calculate the stack size
    _stack_size += p_stack_info.max_stack_size * sizeof(Variant);
    _stack_size += p_stack_info.node_count * sizeof(bool);
    _stack_size += (p_stack_info.max_inputs + p_stack_info.max_outputs) * sizeof(Variant*);
    _stack_size += p_stack_info.flow_size * sizeof(int);
    _stack_size += p_stack_info.pass_size * sizeof(int);

    // Allocate the memory block
    _stack = malloc(_stack_size);
    memset(_stack, 0, _stack_size);

    // Reserve pointers into the stack
    _variant_stack = (Variant*)_stack;
    _execution_bits = (bool*)(_variant_stack + p_stack_info.max_stack_size);
    _inputs = (Variant**)(_execution_bits + p_stack_info.node_count);
    _outputs = (Variant**)(_inputs + p_stack_info.max_inputs);
    _flow = max_flow ? (int*)(_outputs + p_stack_info.max_outputs) : (int*)nullptr;
    _pass = _flow ? (int*)(_flow + max_flow) : (int*)nullptr;

    // Initialize
    _initialize(p_stack_info);
}

OScriptExecutionStack::~OScriptExecutionStack()
{
    // Deallocate the memory block
    ::free(_stack);
}

void OScriptExecutionStack::cleanup_variant_stack()
{
    Variant* stack = _variant_stack;
    for (int i = 0; i < _info.max_stack_size; i++)
        stack[i].~Variant();
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

void OScriptExecutionStack::_initialize(const OScriptExecutionStackInfo& p_stack_info)
{
    // Everything starts false
    for (int i = 0; i < p_stack_info.node_count; i++)
        _execution_bits[i] = false;

    // Allocate variants
    for (int i = 0; i < p_stack_info.max_stack_size; i++)
        memnew_placement(&_variant_stack[i], Variant);
}

void OScriptExecutionStack::dump()
{
    Logger::debug("STACK DETAILS");
    Logger::debug("====================================================");
    Logger::debug(std::format("      Base : {} (max count {})", (void*)_variant_stack, _info.max_stack_size).c_str());
    Logger::debug(std::format(" Exec Bits : {} (max count {})", (void*)_execution_bits, _info.node_count).c_str());
    Logger::debug(std::format("    Inputs : {} (max count {})", (void*)_inputs, _info.max_inputs).c_str());
    Logger::debug(std::format("   Outputs : {} (max count {})", (void*)_outputs, _info.max_outputs).c_str());
    Logger::debug(std::format("      Flow : {} (max count {})", (void*)_flow, _info.flow_size).c_str());
    Logger::debug(std::format("      Pass : {} (max count {})", (void*)_pass, _info.pass_size).c_str());
    Logger::debug(std::format("     Total : {} bytes", _stack_size).c_str());
}

void OScriptExecutionStack::dump_variant_stack()
{
    const int max = Math::min(_info.max_stack_size, _info.max_inputs + _info.max_outputs);

    Logger::debug("STACK:");
    for (int i = 0; i < max; i++)
    {
        const Variant& val = _variant_stack[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_variant_stack[i], i);
        Logger::debug(vformat(text.c_str(), val));
    }
}

void OScriptExecutionStack::dump_input_stack()
{
    Logger::debug("Input stack max size: ", _info.max_inputs);
    for (int i = 0; i < _info.max_inputs; i++)
    {
        Variant* val = _inputs[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_inputs[i], i);
        Logger::debug(vformat(text.c_str(), (val ? *val : "<null>")));
    }
}

void OScriptExecutionStack::dump_output_stack()
{
    Logger::debug("Output stack max size: ", _info.max_outputs);
    for (int i = 0; i < _info.max_outputs; i++)
    {
        Variant* val = _outputs[i];
        std::string text = std::format("{}: [{}]: %s", (void*)&_outputs[i], i);
        Logger::debug(vformat(text.c_str(), (val ? *val : "<null>")));
    }
}