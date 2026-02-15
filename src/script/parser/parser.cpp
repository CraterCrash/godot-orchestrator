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
#include "script/parser/parser.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "core/godot/object/class_db.h"
#include "orchestration/serialization/binary/binary_parser.h"
#include "orchestration/serialization/text/text_parser.h"
#include "script/nodes/flow_control/branch.h"
#include "script/nodes/functions/call_parent_function.h"
#include "script/nodes/functions/event.h"
#include "script/parser/function_analyzer.h"
#include "script/script.h"
#include "script/script_server.h"

#include <functional>
#include <ranges>
#include <string>

#include <godot_cpp/classes/resource_loader.hpp>

#ifdef DEBUG_ENABLED
bool OScriptParser::is_project_ignoring_warnings = false;
OScriptWarning::WarnLevel OScriptParser::warning_levels[OScriptWarning::WARNING_MAX];
LocalVector<OScriptParser::WarningDirectoryRule> OScriptParser::warning_directory_rules;
#endif

HashMap<StringName, OScriptParser::AnnotationInfo> OScriptParser::valid_annotations;

static StringName _find_narrowest_native_or_global_class(const OScriptParser::DataType& p_type) {
    switch (p_type.kind) {
        case OScriptParser::DataType::NATIVE: {
            if (p_type.is_meta_type) {
                return Object::get_class_static(); // `OScriptNativeClass` is not an exposed class.
            }
            return p_type.native_type;
        }
        case OScriptParser::DataType::SCRIPT: {
            Ref<Script> script;
            if (p_type.script_type.is_valid()) {
                script = p_type.script_type;
            } else {
                script = ResourceLoader::get_singleton()->load(p_type.script_path, StringName("Script"));
            }

            if (p_type.is_meta_type) {
                return script.is_valid() ? StringName(script->get_class()) : Script::get_class_static();
            }
            if (script.is_null()) {
                return p_type.native_type;
            }
            if (script->get_global_name() != StringName()) {
                return script->get_global_name();
            }

            Ref<Script> base_script = script->get_base_script();
            if (base_script.is_null()) {
                return script->get_instance_base_type();
            }

            OScriptParser::DataType base_type;
            base_type.kind = OScriptParser::DataType::SCRIPT;
            base_type.builtin_type = Variant::OBJECT;
            base_type.native_type = base_script->get_instance_base_type();
            base_type.script_type = base_script;
            base_type.script_path = base_script->get_path();

            return _find_narrowest_native_or_global_class(base_type);
        }
        case OScriptParser::DataType::CLASS: {
            if (p_type.is_meta_type) {
                return OScript::get_class_static();
            }
            if (p_type.class_type == nullptr) {
                return p_type.native_type;
            }
            if (p_type.class_type->get_global_name() != StringName()) {
                return p_type.class_type->get_global_name();
            }
            return _find_narrowest_native_or_global_class(p_type.class_type->base_type);
        }
        default: {
            ERR_FAIL_V(StringName());
        }
    }
}

