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
#ifndef ORCHESTRATOR_SCRIPT_COMPILED_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_COMPILED_FUNCTION_H

#include "script/utility_functions.h"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Forward declaration
class OScript;
class OScriptCompiler;
class OScriptInstance;
class OScriptLanguage;

class OScriptDataType {
public:
    Vector<OScriptDataType> container_element_types;

    enum Kind {
        VARIANT,
        BUILTIN,
        NATIVE,
        SCRIPT,
        OSCRIPT
    };

    Kind kind = VARIANT;
    Variant::Type builtin_type = Variant::NIL;
    StringName native_type;
    Script* script_type = nullptr;
    Ref<Script> script_type_ref;

    _FORCE_INLINE_ bool has_type() const { return kind != VARIANT; }

    bool is_type(const Variant& p_variant, bool p_allow_implicit_conversion = false) const;
    bool can_contain_object() const;
    void set_container_element_type(int p_index, const OScriptDataType& p_element_type);

    OScriptDataType get_container_element_type(int p_index) const;
    OScriptDataType get_container_element_type_or_variant(int p_index) const;
    bool has_container_element_type(int p_index) const;
    bool has_container_element_types() const;

    bool operator==(const OScriptDataType& p_other) const;
    bool operator!=(const OScriptDataType& p_other) const;

    void operator=(const OScriptDataType& p_other) {
        kind = p_other.kind;
        builtin_type = p_other.builtin_type;
        native_type = p_other.native_type;
        script_type = p_other.script_type;
        script_type_ref = p_other.script_type_ref;
        container_element_types = p_other.container_element_types;
    }

    OScriptDataType() = default;
    OScriptDataType(const OScriptDataType& p_other) { *this = p_other; }

    ~OScriptDataType() = default;
};

