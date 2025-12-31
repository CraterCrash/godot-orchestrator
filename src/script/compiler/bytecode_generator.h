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
#ifndef ORCHESTRATOR_SCRIPT_BYTECODE_GENERATOR_H
#define ORCHESTRATOR_SCRIPT_BYTECODE_GENERATOR_H

#include "script/compiler/code_generator.h"
#include "script/utility_functions.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_map.hpp>

// Aligned with e304b4e43e5d2f5027ab0c475b3f2530e81db207
class OScriptBytecodeGenerator : public OScriptCodeGenerator {

    struct StackSlot {
        Variant::Type type = Variant::NIL;
        bool can_contain_object = true;
        Vector<int> bytecode_indices;

        StackSlot() = default;
        StackSlot(Variant::Type p_type, bool p_can_contain_object) : type(p_type), can_contain_object(p_can_contain_object) {}
    };

    struct CallTarget {
        Address target;
        bool is_new_temporary = false;
        OScriptBytecodeGenerator* generator = nullptr;
        #ifdef DEV_ENABLED
        bool cleaned = false;
        #endif

        void cleanup() {
            #ifdef DEV_ENABLED
            DEV_ASSERT(!cleaned);
            #endif
            if (is_new_temporary) {
                generator->pop_temporary();
            }
            #ifdef DEV_ENABLED
            cleaned = true;
            #endif
        }

        CallTarget(const CallTarget&) = delete;
        CallTarget& operator=(CallTarget&) = delete;

        CallTarget(Address p_target, bool p_new_temp, OScriptBytecodeGenerator* p_generator)
            : target(p_target), is_new_temporary(p_new_temp), generator(p_generator) {}

        ~CallTarget() {
            #ifdef DEV_ENABLED
            DEV_ASSERT(cleaned);
            #endif
        }
    };

    bool ended = false;
    OScriptCompiledFunction* function = nullptr;

    Vector<int> opcodes;
    List<RBMap<StringName, int>> stack_id_stack;
    RBMap<StringName, int> stack_identifiers;
    List<int> stack_identifiers_counts;
    RBMap<StringName, int> local_constants;

    Vector<StackSlot> locals;
    HashSet<int> dirty_locals;

    Vector<StackSlot> temporaries;
    List<int> used_temporaries;
    HashSet<int> temporaries_pending_clear;
    RBMap<Variant::Type, List<int>> temporaries_pool;

    List<OScriptCompiledFunction::StackDebug> stack_debug;
    List<RBMap<StringName, int>> block_identifier_stack;
    RBMap<StringName, int> block_identifiers;

    int max_locals = 0;
    int current_script_node_id = -1;
    int instr_args_max = 0;

    #ifdef DEBUG_ENABLED
    List<int> temp_stack;
    #endif

    HashMap<Variant, int, HashableHasher<Variant>> constant_map;
    RBMap<StringName, int> name_map;
    #ifdef TOOLS_ENABLED
    Vector<StringName> named_globals;
    #endif

    RBMap<GDExtensionPtrOperatorEvaluator, int> operator_func_map;
    RBMap<GDExtensionPtrSetter, int> setters_map;
    RBMap<GDExtensionPtrGetter, int> getters_map;
    RBMap<GDExtensionPtrKeyedSetter, int> keyed_setters_map;
    RBMap<GDExtensionPtrKeyedGetter, int> keyed_getters_map;
    RBMap<GDExtensionPtrIndexedSetter, int> indexed_setters_map;
    RBMap<GDExtensionPtrIndexedGetter, int> indexed_getters_map;
    RBMap<GDExtensionPtrUtilityFunction, int> utility_functions_map;
    RBMap<OScriptUtilityFunctions::FunctionPtr, int> os_functions_map;
    RBMap<GDExtensionPtrConstructor, int> constructors_map;
    RBMap<GDExtensionPtrBuiltInMethod, int> built_in_methods_map;
    RBMap<MethodBind*, int> method_bind_map;
    RBMap<OScriptCompiledFunction*, int> lambdas_map;

    #ifdef DEBUG_ENABLED
    // Keep method and property names for pointer and validated operations.
    // Used when disassembling the bytecode.
    Vector<String> operator_names;
    Vector<String> setter_names;
    Vector<String> getter_names;
    Vector<String> builtin_methods_names;
    Vector<String> constructors_names;
    Vector<String> utilities_names;
    Vector<String> os_utilities_names;

    static void add_debug_name(Vector<String> &vector, int index, const String &name) {
        if (index >= vector.size()) {
            vector.resize(index + 1);
        }
        vector.write[index] = name;
    }
    #endif