static String _get_annotation_error_string(const StringName& p_annotation_name, const Vector<Variant::Type>& p_expected_types, const OScriptParser::DataType& p_provided_type) {
    Vector<String> types;
    for (int i = 0; i < p_expected_types.size(); i++) {
        const Variant::Type& type = p_expected_types[i];
        types.push_back(Variant::get_type_name(type));
        types.push_back("Array[" + Variant::get_type_name(type) + "]");
        switch (type) {
            case Variant::INT: {
                types.push_back("PackedByteArray");
                types.push_back("PackedInt32Array");
                types.push_back("PackedInt64Array");
                break;
            }
            case Variant::FLOAT: {
                types.push_back("PackedFloat32Array");
                types.push_back("PackedFloat64Array");
                break;
            }
            case Variant::STRING: {
                types.push_back("PackedStringArray");
                break;
            }
            case Variant::VECTOR2: {
                types.push_back("PackedVector2Array");
                break;
            }
            case Variant::VECTOR3: {
                types.push_back("PackedVector3Array");
                break;
            }
            case Variant::COLOR: {
                types.push_back("PackedColorArray");
                break;
            }
            case Variant::VECTOR4: {
                types.push_back("PackedVector4Array");
                break;
            }
            default: {
                break;
            }
        }
    }

    String string;
    if (types.size() == 1) {
        string = StringUtils::quote(types[0]);
    } else if (types.size() == 2) {
        string = StringUtils::quote(types[0]) + " or " + StringUtils::quote(types[1]);
    } else if (types.size() >= 3) {
        string = StringUtils::quote(types[0]);
        for (int i = 1; i < types.size() - 1; i++) {
            string += ", " + StringUtils::quote(types[i]);
        }
        string += ", or " + StringUtils::quote(types[types.size() - 1]);
    }

    return vformat(R"("%s" annotation requires a variable of type %s, but type "%s" was given instead.)", p_annotation_name, string, p_provided_type.to_string());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptParser

void OScriptParser::bind_handlers() {
    // Register all statement handlers
    // clang-format off
    register_statement_handler<OScriptNodeBranch,                   &OScriptParser::build_if>();
    register_statement_handler<OScriptNodeTypeCast,                 &OScriptParser::build_type_cast>();
    register_statement_handler<OScriptNodeFunctionResult,           &OScriptParser::build_return>();
    register_statement_handler<OScriptNodeVariableGet,              &OScriptParser::build_variable_get_validated>();
    register_statement_handler<OScriptNodeVariableSet,              &OScriptParser::build_variable_set>();
    register_statement_handler<OScriptNodePropertySet,              &OScriptParser::build_property_set>();
    register_statement_handler<OScriptNodeAssignLocalVariable,      &OScriptParser::build_assign_local_variable>();
    register_statement_handler<OScriptNodeCallMemberFunction,       &OScriptParser::build_call_member_function>();
    register_statement_handler<OScriptNodeCallBuiltinFunction,      &OScriptParser::build_call_builtin_function>();
    register_statement_handler<OScriptNodeCallScriptFunction,       &OScriptParser::build_call_script_function>();
    register_statement_handler<OScriptNodeCallStaticFunction,       &OScriptParser::build_call_static_function>();
    register_statement_handler<OScriptNodeSequence,                 &OScriptParser::build_sequence>();
    register_statement_handler<OScriptNodeWhile,                    &OScriptParser::build_while>();
    register_statement_handler<OScriptNodeArraySet,                 &OScriptParser::build_array_set>();
    register_statement_handler<OScriptNodeArrayClear,               &OScriptParser::build_array_clear>();
    register_statement_handler<OScriptNodeArrayAppend,              &OScriptParser::build_array_append>();
    register_statement_handler<OScriptNodeArrayAddElement,          &OScriptParser::build_array_add_element>();
    register_statement_handler<OScriptNodeArrayRemoveElement,       &OScriptParser::build_array_remove_element>();
    register_statement_handler<OScriptNodeArrayRemoveIndex,         &OScriptParser::build_array_remove_index>();
    register_statement_handler<OScriptNodeDictionarySet,            &OScriptParser::build_dictionary_set_item>();
    register_statement_handler<OScriptNodeChance,                   &OScriptParser::build_chance>();
    register_statement_handler<OScriptNodeDelay,                    &OScriptParser::build_delay>();
    register_statement_handler<OScriptNodeForLoop,                  &OScriptParser::build_for_loop>();
    register_statement_handler<OScriptNodeForEach,                  &OScriptParser::build_for_each>();
    register_statement_handler<OScriptNodeSwitch,                   &OScriptParser::build_switch>();
    register_statement_handler<OScriptNodeSwitchString,             &OScriptParser::build_switch_on_string>();
    register_statement_handler<OScriptNodeSwitchInteger,            &OScriptParser::build_switch_on_integer>();
    register_statement_handler<OScriptNodeSwitchEnum,               &OScriptParser::build_switch_on_enum>();
    register_statement_handler<OScriptNodeRandom,                   &OScriptParser::build_random>();
    register_statement_handler<OScriptNodeInstantiateScene,         &OScriptParser::build_instantiate_scene>();
    register_statement_handler<OScriptNodeAwaitSignal,              &OScriptParser::build_await_signal>();
    register_statement_handler<OScriptNodeEmitMemberSignal,         &OScriptParser::build_emit_member_signal>();
    register_statement_handler<OScriptNodeEmitSignal,               &OScriptParser::build_emit_signal>();
    register_statement_handler<OScriptNodePrintString,              &OScriptParser::build_print_string>();
    register_statement_handler<OScriptNodeDialogueMessage,          &OScriptParser::build_message_dialogue>();
    register_statement_handler<OScriptNodeNew,                      &OScriptParser::build_new_object>();
    register_statement_handler<OScriptNodeFree,                     &OScriptParser::build_free_object>();
    register_statement_handler<OScriptNodeCallParentScriptFunction, &OScriptParser::build_call_super>();
    register_statement_handler<OScriptNodeCallParentMemberFunction, &OScriptParser::build_call_super>();
    // clang-format on

    // Register all expression handlers
    // clang-format off
    register_expression_handler<OScriptNodeSelf,                &OScriptParser::build_self>();
    register_expression_handler<OScriptNodeVariableGet,         &OScriptParser::build_variable_get>();
    register_expression_handler<OScriptNodePropertyGet,         &OScriptParser::build_property_get>();
    register_expression_handler<OScriptNodeSceneTree,           &OScriptParser::build_get_scene_tree>();
    register_expression_handler<OScriptNodeSceneNode,           &OScriptParser::build_get_scene_node>();
    register_expression_handler<OScriptNodeEngineSingleton,     &OScriptParser::build_get_singleton>();
    register_expression_handler<OScriptNodeInputAction,         &OScriptParser::build_input_action>();
    register_expression_handler<OScriptNodeClassConstant,       &OScriptParser::build_constant>();
    register_expression_handler<OScriptNodeGlobalConstant,      &OScriptParser::build_constant>();
    register_expression_handler<OScriptNodeMathConstant,        &OScriptParser::build_constant>();
    register_expression_handler<OScriptNodeSingletonConstant,   &OScriptParser::build_constant>();
    register_expression_handler<OScriptNodeTypeConstant,        &OScriptParser::build_constant>();
    register_expression_handler<OScriptNodeOperator,            &OScriptParser::build_operator>();
    register_expression_handler<OScriptNodeComposeFrom,         &OScriptParser::build_construct_from>();
    register_expression_handler<OScriptNodeCompose,             &OScriptParser::build_construct>();
    register_expression_handler<OScriptNodeDecompose,           &OScriptParser::build_deconstruct>();
    register_expression_handler<OScriptNodeFunctionEntry,       &OScriptParser::build_function_entry>();
    register_expression_handler<OScriptNodeCallMemberFunction,  &OScriptParser::build_pure_call>();
    register_expression_handler<OScriptNodeCallBuiltinFunction, &OScriptParser::build_pure_call>();
    register_expression_handler<OScriptNodeCallScriptFunction,  &OScriptParser::build_pure_call>();
    register_expression_handler<OScriptNodeCallStaticFunction,  &OScriptParser::build_pure_call>();
    register_expression_handler<OScriptNodeLocalVariable,       &OScriptParser::build_get_local_variable>();
    register_expression_handler<OScriptNodeMakeDictionary,      &OScriptParser::build_make_dictionary>();
    register_expression_handler<OScriptNodeMakeArray,           &OScriptParser::build_make_array>();
    register_expression_handler<OScriptNodeArrayGet,            &OScriptParser::build_array_get_at_index>();
    register_expression_handler<OScriptNodeArrayFind,           &OScriptParser::build_array_find_element>();
    register_expression_handler<OScriptNodeSelect,              &OScriptParser::build_select>();
    register_expression_handler<OScriptNodePreload,             &OScriptParser::build_preload>();
    register_expression_handler<OScriptNodeResourcePath,        &OScriptParser::build_resource_path>();
    register_expression_handler<OScriptNodeAutoload,            &OScriptParser::build_get_autoload>();
    register_expression_handler<OScriptNodeDialogueChoice,      &OScriptParser::build_dialogue_choice>();
    // clang-format on
}

OScriptNetKey* OScriptParser::get_net_from_pin(const Ref<OScriptNodePin>& p_pin) {
    const OScriptNetKey pin_key = { p_pin->get_owning_node()->get_id(), p_pin->get_pin_index() };
    return function_info.net_pin_consumers.getptr(pin_key);
}

Ref<OScriptNodePin> OScriptParser::get_target_from_source(const Ref<OScriptNodePin>& p_source) {
    if (p_source.is_valid()) {
        return p_source->get_connection();
    }
    return {};
}

bool OScriptParser::is_break_ahead(int p_source_node_id, int p_source_pin_index, int p_target_node_id) {
    return function_info.is_break_source(p_source_node_id, p_source_pin_index, p_target_node_id);
}

bool OScriptParser::is_break_pin(const Ref<OScriptNodePin>& p_pin) {
    return function_info.loop_break_targets.has({ p_pin->get_owning_node()->get_id(), p_pin->get_pin_index() });
}

bool OScriptParser::is_convergence_point_ahead(int p_target_node_id) {
    if (use_node_convergence) {
        for (const KeyValue<NodeId, NodeId>& E: function_info.divergence_to_merge_point) {
            if (E.value == p_target_node_id) {
                return true;
            }
        }
    }
    return false;
}

OScriptParser::StatementResult OScriptParser::create_stop_result() {
    StatementResult result;
    result.control_flow = StatementResult::STOP;
    return result;
}

OScriptParser::StatementResult OScriptParser::create_divergence_result(const Ref<OScriptNode>& p_node) {
    if (use_node_convergence && p_node.is_valid()) {
        int script_node_id = p_node->get_id();
        if (function_info.divergence_to_merge_point.has(script_node_id)) {
            const OScriptNodePinId merge_pin_id = function_info.divergence_to_merge_pins[script_node_id];
            const Ref<OScriptNode> converge_node = p_node->get_owning_graph()->get_node(merge_pin_id.node);
            if (converge_node.is_valid()) {
                const Ref<OScriptNodePin> converge_pin = converge_node->find_pin(merge_pin_id.pin, PD_Input);
                if (converge_pin.is_valid()) {
                    StatementResult result;
                    result.control_flow = StatementResult::DIVERGENCE_HANDLED;
                    result.convergence_info = StatementResult::ConvergenceInfo {
                        .convergence_node = converge_node,
                        .convergence_node_pin = converge_pin
                    };
                    return result;
                }
            }
        }
    }

    return create_stop_result();
}

OScriptParser::StatementResult OScriptParser::create_statement_result(const Ref<OScriptNode>& p_node, int p_output_index) {
    if (p_node.is_valid()) {
        if (p_output_index == -1) {
            return create_divergence_result(p_node);
        } else {
            // Explicitly wants to continue
            const Ref<OScriptNodePin> output_pin = p_node->find_pin(p_output_index, PD_Output);
            if (output_pin.is_valid()) {
                StatementResult result;
                result.control_flow = StatementResult::CONTINUE;
                result.exit_pin = output_pin;
                return result;
            }
        }
    }

    return create_stop_result();
}

void OScriptParser::set_coroutine() {
    if (current_function) {
        current_function->is_coroutine = true;
    }
}

void OScriptParser::set_return() {
    if (current_suite) {
        current_suite->has_return = true;
    }
}

void OScriptParser::emit_loop_break(int p_loop_node_id) {
    const String break_var_name = function_info.loop_break_variables[p_loop_node_id];
    AssignmentNode* assign_break_var_true = alloc_node<AssignmentNode>();
    assign_break_var_true->assignee = build_identifier(break_var_name);
    assign_break_var_true->assigned_value = create_literal(true);
    add_statement(assign_break_var_true);

    BreakNode* break_node = alloc_node<BreakNode>();
    add_statement(break_node);
}

StringName OScriptParser::create_unique_name(const Ref<OScriptNodePin>& p_pin) {
    return vformat("node_%d_pin_%s", p_pin->get_owning_node()->get_id(), p_pin->get_pin_name());
}

StringName OScriptParser::create_cached_variable_name(const Ref<OScriptNodePin>& p_pin) {
    // const OScriptNetKey pin_key = { p_pin->get_owning_node()->get_id(), p_pin->get_pin_index() };
    // if (function_info.net_variable_allocation.has(pin_key)) {
    //     return function_info.net_variable_allocation[pin_key];
    // }

    const Ref<OScriptNode> source_node = p_pin->get_owning_node();

    // Short-circuit variable lookup
    // Since the variable should be defined in the local scope, we can reference that directly.
    if (const Ref<OScriptNodeVariable>& node = source_node; node.is_valid()) {
        return node->get_variable()->get_variable_name();
    }

    if (const Ref<OScriptNodeFunctionEntry>& node = source_node; node.is_valid()) {
        // Entry nodes should use pin name for arguments.
        return p_pin->get_pin_name();
    }

    if (const Ref<OScriptNodeLocalVariable>& node = source_node; node.is_valid()) {
        const String local_var_name = node->get_variable_name();
        if (!local_var_name.is_empty()) {
            return local_var_name;
        }
    }

    if (const Ref<OScriptNodeDialogueMessage>& node = source_node; node.is_valid()) {
        return vformat("dialogue_%d", node->get_id());
    }

    return create_unique_name(p_pin);
}


bool OScriptParser::has_local_variable(const StringName& p_name) const {
    if (current_suite) {
        return current_suite->has_local(p_name);
    }
    return false;
}

void OScriptParser::add_local_variable(IdentifierNode* p_variable, SuiteNode* p_suite_override) {
    SuiteNode* suite = p_suite_override ? p_suite_override : current_suite;
    if (!suite) {
        push_error(R"(Cannot add a local variable when no block currently exists.)", p_variable);
        return;
    }
    suite->add_local(SuiteNode::Local(p_variable, current_function));
}

OScriptParser::VariableNode* OScriptParser::create_local(const StringName& p_name, ExpressionNode* p_initializer, SuiteNode* p_suite_override) {
    SuiteNode* suite = p_suite_override ? p_suite_override : current_suite;

    if (!suite) {
        push_error(vformat(R"(Cannot create a local named "%s" when no block currently exists.)", p_name));
        return nullptr;
    }

    // After the identifier is created, it checks whether it exists in the current suite by name.
    // If the does exist, the identifier source information is updated to reflect that.
    IdentifierNode* identifier = build_identifier(p_name, suite);

    VariableNode* variable = alloc_node<VariableNode>();
    variable->identifier = identifier;
    variable->export_info.name = p_name;
    if (p_initializer != nullptr) {
        variable->initializer = p_initializer;
        variable->assignments++;
    }

    return variable;
}

OScriptParser::VariableNode* OScriptParser::create_local_and_push(const StringName& p_name, ExpressionNode* p_initializer) {
    VariableNode* variable = create_local(p_name, p_initializer);
    add_statement(variable);

    return variable;
}

void OScriptParser::add_pin_alias(const StringName& p_alias, const Ref<OScriptNodePin>& p_pin, SuiteNode* p_suite_override) {
    SuiteNode* suite = p_suite_override ? p_suite_override : current_suite;
    if (!suite) {
        push_error(vformat(R"(Cannot create a pin alias named "%s" when no block currently exists.)", p_alias));
        return;
    }
    suite->add_alias(p_pin, p_alias);
}

OScriptParser::LiteralNode* OScriptParser::create_literal(const Variant& p_value) {
    LiteralNode* literal = alloc_node<LiteralNode>();
    literal->value = p_value;
    return literal;
}

OScriptParser::SubscriptNode* OScriptParser::create_subscript_attribute(ExpressionNode* p_base, IdentifierNode* p_attribute) {
    SubscriptNode* subscript = alloc_node<SubscriptNode>();
    subscript->base = p_base;
    subscript->attribute = p_attribute;
    subscript->is_attribute = true;
    return subscript;
}

OScriptParser::CallNode* OScriptParser::create_func_call(ExpressionNode* p_base, const StringName& p_function) {
    SubscriptNode* subscript = create_subscript_attribute(p_base, build_identifier(p_function));

    CallNode* call_node = alloc_node<CallNode>();
    call_node->callee = subscript;
    call_node->function_name = subscript->attribute->name;
    return call_node;
}

OScriptParser::CallNode* OScriptParser::create_func_call(const StringName& p_base, const StringName& p_function) {
    return create_func_call(build_identifier(p_base), p_function);
}

OScriptParser::CallNode* OScriptParser::create_func_call(const StringName& p_function) {
    CallNode* call_node = alloc_node<CallNode>();
    call_node->callee = build_identifier(p_function);
    call_node->function_name = p_function;
    return call_node;
}

OScriptParser::IfNode* OScriptParser::create_if(ExpressionNode* p_condition, const Ref<OScriptNodePin>& p_true_pin, const Ref<OScriptNodePin>& p_false_pin) {
    // Branch based on nullness.
    IfNode* if_node = alloc_node<IfNode>();
    if_node->condition = p_condition;

    // Process true flow
    if_node->true_block = build_suite("branch true", p_true_pin);
    if (if_node->true_block) {
        if_node->true_block->parent_if = if_node;
        if (if_node->true_block->has_continue) {
            current_suite->has_continue = true;
        }
    }

    // Process false flow
    if_node->false_block = build_suite("branch false", p_false_pin);
    if (if_node->false_block) {
        if_node->false_block->parent_if = if_node;
        if (if_node->false_block->has_continue) {
            current_suite->has_continue = true;
        }
    }

    if (if_node->true_block && if_node->true_block->statements.is_empty()) {
        PassNode* pass = alloc_node<PassNode>();
        if_node->true_block->statements.push_back(pass);
    }

    if (if_node->false_block && if_node->false_block->statements.is_empty()) {
        PassNode* pass = alloc_node<PassNode>();
        if_node->false_block->statements.push_back(pass);
    }

    // If structures are required to always have a true block while the false block is optional.
    // In the event that the code generates only a false block, this requirement can be solved by applying
    // a logical NOT operator to the condition and swapping the true/false code paths.
    if (if_node->true_block == nullptr && if_node->false_block != nullptr) {
        UnaryOpNode* not_op = alloc_node<UnaryOpNode>();
        not_op->operation = UnaryOpNode::OP_LOGIC_NOT;
        not_op->variant_op = Variant::OP_NOT;
        not_op->operand = p_condition;

        if_node->condition = not_op;
        if_node->true_block = if_node->false_block;
        if_node->false_block = nullptr;
    }

    if (!if_node->true_block && !if_node->false_block) {
        if_node->true_block = alloc_node<SuiteNode>();
        if_node->true_block->parent_block = current_suite;
        if_node->true_block->parent_function = current_function;
        if_node->true_block->parent_if = if_node;
    }

    // Handle return control flow
    if (if_node->true_block && if_node->false_block && if_node->false_block->has_return && if_node->true_block->has_return) {
        current_suite->has_return = true;
    }

    return if_node;
}

OScriptParser::BinaryOpNode* OScriptParser::create_binary_op(VariantOperators::Code p_operator, ExpressionNode* p_lhs, ExpressionNode* p_rhs) {
    BinaryOpNode* binary_op_node = alloc_node<BinaryOpNode>();
    binary_op_node->variant_op = VariantOperators::to_engine(p_operator);
    binary_op_node->left_operand = p_lhs;
    binary_op_node->right_operand = p_rhs;

    switch (p_operator) {
        case VariantOperators::OP_ADD:
            binary_op_node->operation = BinaryOpNode::OP_ADDITION;
            break;
        case VariantOperators::OP_SUBTRACT:
            binary_op_node->operation = BinaryOpNode::OP_SUBTRACTION;
            break;
        case VariantOperators::OP_MULTIPLY:
            binary_op_node->operation = BinaryOpNode::OP_MULTIPLICATION;
            break;
        case VariantOperators::OP_DIVIDE:
            binary_op_node->operation = BinaryOpNode::OP_DIVISION;
            break;
        case VariantOperators::OP_MODULE:
            binary_op_node->operation = BinaryOpNode::OP_MODULO;
            break;
        case VariantOperators::OP_POWER:
            binary_op_node->operation = BinaryOpNode::OP_POWER;
            break;
        case VariantOperators::OP_SHIFT_LEFT:
            binary_op_node->operation = BinaryOpNode::OP_BIT_LEFT_SHIFT;
            break;
        case VariantOperators::OP_SHIFT_RIGHT:
            binary_op_node->operation = BinaryOpNode::OP_BIT_RIGHT_SHIFT;
            break;
        case VariantOperators::OP_BIT_AND:
            binary_op_node->operation = BinaryOpNode::OP_BIT_AND;
            break;
        case VariantOperators::OP_BIT_OR:
            binary_op_node->operation = BinaryOpNode::OP_BIT_OR;
            break;
        case VariantOperators::OP_BIT_XOR:
            binary_op_node->operation = BinaryOpNode::OP_BIT_XOR;
            break;
        case VariantOperators::OP_AND:
            binary_op_node->operation = BinaryOpNode::OP_LOGIC_AND;
            break;
        case VariantOperators::OP_OR:
            binary_op_node->operation = BinaryOpNode::OP_LOGIC_OR;
            break;
        case VariantOperators::OP_IN:
            binary_op_node->operation = BinaryOpNode::OP_CONTENT_TEST;
            break;
        case VariantOperators::OP_EQUAL:
            binary_op_node->operation = BinaryOpNode::OP_COMP_EQUAL;
            break;
        case VariantOperators::OP_NOT_EQUAL:
            binary_op_node->operation = BinaryOpNode::OP_COMP_NOT_EQUAL;
            break;
        case VariantOperators::OP_LESS:
            binary_op_node->operation = BinaryOpNode::OP_COMP_LESS;
            break;
        case VariantOperators::OP_LESS_EQUAL:
            binary_op_node->operation = BinaryOpNode::OP_COMP_LESS_EQUAL;
            break;
        case VariantOperators::OP_GREATER:
            binary_op_node->operation = BinaryOpNode::OP_COMP_GREATER;
            break;
        case VariantOperators::OP_GREATER_EQUAL:
            binary_op_node->operation = BinaryOpNode::OP_COMP_GREATER_EQUAL;
            break;
        default:
            ERR_FAIL_V_MSG(nullptr, "Unsupported binary operator " + itos(p_operator));
    }

    return binary_op_node;
}

void OScriptParser::bind_call_func_args(CallNode* p_call_node, const Ref<OScriptNode>& p_node, int p_arg_offset) {
    // todo: use MethodInfo flags
    //
    // In an ideal world, we would use MethodInfo here to check for METHOD_FLAG_VARARG to indicate whether
    // we would trigger the use of the variadic argument logic; however, older nodes may not have had
    // this flag, so relying on it for older scripts will fail.
    //
    // For now, we'll base the variadic nature of the arguments on the pins themselves for all methods. It
    // should be overwhelmingly safe to do.
    //
    // In the future, we can consider adding a new warning pass to the parser that would compare the MethodInfo
    // with Godot's current MethodInfo, and have a way to update the MethodInfo.
    //
    const Vector<Ref<OScriptNodePin>> inputs = p_node->find_pins(PD_Input);
    for (size_t i = p_arg_offset; i < inputs.size(); i++) {
        const Ref<OScriptNodePin>& input = inputs[i];
        ERR_CONTINUE(input.is_null());
        if (!input->is_execution()) {
            p_call_node->arguments.push_back(resolve_input(input));
        }
    }
}

void OScriptParser::add_statement(Node* p_statement, SuiteNode* p_override_suite) {
    SuiteNode* suite = p_override_suite ? p_override_suite : current_suite;

    if (!suite) {
        push_error(R"(Cannot add statement when no block currently exists.)", p_statement);
        return;
    }

    suite->statements.push_back(p_statement);

    switch (p_statement->type) {
        case Node::Type::CONSTANT: {
            ConstantNode* constant = static_cast<ConstantNode*>(p_statement);
            const SuiteNode::Local& local = suite->get_local(constant->identifier->name);
            if (local.type != SuiteNode::Local::UNDEFINED) {
                const String name = local.type == SuiteNode::Local::CONSTANT ? "constant" : "variable";
                push_error(vformat(R"(There is already a %s named "%s" in this current scope.)",
                    name, constant->identifier->name), constant->identifier);
            }
            suite->add_local(constant, current_function);
            break;
        }
        case Node::Type::VARIABLE: {
            VariableNode* variable = static_cast<VariableNode*>(p_statement);
            const SuiteNode::Local& local = suite->get_local(variable->identifier->name);
            if (local.type != SuiteNode::Local::UNDEFINED) {
                push_error(vformat(R"(There is already a %s named "%s" declared in the current scope.)",
                    local.get_name(), variable->identifier->name), variable->identifier);
            }
            suite->add_local(variable, current_function);
            break;
        }
        default: {
            break; // no-op
        }
    }
}

OScriptParser::SuiteNode* OScriptParser::push_suite() {
    SuiteNode* next = alloc_node<SuiteNode>();
    next->parent_block = current_suite;
    next->parent_function = current_function;

    current_suite = next;
    return current_suite;
}

OScriptParser::SuiteNode* OScriptParser::pop_suite() {
    current_suite = current_suite->parent_block;
    return current_suite;
}

bool OScriptParser::register_annotation(const MethodInfo& p_info, uint32_t p_target_kinds, AnnotationAction p_apply, const Vector<Variant>& p_default_arguments, bool p_is_vararg) {
    ERR_FAIL_COND_V_MSG(valid_annotations.has(p_info.name), false, vformat(R"(Annotation "%s" already registered.)", p_info.name));

    AnnotationInfo new_annotation;
    new_annotation.info = p_info;
    for (const Variant& item : p_default_arguments) {
        new_annotation.info.default_arguments.push_back(item);
    }
    if (p_is_vararg) {
        new_annotation.info.flags |= METHOD_FLAG_VARARG;
    }
    new_annotation.apply = p_apply;
    new_annotation.target_kind = p_target_kinds;

    valid_annotations[p_info.name] = new_annotation;
    return true;
}

void OScriptParser::clear() {
    OScriptParser tmp;
    tmp = *this;
    *this = OScriptParser();

    // After the above reset, we need to rebind this in handlers
    bind_handlers();
}

void OScriptParser::push_error(const String &p_message, const Node *p_origin) {
    // TODO: Improve error reporting by pointing at source code.
    // TODO: Errors might point at more than one place at once (e.g. show previous declaration).
    panic_mode = true;
    // TODO: Improve positional information.
    if (p_origin == nullptr) {
        errors.push_back({ p_message, -1 });
    } else {
        errors.push_back({ p_message, p_origin->script_node_id });
    }
}

#ifdef DEBUG_ENABLED
void OScriptParser::push_warning(const Node *p_source, OScriptWarning::Code p_code, const Vector<String> &p_symbols) {
    ERR_FAIL_NULL(p_source);
    ERR_FAIL_INDEX(p_code, OScriptWarning::WARNING_MAX);

    if (is_project_ignoring_warnings || is_script_ignoring_warnings) {
        return;
    }

    const OScriptWarning::WarnLevel warn_level = warning_levels[p_code];
    if (warn_level == OScriptWarning::IGNORE) {
        return;
    }

    PendingWarning pw;
    pw.source = p_source;
    pw.code = p_code;
    pw.treated_as_error = warn_level == OScriptWarning::ERROR;
    pw.symbols = p_symbols;

    pending_warnings.push_back(pw);
}

void OScriptParser::apply_pending_warnings() {
    for (const PendingWarning &pw : pending_warnings) {
        if (warning_ignored_nodes[pw.code].has(pw.source->script_node_id)) {
            continue;
        }
        if (warning_ignore_start_nodes[pw.code] <= pw.source->script_node_id) {
            continue;
        }

        OScriptWarning warning;
        warning.code = pw.code;
        warning.symbols = pw.symbols;
        warning.node = pw.source->script_node_id;

        if (pw.treated_as_error) {
            push_error(warning.get_message() + String(" (Warning treated as error.)"), pw.source);
            continue;
        }

        List<OScriptWarning>::Element *before = nullptr;
        for (List<OScriptWarning>::Element *E = warnings.front(); E; E = E->next()) {
            if (E->get().node > warning.node) {
                break;
            }
            before = E;
        }
        if (before) {
            warnings.insert_after(before, warning);
        } else {
            warnings.push_front(warning);
        }
    }

    pending_warnings.clear();
}

void OScriptParser::evaluate_warning_directory_rules_for_script_path() {
    is_script_ignoring_warnings = false;
    for (const WarningDirectoryRule &rule : warning_directory_rules) {
        if (script_path.begins_with(rule.directory_path)) {
            switch (rule.decision) {
                case WarningDirectoryRule::DECISION_EXCLUDE:
                    is_script_ignoring_warnings = true;
                    return; // Stop checking rules.
                case WarningDirectoryRule::DECISION_INCLUDE:
                    is_script_ignoring_warnings = false;
                    return; // Stop checking rules.
                case WarningDirectoryRule::DECISION_MAX:
                    return; // Unreachable.
            }
        }
    }
}

#endif

OScriptParser::ExpressionNode* OScriptParser::resolve_input(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null(), create_literal(Variant()));
    ERR_FAIL_COND_V(p_pin->is_execution(), create_literal(Variant()));

    if (!p_pin->has_any_connections()) {
        return build_literal(p_pin);
    }

    const Ref<OScriptNodePin> source_pin = p_pin->get_connections()[0];
    const Ref<OScriptNode> source_node = source_pin->get_owning_node();

    // Check object identity for passthroughs
    if (current_suite) {
        if (current_suite->has_alias(source_pin)) {
            const String alias = current_suite->get_alias(source_pin);
            if (!alias.is_empty()) {
                // Check if an output pin explicitly wants a self reference
                if (alias == "self") {
                    SelfNode* self = alloc_node<SelfNode>();
                    self->script_node_id = source_node->get_id();
                    self->current_class = current_class;
                    return self;
                }

                // Use default identifier resolution
                return build_identifier(alias);
            }
        }
    }

    // For control flow nodes, return identifier to cached variable
    for (const Ref<OScriptNodePin>& input : source_node->find_pins(PD_Input)) {
        if (input.is_valid() && input->is_execution()) {
            const String cache_name = create_cached_variable_name(source_pin);
            return build_identifier(cache_name);
        }
    }

    // Pure nodes always build an expression without caching
    if (source_node->is_pure()) {
        return build_expression(p_pin, source_node, source_pin);
    }

    // For non-pure nodes, cache in a variable
    const String cache_name = create_cached_variable_name(source_pin);
    if (current_suite && !current_suite->has_local(cache_name)) {
        ExpressionNode* expression = build_expression(p_pin, source_node, source_pin);

        VariableNode* local = alloc_node<VariableNode>();
        local->identifier = build_identifier(cache_name);
        local->initializer = expression;
        local->export_info.name = cache_name;
        current_suite->add_local(local, current_function);
    }

    return build_identifier(cache_name);
}

StringName OScriptParser::get_term_name(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null(), "");

    if (!p_pin->has_any_connections()) {
        return "";
    }

    const Ref<OScriptNodePin> source_pin = p_pin->get_connections()[0];
    const Ref<OScriptNode> source_node = source_pin->get_owning_node();
    const uint64_t source_id = source_node->get_id();

    // Check for aliases
    if (current_suite) {
        const uint64_t key = (source_id << 32) | source_pin->get_pin_index();
        if (current_suite->aliases.has(key)) {
            return current_suite->aliases[key];
        }
    }

    // Get or create cached variable
    const String variable_name = create_cached_variable_name(source_pin);
    if (current_suite && !current_suite->has_local(variable_name)) {
        // Build the expression and cache it
        ExpressionNode* expression = build_expression(p_pin, source_node, source_pin);
        create_local_and_push(variable_name, expression);
    }

    return variable_name;
}

