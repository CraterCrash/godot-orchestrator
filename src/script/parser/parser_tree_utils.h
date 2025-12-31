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
#ifndef ORCHESTRATOR_SCRIPT_PARSER_UTILS_H
#define ORCHESTRATOR_SCRIPT_PARSER_UTILS_H

#ifdef DEV_TOOLS
#include "script/parser/parser_nodes.h"

#include <godot_cpp/classes/file_access.hpp>

namespace OScriptParserUtils {

    using DataType = OScriptParserNodes::DataType;
    using AnnotationInfo = OScriptParserNodes::AnnotationInfo;
    using AnnotationAction = OScriptParserNodes::AnnotationAction;
    using Node = OScriptParserNodes::Node;
    using AnnotationNode = OScriptParserNodes::AnnotationNode;
    using ArrayNode = OScriptParserNodes::ArrayNode;
    using AssertNode = OScriptParserNodes::AssertNode;
    using AssignableNode = OScriptParserNodes::AssignableNode;
    using AssignmentNode = OScriptParserNodes::AssignmentNode;
    using AwaitNode = OScriptParserNodes::AwaitNode;
    using BinaryOpNode = OScriptParserNodes::BinaryOpNode;
    using BreakNode = OScriptParserNodes::BreakNode;
    using BreakpointNode = OScriptParserNodes::BreakpointNode;
    using CallNode = OScriptParserNodes::CallNode;
    using CastNode = OScriptParserNodes::CastNode;
    using ClassNode = OScriptParserNodes::ClassNode;
    using ConstantNode = OScriptParserNodes::ConstantNode;
    using ContinueNode = OScriptParserNodes::ContinueNode;
    using DictionaryNode = OScriptParserNodes::DictionaryNode;
    using EnumNode = OScriptParserNodes::EnumNode;
    using ExpressionNode = OScriptParserNodes::ExpressionNode;
    using ForNode = OScriptParserNodes::ForNode;
    using FunctionNode = OScriptParserNodes::FunctionNode;
    using GetNodeNode = OScriptParserNodes::GetNodeNode;
    using IdentifierNode = OScriptParserNodes::IdentifierNode;
    using IfNode = OScriptParserNodes::IfNode;
    using LambdaNode = OScriptParserNodes::LambdaNode;
    using LiteralNode = OScriptParserNodes::LiteralNode;
    using MatchNode = OScriptParserNodes::MatchNode;
    using MatchBranchNode = OScriptParserNodes::MatchBranchNode;
    using ParameterNode = OScriptParserNodes::ParameterNode;
    using PassNode = OScriptParserNodes::PassNode;
    using PatternNode = OScriptParserNodes::PatternNode;
    using PreloadNode = OScriptParserNodes::PreloadNode;
    using ReturnNode = OScriptParserNodes::ReturnNode;
    using SelfNode = OScriptParserNodes::SelfNode;
    using SignalNode = OScriptParserNodes::SignalNode;
    using SubscriptNode = OScriptParserNodes::SubscriptNode;
    using SuiteNode = OScriptParserNodes::SuiteNode;
    using TernaryOpNode = OScriptParserNodes::TernaryOpNode;
    using TypeNode = OScriptParserNodes::TypeNode;
    using TypeTestNode = OScriptParserNodes::TypeTestNode;
    using UnaryOpNode = OScriptParserNodes::UnaryOpNode;
    using VariableNode = OScriptParserNodes::VariableNode;
    using WhileNode = OScriptParserNodes::WhileNode;

    template <typename K, typename V>
    struct HashMapToDictionary {
        Dictionary operator()(const HashMap<K,V>& p_map) const {
            Dictionary d;
            for (const KeyValue<K, V>& E :p_map) {
                d[E.key] = E.value;
            }
            return d;
        }
    };

    class StringBuilder {
        int32_t spaces = 0;
        String buffer;

        void append(const String& p_text);

    public:
        class IndentScope {
            StringBuilder& builder;
        public:
            explicit IndentScope(StringBuilder& p_builder);
            ~IndentScope();
        };

        void push_text(const String& p_text);
        void push_line(const String& p_text);

        const String& get_buffer() const { return buffer; }

        void reset() { buffer.resize(0); }
    };

    class Printer {
        int indent_level = 0;
        String indent;
        String printed;
        bool pending_indent = false;

