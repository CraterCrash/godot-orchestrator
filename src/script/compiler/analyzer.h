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
#ifndef ORCHESTRATOR_SCRIPT_ANALYZER_H
#define ORCHESTRATOR_SCRIPT_ANALYZER_H

#include "script/parser/parser.h"
#include "script/script_cache.h"

// Aligned with e304b4e43e5d2f5027ab0c475b3f2530e81db207
class OScriptAnalyzer {
    OScriptParser* parser = nullptr;

    template<typename F> class Finally {
        F function;
    public:
        explicit Finally(F p_function) : function(p_function) {}
        ~Finally() { function(); }
    };

    const OScriptParser::EnumNode* current_enum = nullptr;
    OScriptParser::LambdaNode* current_lambda = nullptr;
    List<OScriptParser::LambdaNode*> pending_body_resolution_lambdas;
    HashMap<const OScriptParser::ClassNode*, Ref<OScriptParserRef>> external_class_parser_cache;
    bool static_context = false;

    static bool is_packed_scene(const String& p_path);
    static bool is_oscript(const String& p_path);

    void get_class_node_current_scope_classes(OScriptParser::ClassNode* p_class, List<OScriptParser::ClassNode*>* p_list, OScriptParser::Node* p_source);
    void decide_suite_type(OScriptParser::Node* p_suite, OScriptParser::Node* p_statement);

    Ref<OScriptParserRef> find_cached_external_parser_for_class(const OScriptParser::ClassNode* p_class, const Ref<OScriptParserRef>& p_dependant_parser);
    Ref<OScriptParserRef> find_cached_external_parser_for_class(const OScriptParser::ClassNode* p_class, OScriptParser* p_dependant_parser);
    Ref<OScriptParserRef> ensure_cached_external_parser_for_class(const OScriptParser::ClassNode* p_class, const OScriptParser::ClassNode* p_from_class, const char* p_context, const OScriptParser::Node* p_source);

    OScriptParser::DataType resolve_datatype(OScriptParser::TypeNode* p_type);
    OScriptParser::DataType type_from_variant(const Variant& p_value, const OScriptParser::Node* p_source);
    OScriptParser::DataType type_from_property(const PropertyInfo& p_property, bool p_is_arg = false, bool p_is_readonly = false) const;
    OScriptParser::DataType make_global_class_meta_type(const StringName& p_class_name, const OScriptParser::Node* p_source);
    OScriptParser::DataType get_operation_type(Variant::Operator p_operation, const OScriptParser::DataType& p_a, const OScriptParser::DataType& p_b, bool& r_valid, const OScriptParser::Node* p_source);
    OScriptParser::DataType get_operation_type(Variant::Operator p_operation, const OScriptParser::DataType& p_a, bool& r_valid, const OScriptParser::Node* p_source);

    bool has_member_name_conflict_in_native_type(const StringName& p_name, const StringName& p_native_type);
    bool has_member_name_conflict_in_script_class(const StringName& p_name, const OScriptParser::ClassNode* p_current_class, const OScriptParser::Node* p_member);
    bool get_function_signature(OScriptParser::Node* p_source, bool p_is_constructor, OScriptParser::DataType p_base_type, const StringName& p_function, OScriptParser::DataType& r_return_type, List<OScriptParser::DataType>& r_par_types, int& r_default_arg_count, BitField<MethodFlags>& r_method_flags, StringName* r_native_class = nullptr);
    bool function_signature_from_info(const MethodInfo& p_info, OScriptParser::DataType& r_return_type, List<OScriptParser::DataType>& r_par_types, int& r_default_arg_count, BitField<MethodFlags>& r_method_flags);

    Error check_native_member_name_conflict(const StringName& p_member_name, const OScriptParser::Node* p_member, const StringName& p_native_type);
    Error check_class_member_name_conflict(const OScriptParser::ClassNode* p_class, const StringName& p_member_name, const OScriptParser::Node* p_member);

    Error resolve_class_inheritance(OScriptParser::ClassNode* p_class, const OScriptParser::Node* p_source = nullptr);
    Error resolve_class_inheritance(OScriptParser::ClassNode* p_class, bool p_recursive);

    // Resolver Functions
    void resolve_annotation(OScriptParser::AnnotationNode* p_node);
    void resolve_class_member(OScriptParser::ClassNode* p_class, const StringName& p_name, const OScriptParser::Node* p_source = nullptr);
    void resolve_class_member(OScriptParser::ClassNode* p_class, int p_index, const OScriptParser::Node* p_source = nullptr);
    void resolve_class_interface(OScriptParser::ClassNode* p_class, const OScriptParser::Node* p_source = nullptr);
    void resolve_class_interface(OScriptParser::ClassNode* p_class, bool p_recursive);
    void resolve_class_body(OScriptParser::ClassNode* p_class, const OScriptParser::Node* p_source = nullptr);
    void resolve_class_body(OScriptParser::ClassNode* p_class, bool p_recursive);
    void resolve_function_signature(OScriptParser::FunctionNode* p_function, const OScriptParser::Node* p_source = nullptr, bool p_is_lambda = false);
    void resolve_function_body(OScriptParser::FunctionNode* p_function, bool p_is_lambda = false);
    void resolve_node(OScriptParser::Node* p_node, bool p_is_root = true);
    void resolve_suite(OScriptParser::SuiteNode* p_suite);
    void resolve_assignable(OScriptParser::AssignableNode* p_assignable, const char* p_kind);
    void resolve_variable(OScriptParser::VariableNode* p_variable, bool p_is_local);
    void resolve_constant(OScriptParser::ConstantNode* p_constant, bool p_is_local);
    void resolve_parameter(OScriptParser::ParameterNode* p_parameter);
    void resolve_if(OScriptParser::IfNode* p_if);
    void resolve_for(OScriptParser::ForNode* p_for);
    void resolve_while(OScriptParser::WhileNode* p_while);
    void resolve_assert(OScriptParser::AssertNode* p_assert);
    void resolve_match(OScriptParser::MatchNode* p_match);
    void resolve_match_branch(OScriptParser::MatchBranchNode* p_match_branch, OScriptParser::ExpressionNode* p_match_test);
    void resolve_match_pattern(OScriptParser::PatternNode* p_pattern, OScriptParser::ExpressionNode* p_match_test);
    void resolve_return(OScriptParser::ReturnNode* p_return);