    List<int> if_jmp_addrs;
    List<int> for_jmp_addrs;
    List<Address> for_counter_variables;
    List<Address> for_container_variables;
    List<Address> for_range_from_variables;
    List<Address> for_range_to_variables;
    List<Address> for_range_step_variables;
    List<int> while_jmp_addrs;
    List<int> continue_addrs;

    // Used to patch jumps with 'and' / 'or' operators with short-circuit.
    List<int> logic_op_jump_pos1;
    List<int> logic_op_jump_pos2;

    List<Address> ternary_result;
    List<int> ternary_jump_fail_pos;
    List<int> ternary_jump_skip_pos;

    List<List<int>> current_breaks_to_patch;

    void add_stack_identifier(const StringName& p_id, int p_pos);
    void push_stack_identifiers();
    void pop_stack_identifiers();

    int get_name_map_pos(const StringName& p_identifier);
    int get_constant_pos(const Variant& p_value);
    int get_operation_pos(GDExtensionPtrOperatorEvaluator p_operation);
    int get_setter_pos(GDExtensionPtrSetter p_setter);
    int get_getter_pos(GDExtensionPtrGetter p_getter);
    int get_indexed_setter_pos(GDExtensionPtrIndexedSetter p_setter);
    int get_indexed_getter_pos(GDExtensionPtrIndexedGetter p_getter);
    int get_keyed_setter_pos(GDExtensionPtrKeyedSetter p_setter);
    int get_keyed_getter_pos(GDExtensionPtrKeyedGetter p_getter);
    int get_utility_pos(GDExtensionPtrUtilityFunction p_function);
    int get_os_utility_pos(OScriptUtilityFunctions::FunctionPtr p_function);
    int get_constructor_pos(GDExtensionPtrConstructor p_constructor);
    int get_builtin_method_pos(GDExtensionPtrBuiltInMethod p_method);
    int get_method_bind_pos(MethodBind* p_method);
    int get_lambda_function_pos(OScriptCompiledFunction* p_function);

    CallTarget get_call_target(const Address& p_target, Variant::Type p_type = Variant::NIL);

    int address_of(const Address& p_address);

    void append_opcode(OScriptCompiledFunction::Opcode p_code);
    void append_opcode_and_argcount(OScriptCompiledFunction::Opcode p_code, int p_arg_count);
    void append(int p_code);
    void append(const Address& p_address);
    void append(const StringName& p_name);
    void append_op_eval(GDExtensionPtrOperatorEvaluator p_operation);
    void append(GDExtensionPtrSetter p_setter);
    void append(GDExtensionPtrGetter p_getter);
    void append(GDExtensionPtrIndexedSetter p_setter);
    void append(GDExtensionPtrIndexedGetter p_getter);
    void append(GDExtensionPtrKeyedSetter p_setter);
    void append(GDExtensionPtrKeyedGetter p_getter);
    void append(GDExtensionPtrUtilityFunction p_function);
    void append(OScriptUtilityFunctions::FunctionPtr p_function);
    void append(GDExtensionPtrConstructor p_constructor);
    void append(GDExtensionPtrBuiltInMethod p_method);
    void append(MethodBind* p_method);
    void append(OScriptCompiledFunction* p_lambda);

    void patch_jump(int p_address);

public:
    //~ Begin OScriptCodeGenerator Interface
    uint32_t add_parameter(const StringName& p_name, bool p_is_optional, const OScriptDataType& p_type) override;
    uint32_t add_local(const StringName& p_name, const OScriptDataType& p_type) override;
    uint32_t add_local_constant(const StringName& p_name, const Variant& p_value) override;
    uint32_t add_or_get_constant(const Variant& p_value) override;
    uint32_t add_or_get_name(const StringName& p_name) override;
    uint32_t add_temporary(const OScriptDataType& p_type) override;
    void pop_temporary() override;
    void clear_temporaries() override;
    void clear_address(const Address& p_address) override;
    bool is_local_dirty(const Address& p_address) override;
    void start_parameters() override;
    void end_parameters() override;
    void start_block() override;
    void end_block() override;
    void write_start(OScript* p_script, const StringName& p_name, bool p_static, Variant p_rpc_config, const OScriptDataType& p_type) override;
    OScriptCompiledFunction* write_end() override;

    #ifdef DEBUG_ENABLED
    void set_signature(const String& p_signature) override;
    #endif
    void set_initial_node_id(int p_node_id) override;

