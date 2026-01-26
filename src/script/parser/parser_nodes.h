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
#ifndef ORCHESTRATOR_SCRIPT_PARSER_NODES_H
#define ORCHESTRATOR_SCRIPT_PARSER_NODES_H

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Forward declarations
class OScriptNodePin;
class OScriptParser;

namespace OScriptParserNodes
{
    #ifdef TOOLS_ENABLED
    struct ClassDocData {
        String brief;
        String description;
        Vector<Pair<String, String>> tutorials;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
    };

    struct MemberDocData {
        String description;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
    };
    #endif // TOOLS_ENABLED

    struct AnnotationInfo;

    struct AnnotationNode;
    struct ArrayNode;
    struct AssertNode;
    struct AssignableNode;
    struct AssignmentNode;
    struct AwaitNode;
    struct BinaryOpNode;
    struct BreakNode;
    struct BreakpointNode;
    struct CallNode;
    struct CastNode;
    struct ClassNode;
    struct ConstantNode;
    struct ContinueNode;
    struct DictionaryNode;
    struct EnumNode;
    struct ExpressionNode;
    struct ForNode;
    struct FunctionNode;
    struct GetNodeNode;
    struct IdentifierNode;
    struct IfNode;
    struct LambdaNode;
    struct LiteralNode;
    struct MatchNode;
    struct MatchBranchNode;
    struct ParameterNode;
    struct PassNode;
    struct PatternNode;
    struct PreloadNode;
    struct ReturnNode;
    struct SelfNode;
    struct SignalNode;
    struct SubscriptNode;
    struct SuiteNode;
    struct TernaryOpNode;
    struct TypeNode;
    struct TypeTestNode;
    struct UnaryOpNode;
    struct VariableNode;
    struct WhileNode;

    class DataType {
    public:
        enum Kind {
            BUILTIN,
            NATIVE,
            SCRIPT,
            CLASS,
            ENUM,
            VARIANT,
            RESOLVING,
            UNRESOLVED
        };

        enum TypeSource {
            UNDETECTED,
            INFERRED,
            ANNOTATED_EXPLICIT,
            ANNOTATED_INFERRED
        };

        Kind kind = UNRESOLVED;
        TypeSource type_source = UNDETECTED;
        Vector<DataType> container_element_types;

        Variant::Type builtin_type = Variant::NIL;
        StringName native_type;
        StringName enum_type;
        Ref<Script> script_type;
        String script_path;
        ClassNode* class_type = nullptr;

        bool is_constant = false;
        bool is_read_only = false;
        bool is_meta_type = false;
        bool is_pseudo_type = false;
        bool is_coroutine = false;

        MethodInfo method_info;
        HashMap<StringName, int64_t> enum_values;

        _FORCE_INLINE_ bool is_set() const { return kind != RESOLVING && kind != UNRESOLVED; }
        _FORCE_INLINE_ bool is_resolving() const { return kind == RESOLVING; }
        _FORCE_INLINE_ bool has_no_type() const { return type_source == UNDETECTED; }
        _FORCE_INLINE_ bool is_variant() const { return kind == VARIANT || kind == RESOLVING || kind == UNRESOLVED; }
        _FORCE_INLINE_ bool is_hard_type() const { return type_source > INFERRED; }

        String to_string() const;
        PropertyInfo to_property_info(const String& p_name) const;

        _FORCE_INLINE_ String to_string_strict() const {
            return is_hard_type() ? to_string() : "Variant";
        }

        _FORCE_INLINE_ static DataType get_variant_type() {
            DataType type;
            type.kind = VARIANT;
            type.type_source = INFERRED;
            return type;
        }

        _FORCE_INLINE_ void set_container_element_type(int p_index, const DataType& p_type) {
            ERR_FAIL_COND(p_index < 0);
            while (p_index >= container_element_types.size()) {
                container_element_types.push_back(get_variant_type());
            }
            container_element_types.write[p_index] = DataType(p_type);
        }

        _FORCE_INLINE_ int get_container_element_type_count() const {
            return container_element_types.size();
        }

        _FORCE_INLINE_ DataType get_container_element_type(int p_index) const {
            ERR_FAIL_INDEX_V(p_index, container_element_types.size(), get_variant_type());
            return container_element_types[p_index];
        }

        _FORCE_INLINE_ DataType get_container_element_type_or_variant(int p_index) const {
            if (p_index < 0 || p_index >= container_element_types.size()) {
                return get_variant_type();
            }
            return container_element_types[p_index];
        }