    void mark_lambda_use_self();
    void resolve_pending_lambda_bodies();

    // Reduction Functions
    void reduce_expression(OScriptParser::ExpressionNode* p_expression, bool p_is_root = false);
    void reduce_array(OScriptParser::ArrayNode* p_array);
    void reduce_assignment(OScriptParser::AssignmentNode* p_assignment);
    void reduce_await(OScriptParser::AwaitNode* p_await);
    void reduce_binary_op(OScriptParser::BinaryOpNode* p_binary_op);
    void reduce_call(OScriptParser::CallNode* p_call, bool p_is_await = false, bool p_is_root = false);
    void reduce_cast(OScriptParser::CastNode* p_cast);
    void reduce_dictionary(OScriptParser::DictionaryNode* p_dictionary);
    void reduce_get_node(OScriptParser::GetNodeNode* p_get_node);
    void reduce_identifier(OScriptParser::IdentifierNode* p_id, bool p_can_be_builtin = false);
    void reduce_identifier_from_base(OScriptParser::IdentifierNode* p_identifier, OScriptParser::DataType* p_base = nullptr);
    void reduce_identifier_from_base_set_class(OScriptParser::IdentifierNode* p_identifier, OScriptParser::DataType p_identifier_datatype);
    void reduce_lambda(OScriptParser::LambdaNode* p_lambda);
    void reduce_literal(OScriptParser::LiteralNode* p_literal);
    void reduce_preload(OScriptParser::PreloadNode* p_preload);
    void reduce_self(OScriptParser::SelfNode* p_self);
    void reduce_subscript(OScriptParser::SubscriptNode* p_subscript, bool p_can_be_pseudo_type = false);
    void reduce_ternary_op(OScriptParser::TernaryOpNode* p_ternary_op, bool p_is_root = false);
    void reduce_type_test(OScriptParser::TypeTestNode* p_type_test);
    void reduce_unary_op(OScriptParser::UnaryOpNode* p_unary_op);

    Array make_array_from_element_datatype(const OScriptParser::DataType& p_element_datatype, const OScriptParser::Node* p_source_node = nullptr);
    Dictionary make_dictionary_from_element_datatype(const OScriptParser::DataType& p_key_element_datatype, const OScriptParser::DataType& p_value_element_datatype, const OScriptParser::Node* p_source_node = nullptr);

    Variant make_expression_reduced_value(OScriptParser::ExpressionNode* p_expression, bool& is_reduced);
    Variant make_array_reduced_value(OScriptParser::ArrayNode* p_array, bool& is_reduced);
    Variant make_dictionary_reduced_value(OScriptParser::DictionaryNode* p_dictionary, bool& is_reduced);
    Variant make_subscript_reduced_value(OScriptParser::SubscriptNode* p_subscript, bool& is_reduced);
    Variant make_call_reduced_value(OScriptParser::CallNode* p_call, bool& is_reduced);

    bool is_type_compatible(const OScriptParser::DataType& p_target, const OScriptParser::DataType& p_source, bool p_allow_implicit_conversion = false, const OScriptParser::Node* p_source_node = nullptr);

    #ifdef DEBUG_ENABLED
    void is_shadowing(OScriptParser::IdentifierNode* p_identifier, const String& p_context, const bool p_in_local_scope);
    #endif

    void push_error(const String& p_message, const OScriptParser::Node* p_origin = nullptr);
    void mark_node_unsafe(const OScriptParser::Node* p_node);

    void update_const_expression_builtin_type(OScriptParser::ExpressionNode* p_expression, const OScriptParser::DataType& p_type, const char* p_usage, bool p_is_cast = false);
    void update_array_literal_element_type(OScriptParser::ArrayNode* p_array, const OScriptParser::DataType& p_element_type);
    void update_dictionary_literal_element_type(OScriptParser::DictionaryNode* p_dictionary, const OScriptParser::DataType& p_key_element_type, const OScriptParser::DataType& p_value_element_type);
    void validate_call_arg(const List<OScriptParser::DataType>& p_par_types, int p_default_args_count, bool p_is_vararg, const OScriptParser::CallNode* p_call);
    void validate_call_arg(const MethodInfo& p_method, const OScriptParser::CallNode* p_call);

    void downgrade_node_type_source(OScriptParser::Node *p_node);

    Ref<OScript> get_depended_shallow_script(const String &p_path, Error &r_error);

public:
    Error resolve_inheritance();
    Error resolve_interface();
    Error resolve_body();
    Error resolve_dependencies();
    Error analyze();

    Variant make_variable_default_value(OScriptParser::VariableNode* p_variable);

    static bool check_type_compatibility(const OScriptParser::DataType& p_target, const OScriptParser::DataType& p_source, bool p_allow_implicit_conversion = false, const OScriptParser::Node* p_source_node = nullptr);
    static OScriptParser::DataType type_from_metatype(const OScriptParser::DataType& p_type);
    static bool class_exists(const StringName& p_class);

    OScriptAnalyzer(OScriptParser* p_parser);
};

#endif // ORCHESTRATOR_SCRIPT_ANALYZER_H