class OScriptCompiledFunction {
    friend class OScript;
    friend class OScriptCompiler;
    friend class OScriptBytecodeGenerator;
    friend class OScriptLanguage;

public:
    enum Opcode {
        OPCODE_OPERATOR,
		OPCODE_OPERATOR_VALIDATED,
		OPCODE_TYPE_TEST_BUILTIN,
		OPCODE_TYPE_TEST_ARRAY,
		OPCODE_TYPE_TEST_DICTIONARY,
		OPCODE_TYPE_TEST_NATIVE,
		OPCODE_TYPE_TEST_SCRIPT,
		OPCODE_SET_KEYED,
		OPCODE_SET_KEYED_VALIDATED,
		OPCODE_SET_INDEXED_VALIDATED,
		OPCODE_GET_KEYED,
		OPCODE_GET_KEYED_VALIDATED,
		OPCODE_GET_INDEXED_VALIDATED,
		OPCODE_SET_NAMED,
		OPCODE_SET_NAMED_VALIDATED,
		OPCODE_GET_NAMED,
		OPCODE_GET_NAMED_VALIDATED,
		OPCODE_SET_MEMBER,
		OPCODE_GET_MEMBER,
		OPCODE_SET_STATIC_VARIABLE, // Only for OScript.
		OPCODE_GET_STATIC_VARIABLE, // Only for OScript.
		OPCODE_ASSIGN,
		OPCODE_ASSIGN_NULL,
		OPCODE_ASSIGN_TRUE,
		OPCODE_ASSIGN_FALSE,
		OPCODE_ASSIGN_TYPED_BUILTIN,
		OPCODE_ASSIGN_TYPED_ARRAY,
		OPCODE_ASSIGN_TYPED_DICTIONARY,
		OPCODE_ASSIGN_TYPED_NATIVE,
		OPCODE_ASSIGN_TYPED_SCRIPT,
		OPCODE_CAST_TO_BUILTIN,
		OPCODE_CAST_TO_NATIVE,
		OPCODE_CAST_TO_SCRIPT,
		OPCODE_CONSTRUCT, // Only for basic types!
		OPCODE_CONSTRUCT_VALIDATED, // Only for basic types!
		OPCODE_CONSTRUCT_ARRAY,
		OPCODE_CONSTRUCT_TYPED_ARRAY,
		OPCODE_CONSTRUCT_DICTIONARY,
		OPCODE_CONSTRUCT_TYPED_DICTIONARY,
		OPCODE_CALL,
		OPCODE_CALL_RETURN,
		OPCODE_CALL_ASYNC,
		OPCODE_CALL_UTILITY,
		OPCODE_CALL_UTILITY_VALIDATED,
		OPCODE_CALL_OSCRIPT_UTILITY,
		OPCODE_CALL_BUILTIN_TYPE_VALIDATED,
		OPCODE_CALL_SELF_BASE,
		OPCODE_CALL_METHOD_BIND,
		OPCODE_CALL_METHOD_BIND_RET,
		OPCODE_CALL_BUILTIN_STATIC,
		OPCODE_CALL_NATIVE_STATIC,
		OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN,
		OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN,
		OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN,
		OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN,
		OPCODE_AWAIT,
		OPCODE_AWAIT_RESUME,
		OPCODE_CREATE_LAMBDA,
		OPCODE_CREATE_SELF_LAMBDA,
		OPCODE_JUMP,
		OPCODE_JUMP_IF,
		OPCODE_JUMP_IF_NOT,
		OPCODE_JUMP_TO_DEF_ARGUMENT,
		OPCODE_JUMP_IF_SHARED,
		OPCODE_RETURN,
		OPCODE_RETURN_TYPED_BUILTIN,
		OPCODE_RETURN_TYPED_ARRAY,
		OPCODE_RETURN_TYPED_DICTIONARY,
		OPCODE_RETURN_TYPED_NATIVE,
		OPCODE_RETURN_TYPED_SCRIPT,
		OPCODE_ITERATE_BEGIN,
		OPCODE_ITERATE_BEGIN_INT,
		OPCODE_ITERATE_BEGIN_FLOAT,
		OPCODE_ITERATE_BEGIN_VECTOR2,
		OPCODE_ITERATE_BEGIN_VECTOR2I,
		OPCODE_ITERATE_BEGIN_VECTOR3,
		OPCODE_ITERATE_BEGIN_VECTOR3I,
		OPCODE_ITERATE_BEGIN_STRING,
		OPCODE_ITERATE_BEGIN_DICTIONARY,
		OPCODE_ITERATE_BEGIN_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY,
		OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY,
		OPCODE_ITERATE_BEGIN_OBJECT,
		OPCODE_ITERATE_BEGIN_RANGE,
		OPCODE_ITERATE,
		OPCODE_ITERATE_INT,
		OPCODE_ITERATE_FLOAT,
		OPCODE_ITERATE_VECTOR2,
		OPCODE_ITERATE_VECTOR2I,
		OPCODE_ITERATE_VECTOR3,
		OPCODE_ITERATE_VECTOR3I,
		OPCODE_ITERATE_STRING,
		OPCODE_ITERATE_DICTIONARY,
		OPCODE_ITERATE_ARRAY,
		OPCODE_ITERATE_PACKED_BYTE_ARRAY,
		OPCODE_ITERATE_PACKED_INT32_ARRAY,
		OPCODE_ITERATE_PACKED_INT64_ARRAY,
		OPCODE_ITERATE_PACKED_FLOAT32_ARRAY,
		OPCODE_ITERATE_PACKED_FLOAT64_ARRAY,
		OPCODE_ITERATE_PACKED_STRING_ARRAY,
		OPCODE_ITERATE_PACKED_VECTOR2_ARRAY,
		OPCODE_ITERATE_PACKED_VECTOR3_ARRAY,
		OPCODE_ITERATE_PACKED_COLOR_ARRAY,
		OPCODE_ITERATE_PACKED_VECTOR4_ARRAY,
		OPCODE_ITERATE_OBJECT,
		OPCODE_ITERATE_RANGE,
		OPCODE_STORE_GLOBAL,
		OPCODE_STORE_NAMED_GLOBAL,
		OPCODE_TYPE_ADJUST_BOOL,
		OPCODE_TYPE_ADJUST_INT,
		OPCODE_TYPE_ADJUST_FLOAT,
		OPCODE_TYPE_ADJUST_STRING,
		OPCODE_TYPE_ADJUST_VECTOR2,
		OPCODE_TYPE_ADJUST_VECTOR2I,
		OPCODE_TYPE_ADJUST_RECT2,
		OPCODE_TYPE_ADJUST_RECT2I,
		OPCODE_TYPE_ADJUST_VECTOR3,
		OPCODE_TYPE_ADJUST_VECTOR3I,
		OPCODE_TYPE_ADJUST_TRANSFORM2D,
		OPCODE_TYPE_ADJUST_VECTOR4,
		OPCODE_TYPE_ADJUST_VECTOR4I,
		OPCODE_TYPE_ADJUST_PLANE,
		OPCODE_TYPE_ADJUST_QUATERNION,
		OPCODE_TYPE_ADJUST_AABB,
		OPCODE_TYPE_ADJUST_BASIS,
		OPCODE_TYPE_ADJUST_TRANSFORM3D,
		OPCODE_TYPE_ADJUST_PROJECTION,
		OPCODE_TYPE_ADJUST_COLOR,
		OPCODE_TYPE_ADJUST_STRING_NAME,
		OPCODE_TYPE_ADJUST_NODE_PATH,
		OPCODE_TYPE_ADJUST_RID,
		OPCODE_TYPE_ADJUST_OBJECT,
		OPCODE_TYPE_ADJUST_CALLABLE,
		OPCODE_TYPE_ADJUST_SIGNAL,
		OPCODE_TYPE_ADJUST_DICTIONARY,
		OPCODE_TYPE_ADJUST_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY,
		OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY,
		OPCODE_ASSERT,
		OPCODE_BREAKPOINT,
		OPCODE_SCRIPT_NODE,
		OPCODE_END,
        OPCODE_OPERATOR_EVALUATE
    };