        _FORCE_INLINE_ bool has_container_element_type(int p_index) const {
            return p_index >= 0 && p_index < container_element_types.size();
        }

        _FORCE_INLINE_ bool has_container_element_types() const {
            return !container_element_types.is_empty();
        }

        bool is_typed_container_type() const;
        DataType get_typed_container_type() const;

        bool can_reference(const DataType& p_other) const;

        bool operator==(const DataType& p_other) const;
        bool operator!=(const DataType& p_other) const;

        void operator=(const DataType& p_other);

        DataType() = default;
        DataType(const DataType& p_other) { *this = p_other; }

        ~DataType() = default;
    };

    struct Node {
        enum Type {
            NONE,
            ANNOTATION,
            ARRAY,
            ASSERT,
            ASSIGNMENT,
            AWAIT,
            BINARY_OPERATOR,
            BREAK,
            BREAKPOINT,
            CALL,
            CAST,
            CLASS,
            CONSTANT,
            CONTINUE,
            DICTIONARY,
            ENUM,
            FOR,
            FUNCTION,
            GET_NODE,
            IDENTIFIER,
            IF,
            LAMBDA,
            LITERAL,
            MATCH,
            MATCH_BRANCH,
            PARAMETER,
            PASS,
            PATTERN,
            PRELOAD,
            RETURN,
            SELF,
            SIGNAL,
            SUBSCRIPT,
            SUITE,
            TERNARY_OPERATOR,
            TYPE,
            TYPE_TEST,
            UNARY_OPERATOR,
            VARIABLE,
            WHILE
        };

        Type type = NONE;
        int script_node_id = -1;
        Node* next = nullptr;
        DataType data_type;
        List<AnnotationNode*> annotations;

        virtual DataType get_datatype() const { return data_type; }
        virtual void set_datatype(const DataType& p_datatype) { data_type = p_datatype; }
        virtual bool is_expression() const { return false; }

        virtual ~Node() = default;
    };

    struct ExpressionNode : Node {
        bool reduced = false;
        bool is_constant = false;
        Variant reduced_value;

        bool is_expression() const override { return true; }

    protected:
        ~ExpressionNode() override = default;
    };

    struct AnnotationNode : Node {
        StringName name;
        Vector<ExpressionNode*> arguments;
        Vector<Variant> resolved_arguments;

        /** Information of the annotation. Might be null for unknown annotations. */
        AnnotationInfo* info = nullptr;
        PropertyInfo export_info;
        bool is_resolved = false;
        bool is_applied = false;

        bool apply(OScriptParser* p_this, Node* p_target, ClassNode* p_class);
        bool applies_to(uint32_t p_target_kinds) const;

        AnnotationNode() { type = ANNOTATION; }
    };

    struct ArrayNode : ExpressionNode {
        Vector<ExpressionNode*> elements;

        ArrayNode() { type = ARRAY; }
    };

    struct AssertNode : Node {
        ExpressionNode* condition = nullptr;
        ExpressionNode* message = nullptr;

        AssertNode() { type = ASSERT; }
    };

    struct AssignableNode : Node {
        IdentifierNode* identifier = nullptr;
        ExpressionNode* initializer = nullptr;
        TypeNode* datatype_specifier = nullptr;
        bool infer_datatype = false;
        bool use_conversion_assign = false;
        int usages = 0;

        ~AssignableNode() override = default;

    protected:
        AssignableNode() = default;
    };

    struct AssignmentNode : ExpressionNode {
        enum Operation {
            OP_NONE,
            OP_ADDITION,
            OP_SUBTRACTION,
            OP_MULTIPLICATION,
            OP_DIVISION,
            OP_MODULO,
            OP_POWER,
            OP_BIT_SHIFT_LEFT,
            OP_BIT_SHIFT_RIGHT,
            OP_BIT_AND,
            OP_BIT_OR,
            OP_BIT_XOR
        };

        Operation operation = OP_NONE;
        Variant::Operator variant_op = Variant::OP_MAX;
        ExpressionNode* assignee = nullptr;
        ExpressionNode* assigned_value = nullptr;
        bool use_conversion_assign = false;

        AssignmentNode() { type = ASSIGNMENT; }
    };

    struct AwaitNode : ExpressionNode {
        ExpressionNode* to_await = nullptr;

        AwaitNode() { type = AWAIT; }
    };

