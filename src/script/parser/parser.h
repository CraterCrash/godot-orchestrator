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
#ifndef ORCHESTRATOR_SCRIPT_PARSER_H
#define ORCHESTRATOR_SCRIPT_PARSER_H

#include "orchestration/orchestration.h"
#include "script/nodes/script_nodes.h"
#include "script/parser/parser_nodes.h"
#include "script/parser/function_analyzer.h"
#include "script/script_cache.h"
#include "script/script_source.h"
#include "script/script_warning.h"

#include <functional>

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

// todo: aligned with changes as of godot-engine 7ed0b61676
class OScriptParser {
    friend class OScriptAnalyzer;
    friend class OScriptParserRef;

public:

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

    struct ParserError {
        String message;
        int node_id = -1;
    };

    // Temporarily for the refactor to see this
    // todo: make this private again
    static HashMap<StringName, AnnotationInfo> valid_annotations;

private:

    bool use_node_convergence = true;

    ClassNode* head = nullptr;
    ClassNode* current_class = nullptr;
    SuiteNode* current_suite = nullptr;
    LambdaNode* current_lambda = nullptr;
    FunctionNode* current_function = nullptr;
    OScriptFunctionInfo function_info;

    bool in_lambda = false;
    bool lambda_ended = false; // Mark for when a lambda ends, to apply an end of statement as needed
    bool _is_tool = false;
    bool panic_mode = false;
    bool can_break = false;
    bool can_continue = false;
    String script_path;

    List<ParserError> errors;
    List<AnnotationNode*> annotation_stack;
    HashMap<String, Ref<OScriptParserRef>> depended_parsers;

    Node* node_list_head = nullptr;

    template <typename T>
    T* alloc_node() {
        T* node = memnew(T);
        node->next = node_list_head;
        node_list_head = node;
        return node;
    }

    List<OScriptNodePinId> convergence_stack;

    // StatementResult generators return this
    struct StatementResult {
        enum ControlFlow {
            CONTINUE,
            STOP,
            JUMP_TO_NODE,
            DIVERGENCE_HANDLED
        };

        // Denotes control flow handling in outer handler
        ControlFlow control_flow;

        // For continue flow
        Ref<OScriptNodePin> exit_pin;

        // For jump flow
        Ref<OScriptNode> jump_target; // Denotes node to jump to for flow jumps
        Ref<OScriptNodePin> jump_source_pin; // Denotes node to jump from exit pin
        Ref<OScriptNodePin> jump_target_pin; // Denotes node to jump to entry input pin

        // For divergence handled use cases
        struct ConvergenceInfo {
            Ref<OScriptNode> convergence_node; // The node where the current node's paths meet
            Ref<OScriptNodePin> convergence_node_pin; // The pin in the converge node where paths meet
            //Vector<Ref<OScriptNodePin>> converging_pins; // The all output pins that feed the converge node
        };

        std::optional<ConvergenceInfo> convergence_info;
    };

    using StatementHandler = std::function<StatementResult(const Ref<OScriptNode>&)>;
    // using StatementHandler = std::function<Ref<OScriptNodePin>(const Ref<OScriptNode>&)>;
    using ExpressionHandler = std::function<ExpressionNode*(const Ref<OScriptNode>&, const Ref<OScriptNodePin>&)>;

    HashMap<StringName, StatementHandler> _statement_handlers;
    HashMap<StringName, ExpressionHandler> _expression_handlers;

    template <class T, auto handler>
    void register_statement_handler() {
        _statement_handlers[T::get_class_static()] = [this](const Ref<OScriptNode>& node) -> StatementResult {
            const Ref<T> casted = node;
            if (casted.is_valid()) {
                return (this->*handler)(casted);
            }
            StatementResult result;
            result.control_flow = StatementResult::STOP;
            ERR_FAIL_V_MSG(result, "Failed to find statement handler for node " + T::get_class_static()); // NOLINT
        };
    }

    template <class T, auto handler>
    void register_expression_handler() {
        _expression_handlers[T::get_class_static()] = [this](const Ref<OScriptNode>& node, const Ref<OScriptNodePin>& pin) -> ExpressionNode* {
            const Ref<T> casted = node;
            if (casted.is_valid()) {
                return (this->*handler)(casted, pin);
            }
            ERR_FAIL_V_MSG(nullptr, "Failed to find expression handler for node " + T::get_class_static()); // NOLINT
        };
    }

    void bind_handlers();

#ifdef DEBUG_ENABLED
public:
    struct WarningDirectoryRule {
        enum Decision {
            DECISION_EXCLUDE,
            DECISION_INCLUDE,
            DECISION_MAX
        };