    enum Address {
        ADDR_BITS = 24,
        ADDR_MASK = ((1 << ADDR_BITS) - 1),
        ADDR_TYPE_MASK = ~ADDR_MASK,
        ADDR_TYPE_STACK = 0,
        ADDR_TYPE_CONSTANT = 1,
        ADDR_TYPE_MEMBER = 2,
        ADDR_TYPE_MAX = 3,
    };

    enum FixedAddress {
        ADDR_STACK_SELF = 0,
        ADDR_STACK_CLASS = 1,
        ADDR_STACK_NIL = 2,
        FIXED_ADDRESSES_MAX = 3,
        ADDR_SELF = ADDR_STACK_SELF | (ADDR_TYPE_STACK << ADDR_BITS),
        ADDR_CLASS = ADDR_STACK_CLASS | (ADDR_TYPE_STACK << ADDR_BITS),
        ADDR_NIL = ADDR_STACK_NIL | (ADDR_TYPE_STACK << ADDR_BITS),
    };

    struct StackDebug {
        int source_node_id = -1;
        int pos;
        bool added;
        StringName identifier;
    };

private:
    StringName name;
    StringName source;
    bool _static = false;
    Vector<OScriptDataType> argument_types;
    OScriptDataType return_type;
    MethodInfo method_info;
    Variant rpc_config;

    OScript* _script = nullptr;
    int initial_node = 0;
    int argument_count = 0;
    int vararg_index = -1;
    int stack_size = 0;
    int instruction_arg_size = 0;

    SelfList<OScriptCompiledFunction> function_list{ this };
    mutable Variant nil;
    HashMap<int, Variant::Type> temporary_slots;
    List<StackDebug> stack_debug;

    Vector<int> code;
    Vector<int> default_arguments;
    Vector<Variant> constants;
    Vector<StringName> global_names;
    Vector<GDExtensionPtrOperatorEvaluator> operator_funcs;
    Vector<GDExtensionPtrSetter> setters;
    Vector<GDExtensionPtrGetter> getters;
    Vector<GDExtensionPtrKeyedSetter> keyed_setters;
    Vector<GDExtensionPtrKeyedGetter> keyed_getters;
    Vector<GDExtensionPtrIndexedSetter> indexed_setters;
    Vector<GDExtensionPtrIndexedGetter> indexed_getters;
    Vector<GDExtensionPtrBuiltInMethod> builtin_methods;
    Vector<GDExtensionPtrConstructor> constructors;
    Vector<GDExtensionPtrUtilityFunction> utilities;
    Vector<OScriptUtilityFunctions::FunctionPtr> os_utilities;
    Vector<MethodBind*> methods;
    Vector<OScriptCompiledFunction*> lambdas;

    int code_size = 0;
    int default_arg_count = 0;
    int constant_count = 0;
    int global_names_count = 0;
    int operator_funcs_count = 0;
    int setters_count = 0;
    int getters_count = 0;
    int keyed_setters_count = 0;
    int keyed_getters_count = 0;
    int indexed_setters_count = 0;
    int indexed_getters_count = 0;
    int builtin_methods_count = 0;
    int constructors_count = 0;
    int utilities_count = 0;
    int os_utilities_count = 0;
    int methods_count = 0;
    int lambdas_count = 0;

    int* code_ptr = nullptr;
    const int* default_arg_ptr = nullptr;
    mutable Variant* constants_ptr = nullptr;
    const StringName* global_names_ptr = nullptr;
    const GDExtensionPtrOperatorEvaluator* operator_funcs_ptr = nullptr;
    const GDExtensionPtrSetter* setters_ptr = nullptr;
    const GDExtensionPtrGetter* getters_ptr = nullptr;
    const GDExtensionPtrKeyedSetter* keyed_setters_ptr = nullptr;
    const GDExtensionPtrKeyedGetter* keyed_getters_ptr = nullptr;
    const GDExtensionPtrIndexedSetter* indexed_setters_ptr = nullptr;
    const GDExtensionPtrIndexedGetter* indexed_getters_ptr = nullptr;
    const GDExtensionPtrBuiltInMethod* builtin_methods_ptr = nullptr;
    const GDExtensionPtrConstructor* constructors_ptr = nullptr;
    const GDExtensionPtrUtilityFunction* utilities_ptr = nullptr;
    const OScriptUtilityFunctions::FunctionPtr* os_utilities_ptr = nullptr;
    MethodBind** methods_ptr = nullptr;
    OScriptCompiledFunction** _lambdas_ptr = nullptr;