    struct BinaryOpNode : ExpressionNode {
        enum OpType {
            OP_ADDITION,
            OP_SUBTRACTION,
            OP_MULTIPLICATION,
            OP_DIVISION,
            OP_MODULO,
            OP_POWER,
            OP_BIT_LEFT_SHIFT,
            OP_BIT_RIGHT_SHIFT,
            OP_BIT_AND,
            OP_BIT_OR,
            OP_BIT_XOR,
            OP_LOGIC_AND,
            OP_LOGIC_OR,
            OP_CONTENT_TEST,
            OP_COMP_EQUAL,
            OP_COMP_NOT_EQUAL,
            OP_COMP_LESS,
            OP_COMP_LESS_EQUAL,
            OP_COMP_GREATER,
            OP_COMP_GREATER_EQUAL,
        };

        OpType operation = OP_ADDITION;
        Variant::Operator variant_op = Variant::OP_MAX;
        ExpressionNode* left_operand = nullptr;
        ExpressionNode* right_operand = nullptr;

        BinaryOpNode() { type = BINARY_OPERATOR; }
    };

    struct BreakNode : Node {
        BreakNode() { type = BREAK; }
    };

    struct BreakpointNode : Node {
        BreakpointNode() { type = BREAKPOINT; }
    };

    struct CallNode : ExpressionNode {
        ExpressionNode* callee = nullptr;
        Vector<ExpressionNode*> arguments;
        StringName function_name;
        bool is_super = false;
        bool is_static = false;

        _FORCE_INLINE_ Type get_callee_type() const { return callee == nullptr ? Type::NONE : callee->type; }
        _FORCE_INLINE_ CallNode* add_argument(ExpressionNode* p_arg) { arguments.push_back(p_arg); return this; }

        CallNode() { type = CALL; }
    };

    struct CastNode : ExpressionNode {
        ExpressionNode* operand = nullptr;
        TypeNode* cast_type = nullptr;

        CastNode() { type = CAST; }
    };

    struct EnumNode : Node {
        struct Value {
            IdentifierNode* identifier = nullptr;
            ExpressionNode* expression = nullptr;
            EnumNode* parent_enum = nullptr;
            int index = -1;
            bool resolved = false;
            int64_t value = 0;
            int script_node_id = -1;
            #ifdef TOOLS_ENABLED
            MemberDocData doc_data;
            #endif // TOOLS_ENABLED
        };

        IdentifierNode* identifier = nullptr;
        Vector<Value> values;
        Variant dictionary;
        #ifdef TOOLS_ENABLED
        MemberDocData doc_data;
        #endif // TOOLS_ENABLED

        EnumNode() { type = ENUM; }
    };

    struct ClassNode : Node {
        struct Member {
            enum Type {
                UNDEFINED,
                CLASS,
                CONSTANT,
                FUNCTION,
                SIGNAL,
                VARIABLE,
                ENUM,
                ENUM_VALUE,
                GROUP
            };

            Type type = UNDEFINED;

            union {
                ClassNode* m_class = nullptr;
                ConstantNode* constant;
                FunctionNode* function;
                SignalNode* signal;
                VariableNode* variable;
                EnumNode* m_enum;
                AnnotationNode* annotation;
            };

            EnumNode::Value enum_value;

            String get_name() const;
            String get_type_name() const;
            int get_script_node_id() const;
            DataType get_data_type() const;
            Node* get_source_node() const;

            Member() = default;
            Member(ClassNode* p_class) { type = CLASS; m_class = p_class; }
            Member(ConstantNode* p_constant) { type = CONSTANT; constant = p_constant; }
            Member(VariableNode* p_variable) { type = VARIABLE; variable = p_variable; }
            Member(SignalNode* p_signal) { type = SIGNAL; signal = p_signal; }
            Member(FunctionNode* p_function) { type = FUNCTION; function = p_function; }
            Member(EnumNode* p_enum) { type = ENUM; m_enum = p_enum; }
            Member(const EnumNode::Value& p_value) { type = ENUM_VALUE; enum_value = p_value; }
            Member(AnnotationNode* p_annotation) { type = GROUP; annotation = p_annotation; }
        };

        IdentifierNode* identifier = nullptr;
        String icon_path;
        String simplified_icon_path;
        Vector<Member> members;
        HashMap<StringName, int> members_indices;
        ClassNode* outer = nullptr;
        bool tool = false;
        bool extends_used = false;
        bool onready_used = false;
        bool is_abstract = false;
        bool has_static_data = false;
        bool annotated_static_unload = false;
        String extends_path;
        Vector<IdentifierNode*> extends; // List for indexing: Extends A.B.C
        DataType base_type;
        String fqcn;