OScriptParser::ExpressionNode* OScriptParser::build_expression(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null(), nullptr);
    ERR_FAIL_COND_V(p_pin->is_execution(), nullptr);

    if (!p_pin->has_any_connections()) {
        return build_literal(p_pin);
    }

    const Ref<OScriptNodePin> source_pin = p_pin->get_connections()[0];
    const Ref<OScriptNode> source_node = source_pin->get_owning_node();

    if (current_suite) {
        uint64_t node_id = source_node->get_id();
        uint64_t key = (node_id << 32) | source_pin->get_pin_index();
        if (current_suite->aliases.has(key)) {
            return build_identifier(current_suite->aliases[key]);
        }
    }

    // Check if local variable already exists for this network path
    const String cachedVariableName = create_cached_variable_name(source_pin);

    // Control flow nodes always just return a cached identifier?
    for (const Ref<OScriptNodePin>& input : source_node->find_pins(PD_Input)) {
        if (input.is_valid() && input->is_execution()) {
            return build_identifier(cachedVariableName);
        }
    }

    if (current_suite && !current_suite->has_local(cachedVariableName) && source_node->is_pure()) {
        // Dependency node being accessed for the first time.
        // Build its expression on-demand
        ExpressionNode* expression = build_expression(p_pin, source_node, source_pin);
        if (expression) {
            expression->script_node_id = source_node->get_id();
        }

        // For nodes that are considered pure, the computed value will not be cached.
        if (source_node->is_pure()) {
            return expression;
        }

        // Store dependency node's output in a variable
        VariableNode* local_var = alloc_node<VariableNode>();
        local_var->identifier = build_identifier(cachedVariableName);
        local_var->initializer = expression;
        local_var->export_info.name = cachedVariableName;
        local_var->assignments++;
        current_suite->add_local(local_var, current_function);
    }

    return build_identifier(cachedVariableName);
}

OScriptParser::ExpressionNode* OScriptParser::build_expression(const Ref<OScriptNode>& p_node, int p_input_index) {
    return build_expression(p_node->find_pin(p_input_index, PD_Input));
}

OScriptParser::ExpressionNode* OScriptParser::build_expression(const Ref<OScriptNodePin>& p_target, const Ref<OScriptNode>& p_source_node, const Ref<OScriptNodePin>& p_source_pin) {
    const String class_name = p_source_node->get_class();

    ExpressionHandler* handler = _expression_handlers.getptr(class_name);
    if (handler != nullptr) {
        return (*handler)(p_source_node, p_source_pin);
    }

    ERR_FAIL_V_MSG(create_literal(Variant()),
        vformat("Failed to resolve pin input for node %d (%s) and pin %d (%s).",
            p_source_node->get_id(),
            p_source_node->get_class(),
            p_source_pin->get_pin_index(),
            p_source_pin->get_pin_name()));
}

OScriptParser::ExpressionNode* OScriptParser::build_literal(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null(), nullptr);
    return build_literal(p_pin->get_effective_default_value(), p_pin->get_owning_node()->get_id());
}

OScriptParser::ExpressionNode* OScriptParser::build_literal(const Variant& p_value, int p_node_id) {
    LiteralNode* literal = create_literal(p_value);
    literal->script_node_id = p_node_id;
    return literal;
}

OScriptParser::IdentifierNode* OScriptParser::build_identifier(const StringName& p_identifier, SuiteNode* p_override_suite) {
    SuiteNode* suite = p_override_suite ? p_override_suite : current_suite;

    IdentifierNode* identifier = alloc_node<IdentifierNode>();
    identifier->name = p_identifier;
    identifier->suite = suite;

    if (suite != nullptr && suite->has_local(identifier->name)) {
        const SuiteNode::Local& decl = suite->get_local(identifier->name);
        identifier->source_function = decl.source_function;
        switch (decl.type) {
            case SuiteNode::Local::CONSTANT:
                identifier->source = IdentifierNode::LOCAL_CONSTANT;
                identifier->constant_source = decl.constant;
                decl.constant->usages++;
                break;
            case SuiteNode::Local::VARIABLE:
                identifier->source = IdentifierNode::LOCAL_VARIABLE;
                identifier->variable_source = decl.variable;
                decl.variable->usages++;
                break;
            case SuiteNode::Local::PARAMETER:
                identifier->source = IdentifierNode::FUNCTION_PARAMETER;
                identifier->parameter_source = decl.parameter;
                decl.parameter->usages++;
                break;
            case SuiteNode::Local::FOR_VARIABLE:
                identifier->source = IdentifierNode::LOCAL_ITERATOR;
                identifier->bind_source = decl.bind;
                decl.bind->usages++;
                break;
            case SuiteNode::Local::PATTERN_BIND:
                identifier->source = IdentifierNode::LOCAL_BIND;
                identifier->bind_source = decl.bind;
                decl.bind->usages++;
                break;
            case SuiteNode::Local::UNDEFINED:
                ERR_FAIL_V_MSG(nullptr, "Undefined local found.");
        }
    }

    return identifier;
}

OScriptParser::ExpressionNode* OScriptParser::build_self(const Ref<OScriptNodeSelf>& p_self, const Ref<OScriptNodePin>& p_pin) {
    SelfNode* self = alloc_node<SelfNode>();
    self->script_node_id = p_self->get_id();
    self->current_class = current_class;
    return self;
}

OScriptParser::ExpressionNode* OScriptParser::build_variable_get(const Ref<OScriptNodeVariableGet>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return build_identifier(p_node->get_variable()->get_variable_name());
}

OScriptParser::ExpressionNode* OScriptParser::build_property_get(const Ref<OScriptNodePropertyGet>& p_node, const Ref<OScriptNodePin>& p_pin) {
    switch (p_node->get_call_mode()) {
        case OScriptNodeProperty::CallMode::CALL_SELF: {
            return build_identifier(p_node->find_pin(0, PD_Output)->get_pin_name());
        } break;

        case OScriptNodeProperty::CallMode::CALL_INSTANCE: {
            if (p_node->find_pin(0, PD_Input)->has_any_connections()) {
                SubscriptNode* subscript_node = alloc_node<SubscriptNode>();
                subscript_node->script_node_id = p_node->get_id();
                subscript_node->base = resolve_input(p_node->find_pin(0, PD_Input));
                subscript_node->attribute = build_identifier(p_node->get_property().name);
                subscript_node->is_attribute = true;
                return subscript_node;
            }

            return build_identifier(p_node->get_property().name);
        } break;

        case OScriptNodeProperty::CallMode::CALL_NODE_PATH: {
            CallNode* get_node = create_func_call("get_node");
            get_node->arguments.push_back(create_literal(p_node->get_node_path()));

            SubscriptNode* subscript_node = alloc_node<SubscriptNode>();
            subscript_node->script_node_id = p_node->get_id();
            subscript_node->base = get_node;
            subscript_node->attribute = build_identifier(p_node->get_property().name);
            subscript_node->is_attribute = true;
            return subscript_node;
        } break;

        default: {
            ERR_FAIL_V_MSG(nullptr, "An unexpected call mode for property get detected");
        }
    }
}

OScriptParser::ExpressionNode* OScriptParser::build_get_scene_tree(const Ref<OScriptNodeSceneTree>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return create_func_call("get_tree");
}

OScriptParser::ExpressionNode* OScriptParser::build_get_scene_node(const Ref<OScriptNodeSceneNode>& p_node, const Ref<OScriptNodePin>& p_pin) {
    // todo: see if we can fix this
    // We need to build this using this approach since validated calls with GetNodeNode are not
    // currently supported by GDExtension due to how parameter binding works :(
    CallNode* construct_node_path = create_func_call("NodePath");
    construct_node_path->arguments.push_back(create_literal(p_node->get_scene_node_path()));

    CallNode* get_node = create_func_call("get_node");
    get_node->arguments.push_back(construct_node_path);

    return get_node;

    // GetNodeNode* get_node = alloc_node<GetNodeNode>();
    // get_node->script_node_id = p_node->get_id();
    // get_node->full_path = p_node->get_scene_node_path();
    // get_node->use_dollar = false;
    // return get_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_get_singleton(const Ref<OScriptNodeEngineSingleton>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return build_identifier(p_node->get_singleton_name());
}

OScriptParser::ExpressionNode* OScriptParser::build_input_action(const Ref<OScriptNodeInputAction>& p_node, const Ref<OScriptNodePin>& p_pin) {
    String function_name;
    switch (p_node->get_action_mode()) {
        case OScriptNodeInputAction::ActionMode::AM_PRESSED:
        case OScriptNodeInputAction::ActionMode::AM_RELEASED: {
            function_name = "is_action_pressed";
            break;
        }
        case OScriptNodeInputAction::ActionMode::AM_JUST_PRESSED: {
            function_name = "is_action_just_pressed";
            break;
        }
        case OScriptNodeInputAction::ActionMode::AM_JUST_RELEASED: {
            function_name = "is_action_just_released";
            break;
        }
    };

    CallNode* call_node = create_func_call("Input", function_name);
    if (p_node->get_action_mode() == OScriptNodeInputAction::ActionMode::AM_RELEASED) {
        // Godot does not have "is_action_released" method, and they expect for you to use the NOT
        // operator for this check, so we use the unary node to handle that.
        UnaryOpNode* unary = alloc_node<UnaryOpNode>();
        unary->script_node_id = p_node->get_id();
        unary->operand = call_node->callee;
        unary->operation = UnaryOpNode::OP_LOGIC_NOT;
        unary->variant_op = Variant::Operator::OP_NOT;

        call_node->callee = unary;
    }

    call_node->arguments.push_back(create_literal(p_node->get_action_name()));

    return call_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_constant(const Ref<OScriptNodeConstant>& p_node, const Ref<OScriptNodePin>& p_pin) {
    if (const Ref<OScriptNodeTypeConstant>& node = p_node; node.is_valid()) {
        SubscriptNode* subscript = alloc_node<SubscriptNode>();
        subscript->script_node_id = p_node->get_id();
        subscript->base = build_identifier(Variant::get_type_name(node->get_type()));
        subscript->attribute = build_identifier(node->get_constant_name());
        subscript->is_attribute = true;
        return subscript;
    }
    if (const Ref<OScriptNodeGlobalConstant>& node = p_node; node.is_valid()) {
        return build_identifier(node->get_constant_name());
    }
    if (const Ref<OScriptNodeClassConstantBase>& node = p_node; node.is_valid()) {
        SubscriptNode* subscript = alloc_node<SubscriptNode>();
        subscript->script_node_id = p_node->get_id();
        subscript->base = build_identifier(node->get_constant_class_name());
        subscript->attribute = build_identifier(node->get_constant_name());
        subscript->is_attribute = true;
        return subscript;
    }
    if (const Ref<OScriptNodeMathConstant>& node = p_node; node.is_valid()) {
        return build_identifier(node->get_constant_name());
    }
    ERR_FAIL_V_MSG(nullptr, "An unknown constant node: " + p_node->get_class());
}

OScriptParser::ExpressionNode* OScriptParser::build_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin) {
    if (p_node->find_pins(PD_Input).size() == 1) {
        return build_unary_operator(p_node, p_pin);
    }
    return build_binary_operator(p_node, p_pin);
}