    #ifdef DEBUG_ENABLED
    CharString func_cname;
    const char* _func_cname = nullptr;

    Vector<String> operator_names;
    Vector<String> setter_names;
    Vector<String> getter_names;
    Vector<String> builtin_methods_names;
    Vector<String> constructors_names;
    Vector<String> utilities_names;
    Vector<String> os_utilities_names;

    struct Profile {
        StringName signature;
        SafeNumeric<uint64_t> call_count;
        SafeNumeric<uint64_t> self_time;
        SafeNumeric<uint64_t> total_time;
        SafeNumeric<uint64_t> frame_call_count;
        SafeNumeric<uint64_t> frame_self_time;
        SafeNumeric<uint64_t> frame_total_time;
        uint64_t last_frame_call_count = 0;
        uint64_t last_frame_self_time = 0;
        uint64_t last_frame_total_time = 0;
        typedef struct NativeProfile {
            uint64_t call_count;
            uint64_t total_time;
            String signature;
        } NativeProfile;
        HashMap<String, NativeProfile> native_calls;
        HashMap<String, NativeProfile> last_native_calls;
    } profile;
    #endif

    String get_call_error(const String& p_where, const Variant** p_args, int p_arg_count, const Variant& p_result, const GDExtensionCallError& p_error) const;
    String get_callable_call_error(const String& p_where, const Callable& p_callable, const Variant** p_args, int p_arg_count, const Variant& p_result, const GDExtensionCallError& p_error) const;
    Variant get_default_variant_for_data_type(const OScriptDataType& p_type);

public:
    static constexpr int MAX_CALL_DEPTH = 2048; // Limit to avoid crash because of stack overflow

    struct CallState {
        Signal completed;
        OScript* script = nullptr;
        OScriptInstance* instance = nullptr;
        #ifdef DEBUG_ENABLED
        StringName function_name;
        String script_path;
        #endif
        Vector<uint8_t> stack;
        int stack_size = 0;
        int ip = 0;
        int node_id = 0;
        int defarg = 0;
        Variant result;
    };

    _FORCE_INLINE_ StringName get_name() const { return name; }
    _FORCE_INLINE_ StringName get_source() const { return source; }
    _FORCE_INLINE_ OScript* get_script() const { return _script; }
    _FORCE_INLINE_ bool is_static() const { return _static; }
    _FORCE_INLINE_ bool is_vararg() const { return vararg_index >= 0; }
    _FORCE_INLINE_ MethodInfo get_method_info() const { return method_info; }
    _FORCE_INLINE_ int get_argument_count() const { return argument_count; }
    _FORCE_INLINE_ Variant get_rpc_config() const { return rpc_config; }
    _FORCE_INLINE_ int get_max_stack_size() const { return stack_size; }

    Variant get_constant(int p_index) const;
    StringName get_global_name(int p_index) const;

    Variant call(OScriptInstance* p_instance, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error, CallState* p_state = nullptr);
    void debug_get_stack_member_state(int p_node, List<Pair<StringName, int>>* r_stack_vars) const;

    #ifdef DEBUG_ENABLED
    void _profile_native_call(uint64_t p_t_taken, const String& p_function_name, const String& p_instance_class_name = String());
    void disassemble(const Vector<String>& p_code_lines, Vector<String>& r_output) const;
    #endif

    String to_string();

    OScriptCompiledFunction();
    ~OScriptCompiledFunction();
};

/// The state of the executing function that will be saved when the coroutine yields.
class OScriptFunctionState : public RefCounted {
    friend class OScriptCompiledFunction;

    GDCLASS(OScriptFunctionState, RefCounted);

    OScriptCompiledFunction* function = nullptr;
    OScriptCompiledFunction::CallState state;

    Ref<OScriptFunctionState> first_state;
    SelfList<OScriptFunctionState> scripts_list;
    SelfList<OScriptFunctionState> instances_list;

    Variant _signal_callback(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error);

protected:
    static void _bind_methods();

public:
    bool is_valid(bool p_extended_check = false) const;
    Variant resume(const Variant& p_arg = Variant());

    #ifdef DEBUG_ENABLED
    String get_readable_function() const {
        return state.function_name;
    }
    #endif

    void _clear_stack();
    void _clear_connections();

    OScriptFunctionState();
    ~OScriptFunctionState() override;
};


#endif // ORCHESTRATOR_SCRIPT_COMPILED_FUNCTION_H