        #ifdef TOOLS_ENABLED
        ClassDocData doc_data;

        // EnumValue docs are parsed after itself, so we need a method to add/modify the doc property later.
        void set_enum_value_doc_data(const StringName &p_name, const MemberDocData &p_doc_data) {
            ERR_FAIL_INDEX(members_indices[p_name], members.size());
            members.write[members_indices[p_name]].enum_value.doc_data = p_doc_data;
        }
        #endif // TOOLS_ENABLED

        bool resolved_interface = false;
        bool resolved_body = false;

        StringName get_global_name() const;

        Member get_member(const StringName& p_name) const;
        bool has_member(const StringName& p_name) const;

        bool has_function(const StringName& p_name) const;

        template<typename T> void add_member(T* p_node) {
            members_indices[p_node->identifier->name] = members.size();
            members.push_back(Member(p_node));
        }

        void add_member(const EnumNode::Value& p_enum_value);
        void add_member_group(AnnotationNode* p_annotation);

        ClassNode() { type = CLASS; }
    };

    struct ConstantNode : AssignableNode {
        #ifdef TOOLS_ENABLED
        MemberDocData doc_data;
        #endif // TOOLS_ENABLED
        ConstantNode() { type = CONSTANT; }
    };

    struct ContinueNode : Node {
        ContinueNode() { type = CONTINUE; }
    };

    struct DictionaryNode : ExpressionNode {
        struct Pair {
            ExpressionNode* key = nullptr;
            ExpressionNode* value = nullptr;
        };
        Vector<Pair> elements;

        enum Style {
            LUA_TABLE,
            PYTHON_DICT,
        };
        Style style = PYTHON_DICT;

        DictionaryNode() { type = DICTIONARY; }
    };

    struct ForNode : Node {
        IdentifierNode* variable = nullptr;
        TypeNode* datatype_specifier = nullptr;
        bool use_conversion_assign = false;
        ExpressionNode* list = nullptr;
        SuiteNode* loop = nullptr;

        ForNode() { type = FOR; }
    };

    struct FunctionNode : Node {
        IdentifierNode* identifier = nullptr;
        Vector<ParameterNode*> parameters;
        HashMap<StringName, int> parameters_indices;
        ParameterNode* rest_parameter = nullptr;
        TypeNode* return_type = nullptr;
        SuiteNode* body = nullptr;
        bool is_abstract = false;
        bool is_static = false;
        bool is_coroutine = false;
        Variant rpc_config;
        MethodInfo method;
        LambdaNode *source_lambda = nullptr;
        Vector<Variant> default_arg_values;
        bool resolved_signature = false;
        bool resolved_body = false;
        #ifdef TOOLS_ENABLED
        MemberDocData doc_data;
        int min_local_doc_line = 0;
        String signature; // For autocompletion.
        #endif // TOOLS_ENABLED

        _FORCE_INLINE_ bool is_vararg() const { return rest_parameter != nullptr; }

        FunctionNode() { type = FUNCTION; }
    };

    struct GetNodeNode : ExpressionNode {
        String full_path;
        bool use_dollar = true;

        GetNodeNode() { type = GET_NODE; }
    };

    struct IdentifierNode : ExpressionNode {
        enum Source {
            UNDEFINED_SOURCE,
            FUNCTION_PARAMETER,
            LOCAL_VARIABLE,
            LOCAL_CONSTANT,
            LOCAL_ITERATOR,
            LOCAL_BIND,
            MEMBER_VARIABLE,
            MEMBER_CONSTANT,
            MEMBER_FUNCTION,
            MEMBER_SIGNAL,
            MEMBER_CLASS,
            INHERITED_VARIABLE,
            STATIC_VARIABLE,
            NATIVE_CLASS
        };

        StringName name;
        SuiteNode* suite = nullptr;
        Source source = UNDEFINED_SOURCE;

        union {
            ParameterNode* parameter_source = nullptr;
            IdentifierNode* bind_source;
            VariableNode* variable_source;
            ConstantNode* constant_source;
            SignalNode* signal_source;
            FunctionNode* function_source;
        };

        bool function_source_is_static = false;
        FunctionNode* source_function = nullptr;
        int usages = 0;

        IdentifierNode() { type = IDENTIFIER; }
    };

    struct IfNode : Node {
        ExpressionNode* condition = nullptr;
        SuiteNode* true_block = nullptr;
        SuiteNode* false_block = nullptr;

        IfNode() { type = IF; }
    };