OScriptParser::ExpressionNode* OScriptParser::build_unary_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin) {
    UnaryOpNode* unary_op_node = alloc_node<UnaryOpNode>();
    unary_op_node->script_node_id = p_node->get_id();
    unary_op_node->operand = resolve_input(p_node->find_pin(0, PD_Input));

    switch (p_node->get_info().op) {
        case VariantOperators::OP_POSITIVE:
            unary_op_node->operation = UnaryOpNode::OP_POSITIVE;
            unary_op_node->variant_op = Variant::Operator::OP_POSITIVE;
            break;
        case VariantOperators::OP_NEGATE:
            unary_op_node->operation = UnaryOpNode::OP_NEGATIVE;
            unary_op_node->variant_op = Variant::Operator::OP_NEGATE;
            break;
        case VariantOperators::OP_BIT_NEGATE:
            unary_op_node->operation = UnaryOpNode::OP_COMPLEMENT;
            unary_op_node->variant_op = Variant::Operator::OP_BIT_NEGATE;
            break;
        case VariantOperators::OP_NOT:
            unary_op_node->operation = UnaryOpNode::OP_LOGIC_NOT;
            unary_op_node->variant_op = Variant::Operator::OP_NOT;
            break;
        default:
            ERR_FAIL_V_MSG(nullptr, "Unsupported unary operator " + itos(p_node->get_info().op));
    }

    return unary_op_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_binary_operator(const Ref<OScriptNodeOperator>& p_node, const Ref<OScriptNodePin>& p_pin) {
    ExpressionNode* lhs = resolve_input(p_node->find_pin(0, PD_Input));
    ExpressionNode* rhs = resolve_input(p_node->find_pin(1, PD_Input));
    return create_binary_op(p_node->get_info().op, lhs, rhs);
}

OScriptParser::ExpressionNode* OScriptParser::build_construct_from(const Ref<OScriptNodeComposeFrom>& p_node, const Ref<OScriptNodePin>& p_pin) {
    // Shortcut to literals for single pin values where input/output match types
    if (p_node->find_pins(PD_Input).size() == 1) {
        const Ref<OScriptNodePin> input_pin = p_node->find_pin(0, PD_Input);
        if (input_pin.is_valid() && !input_pin->has_any_connections()) {
            const Ref<OScriptNodePin> output_pin = p_node->find_pin(0, PD_Output);
            if (output_pin.is_valid() && output_pin->get_type() == input_pin->get_type()) {
                return resolve_input(input_pin);
            }
        }
    }

    CallNode* call_node = alloc_node<CallNode>();
    call_node->script_node_id = p_node->get_id();
    call_node->callee = build_identifier(Variant::get_type_name(p_node->get_target_type()));
    call_node->function_name = Variant::get_type_name(p_node->get_target_type());

    for (int i = 0; i < p_node->find_pins(PD_Input).size(); i++) {
        ExpressionNode* argument = resolve_input(p_node->find_pin(i, PD_Input));
        call_node->arguments.push_back(argument);
    }

    return call_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_construct(const Ref<OScriptNodeCompose>& p_node, const Ref<OScriptNodePin>& p_pin) {
    // Shortcut to literals for single pin values where input/output match types
    if (p_node->find_pins(PD_Input).size() == 1) {
        const Ref<OScriptNodePin> input_pin = p_node->find_pin(0, PD_Input);
        if (input_pin.is_valid() && !input_pin->has_any_connections()) {
            const Ref<OScriptNodePin> output_pin = p_node->find_pin(0, PD_Output);
            if (output_pin.is_valid() && output_pin->get_type() == input_pin->get_type()) {
                return resolve_input(input_pin);
            }
        }
    }

    CallNode* call_node = alloc_node<CallNode>();
    call_node->script_node_id = p_node->get_id();
    call_node->callee = build_identifier(Variant::get_type_name(p_node->get_type()));
    call_node->function_name = Variant::get_type_name(p_node->get_type());

    for (int i = 0; i < p_node->find_pins(PD_Input).size(); i++) {
        ExpressionNode* argument = resolve_input(p_node->find_pin(i, PD_Input));
        call_node->arguments.push_back(argument);
    }

    return call_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_deconstruct(const Ref<OScriptNodeDecompose>& p_node, const Ref<OScriptNodePin>& p_pin) {
    // Short-circuit attempt to reduce and do a direct pass of values if there is a compose followed by decompose
    const Ref<OScriptNodePin> input_pin = p_node->find_pin(0, PD_Input);
    if (input_pin.is_valid() && input_pin->has_any_connections()) {
        const Ref<OScriptNode> source_node = input_pin->get_connections()[0]->get_owning_node();

        bool reduce = false;
        if (const Ref<OScriptNodeComposeFrom>& compose_from = source_node; compose_from.is_valid()) {
            if (compose_from->get_target_type() == p_node->get_source_type()) {
                reduce = true;
            }
        }
        if (const Ref<OScriptNodeCompose>& compose = source_node; !reduce && compose.is_valid()) {
            if (compose->get_type() == p_node->get_source_type()) {
                reduce = true;
            }
        }

        if (reduce) {
            const int index = p_pin->get_pin_index();
            const Ref<OScriptNodePin> make_input_pin = source_node->find_pin(index, PD_Input);
            return resolve_input(make_input_pin);
        }
    }

    // For now this node is marked pure.
    // But it would be great if we could find a way to cache the value like below in non-pure mode.
    // The problem with non-pure mode below is that it creates a type resolution issue, IDK yet know why.
    SubscriptNode* subscript = alloc_node<SubscriptNode>();
    subscript->script_node_id = p_node->get_id();
    subscript->base = resolve_input(p_node->find_pin(0, PD_Input));
    subscript->attribute = build_identifier(p_pin->get_pin_name());
    subscript->is_attribute = true;

    return subscript;
}

OScriptParser::ExpressionNode* OScriptParser::build_function_entry(const Ref<OScriptNodeFunctionEntry>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return build_identifier(p_pin->get_pin_name());
}

OScriptParser::ExpressionNode* OScriptParser::build_pure_call(const Ref<OScriptNodeCallFunction>& p_node, const Ref<OScriptNodePin>& p_pin) {
    CallNode* call_node = alloc_node<CallNode>();
    call_node->script_node_id = p_node->get_id();

    if (const Ref<OScriptNodeCallMemberFunction>& member_func = p_node; member_func.is_valid()) {
        const MethodInfo& method = member_func->get_function();

        int argument_offset = 0;
        const Ref<OScriptNodePin> instance_pin = p_node->find_pin(0, PD_Input);
        if (instance_pin.is_valid() && instance_pin->has_any_connections()) {
            const String instance_term = get_term_name(instance_pin);
            argument_offset = 1;
            SubscriptNode* subscript = alloc_node<SubscriptNode>();
            subscript->base = build_identifier(instance_term);
            subscript->attribute = build_identifier(method.name);
            subscript->is_attribute = true;
            call_node->callee = subscript;

            if (member_func->is_chained()) {
                bool has_return_value = MethodUtils::has_return_value(method);
                const Ref<OScriptNodePin> chain_pin = member_func->find_pin(has_return_value ? 1 : 0, PD_Output);
                if (chain_pin.is_valid() && chain_pin->has_any_connections()) {
                    current_suite->add_alias(chain_pin, instance_term);
                }
            }

        } else {
            call_node->callee = build_identifier(method.name);

            if (member_func->is_chained()) {
                bool has_return_value = MethodUtils::has_return_value(method);
                const Ref<OScriptNodePin> chain_pin = member_func->find_pin(has_return_value ? 1 : 0, PD_Output);
                if (chain_pin.is_valid() && chain_pin->has_any_connections()) {
                    // The "self" alias is a special use case handled by resolve_input to create an inlined
                    // SelfNode as long as the output source/pin pair have the alias registered.
                    current_suite->add_alias(chain_pin, "self");
                }
            }
        }

        call_node->function_name = method.name;

        // Call member functions always have first argument as target object
        bind_call_func_args(call_node, member_func, 1);

    } else if (const Ref<OScriptNodeCallBuiltinFunction>& builtin_func = p_node; builtin_func.is_valid()) {
        const MethodInfo& method = builtin_func->get_method_info();

        call_node->callee = build_identifier(method.name);
        call_node->function_name = method.name;

        bind_call_func_args(call_node, builtin_func);

    } else if (const Ref<OScriptNodeCallScriptFunction>& script_func = p_node; script_func.is_valid()) {
        const Ref<OScriptFunction> function = script_func->get_function();

        call_node->callee = build_identifier(function->get_function_name());
        call_node->function_name = function->get_function_name();

        bind_call_func_args(call_node, script_func);
    }

    return call_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_get_local_variable(const Ref<OScriptNodeLocalVariable>& p_node, const Ref<OScriptNodePin>& p_pin) {
    String variable_name = p_node->get_variable_name();
    if (variable_name.is_empty()) {
        variable_name = create_cached_variable_name(p_pin);
    }

    if (!current_suite->has_local(variable_name)) {
        // Only need to register the local variable once on its first use.
        VariableNode* local_var = alloc_node<VariableNode>();
        local_var->script_node_id = p_node->get_id();
        local_var->identifier = build_identifier(create_unique_name(p_node->find_pin(0, PD_Output)));
        local_var->datatype_specifier = build_type(p_node->find_pin(0, PD_Output)->get_property_info());
        local_var->export_info.name = local_var->identifier->name;
        current_suite->statements.push_back(local_var);
        current_suite->add_local(local_var, current_function);
    }

    IdentifierNode* identifier = build_identifier(variable_name);
    identifier->script_node_id = p_node->get_id();
    return identifier;
}

OScriptParser::ExpressionNode* OScriptParser::build_make_dictionary(const Ref<OScriptNodeMakeDictionary>& p_node, const Ref<OScriptNodePin>& p_pin) {
    DictionaryNode* dict_node = alloc_node<DictionaryNode>();
    dict_node->script_node_id = p_node->get_id();

    const Vector<Ref<OScriptNodePin>> inputs = p_node->find_pins(PD_Input);
    for (int32_t index = 0; index < inputs.size(); index += 2) {
        dict_node->elements.push_back({ build_expression(inputs[index]), build_expression(inputs[index + 1]) });
    }

    return dict_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_make_array(const Ref<OScriptNodeMakeArray>& p_node, const Ref<OScriptNodePin>& p_pin) {
    ArrayNode* array_node = alloc_node<ArrayNode>();
    array_node->script_node_id = p_node->get_id();
    const Vector<Ref<OScriptNodePin>> inputs = p_node->find_pins(PD_Input);
    for (const Ref<OScriptNodePin>& input : inputs) {
        array_node->elements.push_back(build_expression(input));
    }
    return array_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_array_get_at_index(const Ref<OScriptNodeArrayGet>& p_node, const Ref<OScriptNodePin>& p_pin) {
    SubscriptNode* subscript_node = alloc_node<SubscriptNode>();
    subscript_node->script_node_id = p_node->get_id();
    subscript_node->base = build_expression(p_node->find_pin(0, PD_Input));
    subscript_node->index = build_expression(p_node->find_pin(1, PD_Input));
    return subscript_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_array_find_element(const Ref<OScriptNodeArrayFind>& p_node, const Ref<OScriptNodePin>& p_pin) {
    if (p_pin->get_pin_index() == 0) {
        // Returns the input array
        const Ref<OScriptNodePin> array_out = p_node->find_pin(0, PD_Output);
        if (array_out.is_valid() && array_out->has_any_connections()) {
            const String array_out_name = create_cached_variable_name(array_out);
            if (!current_suite->has_local(array_out_name)) {
                // Only need to register the local named variable once on its first use.
                VariableNode* local_var = alloc_node<VariableNode>();
                local_var->script_node_id = p_node->get_id();
                local_var->initializer = build_expression(p_node->find_pin(0, PD_Input));
                local_var->identifier = build_identifier(array_out_name);
                local_var->datatype_specifier = build_type(array_out->get_property_info());
                local_var->export_info.name = local_var->identifier->name;
                current_suite->statements.push_back(local_var);
                current_suite->add_local(local_var, current_function);
            }
            // Use a named variable
            return build_identifier(array_out_name);
        }
        // Fallback with a local variable
        return build_expression(p_node->find_pin(0, PD_Input));
    }

    // Index, this should be inlined
    SubscriptNode* subscript = alloc_node<SubscriptNode>();
    subscript->script_node_id = p_node->get_id();
    subscript->base = build_expression(p_node->find_pin(0, PD_Input));
    subscript->attribute = build_identifier("find");
    subscript->is_attribute = true;

    CallNode* call_node = alloc_node<CallNode>();
    call_node->script_node_id = p_node->get_id();
    call_node->callee = subscript;
    call_node->function_name = "find";
    call_node->arguments.push_back(build_expression(p_node->find_pin(1, PD_Input)));
    call_node->arguments.push_back(build_literal(0, p_node->get_id()));

    return call_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_select(const Ref<OScriptNodeSelect>& p_node, const Ref<OScriptNodePin>& p_pin) {
    TernaryOpNode* ternary_op_node = alloc_node<TernaryOpNode>();
    ternary_op_node->condition = resolve_input(p_node->find_pin(2, PD_Input));
    ternary_op_node->true_expr = resolve_input(p_node->find_pin(0, PD_Input));
    ternary_op_node->false_expr = resolve_input(p_node->find_pin(1, PD_Input));
    return ternary_op_node;
}

OScriptParser::ExpressionNode* OScriptParser::build_preload(const Ref<OScriptNodePreload>& p_node, const Ref<OScriptNodePin>& p_pin) {
    // During OScriptAnalyzer, resources marked as Preload are loaded so they're available before the
    // script begins to execute in the game loop.
    PreloadNode* preload = alloc_node<PreloadNode>();
    preload->path = create_literal(p_node->get_resource_path());
    return preload;
}

OScriptParser::ExpressionNode* OScriptParser::build_resource_path(const Ref<OScriptNodeResourcePath>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return create_literal(p_node->get_resource_path());
}

OScriptParser::ExpressionNode* OScriptParser::build_get_autoload(const Ref<OScriptNodeAutoload>& p_node, const Ref<OScriptNodePin>& p_pin) {
    return build_identifier(p_node->get_autoload_name());
}

OScriptParser::ExpressionNode* OScriptParser::build_dialogue_choice(const Ref<OScriptNodeDialogueChoice>& p_node, const Ref<OScriptNodePin>& p_pin) {
    DictionaryNode* data = alloc_node<DictionaryNode>();
    const Ref<OScriptNodePin> text_pin = p_node->find_pin(0, PD_Input);
    data->elements.push_back({ create_literal("text"), resolve_input(text_pin) });

    const Ref<OScriptNodePin> visible_pin = p_node->find_pin(1, PD_Input);
    data->elements.push_back({ create_literal("visible"), resolve_input(visible_pin) });

    return data;
}

void OScriptParser::build_statements(const Ref<OScriptNodePin>& p_source_pin, const Ref<OScriptNodePin>& p_target_pin, SuiteNode* p_suite) {
    Ref<OScriptNodePin> target_pin = p_target_pin;

    while (target_pin.is_valid()) {
        const Ref<OScriptNode> target_node = target_pin->get_owning_node();

        const OScriptNodePinId target_id = { target_node->get_id(), target_pin->get_pin_index() };

        if (use_node_convergence && !convergence_stack.is_empty()) {
            const OScriptNodePinId& converge_id = convergence_stack.back()->get();
            if (converge_id == target_id) {
                // Reached converging node, don't process this
                return;
            }
        }

        if (is_break_pin(target_pin)) {
            // This is a traversal from a pin that links into a loop's break pin.
            emit_loop_break(target_id.node);
            return;
        }

        OScriptNodePinId convergence_pin = { -1, -1 };
        if (use_node_convergence && function_info.divergence_to_merge_point.has(target_id.node)) {
            convergence_pin = function_info.divergence_to_merge_pins[target_id.node];
            convergence_stack.push_back(convergence_pin);
        }

        const StatementResult result = build_statement(target_node);

        if (use_node_convergence && convergence_pin.node >= 0) {
            convergence_stack.pop_back();
        }

        switch (result.control_flow) {
            case StatementResult::CONTINUE: {
                // Normal logic where we examine the exit pin
                if (result.exit_pin.is_null()) {
                    return;
                }
                if (!result.exit_pin->is_execution()) {
                    push_error(R"(Parser bug: Output pin connection should be a control flow pin)");
                    return;
                }
                target_pin = get_target_from_source(result.exit_pin);
                break;
            }
            case StatementResult::STOP: {
                // Handler requested stop, likely hitting converging nodes
                return;
            }
            case StatementResult::JUMP_TO_NODE: {
                if (use_node_convergence) {
                    // Handler suggests we jump
                    if (result.jump_target.is_valid() && result.jump_target_pin.is_valid()) {
                        target_pin = result.jump_target_pin;
                    } else {
                        return;
                    }
                }
                break;
            }
            case StatementResult::DIVERGENCE_HANDLED: {
                if (use_node_convergence) {
                    // Handler built a divergence and all paths converge
                    if (result.convergence_info.has_value()) {
                        const StatementResult::ConvergenceInfo& ci = result.convergence_info.value();
                        target_pin = ci.convergence_node_pin;
                    } else {
                        return;
                    }
                }
                break;
            }
            default: {
                // Do nothing
                break;
            }
        }
    }
}

OScriptParser::StatementResult OScriptParser::build_statement(const Ref<OScriptNode>& p_script_node) {
    const StringName class_name = p_script_node->get_class();
    StatementHandler* handler = _statement_handlers.getptr(class_name);
    ERR_FAIL_NULL_V_MSG(handler, {}, "No handler defined for node type " + class_name);

    return (*handler)(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_type_cast(const Ref<OScriptNodeTypeCast>& p_script_node) { // NOLINT
    // Orchestrator's TypeCast node is effectively a combination of a Cast and If node pair.
    // So the logic here is to combine both statements in logical flow.

    const Ref<OScriptNodePin> input_pin = p_script_node->find_pin(1, PD_Input);
    const Ref<OScriptNodePin> true_pin = p_script_node->find_pin(0, PD_Output);
    const Ref<OScriptNodePin> false_pin = p_script_node->find_pin(1, PD_Output);
    const Ref<OScriptNodePin> casted_pin = p_script_node->find_pin(2, PD_Output);

    // Short-circuit and simply set the output object pin as null literal and exit.
    if (!true_pin->has_any_connections() && !false_pin->has_any_connections()) {
        const String casted_name = create_cached_variable_name(casted_pin);
        create_local_and_push(casted_name, create_literal(Variant()));
        return create_stop_result();
    }

    // Either true or false pins have connections, so the cast must be performed to decide path
    CastNode* cast_node = alloc_node<CastNode>();
    cast_node->operand = resolve_input(input_pin);
    cast_node->cast_type = build_type(casted_pin->get_property_info());
    cast_node->script_node_id = p_script_node->get_id();
    cast_node->operand->script_node_id = p_script_node->get_id();
    cast_node->cast_type->script_node_id = p_script_node->get_id();

    // There are two ways to handle the branch logic, and it depends on where the variable is declared.
    //
    // In one way this can be written where you use the cast as the branch condition, and then in the
    // true block, perform a recast to a variable for the output pin. This creates a situation where
    // the cast is re-evaluated twice, which is highly inefficient.
    //
    // The second is where the cast is performed and assigned to a variable before the branch, and then
    // the condition evaluates whether the variable is null. This is far more idiomatic in terms of
    // how to do this, and generates one less overall VM operation.
    //
    // So here we define the variable that be used as the source for the object output pin for either
    // of the branch traversal cases.
    const String casted_name = create_cached_variable_name(casted_pin);
    create_local_and_push(casted_name, cast_node);
    add_pin_alias(casted_name, casted_pin);

    // Branch based on nullness.
    add_statement(create_if(build_identifier(casted_name), true_pin, false_pin));

    // type cast control flow is based on the if condition, so no "next"
    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_if(const Ref<OScriptNodeBranch>& p_script_node) {
    const Ref<OScriptNodePin> cond_pin = p_script_node->find_pin(1, PD_Input);
    const Ref<OScriptNodePin> true_pin = p_script_node->find_pin(0, PD_Output);
    const Ref<OScriptNodePin> false_pin = p_script_node->find_pin(1, PD_Output);

    IfNode* if_node = create_if(resolve_input(cond_pin), true_pin, false_pin);
    if_node->script_node_id = p_script_node->get_id();
    if_node->condition->script_node_id = p_script_node->get_id();
    add_statement(if_node);

    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_return(const Ref<OScriptNodeFunctionResult>& p_script_node) {
    ReturnNode* return_node = alloc_node<ReturnNode>();
    return_node->script_node_id = p_script_node->get_id();
    set_return();

    if (p_script_node.is_valid()) {
        for (const Ref<OScriptNodePin>& input : p_script_node->find_pins(PD_Input)) {
            if (input.is_valid() && !input->is_execution()) {
                // Returns are only permitted to return 1 value
                return_node->return_value = resolve_input(input);
                break;
            }
        }
    }

    add_statement(return_node);
    return create_stop_result();
}

OScriptParser::StatementResult OScriptParser::build_variable_get_validated(const Ref<OScriptNodeVariableGet>& p_script_node) {
    Ref<OScriptVariable> variable = p_script_node->get_variable();
    if (!variable.is_valid()) {
        push_error("Variable reference is invalid");
        return create_stop_result();
    }

    BinaryOpNode* is_object = alloc_node<BinaryOpNode>();
    is_object->left_operand = create_literal(variable->get_variable_type());
    is_object->right_operand = create_literal(Variant::OBJECT);
    is_object->operation = BinaryOpNode::OP_COMP_EQUAL;
    is_object->variant_op = Variant::OP_EQUAL;
    is_object->script_node_id = p_script_node->get_id();

    CastNode* type_cast = alloc_node<CastNode>();
    type_cast->cast_type = build_type(PropertyInfo(Variant::OBJECT, "x"));
    type_cast->operand = build_identifier(variable->get_variable_name());

    BinaryOpNode* and_op = alloc_node<BinaryOpNode>();
    and_op->left_operand = is_object;
    and_op->right_operand = type_cast;
    and_op->operation = BinaryOpNode::OP_LOGIC_AND;
    and_op->variant_op = Variant::OP_AND;

    add_pin_alias(variable->get_variable_name(), p_script_node->find_pin(2, PD_Output));

    const Ref<OScriptNodePin> true_pin = p_script_node->find_pin(0, PD_Output);
    const Ref<OScriptNodePin> false_pin = p_script_node->find_pin(1, PD_Output);
    IfNode* if_node = create_if(and_op, true_pin, false_pin);
    if_node->script_node_id = p_script_node->get_id();

    add_statement(if_node);

    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_variable_set(const Ref<OScriptNodeVariableSet>& p_script_node) {
    if (!p_script_node.is_valid()) {
        return create_stop_result();
    }

    const Ref<OScriptVariable> variable = p_script_node->get_variable();
    if (!variable.is_valid()) {
        return create_stop_result();
    }

    const String variable_name = variable->get_variable_name();
    const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(1, PD_Input);

    AssignmentNode* assign = alloc_node<AssignmentNode>();
    assign->assignee = build_identifier(variable_name);
    assign->assigned_value = resolve_input(value_pin);
    assign->script_node_id = p_script_node->get_id();
    assign->assignee->script_node_id = p_script_node->get_id();
    assign->assigned_value->script_node_id = p_script_node->get_id();
    add_statement(assign);

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_property_set(const Ref<OScriptNodePropertySet>& p_script_node) {
    const String property_name = p_script_node->get_property().name;

    switch (p_script_node->get_call_mode()) {
        case OScriptNodeProperty::CALL_SELF: {
            const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(1, PD_Input);
            AssignmentNode* assign = alloc_node<AssignmentNode>();
            assign->assignee = build_identifier(property_name);
            assign->assigned_value = resolve_input(value_pin);
            assign->script_node_id = p_script_node->get_id();
            assign->assignee->script_node_id = p_script_node->get_id();
            assign->assigned_value->script_node_id = p_script_node->get_id();
            add_statement(assign);
            break;
        }
        case OScriptNodeProperty::CALL_INSTANCE: {
            const Ref<OScriptNodePin> object_pin = p_script_node->find_pin(1, PD_Input);
            const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(2, PD_Input);

            AssignmentNode* assign = alloc_node<AssignmentNode>();
            if (object_pin->has_any_connections()) {
                // In this case refer to the base object
                SubscriptNode* subscript = create_subscript_attribute(resolve_input(object_pin), build_identifier(property_name));
                assign->assignee = subscript;
                assign->assigned_value = resolve_input(value_pin);
            } else {
                // In this case refer to self
                assign->assignee = build_identifier(property_name);
                assign->assigned_value = resolve_input(value_pin);
            }
            assign->script_node_id = p_script_node->get_id();
            assign->assignee->script_node_id = p_script_node->get_id();
            assign->assigned_value->script_node_id = p_script_node->get_id();
            add_statement(assign);
            break;
        }
        case OScriptNodeProperty::CALL_NODE_PATH: {
            const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(1, PD_Input);

            CallNode* get_node_call = create_func_call("get_node");
            get_node_call->arguments.push_back(create_literal(p_script_node->get_node_path()));
            SubscriptNode* subscript = create_subscript_attribute(get_node_call, build_identifier(property_name));

            AssignmentNode* assign = alloc_node<AssignmentNode>();
            assign->assignee = subscript;
            assign->assigned_value = resolve_input(value_pin);
            assign->script_node_id = p_script_node->get_id();
            assign->assignee->script_node_id = p_script_node->get_id();
            assign->assigned_value->script_node_id = p_script_node->get_id();
            add_statement(assign);
            break;
        };
    };

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_assign_local_variable(const Ref<OScriptNodeAssignLocalVariable>& p_script_node) {
    const Ref<OScriptNodePin> variable_pin = p_script_node->find_pin(1, PD_Input);
    const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(2, PD_Input);

    AssignmentNode* assign = alloc_node<AssignmentNode>();
    assign->assignee = resolve_input(variable_pin);
    assign->assigned_value = resolve_input(value_pin);
    assign->script_node_id = p_script_node->get_id();
    assign->assignee->script_node_id = p_script_node->get_id();
    assign->assigned_value->script_node_id = p_script_node->get_id();
    add_statement(assign);

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_call_member_function(const Ref<OScriptNodeCallMemberFunction>& p_script_node) {
    const MethodInfo& method = p_script_node->get_function();
    const bool has_return_value = MethodUtils::has_return_value(method);

    const long input_pin_count = p_script_node->find_pins(PD_Input).size();
    ERR_FAIL_COND_V(input_pin_count == 0, create_stop_result());

    int argument_offset = 1;
    bool has_execution_pins = false;

    Ref<OScriptNodePin> base_input_pin = p_script_node->find_pin(0, PD_Input);
    if (base_input_pin->is_execution()) {
        argument_offset = 2;
        has_execution_pins = true;
        base_input_pin = p_script_node->find_pin(1, PD_Input);
    }
    ERR_FAIL_COND_V(!base_input_pin.is_valid(), create_stop_result());

    Ref<OScriptNodePin> chain_pin;
    if (p_script_node->is_chained()) {
        int chain_offset = has_execution_pins ? 1 : 0;
        chain_pin = p_script_node->find_pin(chain_offset + (has_return_value ? 1  : 0), PD_Output);
    }

    Ref<OScriptNodePin> result_pin;
    if (has_return_value) {
        result_pin = p_script_node->find_pin(has_execution_pins ? 1 : 0, PD_Output);
    }

    CallNode* call_node = nullptr;
    if (base_input_pin->has_any_connections()) {
        // Object base is resolved from an input
        if (chain_pin.is_valid() && chain_pin->has_any_connections()) {
            // In this case we need to create a term to pass the value along the node
            const String base_term = get_term_name(base_input_pin);
            call_node = create_func_call(base_term, method.name);
            // Chain should pass through the base term
            current_suite->add_alias(chain_pin, base_term);
        } else {
            call_node = create_func_call(resolve_input(base_input_pin), method.name);
        }
    } else {
        // Calling on self.
        call_node = create_func_call(method.name);

        if (chain_pin.is_valid() && chain_pin->has_any_connections()) {
            // The "self" alias is a special use case handled by resolve_input to create an inlined
            // SelfNode as long as the output source/pin pair have the alias registered.
            current_suite->add_alias(chain_pin, "self");
        }
    }

    bind_call_func_args(call_node, p_script_node, argument_offset);
    call_node->script_node_id = p_script_node->get_id();

    if (has_return_value && result_pin.is_valid() && result_pin->has_any_connections()) {
        const String result_name = create_cached_variable_name(result_pin);
        create_local_and_push(result_name, call_node);
    } else {
        add_statement(call_node);
    }

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_call_builtin_function(const Ref<OScriptNodeCallBuiltinFunction>& p_script_node) {
    const MethodInfo& method = p_script_node->get_method_info();

    CallNode* call_node = create_func_call(method.name);
    call_node->script_node_id = p_script_node->get_id();
    bind_call_func_args(call_node, p_script_node);

    Node* statement = call_node;
    if (MethodUtils::has_return_value(method)) {
        for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
            if (!output->is_execution() && output->has_any_connections()) {
                statement = create_local(create_unique_name(output), call_node);
                break;
            }
        }
    }

    add_statement(statement);

    return p_script_node->has_execution_pins() ? create_statement_result(p_script_node, 0) : create_stop_result();
}

OScriptParser::StatementResult OScriptParser::build_call_script_function(const Ref<OScriptNodeCallScriptFunction>& p_script_node) {
    const Ref<OScriptFunction> function = p_script_node->get_function();

    CallNode* call_node = create_func_call(function->get_function_name());
    call_node->script_node_id = p_script_node->get_id();

    int pin_offset = 0;
    const Ref<OScriptNodePin> base_output_pin = p_script_node->find_pin(0, PD_Output);
    if (base_output_pin.is_valid() && base_output_pin->is_execution()) {
        pin_offset = 1;
    }

    bind_call_func_args(call_node, p_script_node, pin_offset);

    Node* statement = call_node;
    if (MethodUtils::has_return_value(function->get_method_info())) {
        for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
            if (output.is_valid() && !output->is_execution() && output->has_any_connections()) {
                statement = create_local(create_unique_name(output), call_node);
                break;
            }
        }
    }

    add_statement(statement);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_call_static_function(const Ref<OScriptNodeCallStaticFunction>& p_script_node) {
    const MethodInfo method = p_script_node->get_target_method();

    CallNode* call_node = create_func_call(p_script_node->get_target_class_name(), method.name);
    call_node->is_static = true;
    call_node->script_node_id = p_script_node->get_id();

    int pin_offset = 0;
    const Ref<OScriptNodePin> base_output_pin = p_script_node->find_pin(0, PD_Output);
    if (base_output_pin.is_valid() && base_output_pin->is_execution()) {
        pin_offset = 1;
    }

    bind_call_func_args(call_node, p_script_node, pin_offset);

    Node* statement = call_node;
    if (MethodUtils::has_return_value(method)) {
        for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
            if (output.is_valid() && !output->is_execution() && output->has_any_connections()) {
                statement = create_local(create_unique_name(output), call_node);
                break;
            }
        }
    }

    add_statement(statement);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_call_super(const Ref<OScriptNodeCallFunction>& p_script_node) {
    if (const Ref<OScriptNodeCallParentMemberFunction>& node = p_script_node; node.is_valid()) {
        const MethodInfo method = node->get_method_info();
        CallNode* call_node = create_func_call(node->get_target_class(), method.name);
        call_node->is_super = true;
        call_node->script_node_id = p_script_node->get_id();

        bind_call_func_args(call_node, p_script_node);

        Node* statement = call_node;
        if (MethodUtils::has_return_value(method)) {
            for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
                if (output.is_valid() && !output->is_execution() && output->has_any_connections()) {
                    statement = create_local(create_unique_name(output), call_node);
                    break;
                }
            }
        }

        add_statement(statement);
        return create_statement_result(p_script_node, 0);
    }

    const Ref<OScriptNodeCallParentScriptFunction> node = p_script_node;
    if (node.is_valid()) {
        const MethodInfo method = node->get_method_info();
        CallNode* call_node = create_func_call(method.name);
        call_node->is_super = true;
        call_node->script_node_id = p_script_node->get_id();

        bind_call_func_args(call_node, p_script_node);

        Node* statement = call_node;
        if (MethodUtils::has_return_value(method)) {
            for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
                if (output.is_valid() && !output->is_execution() && output->has_any_connections()) {
                    statement = create_local(create_unique_name(output), call_node);
                    break;
                }
            }
        }

        add_statement(statement);
        return create_statement_result(p_script_node, 0);
    }

    return create_stop_result();
}

OScriptParser::StatementResult OScriptParser::build_sequence(const Ref<OScriptNodeSequence>& p_script_node) {
    for (const Ref<OScriptNodePin>& output : p_script_node->find_pins(PD_Output)) {
        if (output.is_valid() && output->has_any_connections()) {
            const Ref<OScriptNodePin> start_node_pin = output->get_connections()[0];
            // Given that a SequenceNode does not introduce any special scope, we append
            // all statements to the current suite.
            build_statements(output, start_node_pin, current_suite);
        }
    }

    // Sequence terminates after all statement branches are executed in order.
    return create_stop_result();
}

OScriptParser::StatementResult OScriptParser::build_while(const Ref<OScriptNodeWhile>& p_script_node) {
    WhileNode *while_node = alloc_node<WhileNode>();
    while_node->script_node_id = p_script_node->get_id();

    while_node->condition = resolve_input(p_script_node->find_pin(1, PD_Input));
    if (while_node->condition == nullptr) {
        push_error(R"(Expected conditional expression for "while".)");
        return create_stop_result();
    }

    // Save break/continue state.
    bool could_break = can_break;
    bool could_continue = can_continue;

    // Allow break/continue.
    can_break = true;
    can_continue = true;

    SuiteNode *suite = alloc_node<SuiteNode>();
    suite->is_in_loop = true;

    const Ref<OScriptNodePin> repeat_pin = p_script_node->find_pin(0, PD_Output);
    if (repeat_pin.is_valid() && repeat_pin->has_any_connections()) {
        while_node->loop = build_suite("while loop", repeat_pin, suite);
    }

    add_statement(while_node);

    // Reset break/continue state.
    can_break = could_break;
    can_continue = could_continue;

    // Done pin
    return create_statement_result(p_script_node, 1);
}

OScriptParser::StatementResult OScriptParser::build_array_set(const Ref<OScriptNodeArraySet>& p_script_node) {
    const Ref<OScriptNodePin> array_pin = p_script_node->find_pin(1, PD_Input);

    SubscriptNode* subscript_node = alloc_node<SubscriptNode>();
    subscript_node->base = resolve_input(array_pin);
    subscript_node->index = resolve_input(p_script_node->find_pin(2, PD_Input));

    AssignmentNode* assign = alloc_node<AssignmentNode>();
    assign->assignee = subscript_node;
    assign->assigned_value = resolve_input(p_script_node->find_pin(3, PD_Input));
    assign->assignee->script_node_id = p_script_node->get_id();
    assign->assigned_value->script_node_id = p_script_node->get_id();
    add_statement(assign);

    const Ref<OScriptNodePin> output = p_script_node->find_pin(1, PD_Output);
    if (output.is_valid() && output->has_any_connections()) {
        create_local_and_push(create_cached_variable_name(output), resolve_input(array_pin));
    }

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_array_clear(const Ref<OScriptNodeArrayClear>& p_script_node) {
    const String array_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    CallNode* clear_func = create_func_call(array_term, "clear");
    clear_func->script_node_id = p_script_node->get_id();
    add_statement(clear_func);

    add_pin_alias(array_term, p_script_node->find_pin(1, PD_Output));

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_array_append(const Ref<OScriptNodeArrayAppend>& p_script_node) {
    const String target_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    ExpressionNode* source = resolve_input(p_script_node->find_pin(2, PD_Input));

    CallNode* call_node = create_func_call(target_term, "append_array");
    call_node->arguments.push_back(source);
    call_node->script_node_id = p_script_node->get_id();
    add_statement(call_node);

    const Ref<OScriptNodePin> array_out_pin = p_script_node->find_pin(1, PD_Output);
    if (array_out_pin.is_valid() && array_out_pin->has_any_connections()) {
        add_pin_alias(target_term, array_out_pin);
    }

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_array_add_element(const Ref<OScriptNodeArrayAddElement>& p_script_node) {
    const String array_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    ExpressionNode* element = resolve_input(p_script_node->find_pin(2, PD_Input));

    const Ref<OScriptNodePin> array_index = p_script_node->find_pin(2, PD_Output);
    if (array_index.is_valid() && array_index->has_any_connections()) {
        const String array_index_name = create_cached_variable_name(array_index);
        if (!has_local_variable(array_index_name)) {
            CallNode* call_size = create_func_call(array_term, "size");
            create_local_and_push(array_index_name, call_size);
        }
    }

    CallNode* call_node = create_func_call(array_term, "append");
    call_node->arguments.push_back(element);
    call_node->script_node_id = p_script_node->get_id();
    add_statement(call_node);
    add_pin_alias(array_term, p_script_node->find_pin(1, PD_Output));

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_array_remove_element(const Ref<OScriptNodeArrayRemoveElement>& p_script_node) {
    const String array_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    ExpressionNode* element = resolve_input(p_script_node->find_pin(2, PD_Input));

    const Ref<OScriptNodePin> element_removed = p_script_node->find_pin(2, PD_Output);
    if (element_removed.is_valid() && element_removed->has_any_connections()) {
        // In this case we need to take a more expensive path as we need to use find/remove_at
        // so that we can track whether the element exists for removal.
        const String element_removed_name = create_cached_variable_name(element_removed);
        if (!has_local_variable(element_removed_name)) {
            create_local_and_push(element_removed_name, create_literal(false));
        }

        CallNode* call_node = create_func_call(array_term, "find");
        call_node->arguments.push_back(element);
        call_node->arguments.push_back(build_literal(0, p_script_node->get_id()));
        call_node->script_node_id = p_script_node->get_id();

        const String find_var_name = vformat("temp_node_%d_find", p_script_node->get_id());
        create_local_and_push(find_var_name, call_node);

        IfNode* if_node = alloc_node<IfNode>();
        if_node->condition = create_binary_op(VariantOperators::OP_NOT_EQUAL, build_identifier(find_var_name), create_literal(-1));
        if_node->condition->script_node_id = p_script_node->get_id();

        if_node->true_block = push_suite();

        CallNode* remove_call = create_func_call(array_term, "remove_at");
        remove_call->arguments.push_back(build_identifier(find_var_name));
        remove_call->script_node_id = p_script_node->get_id();
        add_statement(remove_call);

        AssignmentNode* removed_assign = alloc_node<AssignmentNode>();
        removed_assign->assignee = build_identifier(element_removed_name);
        removed_assign->assigned_value = create_literal(true);
        removed_assign->script_node_id = p_script_node->get_id();
        removed_assign->assignee->script_node_id = p_script_node->get_id();
        removed_assign->assigned_value->script_node_id = p_script_node->get_id();
        add_statement(removed_assign);

        pop_suite();
        if_node->true_block->parent_if = if_node;
        add_statement(if_node);

    } else {
        CallNode* call_node = create_func_call(array_term, "erase");
        call_node->arguments.push_back(element);
        call_node->script_node_id = p_script_node->get_id();
        add_statement(call_node);
    }

    add_pin_alias(array_term, p_script_node->find_pin(1, PD_Output));
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_array_remove_index(const Ref<OScriptNodeArrayRemoveIndex>& p_script_node) {
    const String array_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    ExpressionNode* index = resolve_input(p_script_node->find_pin(2, PD_Input));

    CallNode* call_node = create_func_call(array_term, "remove_at");
    call_node->arguments.push_back(index);
    call_node->script_node_id = p_script_node->get_id();
    add_statement(call_node);
    add_pin_alias(array_term, p_script_node->find_pin(1, PD_Output));

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_dictionary_set_item(const Ref<OScriptNodeDictionarySet>& p_script_node) {
    const String dict_term = get_term_name(p_script_node->find_pin(1, PD_Input));
    ExpressionNode* key = resolve_input(p_script_node->find_pin(2, PD_Input));
    ExpressionNode* value = resolve_input(p_script_node->find_pin(3, PD_Input));

    const Ref<OScriptNodePin> old_value_pin = p_script_node->find_pin(3, PD_Output);
    if (old_value_pin->has_any_connections()) {
        CallNode* get_old_value = create_func_call(dict_term, "get");
        get_old_value->arguments.push_back(key);
        get_old_value->arguments.push_back(create_literal(Variant()));
        get_old_value->script_node_id = p_script_node->get_id();

        const String old_value_term = create_cached_variable_name(old_value_pin);
        create_local_and_push(old_value_term, get_old_value);
        add_pin_alias(old_value_term, old_value_pin);
    }

    const Ref<OScriptNodePin> dict_out = p_script_node->find_pin(1, PD_Output);
    CallNode* call_set = create_func_call(dict_term, "set");
    call_set->arguments.push_back(key);
    call_set->arguments.push_back(value);
    call_set->script_node_id = p_script_node->get_id();
    add_pin_alias(dict_term, dict_out);

    const Ref<OScriptNodePin> replaced = p_script_node->find_pin(2, PD_Output);
    if (replaced->has_any_connections()) {
        const String replaced_term = create_cached_variable_name(replaced);
        create_local_and_push(replaced_term, call_set);
        add_pin_alias(replaced_term, replaced);
    } else {
        add_statement(call_set);
    }

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_chance(const Ref<OScriptNodeChance>& p_script_node) {
    const Ref<OScriptNodePin> lower_pin = p_script_node->find_pin(0, PD_Output);
    const Ref<OScriptNodePin> upper_pin = p_script_node->find_pin(1, PD_Output);

    // Short-circuit, if this node does not have any output connections end code path
    if (!lower_pin->has_any_connections() && !upper_pin->has_any_connections()) {
        return create_stop_result();
    }

    CallNode* lhs = alloc_node<CallNode>();
    lhs->callee = build_identifier("randi_range");
    lhs->function_name = "randi_range";
    lhs->arguments.push_back(build_literal(0, p_script_node->get_id()));
    lhs->arguments.push_back(build_literal(100, p_script_node->get_id()));
    lhs->script_node_id = p_script_node->get_id();
    lhs->callee->script_node_id = p_script_node->get_id();

    ExpressionNode* rhs = create_literal(p_script_node->get_chance());
    rhs->script_node_id = p_script_node->get_id();

    BinaryOpNode* branch_condition = create_binary_op(VariantOperators::OP_LESS_EQUAL, lhs, rhs);
    branch_condition->script_node_id = p_script_node->get_id();

    IfNode* if_node = create_if(branch_condition, lower_pin, upper_pin);
    if_node->script_node_id = p_script_node->get_id();
    add_statement(if_node);

    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_delay(const Ref<OScriptNodeDelay>& p_script_node) {
    // get_tree().create_timer(<duration>)
    CallNode* call_create_timer = create_func_call(create_func_call("get_tree"), "create_timer");
    call_create_timer->arguments.push_back(create_literal(p_script_node->get_duration()));
    call_create_timer->script_node_id = p_script_node->get_id();

    // Get the timeout signal from the base
    SubscriptNode* timeout = create_subscript_attribute(call_create_timer, build_identifier("timeout"));
    timeout->script_node_id = p_script_node->get_id();

    // Await on the signal
    AwaitNode* await_node = alloc_node<AwaitNode>();
    await_node->to_await = timeout;
    await_node->script_node_id = p_script_node->get_id();

    set_coroutine();
    add_statement(await_node);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_for_loop(const Ref<OScriptNodeForLoop>& p_script_node) {
    // Check if this is a breakable loop
    String break_var_name;
    if (function_info.loop_break_variables.has(p_script_node->get_id())) {
        break_var_name = function_info.loop_break_variables[p_script_node->get_id()];
        if (!has_local_variable(break_var_name)) {
            create_local_and_push(break_var_name, create_literal(false));
        }
    }

    // todo: need to guard that control flow from the loop does not re-enter the for

    // Because range upper bounds is exclusive
    BinaryOpNode* add_op = create_binary_op(
        VariantOperators::OP_ADD, resolve_input(p_script_node->find_pin(2, PD_Input)), create_literal(1));

    CallNode* call_node = alloc_node<CallNode>();
    call_node->callee = build_identifier("_oscript_internal_range");
    call_node->function_name = "_oscript_internal_range";
    call_node->arguments.push_back(resolve_input(p_script_node->find_pin(1, PD_Input)));
    call_node->arguments.push_back(add_op);
    call_node->script_node_id = p_script_node->get_id();
    call_node->callee->script_node_id = p_script_node->get_id();

    ForNode* for_node = alloc_node<ForNode>();
    for_node->variable = build_identifier(vformat("for_var_%d", p_script_node->get_id()));
    for_node->variable->data_type.builtin_type = Variant::INT;
    for_node->list = call_node;
    for_node->script_node_id = p_script_node->get_id();
    for_node->variable->script_node_id = p_script_node->get_id();
    for_node->list->script_node_id = p_script_node->get_id();

    // Save break/continue state
    bool could_break = can_break;
    bool could_continue = can_continue;

    can_break = true;
    can_continue = true;

    if (has_local_variable(for_node->variable->name)) {
        push_error(vformat(R"(There is already a variable named "%s".)", for_node->variable->name), for_node->variable);
    }

    // We cannot use push because when the suite builds, it sets the context.
    SuiteNode* suite = alloc_node<SuiteNode>();
    suite->script_node_id = p_script_node->get_id();

    // Setup index iteration variable in nested suite
    add_local_variable(for_node->variable, suite);
    add_pin_alias(for_node->variable->name, p_script_node->find_pin(1, PD_Output), suite);

    // Setup for loop
    // When suite finishes, it's popped.
    current_suite->is_in_loop = true;
    for_node->loop = build_suite("for loop body", p_script_node->find_pin(0, PD_Output), suite);

    can_break = could_break;
    can_continue = could_continue;

    add_statement(for_node);

    if (!break_var_name.is_empty()) {
        BinaryOpNode* if_cond = create_binary_op(VariantOperators::OP_EQUAL, build_identifier(break_var_name), create_literal(true));
        add_statement(create_if(if_cond, p_script_node->find_pin(3, PD_Output), p_script_node->find_pin(2, PD_Output)));
        return create_divergence_result(p_script_node);
    }

    // Always leave completed with no break
    return create_statement_result(p_script_node, 2);
}

OScriptParser::StatementResult OScriptParser::build_for_each(const Ref<OScriptNodeForEach>& p_script_node) {
    // Check if this is a breakable loop
    String break_var_name;
    if (function_info.loop_break_variables.has(p_script_node->get_id())) {
        break_var_name = function_info.loop_break_variables[p_script_node->get_id()];
        if (!has_local_variable(break_var_name)) {
            create_local_and_push(break_var_name, create_literal(false));
        }
    }

    // todo: need to guard that control flow from the loop does not re-enter the for

    // The ForEach node is a bit unique in that it outputs two values per loop iteration,
    // the array item and it's index. Right now there is not a clean way to expose both
    // of these without trade-offs with how the compiler/vm are designed.
    //
    // So the way this works is that if the index-pin is not connected, the ForEach loop
    // will use the standard "for element in array" syntax by specifying the input array
    // as the list item in the ForNode. If the index pin is connected, then the list is
    // populated with a size call and used in a range-based for loop. In addition, the
    // first operation in the for loop will be to assign the element variable with the
    // array item using the "element = array[index]" syntax.
    //
    ExpressionNode* for_list = nullptr;
    const bool is_index_required = p_script_node->find_pin(2, PD_Output)->has_any_connections();
    if (!is_index_required) {
        // Uses the simple "for element in array" syntax
        for_list = resolve_input(p_script_node->find_pin(1, PD_Input));
    } else {
        ExpressionNode* array = resolve_input(p_script_node->find_pin(1, PD_Input));
        CallNode* array_size = create_func_call(array, "size");

        CallNode* call_node = alloc_node<CallNode>();
        call_node->callee = build_identifier("_oscript_internal_range");
        call_node->function_name = "_oscript_internal_range";
        call_node->arguments.push_back(create_literal(0));
        call_node->arguments.push_back(array_size);

        for_list = call_node;
    }

    ForNode* for_node = alloc_node<ForNode>();
    for_node->variable = build_identifier(vformat("for_var_%d", p_script_node->get_id()));
    if (is_index_required) {
        for_node->variable->data_type.builtin_type = Variant::INT;
    }
    for_node->list = for_list;
    for_node->script_node_id = p_script_node->get_id();
    for_node->variable->script_node_id = p_script_node->get_id();
    for_node->list->script_node_id = p_script_node->get_id();

    // Save break/continue state
    bool could_break = can_break;
    bool could_continue = can_continue;

    can_break = true;
    can_continue = true;

    if (has_local_variable(for_node->variable->name)) {
        push_error(vformat(R"(There is already a variable named "%s".)", for_node->variable->name), for_node->variable);
    }

    // We cannot use push because when the suite builds, it sets the context.
    SuiteNode* suite = alloc_node<SuiteNode>();
    suite->parent_block = current_suite;
    suite->parent_function = current_function;
    suite->script_node_id = p_script_node->get_id();

    // Setup element variable in nested suite
    add_local_variable(for_node->variable, suite);

    if (is_index_required) {
        // Set up the index pin variable, and create assignment operation
        const String index_name = vformat("for_elem_%d", p_script_node->get_id());

        SubscriptNode* subscript = alloc_node<SubscriptNode>();
        subscript->base = resolve_input(p_script_node->find_pin(1, PD_Input));
        subscript->index = build_identifier(for_node->variable->name, suite);
        subscript->base->script_node_id = p_script_node->get_id();
        subscript->index->script_node_id = p_script_node->get_id();

        VariableNode* index = create_local(index_name, subscript, suite);
        add_statement(index, suite);

        add_pin_alias(index_name, p_script_node->find_pin(1, PD_Output), suite);
        add_pin_alias(for_node->variable->name, p_script_node->find_pin(2, PD_Output), suite);
    } else {
        add_pin_alias(for_node->variable->name, p_script_node->find_pin(1, PD_Output), suite);
    }

    // Setup for loop
    // When suite finishes, it's popped.
    current_suite->is_in_loop = true;
    for_node->loop = build_suite("for loop body", p_script_node->find_pin(0, PD_Output), suite);

    can_break = could_break;
    can_continue = could_continue;

    add_statement(for_node);

    if (!break_var_name.is_empty()) {
        BinaryOpNode* if_cond = create_binary_op(VariantOperators::OP_EQUAL, build_identifier(break_var_name), create_literal(true));
        add_statement(create_if(if_cond, p_script_node->find_pin(4, PD_Output), p_script_node->find_pin(3, PD_Output)));
        return create_divergence_result(p_script_node);
    }

    // Always leave completed with no break
    return create_statement_result(p_script_node, 3);
}

// todo: if multiple switch pins converge they should be grouped together in the AST.
OScriptParser::StatementResult OScriptParser::build_switch(const Ref<OScriptNodeSwitch>& p_script_node) {
    const Vector<Ref<OScriptNodePin>> input_pins = p_script_node->find_pins(PD_Input);
    const uint64_t input_pins_size = input_pins.size();
    if (input_pins_size < 2) {
        // Should never happen
        push_error("Unexpected number of input pins must be two or greater.");
        return create_stop_result();
    }
    else if (input_pins_size == 2) {
        // No need to scope or treat this as an if block but instead more like a sequence
        const Ref<OScriptNodePin> true_pin = p_script_node->find_pin(1, PD_Output);
        if (true_pin.is_valid() && true_pin->has_any_connections()) {
            ExpressionNode* true_literal = create_literal(true);
            BinaryOpNode* default_cond = create_binary_op(VariantOperators::OP_EQUAL, true_literal, true_literal);
            IfNode* default_scope = create_if(default_cond, true_pin, nullptr);
            true_literal->script_node_id = p_script_node->get_id();
            default_cond->script_node_id = p_script_node->get_id();
            default_scope->script_node_id = p_script_node->get_id();
            add_statement(default_scope);
        }
        return create_statement_result(p_script_node, 0);
    }
    else {
        // In this case we always start at input pin index 2 and compare against input pin 1.
        // This provides for output pin 1 to be treated as the "else" block.
        const Vector<Ref<OScriptNodePin>> output_pins = p_script_node->find_pins(PD_Output);
        const uint64_t output_pins_size = output_pins.size();
        if (input_pins_size != output_pins_size) {
            // Should never happen
            push_error("Unexpected difference of input and output pins for switch node.");
            return create_stop_result();
        }

        IfNode* base_if = nullptr;
        IfNode* prev_if = nullptr;
        for (uint64_t i = 2; i < input_pins_size; i++) {
            ExpressionNode* lhs = resolve_input(input_pins[1]); // Always value pin
            ExpressionNode* rhs = resolve_input(input_pins[i]); // Case pin
            BinaryOpNode* cond = create_binary_op(VariantOperators::OP_EQUAL, lhs, rhs);

            cond->script_node_id = p_script_node->get_id();
            lhs->script_node_id = p_script_node->get_id();
            rhs->script_node_id = p_script_node->get_id();

            if (base_if == nullptr) {
                base_if = create_if(cond, output_pins[i], nullptr);
                base_if->script_node_id = p_script_node->get_id();
                prev_if = base_if;
            } else {
                // ElseIf
                SuiteNode* previous_suite = current_suite;
                SuiteNode* elseif_block = alloc_node<SuiteNode>();
                elseif_block->parent_function = current_function;
                elseif_block->parent_block = current_suite;
                current_suite = elseif_block;

                IfNode* elif_node = create_if(cond, output_pins[i], nullptr);
                elif_node->script_node_id = p_script_node->get_id();
                elseif_block->statements.push_back(elif_node);
                prev_if->false_block = elseif_block;

                current_suite = previous_suite;
                prev_if = elif_node;
            }
        }

        // At the end create a final else block that exists output pin 1
        SuiteNode* else_suite = build_suite("else block", output_pins[1]);
        prev_if->false_block = else_suite;

        // Add the base if and continue out output pin 0.
        add_statement(base_if);
        return create_statement_result(p_script_node, 0);
    }
}

OScriptParser::StatementResult OScriptParser::build_switch_on_string(const Ref<OScriptNodeSwitchString>& p_script_node) {
    MatchNode* match_node = alloc_node<MatchNode>();
    match_node->test = resolve_input(p_script_node->find_pin(1, PD_Input));
    match_node->script_node_id = p_script_node->get_id();

    for (const Ref<OScriptNodePin>& output_pin : p_script_node->find_pins(PD_Output)) {
        if (output_pin.is_valid() && output_pin->has_any_connections()) {
            MatchBranchNode* branch = alloc_node<MatchBranchNode>();
            branch->script_node_id = p_script_node->get_id();
            PatternNode* pattern = alloc_node<PatternNode>();
            pattern->script_node_id = p_script_node->get_id();

            const String pin_name = output_pin->get_label();
            if (pin_name != "Default") {
                pattern->pattern_type = PatternNode::PT_LITERAL;
                pattern->literal = create_literal(pin_name);
            } else {
                pattern->pattern_type = PatternNode::PT_WILDCARD;
            }

            branch->patterns.push_back(pattern);
            match_node->branches.push_back(branch);
            branch->block = build_suite("match branch block", output_pin);
        }
    }

    add_statement(match_node);
    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_switch_on_integer(const Ref<OScriptNodeSwitchInteger>& p_script_node) {
    MatchNode* match_node = alloc_node<MatchNode>();
    match_node->test = resolve_input(p_script_node->find_pin(1, PD_Input));
    match_node->test->script_node_id = p_script_node->get_id();

    for (const Ref<OScriptNodePin>& output_pin : p_script_node->find_pins(PD_Output)) {
        if (output_pin.is_valid() && output_pin->has_any_connections()) {
            MatchBranchNode* branch = alloc_node<MatchBranchNode>();
            branch->script_node_id = p_script_node->get_id();
            PatternNode* pattern = alloc_node<PatternNode>();
            pattern->script_node_id = p_script_node->get_id();

            const String pin_name = output_pin->get_label();
            if (pin_name != "Default") {
                pattern->pattern_type = PatternNode::PT_LITERAL;
                pattern->literal = create_literal(pin_name.to_int());
            } else {
                pattern->pattern_type = PatternNode::PT_WILDCARD;
            }

            branch->patterns.push_back(pattern);
            match_node->branches.push_back(branch);
            branch->block = build_suite("match branch block", output_pin);
        }
    }

    bool all_have_returns = true;
    for (MatchBranchNode* branch : match_node->branches) {
        if (!branch->block->has_return) {
            all_have_returns = false;
            break;
        }
    }

    add_statement(match_node);

    if (all_have_returns) {
        set_return();
    }

    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_switch_on_enum(const Ref<OScriptNodeSwitchEnum>& p_script_node) {
    MatchNode* match_node = alloc_node<MatchNode>();
    match_node->test = resolve_input(p_script_node->find_pin(1, PD_Input));
    match_node->test->script_node_id = p_script_node->get_id();

    const EnumInfo& ei = ExtensionDB::get_global_enum(p_script_node->get_enum_name());
    for (const Ref<OScriptNodePin>& output_pin : p_script_node->find_pins(PD_Output)) {
        if (output_pin.is_valid()) {
            for (const EnumValue& value : ei.values) {
                if (output_pin->get_generated_default_value() == Variant(value.value)) {
                    if (output_pin->has_any_connections()) {
                        MatchBranchNode* branch = alloc_node<MatchBranchNode>();
                        branch->script_node_id = p_script_node->get_id();
                        PatternNode* pattern = alloc_node<PatternNode>();
                        pattern->script_node_id = p_script_node->get_id();
                        pattern->pattern_type = PatternNode::PT_EXPRESSION;
                        pattern->expression = build_identifier(value.name);
                        branch->patterns.push_back(pattern);
                        match_node->branches.push_back(branch);
                        branch->block = build_suite("match branch block", output_pin);
                    }
                }
            }
        }
    }

    add_statement(match_node);
    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_random(const Ref<OScriptNodeRandom>& p_script_node) {
    const int num_possibilities = p_script_node->get_possibility_count();

    // Short-circuit
    // If there is only one choice, we can treat this node as a no-op
    if (num_possibilities == 1) {
        const Ref<OScriptNodePin> out = p_script_node->find_pin(0, PD_Output);
        if (out.is_null() || !out->has_any_connections()) {
            return {};
        }
        return create_statement_result(p_script_node, 0);
    }

    // Short-circuit
    // If none of the output paths have connections, treat as no-op
    bool connections = false;
    for (int i = 0; i < num_possibilities; i++) {
        const Ref<OScriptNodePin> output = p_script_node->find_pin(i, PD_Output);
        if (output.is_valid() && output->has_any_connections()) {
            connections = true;
            break;
        }
    }
    if (!connections) {
        return {};
    }

    CallNode* random_value = alloc_node<CallNode>();
    random_value->callee = build_identifier("randi_range");
    random_value->callee->script_node_id = p_script_node->get_id();
    random_value->function_name = "randi_range";
    random_value->arguments.push_back(create_literal(1));
    random_value->arguments.push_back(create_literal(num_possibilities));
    random_value->script_node_id = p_script_node->get_id();

    MatchNode* match_node = alloc_node<MatchNode>();
    match_node->test = random_value;
    match_node->script_node_id = p_script_node->get_id();

    for (int i = 1; i <= num_possibilities; i++) {
        const Ref<OScriptNodePin> output_pin = p_script_node->find_pin(i - 1, PD_Output);
        if (output_pin.is_valid() && output_pin->has_any_connections()) {
            MatchBranchNode* match_branch = alloc_node<MatchBranchNode>();
            match_branch->script_node_id = p_script_node->get_id();
            PatternNode* pattern = alloc_node<PatternNode>();
            pattern->script_node_id = p_script_node->get_id();
            pattern->pattern_type = PatternNode::PT_LITERAL;
            pattern->literal = create_literal(i);
            match_branch->patterns.push_back(pattern);
            match_node->branches.push_back(match_branch);
            match_branch->block = build_suite("match branch block", output_pin);
        }
    }

    add_statement(match_node);

    // Control path is dynamic
    return create_stop_result();
}

OScriptParser::StatementResult OScriptParser::build_instantiate_scene(const Ref<OScriptNodeInstantiateScene>& p_script_node) {
    const Ref<OScriptNodePin> scene_pin = p_script_node->find_pin(1, PD_Output);
    const String scene_term = create_cached_variable_name(scene_pin);

    // todo: consider having the node operate via toggle to always create a new instance when traversed.
    CallNode* call_node = create_func_call("_oscript_internal_instantiate_scene");
    call_node->arguments.push_back(resolve_input(p_script_node->find_pin(1, PD_Input)));
    call_node->script_node_id = p_script_node->get_id();

    create_local_and_push(scene_term, call_node);
    add_pin_alias(scene_term, scene_pin);

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_await_signal(const Ref<OScriptNodeAwaitSignal>& p_script_node) {
    SubscriptNode* the_signal = alloc_node<SubscriptNode>();
    the_signal->base = resolve_input(p_script_node->find_pin(1, PD_Input));
    the_signal->index = resolve_input(p_script_node->find_pin(2, PD_Input));
    the_signal->base->script_node_id = p_script_node->get_id();
    the_signal->index->script_node_id = p_script_node->get_id();
    the_signal->script_node_id = p_script_node->get_id();

    // Await on the signal
    AwaitNode* await_node = alloc_node<AwaitNode>();
    await_node->to_await = the_signal;
    await_node->script_node_id = p_script_node->get_id();
    set_coroutine();

    const String result_term = vformat("node_%s_result", p_script_node->get_id());
    create_local_and_push(result_term, await_node);

    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_emit_member_signal(const Ref<OScriptNodeEmitMemberSignal>& p_script_node) {
    const Ref<OScriptNodePin> member_pin = p_script_node->find_pin(1, PD_Input);
    CallNode* call_node = create_func_call(resolve_input(member_pin), "emit_signal");
    call_node->script_node_id = p_script_node->get_id();
    call_node->arguments.push_back(create_literal(p_script_node->get_signal_info().name));

    const int inputs = p_script_node->find_pins(PD_Input).size() - 2; // execution and instance
    for (int i = 0; i < inputs; i++) {
        const Ref<OScriptNodePin> input = p_script_node->find_pin(i + 2, PD_Input);
        call_node->arguments.push_back(resolve_input(input));
    }

    add_statement(call_node);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_emit_signal(const Ref<OScriptNodeEmitSignal>& p_script_node) {
    CallNode* emit_signal = create_func_call("emit_signal");
    emit_signal->script_node_id = p_script_node->get_id();
    emit_signal->arguments.push_back(create_literal(p_script_node->get_signal_name()));
    const Ref<OScriptSignal> the_signal = p_script_node->get_signal();
    if (the_signal.is_valid()) {
        for (const Ref<OScriptNodePin>& input : p_script_node->find_pins(PD_Input)) {
            if (input->is_execution()) {
                continue;
            }
            emit_signal->arguments.push_back(resolve_input(input));
        }
    }
    add_statement(emit_signal);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_print_string(const Ref<OScriptNodePrintString>& p_script_node) {
    #if TOOLS_ENABLED
    // PrintString only is compiled when not in an exported game.
    CallNode* call_node = create_func_call("_oscript_internal_print_string");
    call_node->script_node_id = p_script_node->get_id();
    call_node->arguments.push_back(create_literal(is_tool()));
    for (const Ref<OScriptNodePin>& input : p_script_node->find_pins(PD_Input)) {
        if (input->is_execution()) {
            continue;
        }
        call_node->arguments.push_back(resolve_input(input));
    }
    add_statement(call_node);
    #endif
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_message_dialogue(const Ref<OScriptNodeDialogueMessage>& p_script_node) {
    const Ref<OScriptNodePin> character_name = p_script_node->find_pin(1, PD_Input);
    const Ref<OScriptNodePin> message = p_script_node->find_pin(2, PD_Input);
    const Ref<OScriptNodePin> scene = p_script_node->find_pin(3, PD_Input);

    DictionaryNode* options = alloc_node<DictionaryNode>();
    options->elements.push_back({ create_literal("character_name"), resolve_input(character_name) });
    options->elements.push_back({ create_literal("message"), resolve_input(message) });

    const int choice_count = p_script_node->get_choices();
    if (choice_count > 0) {
        DictionaryNode* choices = alloc_node<DictionaryNode>();
        for (int i = 0; i < p_script_node->get_choices(); i++) {
            const Ref<OScriptNodePin> choice_pin = p_script_node->find_pin(4 + i, PD_Input);
            choices->elements.push_back({ create_literal(i), resolve_input(choice_pin) });
        }
        options->elements.push_back({ create_literal("options"), choices });
    }

    CallNode* call_node = create_func_call("_oscript_internal_show_dialogue");
    call_node->arguments.push_back(create_func_call("get_parent"));
    call_node->arguments.push_back(resolve_input(scene));
    call_node->arguments.push_back(options);
    call_node->script_node_id = p_script_node->get_id();

    const String dialogue_node = create_cached_variable_name(p_script_node->find_pin(0, PD_Input));
    create_local_and_push(dialogue_node, call_node);

    SubscriptNode* the_signal = alloc_node<SubscriptNode>();
    the_signal->base = build_identifier(dialogue_node);
    the_signal->attribute = build_identifier("show_message_finished");
    the_signal->is_attribute = true;

    // Await on the signal
    AwaitNode* await_node = alloc_node<AwaitNode>();
    await_node->to_await = the_signal;
    set_coroutine();

    const String await_result = vformat("dialogue_%d_signal_result", p_script_node->get_id());
    create_local_and_push(await_result, await_node);

    if (choice_count == 0) {
        return create_statement_result(p_script_node, 0);
    }

    CallNode* arg_get = create_func_call(dialogue_node, "get");
    arg_get->arguments.push_back(create_literal("selection"));

    // When there are choices we determine path based on result
    MatchNode* match_node = alloc_node<MatchNode>();
    match_node->test = arg_get;
    for (int i = 0; i < choice_count; i++) {
        const Ref<OScriptNodePin> output_pin = p_script_node->find_pin(4 + i, PD_Output);
        if (output_pin.is_valid() && output_pin->has_any_connections()) {
            MatchBranchNode* branch = alloc_node<MatchBranchNode>();
            PatternNode* pattern = alloc_node<PatternNode>();
            pattern->pattern_type = PatternNode::PT_LITERAL;
            pattern->literal = create_literal(i);
            branch->patterns.push_back(pattern);
            match_node->branches.push_back(branch);
            branch->block = build_suite("match branch block", output_pin);
        }
    }

    add_statement(match_node);

    return create_divergence_result(p_script_node);
}

OScriptParser::StatementResult OScriptParser::build_new_object(const Ref<OScriptNodeNew>& p_script_node) {
    const Ref<OScriptNodePin> value_pin = p_script_node->find_pin(1, PD_Output);
    CallNode* new_object = create_func_call(p_script_node->get_allocated_class_name(), "new");
    new_object->script_node_id = p_script_node->get_id();
    create_local_and_push(create_cached_variable_name(value_pin), new_object);
    return create_statement_result(p_script_node, 0);
}

OScriptParser::StatementResult OScriptParser::build_free_object(const Ref<OScriptNodeFree>& p_script_node) {
    const Ref<OScriptNodePin> object_pin = p_script_node->find_pin(1, PD_Input);

    if (object_pin->has_any_connections()) {
        const StringName class_name = object_pin->get_connection()->get_property_info().class_name;

        bool is_node = false;
        if (ScriptServer::is_global_class(class_name)) {
            const StringName native_base = ScriptServer::get_global_class_native_base(class_name);
            is_node = ClassDB::is_parent_class(native_base, "Node");
        } else {
            is_node = ClassDB::is_parent_class(class_name, "Node");
        }

        CallNode* free_object = create_func_call(resolve_input(object_pin), is_node ? "queue_free" : "free");
        free_object->script_node_id = p_script_node->get_id();
        add_statement(free_object);
    }

    return create_statement_result(p_script_node, 0);
}

OScriptParser::ClassNode* OScriptParser::build_class(Orchestration* p_orchestration) {
    _is_tool = p_orchestration->get_tool();

    ClassNode* clazz = alloc_node<ClassNode>();
    clazz->fqcn = OScript::canonicalize_path(script_path);
    current_class = clazz;

    if (p_orchestration->get_base_type().begins_with("res://")) {
        clazz->extends_path = p_orchestration->get_base_type();
        clazz->extends_used = true;
    } else {
        IdentifierNode* base = build_identifier(p_orchestration->get_base_type());
        clazz->extends.push_back(base);
        clazz->extends_used = true;
    }

    if (!p_orchestration->get_global_name().is_empty()) {
         clazz->identifier = build_identifier(p_orchestration->get_global_name());
         clazz->fqcn = clazz->identifier->name;
    }

    if (!p_orchestration->get_icon_path().is_empty()) {
        clazz->icon_path = p_orchestration->get_icon_path();
        if (clazz->icon_path.is_empty() || clazz->icon_path.is_absolute_path()) {
            clazz->simplified_icon_path = clazz->icon_path.simplify_path();
        } else if (clazz->icon_path.is_relative_path()) {
            clazz->simplified_icon_path = script_path.get_base_dir().path_join(clazz->icon_path).simplify_path();
        } else {
            clazz->simplified_icon_path = clazz->icon_path;
        }
    }

    for (const Ref<OScriptVariable>& variable : p_orchestration->get_variables()) {
        VariableNode* node = build_variable(variable);
        clazz->add_member(node);
    }

    for (const Ref<OScriptSignal>& signal : p_orchestration->get_custom_signals()) {
        SignalNode* node = build_signal(signal);
        clazz->add_member(node);
    }

    for (const Ref<OScriptGraph>& graph : p_orchestration->get_graphs()) {
        if (graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION)) {
            // This physical function
            const Ref<OScriptFunction> function = graph->get_functions()[0];
            if (function.is_valid()) {
                FunctionNode* node = build_function(function, graph);
                clazz->add_member(node);
            }
        }
        else if (graph->get_flags().has_flag(OScriptGraph::GF_EVENT)) {
            for (const Ref<OScriptFunction>& function : graph->get_functions()) {
                if (function.is_valid()) {
                    FunctionNode* node = build_function(function, graph);
                    clazz->add_member(node);
                }
            }
        }
    }

    #ifdef TOOLS_ENABLED
    if (!p_orchestration->get_brief_description().is_empty()) {
        clazz->doc_data.brief = p_orchestration->get_brief_description();
    }
    if (!p_orchestration->get_description().is_empty()) {
        clazz->doc_data.description = p_orchestration->get_description();
    }
    #endif

    return clazz;
}

OScriptParser::VariableNode* OScriptParser::build_variable(const Ref<OScriptVariable>& p_variable) {
    IdentifierNode* identifier = build_identifier(p_variable->get_variable_name());

    IdentifierNode* type_name = alloc_node<IdentifierNode>();
    type_name->name = p_variable->get_variable_type_name();

    TypeNode* type = alloc_node<TypeNode>();
    type->type_chain.push_back(type_name);

    VariableNode* node = alloc_node<VariableNode>();
    node->identifier = identifier;
    node->export_info = p_variable->get_info();
    node->export_info.usage &= ~PROPERTY_USAGE_SCRIPT_VARIABLE;
    node->datatype_specifier = type;

    if (p_variable->is_exported()) {
         AnnotationNode* annotation = memnew(AnnotationNode);
         annotation->name = "@export";
         annotation->info = &valid_annotations[annotation->name];
         annotation->applies_to(AnnotationInfo::TargetKind::VARIABLE);
         node->annotations.push_back(annotation);
    }

    if (p_variable->get_default_value().get_type() != Variant::NIL) {
        LiteralNode* default_value = alloc_node<LiteralNode>();
        default_value->value = p_variable->get_default_value();
        if (!p_variable->is_constant()) {
            default_value->is_constant = false;
        }
        node->initializer = default_value;
        node->assignments++;
    }

    #ifdef TOOLS_ENABLED
    if (!p_variable->get_description().is_empty()) {
        node->doc_data.description = p_variable->get_description();
    }
    #endif

    return node;
}

OScriptParser::SignalNode* OScriptParser::build_signal(const Ref<OScriptSignal>& p_signal) {
    SignalNode* signal = alloc_node<SignalNode>();
    signal->identifier = build_identifier(p_signal->get_signal_name());

    const MethodInfo method_info = p_signal->get_method_info();
    signal->method = method_info;
    signal->method.name = "";
    signal->method.arguments.clear();

    for (const PropertyInfo& property : method_info.arguments) {
        ParameterNode* param = build_parameter(property);
        if (param == nullptr) {
            push_error("Expected signal parameter");
            break;
        }

        if (param->initializer != nullptr) {
            push_error(R"(Signal parameters cannot have a default value.")");
        }

        if (signal->parameters_indices.has(param->identifier->name)) {
            push_error(vformat(R"(Parameter with name "%s" was already declared for this signal.")", param->identifier->name));
        } else {
            signal->parameters_indices[param->identifier->name] = signal->parameters.size();
            signal->parameters.push_back(param);
        }
    }

    #ifdef TOOLS_ENABLED
    signal->doc_data.description = p_signal->get_description();
    #endif

    return signal;
}

OScriptParser::FunctionNode* OScriptParser::build_function(const Ref<OScriptFunction>& p_function, const Ref<OScriptGraph>& p_graph) {

    FunctionNode* function_node = alloc_node<FunctionNode>();
    FunctionNode* prev_function = current_function;
    current_function = function_node;

    function_node->identifier = alloc_node<IdentifierNode>();
    function_node->identifier->name = p_function->get_function_name();
    function_node->script_node_id = p_function->get_owning_node_id();
    function_node->method = p_function->get_method_info();
    function_node->method.name = "";
    function_node->method.flags = METHOD_FLAGS_DEFAULT;
    function_node->method.arguments.clear();
    function_node->method.return_val = PropertyInfo();

    for (const PropertyInfo& argument : p_function->get_method_info().arguments) {
        ParameterNode* param = build_parameter(argument);
        function_node->parameters_indices[param->identifier->name] = function_node->parameters.size();
        function_node->parameters.push_back(param);
    }

    for (const Variant& default_value : p_function->get_method_info().default_arguments) {
        function_node->default_arg_values.push_back(default_value);
    }

    function_node->return_type = build_type(p_function->get_method_info().return_val);

    #ifdef TOOLS_ENABLED
    function_node->doc_data.description = p_function->get_description();
    #endif

    // Perform function graph pre-pass analysis
    OScriptFunctionAnalyzer analyzer;
    function_info = analyzer.analyze_function(p_function);
    // UtilityFunctions::print(function_info.to_string());

    bool has_body = false;
    Ref<OScriptNodePin> source_pin;
    const Ref<OScriptNode> entry = p_function->get_owning_node();
    if (entry.is_valid()) {
        const Ref<OScriptNodePin> output = entry->find_pin(0, PD_Output);
        if (output.is_valid()) {
            has_body = true;
            source_pin = output;
        }
    } else {
        ERR_PRINT(vformat("Function %s entry node is not bound.", p_function->get_function_name()));
    }

    // Whether function has body or not, it needs a suite.
    SuiteNode* body = alloc_node<SuiteNode>();
    if (has_body) {
        // Apply function parameters
        for (ParameterNode* parameter : function_node->parameters) {
            body->add_local(parameter, current_function);
        }

        // Apply function local variables
        for (const KeyValue<NodeId, StringName>& local_var : function_info.local_variables) {
            const Ref<OScriptNodeLocalVariable> var_node = p_function->get_graph()->get_node(local_var.key);
            if (var_node.is_valid()) {
                const Ref<OScriptNodePin> pin = var_node->find_pin(0, PD_Output);
                if (pin.is_valid()) {
                    VariableNode* local = create_local(create_cached_variable_name(pin), nullptr, body);
                    local->datatype_specifier = build_type(pin->get_property_info());
                    body->add_local(local, function_node);
                    add_statement(local, body);
                }
            }
        }

        // Parse body
        const String suite_name = vformat("Function %s", p_function->get_function_name());
        function_node->body = build_suite(suite_name, source_pin, body);
    } else {
        // Function does not have a body, assign empty suite
        function_node->body = body;
    }

    current_function = prev_function;
    return function_node;
}

OScriptParser::ParameterNode* OScriptParser::build_parameter(const PropertyInfo& p_property) {
    ParameterNode* param = alloc_node<ParameterNode>();
    param->identifier = build_identifier(p_property.name);
    param->datatype_specifier = build_type(p_property);
    return param;
}

OScriptParser::TypeNode* OScriptParser::build_type(const PropertyInfo& p_property) {
    TypeNode* type = alloc_node<TypeNode>();

    if (p_property.usage & PROPERTY_USAGE_CLASS_IS_ENUM || p_property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD) {
        if (p_property.class_name.contains(".")) {
            const PackedStringArray parts = p_property.class_name.split(".", false);
            for (const String& part : parts) {
                IdentifierNode* element = build_identifier(part);
                type->type_chain.push_back(element);
            }
        }
    } else if (p_property.type == Variant::ARRAY && p_property.hint == PROPERTY_HINT_ARRAY_TYPE) {
        // Typed Array
        TypeNode* element = alloc_node<TypeNode>();
        element->type_chain.push_back(build_identifier(p_property.hint_string));
        type->container_types.push_back(element);
    } else if (p_property.type == Variant::DICTIONARY && p_property.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
        // Typed Dictionary
        const PackedStringArray parts = p_property.hint_string.split(";", false);
        for (const String& part : parts) {
            TypeNode* container_type = alloc_node<TypeNode>();
            container_type->type_chain.push_back(build_identifier(part));
            type->container_types.push_back(container_type);
        }
    } else if (p_property.type == Variant::OBJECT && !p_property.class_name.is_empty()) {
        type->type_chain.push_back(build_identifier(p_property.class_name));
    } else if (p_property.type == Variant::NIL) {
        if (p_property.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
            type->type_chain.push_back(build_identifier("Variant"));
        }
    } else {
        type->type_chain.push_back(build_identifier(Variant::get_type_name(p_property.type)));
    }

    return type;
}

OScriptParser::SuiteNode* OScriptParser::build_suite(const String& p_name, const Ref<OScriptNodePin>& p_source_pin, SuiteNode* p_suite) {
    // Use provided suite if given, otherwise create a new one
    SuiteNode* suite = p_suite != nullptr ? p_suite : alloc_node<SuiteNode>();

    // Push suite onto the stack
    suite->parent_block = current_suite;
    suite->parent_function = current_function;
    current_suite = suite;

    // Push down loop context to nested suites
    if (suite->parent_block != nullptr && suite->parent_block->is_in_loop) {
        suite->is_in_loop = true;
    }

    if (p_source_pin.is_valid() && p_source_pin->has_any_connections()) {
        const Ref<OScriptNodePin> target_pin = p_source_pin->get_connection();
        // Build statements for the suite
        build_statements(p_source_pin, target_pin, suite);
    }

    // Pop the suite to the parent
    current_suite = suite->parent_block;

    // Return the just built suite block
    return suite;
}

template <PropertyHint t_hint, Variant::Type t_type>
bool OScriptParser::export_annotations(AnnotationNode *p_annotation, Node *p_target, ClassNode *p_class) {
	ERR_FAIL_COND_V_MSG(p_target->type != Node::VARIABLE, false, vformat(R"("%s" annotation can only be applied to variables.)", p_annotation->name));
	ERR_FAIL_NULL_V(p_class, false);

	VariableNode *variable = static_cast<VariableNode *>(p_target);
	if (variable->is_static) {
		push_error(vformat(R"(Annotation "%s" cannot be applied to a static variable.)", p_annotation->name), p_annotation);
		return false;
	}
	if (variable->exported) {
		push_error(vformat(R"(Annotation "%s" cannot be used with another "@export" annotation.)", p_annotation->name), p_annotation);
		return false;
	}

	variable->exported = true;

	variable->export_info.type = t_type;
	variable->export_info.hint = t_hint;

	String hint_string;
	for (int i = 0; i < p_annotation->resolved_arguments.size(); i++) {
		String arg_string = String(p_annotation->resolved_arguments[i]);

		if (p_annotation->name != StringName("@export_placeholder")) {
			if (arg_string.is_empty()) {
				push_error(vformat(R"(Argument %d of annotation "%s" is empty.)", i + 1, p_annotation->name), p_annotation->arguments[i]);
				return false;
			}
			if (arg_string.contains(",")) {
				push_error(vformat(R"(Argument %d of annotation "%s" contains a comma. Use separate arguments instead.)", i + 1, p_annotation->name), p_annotation->arguments[i]);
				return false;
			}
		}

		// WARNING: Do not merge with the previous `if` because there `!=`, not `==`!
		if (p_annotation->name == StringName("@export_flags")) {
			const int64_t max_flags = 32;
			PackedStringArray t = arg_string.split(":", true, 1);
			if (t[0].is_empty()) {
				push_error(vformat(R"(Invalid argument %d of annotation "@export_flags": Expected flag name.)", i + 1), p_annotation->arguments[i]);
				return false;
			}
			if (t.size() == 2) {
				if (t[1].is_empty()) {
					push_error(vformat(R"(Invalid argument %d of annotation "@export_flags": Expected flag value.)", i + 1), p_annotation->arguments[i]);
					return false;
				}
				if (!t[1].is_valid_int()) {
					push_error(vformat(R"(Invalid argument %d of annotation "@export_flags": The flag value must be a valid integer.)", i + 1), p_annotation->arguments[i]);
					return false;
				}
				int64_t value = t[1].to_int();
				if (value < 1 || value >= (1LL << max_flags)) {
					push_error(vformat(R"(Invalid argument %d of annotation "@export_flags": The flag value must be at least 1 and at most 2 ** %d - 1.)", i + 1, max_flags), p_annotation->arguments[i]);
					return false;
				}
			} else if (i >= max_flags) {
				push_error(vformat(R"(Invalid argument %d of annotation "@export_flags": Starting from argument %d, the flag value must be specified explicitly.)", i + 1, max_flags + 1), p_annotation->arguments[i]);
				return false;
			}
		} else if (p_annotation->name == StringName("@export_node_path")) {
			String native_class = arg_string;
			if (ScriptServer::is_global_class(arg_string)) {
				native_class = ScriptServer::get_global_class_native_base(arg_string);
			}
			if (!ClassDB::class_exists(native_class) || !GDE::ClassDB::is_class_exposed(native_class)) {
				push_error(vformat(R"(Invalid argument %d of annotation "@export_node_path": The class "%s" was not found in the global scope.)", i + 1, arg_string), p_annotation->arguments[i]);
				return false;
			} else if (!ClassDB::is_parent_class(native_class, StringName("Node"))) {
				push_error(vformat(R"(Invalid argument %d of annotation "@export_node_path": The class "%s" does not inherit "Node".)", i + 1, arg_string), p_annotation->arguments[i]);
				return false;
			}
		}

		if (i > 0) {
			hint_string += ",";
		}
		hint_string += arg_string;
	}
	variable->export_info.hint_string = hint_string;

	// This is called after the analyzer is done finding the type, so this should be set here.
	DataType export_type = variable->get_datatype();

	// Use initializer type if specified type is `Variant`.
	if (export_type.is_variant() && variable->initializer != nullptr && variable->initializer->data_type.is_set()) {
		export_type = variable->initializer->get_datatype();
		export_type.type_source = DataType::INFERRED;
	}

	const Variant::Type original_export_type_builtin = export_type.builtin_type;

	// Process array and packed array annotations on the element type.
	bool is_array = false;
	if (export_type.builtin_type == Variant::ARRAY && export_type.has_container_element_type(0)) {
		is_array = true;
		export_type = export_type.get_container_element_type(0);
	} else if (export_type.is_typed_container_type()) {
		is_array = true;
		export_type = export_type.get_typed_container_type();
		export_type.type_source = variable->data_type.type_source;
	}

	bool is_dict = false;
	if (export_type.builtin_type == Variant::DICTIONARY && export_type.has_container_element_types()) {
		is_dict = true;
		DataType inner_type = export_type.get_container_element_type_or_variant(1);
		export_type = export_type.get_container_element_type_or_variant(0);
		export_type.set_container_element_type(0, inner_type); // Store earlier extracted value within key to separately parse after.
	}

	bool use_default_variable_type_check = true;

	if (p_annotation->name == StringName("@export_range")) {
		if (export_type.builtin_type == Variant::INT) {
			variable->export_info.type = Variant::INT;
		}
	} else if (p_annotation->name == StringName("@export_multiline")) {
		use_default_variable_type_check = false;

		if (export_type.builtin_type != Variant::STRING && export_type.builtin_type != Variant::DICTIONARY) {
			Vector<Variant::Type> expected_types = { Variant::STRING, Variant::DICTIONARY };
			push_error(_get_annotation_error_string(p_annotation->name, expected_types, variable->get_datatype()), p_annotation);
			return false;
		}

		if (export_type.builtin_type == Variant::DICTIONARY) {
			variable->export_info.type = Variant::DICTIONARY;
		}
	} else if (p_annotation->name == StringName("@export")) {
		use_default_variable_type_check = false;

		if (variable->datatype_specifier == nullptr && variable->initializer == nullptr) {
			push_error(R"(Cannot use simple "@export" annotation with variable without type or initializer, since type can't be inferred.)", p_annotation);
			return false;
		}

		if (export_type.has_no_type()) {
			push_error(R"(Cannot use simple "@export" annotation because the type of the initialized value can't be inferred.)", p_annotation);
			return false;
		}

		switch (export_type.kind) {
			case DataType::BUILTIN: {
			    variable->export_info.type = export_type.builtin_type;
			    variable->export_info.hint = PROPERTY_HINT_NONE;
			    variable->export_info.hint_string = String();
			    break;
			}
			case DataType::NATIVE:
			case DataType::SCRIPT:
			case DataType::CLASS: {
				const StringName class_name = _find_narrowest_native_or_global_class(export_type);
				if (ClassDB::is_parent_class(export_type.native_type, StringName("Resource"))) {
					variable->export_info.type = Variant::OBJECT;
					variable->export_info.hint = PROPERTY_HINT_RESOURCE_TYPE;
					variable->export_info.hint_string = class_name;
				} else if (ClassDB::is_parent_class(export_type.native_type, StringName("Node"))) {
					variable->export_info.type = Variant::OBJECT;
					variable->export_info.hint = PROPERTY_HINT_NODE_TYPE;
					variable->export_info.hint_string = class_name;
				} else {
					push_error(R"(Export type can only be built-in, a resource, a node, or an enum.)", p_annotation);
					return false;
				}
			    break;
			}
			case DataType::ENUM: {
				if (export_type.is_meta_type) {
					variable->export_info.type = Variant::DICTIONARY;
				} else {
					variable->export_info.type = Variant::INT;
					variable->export_info.hint = PROPERTY_HINT_ENUM;

					String enum_hint_string;
					bool first = true;
					for (const KeyValue<StringName, int64_t> &E : export_type.enum_values) {
						if (!first) {
							enum_hint_string += ",";
						} else {
							first = false;
						}
						enum_hint_string += String(E.key).capitalize().xml_escape();
						enum_hint_string += ":";
						enum_hint_string += String::num_int64(E.value).xml_escape();
					}

					variable->export_info.hint_string = enum_hint_string;
					variable->export_info.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
					variable->export_info.class_name = String(export_type.native_type).replace("::", ".");
				}
			    break;
			}
			case DataType::VARIANT: {
				if (export_type.is_variant()) {
					variable->export_info.type = Variant::NIL;
					variable->export_info.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
				}
			    break;
			}
			default: {
			    push_error(R"(Export type can only be built-in, a resource, a node, or an enum.)", p_annotation);
			    return false;
			}
		}

		if (variable->export_info.hint == PROPERTY_HINT_NODE_TYPE && !ClassDB::is_parent_class(p_class->base_type.native_type, StringName("Node"))) {
			push_error(vformat(R"(Node export is only supported in Node-derived classes, but the current class inherits "%s".)", p_class->base_type.to_string()), p_annotation);
			return false;
		}

		if (is_dict) {
			String key_prefix = itos(variable->export_info.type);
			if (variable->export_info.hint) {
				key_prefix += "/" + itos(variable->export_info.hint);
			}
			key_prefix += ":" + variable->export_info.hint_string;

			// Now parse value.
			export_type = export_type.get_container_element_type(0);

			if (export_type.is_variant() || export_type.has_no_type()) {
				export_type.kind = DataType::BUILTIN;
			}

			switch (export_type.kind) {
				case DataType::BUILTIN: {
				    variable->export_info.type = export_type.builtin_type;
				    variable->export_info.hint = PROPERTY_HINT_NONE;
				    variable->export_info.hint_string = String();
				    break;
				}
				case DataType::NATIVE:
				case DataType::SCRIPT:
				case DataType::CLASS: {
					const StringName class_name = _find_narrowest_native_or_global_class(export_type);
					if (ClassDB::is_parent_class(export_type.native_type, StringName("Resource"))) {
						variable->export_info.type = Variant::OBJECT;
						variable->export_info.hint = PROPERTY_HINT_RESOURCE_TYPE;
						variable->export_info.hint_string = class_name;
					} else if (ClassDB::is_parent_class(export_type.native_type, StringName("Node"))) {
						variable->export_info.type = Variant::OBJECT;
						variable->export_info.hint = PROPERTY_HINT_NODE_TYPE;
						variable->export_info.hint_string = class_name;
					} else {
						push_error(R"(Export type can only be built-in, a resource, a node, or an enum.)", p_annotation);
						return false;
					}
				    break;
				}
				case DataType::ENUM: {
					if (export_type.is_meta_type) {
						variable->export_info.type = Variant::DICTIONARY;
					} else {
						variable->export_info.type = Variant::INT;
						variable->export_info.hint = PROPERTY_HINT_ENUM;

						String enum_hint_string;
						bool first = true;
						for (const KeyValue<StringName, int64_t> &E : export_type.enum_values) {
							if (!first) {
								enum_hint_string += ",";
							} else {
								first = false;
							}
							enum_hint_string += String(E.key).capitalize().xml_escape();
							enum_hint_string += ":";
							enum_hint_string += String::num_int64(E.value).xml_escape();
						}

						variable->export_info.hint_string = enum_hint_string;
						variable->export_info.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
						variable->export_info.class_name = String(export_type.native_type).replace("::", ".");
					}
				    break;
				}
				default: {
				    push_error(R"(Export type can only be built-in, a resource, a node, or an enum.)", p_annotation);
				    return false;
				}
			}

			if (variable->export_info.hint == PROPERTY_HINT_NODE_TYPE && !ClassDB::is_parent_class(p_class->base_type.native_type, StringName("Node"))) {
				push_error(vformat(R"(Node export is only supported in Node-derived classes, but the current class inherits "%s".)", p_class->base_type.to_string()), p_annotation);
				return false;
			}

			String value_prefix = itos(variable->export_info.type);
			if (variable->export_info.hint) {
				value_prefix += "/" + itos(variable->export_info.hint);
			}
			value_prefix += ":" + variable->export_info.hint_string;

			variable->export_info.type = Variant::DICTIONARY;
			variable->export_info.hint = PROPERTY_HINT_TYPE_STRING;
			variable->export_info.hint_string = key_prefix + ";" + value_prefix;
			variable->export_info.usage = PROPERTY_USAGE_DEFAULT;
			variable->export_info.class_name = StringName();
		}
	} else if (p_annotation->name == StringName("@export_enum")) {
		use_default_variable_type_check = false;

		Variant::Type enum_type = Variant::INT;

		if (export_type.kind == DataType::BUILTIN && export_type.builtin_type == Variant::STRING) {
			enum_type = Variant::STRING;
		}

		variable->export_info.type = enum_type;

		if (!export_type.is_variant() && (export_type.kind != DataType::BUILTIN || export_type.builtin_type != enum_type)) {
			Vector<Variant::Type> expected_types = { Variant::INT, Variant::STRING };
			push_error(_get_annotation_error_string(p_annotation->name, expected_types, variable->get_datatype()), p_annotation);
			return false;
		}
	}

	if (use_default_variable_type_check) {
		// Validate variable type with export.
		if (!export_type.is_variant() && (export_type.kind != DataType::BUILTIN || export_type.builtin_type != t_type)) {
			// Allow float/int conversion.
			if ((t_type != Variant::FLOAT || export_type.builtin_type != Variant::INT) && (t_type != Variant::INT || export_type.builtin_type != Variant::FLOAT)) {
				Vector<Variant::Type> expected_types = { t_type };
				push_error(_get_annotation_error_string(p_annotation->name, expected_types, variable->get_datatype()), p_annotation);
				return false;
			}
		}
	}

	if (is_array) {
		String hint_prefix = itos(variable->export_info.type);
		if (variable->export_info.hint) {
			hint_prefix += "/" + itos(variable->export_info.hint);
		}
		variable->export_info.type = original_export_type_builtin;
		variable->export_info.hint = PROPERTY_HINT_TYPE_STRING;
		variable->export_info.hint_string = hint_prefix + ":" + variable->export_info.hint_string;
		variable->export_info.usage = PROPERTY_USAGE_DEFAULT;
		variable->export_info.class_name = StringName();
	}

	return true;
}

Error OScriptParser::parse(Orchestration* p_orchestration, const String& p_script_path) {
    ERR_FAIL_NULL_V_MSG(p_orchestration, ERR_PARSE_ERROR, "Orchestration was null and cannot be parsed.");

    script_path = p_script_path;
    head = build_class(p_orchestration);

    return OK;
}

Error OScriptParser::parse(const OScriptSource& p_source, const String& p_script_path) {
    switch (p_source.get_type()) {
        case OScriptSource::BINARY: {
            OrchestrationBinaryParser parser;
            Ref<Orchestration> orchestration = parser.load(p_script_path);
            if (orchestration.is_valid()) {
                return parse(orchestration.ptr(), p_script_path);
            }
            return ERR_PARSE_ERROR;
        }
        default: {
            OrchestrationTextParser parser;
            Ref<Orchestration> orchestration = parser.load(p_script_path);
            if (orchestration.is_valid()) {
                return parse(orchestration.ptr(), p_script_path);
            }
            return ERR_PARSE_ERROR;
        }
    }
}

Ref<OScriptParserRef> OScriptParser::get_depended_parser_for(const String& p_path) {
    Ref<OScriptParserRef> ref;
    if (depended_parsers.has(p_path)) {
        ref = depended_parsers[p_path];
    } else {
        Error err = OK;
        ref = OScriptCache::get_parser(p_path, OScriptParserRef::EMPTY, err, script_path);
        if (ref.is_valid()) {
            depended_parsers[p_path] = ref;
        }
    }
    return ref;
}

const HashMap<String, Ref<OScriptParserRef>>& OScriptParser::get_depended_parsers() {
    return depended_parsers;
}

OScriptParser::ClassNode* OScriptParser::find_class(const String& p_qualified_name) const {
    String first = p_qualified_name.get_slice("::", 0);

    PackedStringArray class_names;
    ClassNode *result = nullptr;
    // Empty initial name means start at the head.
    if (first.is_empty() || (head->identifier && first == head->identifier->name)) {
        class_names = p_qualified_name.split("::");
        result = head;
    } else if (p_qualified_name.begins_with(script_path)) {
        // Script path could have a class path separator("::") in it.
        class_names = p_qualified_name.trim_prefix(script_path).split("::");
        result = head;
    } else if (head->has_member(first)) {
        class_names = p_qualified_name.split("::");
        ClassNode::Member member = head->get_member(first);
        if (member.type == ClassNode::Member::CLASS) {
            result = member.m_class;
        }
    }

    // Starts at index 1 because index 0 was handled above.
    for (int i = 1; result != nullptr && i < class_names.size(); i++) {
        const String &current_name = class_names[i];
        ClassNode *next = nullptr;
        if (result->has_member(current_name)) {
            ClassNode::Member member = result->get_member(current_name);
            if (member.type == ClassNode::Member::CLASS) {
                next = member.m_class;
            }
        }
        result = next;
    }

    return result;
}

bool OScriptParser::has_class(const ClassNode* p_class) const {
    if (head->fqcn.is_empty() && p_class->fqcn.get_slice("::", 0).is_empty()) {
        return p_class == head;
    } else if (p_class->fqcn.begins_with(head->fqcn)) {
        return find_class(p_class->fqcn.trim_prefix(head->fqcn)) == p_class;
    }

    return false;
}

// This function is used to determine that a type is "built-in" as opposed to native
// and custom classes. So `Variant::NIL` and `Variant::OBJECT` are excluded:
// `Variant::NIL` - `null` is literal, not a type.
// `Variant::OBJECT` - `Object` should be treated as a class, not as a built-in type.
static HashMap<StringName, Variant::Type> builtin_types;
Variant::Type OScriptParser::get_builtin_type(const StringName &p_type) {
    if (unlikely(builtin_types.is_empty())) {
        for (int i = 0; i < Variant::VARIANT_MAX; i++) {
            Variant::Type type = static_cast<Variant::Type>(i);
            if (type != Variant::NIL && type != Variant::OBJECT) {
                builtin_types[Variant::get_type_name(type)] = type;
            }
        }
    }

    if (builtin_types.has(p_type)) {
        return builtin_types[p_type];
    }

    return Variant::VARIANT_MAX;
}

OScriptParser::OScriptParser() {
    use_node_convergence = ORCHESTRATOR_GET("settings/runtime/use_node_convergence", true);

    bind_handlers();

    if (unlikely(valid_annotations.is_empty())) {
        register_annotation(MethodInfo("@export"), AnnotationInfo::VARIABLE, &OScriptParser::export_annotations<PROPERTY_HINT_NONE, Variant::NIL>);
    }
}

OScriptParser::~OScriptParser() {
    while (node_list_head != nullptr) {
        Node *element = node_list_head;
        node_list_head = node_list_head->next;
        memdelete(element);
    }
}