    void write_type_adjust(const Address& p_target, Variant::Type p_new_type) override;
    void write_unary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_operand) override;
    void write_binary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_left, const Address& p_right) override;
    void write_type_test(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) override;
    void write_and_left_operand(const Address& p_left_operand) override;
    void write_and_right_operand(const Address& p_right_operand) override;
    void write_end_and(const Address& p_target) override;
    void write_or_left_operand(const Address& p_left_operand) override;
    void write_or_right_operand(const Address& p_right_operand) override;
    void write_end_or(const Address& p_target) override;
    void write_start_ternary(const Address& p_target) override;
    void write_ternary_condition(const Address& p_condition) override;
    void write_ternary_true_expr(const Address& p_expr) override;
    void write_ternary_false_expr(const Address& p_expr) override;
    void write_end_ternary() override;
    void write_set(const Address& p_target, const Address& p_index, const Address& p_source) override;
    void write_get(const Address& p_target, const Address& p_index, const Address& p_source) override;
    void write_set_named(const Address& p_target, const StringName& p_name, const Address& p_source) override;
    void write_get_named(const Address& p_target, const StringName& p_name, const Address& p_source) override;
    void write_set_member(const Address& p_value, const StringName& p_name) override;
    void write_get_member(const Address& p_target, const StringName& p_name) override;
    void write_set_static_variable(const Address& p_value, const Address& p_class, int p_index) override;
    void write_get_static_variable(const Address& p_target, const Address& p_class, int p_index) override;
    void write_assign(const Address& p_target, const Address& p_source) override;
    void write_assign_with_conversion(const Address& p_target, const Address& p_source) override;
    void write_assign_null(const Address& p_target) override;
    void write_assign_true(const Address& p_target) override;
    void write_assign_false(const Address& p_target) override;
    void write_assign_default_parameter(const Address& p_dst, const Address& p_src, bool p_use_conversion) override;
    void write_store_global(const Address& p_dest, int p_global_index) override;
    void write_store_named_global(const Address& p_dest, const StringName& p_global) override;
    void write_cast(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) override;
    void write_call(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_super_call(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_call_async(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_call_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) override;
    void write_call_oscript_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) override;
    void write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, bool p_is_static, const Vector<Address>& p_arguments) override;
    void write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) override;
    void write_call_builtin_type_static(const Address& p_target, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) override;
    void write_call_native_static(const Address& p_target, const StringName& p_class, const StringName& p_method, const Vector<Address>& p_arguments) override;
    void write_call_native_static_validated(const Address& p_target, MethodBind* p_method, const Vector<Address>& p_arguments) override;
    void write_call_method_bind(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) override;
    void write_call_method_bind_validated(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) override;
    void write_call_self(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_call_self_async(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_call_script_function(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) override;
    void write_lambda(const Address& p_target, OScriptCompiledFunction* p_function, const Vector<Address>& p_captures, bool p_use_self) override;
    void write_construct(const Address& p_target, Variant::Type p_type, const Vector<Address>& p_arguments) override;
    void write_construct_array(const Address& p_target, const Vector<Address>& p_arguments) override;
    void write_construct_typed_array(const Address& p_target, const OScriptDataType& p_element_type, const Vector<Address>& p_arguments) override;
    void write_construct_dictionary(const Address& p_target, const Vector<Address>& p_arguments) override;
    void write_construct_typed_dictionary(const Address& p_target, const OScriptDataType& p_key_type, const OScriptDataType& p_value_type, const Vector<Address>& p_arguments) override;
    void write_await(const Address& p_target, const Address& p_operand) override;
    void write_if(const Address& p_condition) override;
    void write_else() override;
    void write_endif() override;
    void write_jump_if_shared(const Address& p_value) override;
    void write_end_jump_if_shared() override;
    void start_for(const OScriptDataType& p_iterator_type, const OScriptDataType& p_list_type, bool p_is_range) override;
    void write_for_list_assignment(const Address& p_list) override;
    void write_for_range_assignment(const Address& p_from, const Address& p_to, const Address& p_step) override;
    void write_for(const Address& p_variable, bool p_use_conversion, bool p_is_range) override;
    void write_endfor(bool p_is_range) override;
    void start_while_condition() override;
    void write_while(const Address& p_condition) override;
    void write_endwhile() override;
    void write_break() override;
    void write_continue() override;
    void write_breakpoint() override;
    void write_newline(int p_node) override;
    void write_return(const Address& p_return_value) override;
    void write_assert(const Address& p_test, const Address& p_message) override;
    //~ End OScriptCodeGenerator Interface

    ~OScriptBytecodeGenerator() override;
};

#endif // ORCHESTRATOR_SCRIPT_BYTECODE_GENERATOR_H