    struct LambdaNode : ExpressionNode {
        FunctionNode* function = nullptr;
        FunctionNode* parent_function = nullptr;
        LambdaNode* parent_lambda = nullptr;
        Vector<IdentifierNode*> captures;
        HashMap<StringName, int> captures_indices;
        bool use_self = false;

        bool has_name() const { return function && function->identifier; }

        LambdaNode() { type = LAMBDA; }
    };

    struct LiteralNode : ExpressionNode {
        Variant value;

        LiteralNode() { type = LITERAL; }
    };

    struct MatchNode : Node {
        ExpressionNode* test = nullptr;
        Vector<MatchBranchNode*> branches;

        MatchNode() { type = MATCH; }
    };

    struct MatchBranchNode : Node {
        Vector<PatternNode*> patterns;
        SuiteNode* block = nullptr;
        bool has_wildcard = false;
        SuiteNode* guard_body = nullptr;

        MatchBranchNode() { type = MATCH_BRANCH; }
    };

    struct ParameterNode : AssignableNode {
        ParameterNode() { type = PARAMETER; }
    };

    struct PassNode : Node {
        PassNode() { type = PASS; }
    };

    struct PatternNode : Node {
        enum Type {
            PT_LITERAL,
            PT_EXPRESSION,
            PT_BIND,
            PT_ARRAY,
            PT_DICTIONARY,
            PT_REST,
            PT_WILDCARD,
        };
        Type pattern_type = PT_LITERAL;

        union {
            LiteralNode* literal = nullptr;
            IdentifierNode* bind;
            ExpressionNode* expression;
        };
        Vector<PatternNode*> array;
        bool rest_used = false; // For array/dict patterns.

        struct Pair {
            ExpressionNode* key = nullptr;
            PatternNode* value_pattern = nullptr;
        };
        Vector<Pair> dictionary;

        HashMap<StringName, IdentifierNode*> binds;

        bool has_bind(const StringName& p_name);
        IdentifierNode* get_bind(const StringName& p_name);

        PatternNode() { type = PATTERN; }
    };

    struct PreloadNode : ExpressionNode {
        ExpressionNode* path = nullptr;
        String resolved_path;
        Ref<Resource> resource;

        PreloadNode() { type = PRELOAD; }
    };

    struct ReturnNode : Node {
        ExpressionNode* return_value = nullptr;
        bool void_return = false;

        ReturnNode() { type = RETURN; }
    };

    struct SelfNode : ExpressionNode {
        ClassNode* current_class = nullptr;

        SelfNode() { type = SELF; }
    };

    struct SignalNode : Node {
        IdentifierNode* identifier = nullptr;
        Vector<ParameterNode*> parameters;
        HashMap<StringName, int> parameters_indices;
        MethodInfo method;
        int usages = 0;
        #ifdef TOOLS_ENABLED
        MemberDocData doc_data;
        #endif // TOOLS_ENABLED

        SignalNode() { type = SIGNAL; }
    };

    struct SubscriptNode : ExpressionNode {
        ExpressionNode* base = nullptr;
        union {
            ExpressionNode* index = nullptr;
            IdentifierNode* attribute;
        };
        bool is_attribute = false;

        SubscriptNode() { type = SUBSCRIPT; }
    };

    struct VariableNode : AssignableNode {
        enum Style {
            NONE,
            INLINE,
            SETGET
        };

        Style style = NONE;

        union {
            FunctionNode* setter = nullptr;
            IdentifierNode* setter_pointer;
        };
        IdentifierNode* setter_parameter = nullptr;

        union {
            FunctionNode* getter = nullptr;
            IdentifierNode* getter_pointer;
        };

        bool exported = false;
        bool onready = false;
        PropertyInfo export_info;
        int assignments = 0;
        bool is_static = false;
        #ifdef TOOLS_ENABLED
        MemberDocData doc_data;
        #endif // TOOLS_ENABLED

        VariableNode() { type = VARIABLE; }
    };

    struct SuiteNode : Node {
        struct Local {
            enum Type {
                UNDEFINED,
                CONSTANT,
                VARIABLE,
                PARAMETER,
                FOR_VARIABLE,
                PATTERN_BIND
            };
            Type type = UNDEFINED;

            union {
                ConstantNode* constant = nullptr;
                VariableNode* variable;
                ParameterNode* parameter;
                IdentifierNode* bind;
            };

            StringName name;
            FunctionNode* source_function = nullptr;
            int script_node_id = -1;

            DataType get_data_type() const;
            String get_name() const;