        void increase_indent();
        void decrease_indent();
        void push_line(const String &p_line = String());
        void push_text(const String &p_text);

        void print_annotation(const AnnotationNode *p_annotation);
        void print_array(ArrayNode *p_array);
        void print_assert(AssertNode *p_assert);
        void print_assignment(AssignmentNode *p_assignment);
        void print_await(AwaitNode *p_await);
        void print_binary_op(BinaryOpNode *p_binary_op);
        void print_call(CallNode *p_call);
        void print_cast(CastNode *p_cast);
        void print_class(ClassNode *p_class);
        void print_constant(ConstantNode *p_constant);
        void print_dictionary(DictionaryNode *p_dictionary);
        void print_expression(ExpressionNode *p_expression);
        void print_enum(EnumNode *p_enum);
        void print_for(ForNode *p_for);
        void print_function(FunctionNode *p_function, const String &p_context = "Function");
        void print_get_node(GetNodeNode *p_get_node);
        void print_if(IfNode *p_if, bool p_is_elif = false);
        void print_identifier(IdentifierNode *p_identifier);
        void print_lambda(LambdaNode *p_lambda);
        void print_literal(LiteralNode *p_literal);
        void print_match(MatchNode *p_match);
        void print_match_branch(MatchBranchNode *p_match_branch);
        void print_match_pattern(PatternNode *p_match_pattern);
        void print_parameter(ParameterNode *p_parameter);
        void print_preload(PreloadNode *p_preload);
        void print_return(ReturnNode *p_return);
        void print_self(SelfNode *p_self);
        void print_signal(SignalNode *p_signal);
        void print_statement(Node *p_statement);
        void print_subscript(SubscriptNode *p_subscript);
        void print_suite(SuiteNode *p_suite);
        void print_ternary_op(TernaryOpNode *p_ternary_op);
        void print_type(TypeNode *p_type);
        void print_type_test(TypeTestNode *p_type_test);
        void print_unary_op(UnaryOpNode *p_unary_op);
        void print_variable(VariableNode *p_variable);
        void print_while(WhileNode *p_while);

    public:
        String print_tree(ClassNode* p_class);
    };

    class Writer {
        Ref<FileAccess> file;
        StringBuilder buffer;

        void push_line(const String& p_text) { buffer.push_line(p_text); }

        String yesno(const String& p_label, bool p_value);

        void write_yesno(const String& p_label, bool p_expression, bool p_default = false);
        void write_string(const String& p_label, const String& p_value);
        void write_method_info(const String& p_label, const MethodInfo& p_value, bool p_write_always = false);
        void write_property_info(const String& p_label, const PropertyInfo& p_value, bool p_write_always = false);
        void write_dictionary(const String& p_label, const Dictionary& p_value, bool p_write_always = false);

        void write_datatype(const DataType& p_type);
        void write_annotations(const List<AnnotationNode*>& p_annotations);
        void write_node(Node* p_node);
        void write_class(ClassNode* p_class);
        void write_assignable(AssignableNode* p_assignable);
        void write_variable(VariableNode* p_variable);
        void write_constant(ConstantNode* p_constant);
        void write_signal(SignalNode* p_signal);
        void write_parameter(ParameterNode* p_parameter);
        void write_function(FunctionNode* p_function);
        void write_suite(SuiteNode* p_suite);
        void write_call(CallNode* p_call);
        void write_return(ReturnNode* p_return);
        void write_binary_op(BinaryOpNode* p_binary_op);
        void write_unary_op(UnaryOpNode* p_unary_op);
        void write_if(IfNode* p_if);
        void write_expression(ExpressionNode* p_expression);
        void write_literal(LiteralNode* p_literal);
        void write_identifier(IdentifierNode* p_identifier);
        void write_cast(CastNode* p_cast);
        void write_type(TypeNode* p_type);
        void write_self(SelfNode* p_self);
        void write_subscript(SubscriptNode* p_subscript);
        void write_assignment(AssignmentNode* p_assignment);
        void write_await(AwaitNode* p_await);
        void write_for(ForNode* p_for);
        void write_match(MatchNode* p_match);
        void write_match_branch(MatchBranchNode* p_match_branch);
        void write_pattern(PatternNode* p_pattern);

    public:
        String write_tree(ClassNode* p_class);
    };
}
#endif

#endif // ORCHESTRATOR_SCRIPT_PARSER_UTILS_H