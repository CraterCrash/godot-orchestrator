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
#ifndef ORCHESTRATOR_SCRIPT_COMPILER_H
#define ORCHESTRATOR_SCRIPT_COMPILER_H

#include "script/compiler/code_generator.h"
#include "script/parser/parser.h"

#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

/// Forward declaration
class OScript;

// Aligned with e304b4e43e5d2f5027ab0c475b3f2530e81db207
class OScriptCompiler {
    const OScriptParser* parser = nullptr;
    HashSet<OScript*> parsed_classes;
    HashSet<OScript*> parsing_classes;
    OScript* main_script = nullptr;

    struct CompilerContext {
        OScript* script = nullptr;
        const OScriptParser::ClassNode* class_node = nullptr;
        const OScriptParser::FunctionNode* function_node = nullptr;
        StringName function_name;
        OScriptCodeGenerator* generator = nullptr;
        HashMap<StringName, OScriptCodeGenerator::Address> parameters;
        HashMap<StringName, OScriptCodeGenerator::Address> locals;
        List<HashMap<StringName, OScriptCodeGenerator::Address>> locals_stack;
        bool is_static = false;

        OScriptCodeGenerator::Address add_local(const StringName& p_name, const OScriptDataType& p_type);
        OScriptCodeGenerator::Address add_local_constant(const StringName& p_name, const Variant& p_value);
        OScriptCodeGenerator::Address add_temporary(const OScriptDataType& p_type = OScriptDataType());
        OScriptCodeGenerator::Address add_constant(const Variant& p_value);

        void start_block();
        void end_block();
    };

    StringName source;
    String error;
    int err_node_id = -1;
    OScriptParser::ExpressionNode* awaited_node = nullptr;
    bool has_static_data = false;

    bool is_class_member_property(CompilerContext& p_context, const StringName& p_name);
    bool is_class_member_property(OScript* p_owner, const StringName& p_name);
    bool is_local_or_parameter(CompilerContext& p_context, const StringName& p_name);
    bool has_utility_function(const StringName& p_name);

    void set_error(const String& p_error, const OScriptParser::Node* p_node);

    OScriptDataType resolve_type(const OScriptParser::DataType& p_type, OScript* p_owner, bool p_handle_metatype = true);

    List<OScriptCodeGenerator::Address> add_block_locals(CompilerContext& p_context, const OScriptParser::SuiteNode* p_block);
    void clear_block_locals(CompilerContext& p_context, const List<OScriptCodeGenerator::Address>& p_locals);

    Error parse_setter_getter(OScript* p_script, const OScriptParser::ClassNode* p_class, const OScriptParser::VariableNode* p_variable, bool p_is_setter);

    OScriptCompiledFunction* parse_function(Error& r_error, OScript* p_script, const OScriptParser::ClassNode* p_class, const OScriptParser::FunctionNode* p_func, bool p_for_ready = false, bool p_for_lambda = false);
    OScriptCompiledFunction* make_static_initializer(Error& r_error, OScript* p_script, const OScriptParser::ClassNode* p_class);

    Error parse_block(CompilerContext& p_context, const OScriptParser::SuiteNode* p_block, bool p_add_locals = true, bool p_clear_locals = true);

    OScriptCodeGenerator::Address parse_expression(CompilerContext& p_context, Error& r_error, const OScriptParser::ExpressionNode* p_expression, bool p_root = false, bool p_initializer = false);
    OScriptCodeGenerator::Address parse_match_pattern(CompilerContext& p_context, Error& r_error, const OScriptParser::PatternNode* p_node, const OScriptCodeGenerator::Address& p_value_addr, const OScriptCodeGenerator::Address& p_type_addr, const OScriptCodeGenerator::Address& p_prev_test, bool p_is_first, bool p_is_nested);

    Error prepare_compilation(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state);
    Error compile_class(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state);

public:
    static void convert_to_initializer_type(Variant& p_variant, const OScriptParser::VariableNode* p_node);
    static void make_scripts(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state);
    Error compile(const OScriptParser* p_parser, OScript* p_script, bool p_keep_state = false);

    String get_error() const;
    int get_error_node_id() const;

    OScriptCompiler() = default;
};

#endif // ORCHESTRATOR_SCRIPT_COMPILER_H