            Local() = default;

            Local(ConstantNode* p_constant, FunctionNode* p_function) {
                type = CONSTANT;
                constant = p_constant;
                name = p_constant->identifier->name;
                source_function = p_function;
                script_node_id = p_constant->script_node_id;
            }

            Local(VariableNode* p_variable, FunctionNode* p_function) {
                type = VARIABLE;
                variable = p_variable;
                name = p_variable->identifier->name;
                source_function = p_function;
                script_node_id = p_function->script_node_id;
            }

            Local(ParameterNode* p_parameter, FunctionNode* p_function) {
                type = PARAMETER;
                parameter = p_parameter;
                name = p_parameter->identifier->name;
                source_function = p_function;
                script_node_id = p_function->script_node_id;
            }

            Local(IdentifierNode* p_identifier, FunctionNode* p_function) {
                type = FOR_VARIABLE;
                bind = p_identifier;
                name = p_identifier->name;
                source_function = p_function;
                script_node_id = p_function->script_node_id;
            }
        };

        SuiteNode* parent_block = nullptr;
        Vector<Node*> statements;

        Local empty;
        Vector<Local> locals;
        HashMap<StringName, int> locals_indices;
        HashMap<uint64_t, StringName> aliases;

        FunctionNode* parent_function = nullptr;
        IfNode* parent_if = nullptr;

        bool has_return = false;
        bool has_continue = false;
        bool has_unreachable_code = false;
        bool is_in_loop = false;

        bool has_local(const StringName& p_name) const;
        void add_local(const Local& p_local);
        const Local& get_local(const StringName& p_name) const;

        template<typename T> void add_local(T* p_local, FunctionNode* p_source_function) {
            locals_indices[p_local->identifier->name] = locals.size();
            locals.push_back(Local(p_local, p_source_function));
        }

        bool has_alias(const Ref<OScriptNodePin>& p_pin);
        void add_alias(const Ref<OScriptNodePin>& p_output, const StringName& p_alias);
        StringName get_alias(const Ref<OScriptNodePin>& p_pin) const;

        static uint64_t create_alias_key(const Ref<OScriptNodePin>& p_pin);

        SuiteNode() { type = SUITE; }
    };

    struct TernaryOpNode : ExpressionNode {
        // Only one ternary operation exists, so no abstraction here.
        ExpressionNode* condition = nullptr;
        ExpressionNode* true_expr = nullptr;
        ExpressionNode* false_expr = nullptr;

        TernaryOpNode() { type = TERNARY_OPERATOR; }
    };

    struct TypeNode : Node {
        Vector<IdentifierNode*> type_chain;
        Vector<TypeNode*> container_types;

        TypeNode* get_container_type_or_null(int p_index) const;

        TypeNode() { type = TYPE; }
    };

    struct TypeTestNode : ExpressionNode {
        ExpressionNode* operand = nullptr;
        TypeNode* test_type = nullptr;
        DataType test_datatype;

        TypeTestNode() { type = TYPE_TEST; }
    };

    struct UnaryOpNode : ExpressionNode {
        enum OpType {
            OP_POSITIVE,
            OP_NEGATIVE,
            OP_COMPLEMENT,
            OP_LOGIC_NOT,
        };

        OpType operation = OP_POSITIVE;
        Variant::Operator variant_op = Variant::OP_MAX;
        ExpressionNode* operand = nullptr;

        UnaryOpNode() {
            type = UNARY_OPERATOR;
        }
    };

    struct WhileNode : Node {
        ExpressionNode* condition = nullptr;
        SuiteNode* loop = nullptr;

        WhileNode() { type = WHILE; }
    };

    typedef bool (OScriptParser::*AnnotationAction)(AnnotationNode* p_node, Node* p_target, ClassNode* p_class);

    struct AnnotationInfo {
        enum TargetKind {
            NONE = 0,
            SCRIPT = 1 << 0,
            CLASS = 1 << 1,
            VARIABLE = 1 << 2,
            CONSTANT = 1 << 3,
            SIGNAL = 1 << 4,
            FUNCTION = 1 << 5,
            STATEMENT = 1 << 6,
            STANDALONE = 1 << 7,
            CLASS_LEVEL = CLASS | VARIABLE | CONSTANT | SIGNAL | FUNCTION,
        };

        uint32_t target_kind = 0;
        AnnotationAction apply = nullptr;
        MethodInfo info;
    };

}
#endif // ORCHESTRATOR_SCRIPT_PARSER_NODES_H