        String directory_path;
        Decision decision = DECISION_EXCLUDE;
    };

private:
    struct PendingWarning {
        const Node *source = nullptr;
        OScriptWarning::Code code = OScriptWarning::WARNING_MAX;
        bool treated_as_error = false;
        Vector<String> symbols;
    };

    static bool is_project_ignoring_warnings;
    static OScriptWarning::WarnLevel warning_levels[OScriptWarning::WARNING_MAX];
    static LocalVector<WarningDirectoryRule> warning_directory_rules;

    List<OScriptWarning> warnings;
    List<PendingWarning> pending_warnings;
    bool is_script_ignoring_warnings = false;
    HashSet<int> warning_ignored_nodes[OScriptWarning::WARNING_MAX];
    int warning_ignore_start_nodes[OScriptWarning::WARNING_MAX];
    HashSet<int> unsafe_nodes;
#endif // DEBUG_ENABLED

    OScriptNetKey* get_net_from_pin(const Ref<OScriptNodePin>& p_pin);

    Ref<OScriptNodePin> get_target_from_source(const Ref<OScriptNodePin>& p_source);

    bool is_break_ahead(int p_source_node_id, int p_source_pin_index, int p_target_node_id);
    bool is_break_pin(const Ref<OScriptNodePin>& p_pin);
    bool is_convergence_point_ahead(int p_target_node_id);

    StatementResult create_stop_result();
    StatementResult create_divergence_result(const Ref<OScriptNode>& p_node);
    StatementResult create_statement_result(const Ref<OScriptNode>& p_node, int p_output_index = -1);

    // Control flow semantics
    void set_coroutine();
    void set_return();
    void emit_loop_break(int p_loop_node_id);

    // Creates a unique name for the pin.
    StringName create_unique_name(const Ref<OScriptNodePin>& p_pin);
    StringName create_cached_variable_name(const Ref<OScriptNodePin>& p_pin);

    // Variables and locals
    bool has_local_variable(const StringName& p_name) const;
    void add_local_variable(IdentifierNode* p_variable, SuiteNode* p_suite_override = nullptr);
    VariableNode* create_local(const StringName& p_name, ExpressionNode* p_initializer = nullptr, SuiteNode* p_suite_override = nullptr);
    VariableNode* create_local_and_push(const StringName& p_name, ExpressionNode* p_initializer = nullptr);

    // Useful to register that a specific pin will return a cached variable by the given name.
    // This is used for passing objects constructed in earlier passes to later nodes.
    void add_pin_alias(const StringName& p_alias, const Ref<OScriptNodePin>& p_pin, SuiteNode* p_suite_override = nullptr);

    // Helper methods to create often used node cases
    LiteralNode* create_literal(const Variant& p_value);
    SubscriptNode* create_subscript_attribute(ExpressionNode* p_base, IdentifierNode* p_attribute);
    CallNode* create_func_call(ExpressionNode* p_base, const StringName& p_function); // object-based function
    CallNode* create_func_call(const StringName& p_base, const StringName& p_function); // object-based function by name
    CallNode* create_func_call(const StringName& p_function); // free function or script call
    IfNode* create_if(ExpressionNode* p_condition, const Ref<OScriptNodePin>& p_true_pin, const Ref<OScriptNodePin>& p_false_pin);
    BinaryOpNode* create_binary_op(VariantOperators::Code p_operator, ExpressionNode* p_lhs, ExpressionNode* p_rhs);

    void bind_call_func_args(CallNode* p_call_node, const Ref<OScriptNode>& p_node, int p_arg_offset = 0);

    // Adds the statement to the existing suite
    void add_statement(Node* p_statement, SuiteNode* p_override_suite = nullptr);

    // Creates a new suite/block and returns the instance
    SuiteNode* push_suite();
    // Pops the current suite/block and returns the parent
    SuiteNode* pop_suite();

    static bool register_annotation(const MethodInfo& p_info, uint32_t p_target_kinds, AnnotationAction p_apply, const Vector<Variant>& p_default_arguments = Vector<Variant>(), bool p_is_vararg = false);

    void clear();

    void push_error(const String &p_message, const Node *p_origin = nullptr);
    #ifdef DEBUG_ENABLED
    void push_warning(const Node *p_source, OScriptWarning::Code p_code, const Vector<String> &p_symbols);
    template <typename... Symbols>
    void push_warning(const Node *p_source, OScriptWarning::Code p_code, const Symbols &...p_symbols) {
        push_warning(p_source, p_code, Vector<String>{ p_symbols... });
    }
    void apply_pending_warnings();
    void evaluate_warning_directory_rules_for_script_path();
    #endif // DEBUG_ENABLED

    // todo: enhancements
    //      1. Select node should allow for adding new operands when not bool, see UE implementation.
    //      2. A branch with no true/false nodes should be optimized out. Right now its left in as:
    //              ...
    //              21: jump-if-not const(false) to 26
    //              24: jump 26
    //              26: == END ==
    //

