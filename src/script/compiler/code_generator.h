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
#ifndef ORCHESTRATOR_SCRIPT_CODE_GENERATOR_H
#define ORCHESTRATOR_SCRIPT_CODE_GENERATOR_H

#include "script/compiler/compiled_function.h"

class OScriptCodeGenerator {
public:
    struct Address {
        enum AddressMode {
            SELF,
            CLASS,
            MEMBER,
            CONSTANT,
            LOCAL_VARIABLE,
            FUNCTION_PARAMETER,
            TEMPORARY,
            NIL
        };

        AddressMode mode = NIL;
        uint32_t address = 0;
        OScriptDataType type;

        Address() = default;
        explicit Address(AddressMode p_mode, const OScriptDataType& p_type = OScriptDataType()) { mode = p_mode; type = p_type; }
        Address(AddressMode p_mode, uint32_t p_address, const OScriptDataType& p_type = OScriptDataType()) { mode = p_mode; address = p_address; type = p_type; }
    };

    virtual uint32_t add_parameter(const StringName& p_name, bool p_is_optional, const OScriptDataType& p_type) = 0;
    virtual uint32_t add_local(const StringName& p_name, const OScriptDataType& p_type) = 0;
    virtual uint32_t add_local_constant(const StringName& p_name, const Variant& p_value) = 0;
    virtual uint32_t add_or_get_constant(const Variant& p_value) = 0;
    virtual uint32_t add_or_get_name(const StringName& p_name) = 0;
    virtual uint32_t add_temporary(const OScriptDataType& p_type) = 0;
    virtual void pop_temporary() = 0;
    virtual void clear_temporaries() = 0;
    virtual void clear_address(const Address& p_address) = 0;
    virtual bool is_local_dirty(const Address& p_address) = 0;

    virtual void start_parameters() = 0;
    virtual void end_parameters() = 0;

    virtual void start_block() = 0;
    virtual void end_block() = 0;

    virtual void write_start(OScript* p_script, const StringName& p_name, bool p_static, Variant p_rpc_config, const OScriptDataType& p_type) = 0;
    virtual OScriptCompiledFunction* write_end() = 0;

    #ifdef DEBUG_ENABLED
    virtual void set_signature(const String &p_signature) = 0;
    #endif

    virtual void set_initial_node_id(int p_node_id) = 0;

    virtual void write_type_adjust(const Address& p_target, Variant::Type p_new_type) = 0;
    virtual void write_unary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_operand) = 0;
    virtual void write_binary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_left, const Address& p_right) = 0;
    virtual void write_type_test(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) = 0;
    virtual void write_and_left_operand(const Address& p_left_operand) = 0;
    virtual void write_and_right_operand(const Address& p_right_operand) = 0;
    virtual void write_end_and(const Address& p_target) = 0;
    virtual void write_or_left_operand(const Address& p_left_operand) = 0;
    virtual void write_or_right_operand(const Address& p_right_operand) = 0;
    virtual void write_end_or(const Address& p_target) = 0;
    virtual void write_start_ternary(const Address& p_target) = 0;
    virtual void write_ternary_condition(const Address& p_condition) = 0;
    virtual void write_ternary_true_expr(const Address& p_expr) = 0;
    virtual void write_ternary_false_expr(const Address& p_expr) = 0;
    virtual void write_end_ternary() = 0;
    virtual void write_set(const Address& p_target, const Address& p_index, const Address& p_source) = 0;
    virtual void write_get(const Address& p_target, const Address& p_index, const Address& p_source) = 0;
    virtual void write_set_named(const Address& p_target, const StringName& p_name, const Address& p_source) = 0;
    virtual void write_get_named(const Address& p_target, const StringName& p_name, const Address& p_source) = 0;
    virtual void write_set_member(const Address& p_value, const StringName& p_name) = 0;
    virtual void write_get_member(const Address& p_target, const StringName& p_name) = 0;
    virtual void write_set_static_variable(const Address& p_value, const Address& p_class, int p_index) = 0;
    virtual void write_get_static_variable(const Address& p_target, const Address& p_class, int p_index) = 0;
    virtual void write_assign(const Address& p_target, const Address& p_source) = 0;
    virtual void write_assign_with_conversion(const Address& p_target, const Address& p_source) = 0;
    virtual void write_assign_null(const Address& p_target) = 0;
    virtual void write_assign_true(const Address& p_target) = 0;
    virtual void write_assign_false(const Address& p_target) = 0;
    virtual void write_assign_default_parameter(const Address& p_dst, const Address& p_src, bool p_use_conversion) = 0;
    virtual void write_store_global(const Address& p_dest, int p_global_index) = 0;
    virtual void write_store_named_global(const Address& p_dest, const StringName& p_global) = 0;
    virtual void write_cast(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) = 0;
    virtual void write_call(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_super_call(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_async(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_oscript_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, bool p_is_static, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_builtin_type_static(const Address& p_target, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_native_static(const Address& p_target, const StringName& p_class, const StringName& p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_native_static_validated(const Address& p_target, MethodBind* p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_method_bind(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_method_bind_validated(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_self(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_self_async(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_call_script_function(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) = 0;
    virtual void write_lambda(const Address& p_target, OScriptCompiledFunction* p_function, const Vector<Address>& p_captures, bool p_use_self) = 0;
    virtual void write_construct(const Address& p_target, Variant::Type p_type, const Vector<Address>& p_arguments) = 0;
    virtual void write_construct_array(const Address& p_target, const Vector<Address>& p_arguments) = 0;
    virtual void write_construct_typed_array(const Address& p_target, const OScriptDataType& p_element_type, const Vector<Address>& p_arguments) = 0;
    virtual void write_construct_dictionary(const Address& p_target, const Vector<Address>& p_arguments) = 0;
    virtual void write_construct_typed_dictionary(const Address& p_target, const OScriptDataType& p_key_type, const OScriptDataType& p_value_type, const Vector<Address>& p_arguments) = 0;
    virtual void write_await(const Address& p_target, const Address& p_operand) = 0;
    virtual void write_if(const Address& p_condition) = 0;
    virtual void write_else() = 0;
    virtual void write_endif() = 0;
    virtual void write_jump_if_shared(const Address& p_value) = 0;
    virtual void write_end_jump_if_shared() = 0;
    virtual void start_for(const OScriptDataType& p_iterator_type, const OScriptDataType& p_list_type, bool p_is_range) = 0;
    virtual void write_for_list_assignment(const Address& p_list) = 0;
    virtual void write_for_range_assignment(const Address& p_from, const Address& p_to, const Address& p_step) = 0;
    virtual void write_for(const Address& p_variable, bool p_use_conversion, bool p_is_range) = 0;
    virtual void write_endfor(bool p_is_range) = 0;
    virtual void start_while_condition() = 0;
    virtual void write_while(const Address& p_condition) = 0;
    virtual void write_endwhile() = 0;
    virtual void write_break() = 0;
    virtual void write_continue() = 0;
    virtual void write_breakpoint() = 0;
    virtual void write_newline(int p_node) = 0;
    virtual void write_return(const Address& p_value) = 0;
    virtual void write_assert(const Address& p_test, const Address& p_message) = 0;

    virtual ~OScriptCodeGenerator() = default;
};

#endif // ORCHESTRATOR_SCRIPT_CODE_GENERATOR_H