    // Inlines expressions as possible, should be used for expression nodes or arguments
    ExpressionNode* resolve_input(const Ref<OScriptNodePin>& p_pin);
    // Use for statements that need a named variable
    StringName get_term_name(const Ref<OScriptNodePin>& p_pin);

    // Expressions
    ExpressionNode* build_expression(const Ref<OScriptNodePin>& p_pin);
    ExpressionNode* build_expression(const Ref<OScriptNode>& p_node, int p_input_index);
    ExpressionNode* build_expression(const Ref<OScriptNodePin>& p_target, const Ref<OScriptNode>& p_source_node, const Ref<OScriptNodePin>& p_source_pin);
    ExpressionNode* build_literal(const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_literal(const Variant& p_value, int p_node_id); // vars validated
    IdentifierNode* build_identifier(const StringName& p_name, SuiteNode* p_override_suite = nullptr); // vars validated
    ExpressionNode* build_self(const Ref<OScriptNodeSelf>& p_self, const Ref<OScriptNodePin>& p_pin); // var validated
    ExpressionNode* build_variable_get(const Ref<OScriptNodeVariableGet>& p_node, const Ref<OScriptNodePin>& p_pin); // var validated
    ExpressionNode* build_property_get(const Ref<OScriptNodePropertyGet>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_get_scene_tree(const Ref<OScriptNodeSceneTree>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_get_scene_node(const Ref<OScriptNodeSceneNode>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_get_singleton(const Ref<OScriptNodeEngineSingleton>& p_node, const Ref<OScriptNodePin>& p_pin); // todo: creates a variable, need to find out how to avoid it
    ExpressionNode* build_input_action(const Ref<OScriptNodeInputAction>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_constant(const Ref<OScriptNodeConstant>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_unary_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_binary_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_construct_from(const Ref<OScriptNodeComposeFrom>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_construct(const Ref<OScriptNodeCompose>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_deconstruct(const Ref<OScriptNodeDecompose>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_function_entry(const Ref<OScriptNodeFunctionEntry>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_pure_call(const Ref<OScriptNodeCallFunction>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_get_local_variable(const Ref<OScriptNodeLocalVariable>& p_node, const Ref<OScriptNodePin>& p_pin);
    ExpressionNode* build_make_dictionary(const Ref<OScriptNodeMakeDictionary>& p_node, const Ref<OScriptNodePin>& p_pin);
    ExpressionNode* build_make_array(const Ref<OScriptNodeMakeArray>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_array_get_at_index(const Ref<OScriptNodeArrayGet>& p_node, const Ref<OScriptNodePin>& p_pin);
    ExpressionNode* build_array_find_element(const Ref<OScriptNodeArrayFind>& p_node, const Ref<OScriptNodePin>& p_pin);
    ExpressionNode* build_select(const Ref<OScriptNodeSelect>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_preload(const Ref<OScriptNodePreload>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_resource_path(const Ref<OScriptNodeResourcePath>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_get_autoload(const Ref<OScriptNodeAutoload>& p_node, const Ref<OScriptNodePin>& p_pin); // vars validated
    ExpressionNode* build_dialogue_choice(const Ref<OScriptNodeDialogueChoice>& p_node, const Ref<OScriptNodePin>& p_pin);

    // todo: guard against infinite parser loops where one later node ties back to an earlier node - check how this was handled in old impl

    // Statements
    void build_statements(const Ref<OScriptNodePin>& p_source_pin, const Ref<OScriptNodePin>& p_target_pin, SuiteNode* p_suite);
    StatementResult build_statement(const Ref<OScriptNode>& p_script_node);
    StatementResult build_type_cast(const Ref<OScriptNodeTypeCast>& p_script_node); // vars validated
    StatementResult build_if(const Ref<OScriptNodeBranch>& p_script_node); // vars validated
    StatementResult build_return(const Ref<OScriptNodeFunctionResult>& p_script_node); // vars validated
    StatementResult build_variable_get_validated(const Ref<OScriptNodeVariableGet>& p_script_node); // vars validated
    StatementResult build_variable_set(const Ref<OScriptNodeVariableSet>& p_script_node); // vars validated
    StatementResult build_property_set(const Ref<OScriptNodePropertySet>& p_script_node); // vars validated
    StatementResult build_assign_local_variable(const Ref<OScriptNodeAssignLocalVariable>& p_script_node);
    StatementResult build_call_member_function(const Ref<OScriptNodeCallMemberFunction>& p_script_node); // vars validated
    StatementResult build_call_builtin_function(const Ref<OScriptNodeCallBuiltinFunction>& p_script_node); // vars validated
    StatementResult build_call_script_function(const Ref<OScriptNodeCallScriptFunction>& p_script_node); // vars_validated
    StatementResult build_call_static_function(const Ref<OScriptNodeCallStaticFunction>& p_script_node); // vars_validated
    StatementResult build_call_super(const Ref<OScriptNodeCallFunction>& p_script_node); // vars_validated
    StatementResult build_sequence(const Ref<OScriptNodeSequence>& p_script_node); // vars validated
    StatementResult build_while(const Ref<OScriptNodeWhile>& p_script_node); // vars validated
    StatementResult build_array_set(const Ref<OScriptNodeArraySet>& p_script_node);
    StatementResult build_array_clear(const Ref<OScriptNodeArrayClear>& p_script_node); // vars validated
    StatementResult build_array_append(const Ref<OScriptNodeArrayAppend>& p_script_node); // vars validated
    StatementResult build_array_add_element(const Ref<OScriptNodeArrayAddElement>& p_script_node); // vars validated
    StatementResult build_array_remove_element(const Ref<OScriptNodeArrayRemoveElement>& p_script_node); // vars validated
    StatementResult build_array_remove_index(const Ref<OScriptNodeArrayRemoveIndex>& p_script_node); // vars validated
    StatementResult build_dictionary_set_item(const Ref<OScriptNodeDictionarySet>& p_script_node); // vars validated
    StatementResult build_chance(const Ref<OScriptNodeChance>& p_script_node); // vars validated
    StatementResult build_delay(const Ref<OScriptNodeDelay>& p_script_node); // vars validated
    StatementResult build_for_loop(const Ref<OScriptNodeForLoop>& p_script_node); // vars validated
    StatementResult build_for_each(const Ref<OScriptNodeForEach>& p_script_node); // vars validated
    StatementResult build_switch(const Ref<OScriptNodeSwitch>& p_script_node);
    StatementResult build_switch_on_string(const Ref<OScriptNodeSwitchString>& p_script_node);
    StatementResult build_switch_on_integer(const Ref<OScriptNodeSwitchInteger>& p_script_node);
    StatementResult build_switch_on_enum(const Ref<OScriptNodeSwitchEnum>& p_script_node);
    StatementResult build_random(const Ref<OScriptNodeRandom>& p_script_node); // vars validated
    StatementResult build_instantiate_scene(const Ref<OScriptNodeInstantiateScene>& p_script_node); // vars validated
    StatementResult build_await_signal(const Ref<OScriptNodeAwaitSignal>& p_script_node);
    StatementResult build_emit_member_signal(const Ref<OScriptNodeEmitMemberSignal>& p_script_node); // vars validated
    StatementResult build_emit_signal(const Ref<OScriptNodeEmitSignal>& p_script_node); // vars validated
    StatementResult build_print_string(const Ref<OScriptNodePrintString>& p_script_node);
    StatementResult build_message_dialogue(const Ref<OScriptNodeDialogueMessage>& p_script_node);
    StatementResult build_new_object(const Ref<OScriptNodeNew>& p_script_node);
    StatementResult build_free_object(const Ref<OScriptNodeFree>& p_script_node);

    // Program Logic
    ClassNode* build_class(Orchestration* p_orchestration);
    VariableNode* build_variable(const Ref<OScriptVariable>& p_variable);
    SignalNode* build_signal(const Ref<OScriptSignal>& p_signal);
    FunctionNode* build_function(const Ref<OScriptFunction>& p_function, const Ref<OScriptGraph>& p_graph);
    ParameterNode* build_parameter(const PropertyInfo& p_property);
    TypeNode* build_type(const PropertyInfo& p_property);
    SuiteNode* build_suite(const String& p_name, const Ref<OScriptNodePin>& p_source_pin, SuiteNode* p_suite = nullptr);

    // Annotations
    template <PropertyHint t_hint, Variant::Type t_type>
    bool export_annotations(AnnotationNode* p_annotation, Node* p_target, ClassNode* p_class);

public:
    Error parse(Orchestration* p_orchestration, const String& p_script_path);
    Error parse(const OScriptSource& p_source, const String& p_script_path);

    ClassNode* get_tree() const { return head; }
    bool is_tool() const { return _is_tool; }

    Ref<OScriptParserRef> get_depended_parser_for(const String &p_path);
    const HashMap<String, Ref<OScriptParserRef>> &get_depended_parsers();

    ClassNode* find_class(const String& p_qualified_name) const;
    bool has_class(const ClassNode* p_class) const;

    const List<ParserError>& get_errors() const { return errors; }
    #ifdef DEBUG_ENABLED
    const List<OScriptWarning>& get_warnings() const { return warnings; }
    #endif

    static Variant::Type get_builtin_type(const StringName& p_type);

    OScriptParser();
    ~OScriptParser();
};


#endif // ORCHESTRATOR_SCRIPT_PARSER_H