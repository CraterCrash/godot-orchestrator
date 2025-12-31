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
#include "script/parser/parser_tree_utils.h"

#ifdef DEV_TOOLS

void OScriptParserUtils::StringBuilder::append(const String& p_text) {
    for (int i = 0; i < spaces; i++) {
        buffer += " ";
    }
    buffer += p_text;
}

void OScriptParserUtils::StringBuilder::push_text(const String& p_text) {
    append(p_text);
}

void OScriptParserUtils::StringBuilder::push_line(const String& p_text) {
    append(vformat("%s\n", p_text));
}

OScriptParserUtils::StringBuilder::IndentScope::IndentScope(StringBuilder& p_builder) : builder(p_builder) {
    builder.spaces += 4;
}

OScriptParserUtils::StringBuilder::IndentScope::~IndentScope() {
    builder.spaces -= 4;
    builder.spaces = MAX(0, builder.spaces);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Printer

void OScriptParserUtils::Printer::increase_indent() {
	indent_level++;
	indent = "";
	for (int i = 0; i < indent_level * 4; i++) {
		if (i % 4 == 0) {
			indent += "|";
		} else {
			indent += " ";
		}
	}
}

void OScriptParserUtils::Printer::decrease_indent() {
	indent_level--;
	indent = "";
	for (int i = 0; i < indent_level * 4; i++) {
		if (i % 4 == 0) {
			indent += "|";
		} else {
			indent += " ";
		}
	}
}

void OScriptParserUtils::Printer::push_line(const String &p_line) {
	if (!p_line.is_empty()) {
		push_text(p_line);
	}
	printed += "\n";
	pending_indent = true;
}

void OScriptParserUtils::Printer::push_text(const String &p_text) {
	if (pending_indent) {
		printed += indent;
		pending_indent = false;
	}
	printed += p_text;
}

void OScriptParserUtils::Printer::print_annotation(const AnnotationNode* p_annotation) {
	push_text(p_annotation->name);
	push_text(" (");
	for (int i = 0; i < p_annotation->arguments.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		print_expression(p_annotation->arguments[i]);
	}
	push_line(")");
}

void OScriptParserUtils::Printer::print_array(ArrayNode* p_array) {
	push_text("[ ");
	for (int i = 0; i < p_array->elements.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		print_expression(p_array->elements[i]);
	}
	push_text(" ]");
}

void OScriptParserUtils::Printer::print_assert(AssertNode* p_assert) {
	push_text("Assert ( ");
	print_expression(p_assert->condition);
	push_line(" )");
}

void OScriptParserUtils::Printer::print_assignment(AssignmentNode* p_assignment) {
	switch (p_assignment->assignee->type) {
		case Node::IDENTIFIER:
			print_identifier(static_cast<IdentifierNode* >(p_assignment->assignee));
			break;
		case Node::SUBSCRIPT:
			print_subscript(static_cast<SubscriptNode* >(p_assignment->assignee));
			break;
		default:
			break; // Unreachable.
	}

	push_text(" ");
	switch (p_assignment->operation) {
		case AssignmentNode::OP_ADDITION:
			push_text("+");
			break;
		case AssignmentNode::OP_SUBTRACTION:
			push_text("-");
			break;
		case AssignmentNode::OP_MULTIPLICATION:
			push_text("*");
			break;
		case AssignmentNode::OP_DIVISION:
			push_text("/");
			break;
		case AssignmentNode::OP_MODULO:
			push_text("%");
			break;
		case AssignmentNode::OP_POWER:
			push_text("**");
			break;
		case AssignmentNode::OP_BIT_SHIFT_LEFT:
			push_text("<<");
			break;
		case AssignmentNode::OP_BIT_SHIFT_RIGHT:
			push_text(">>");
			break;
		case AssignmentNode::OP_BIT_AND:
			push_text("&");
			break;
		case AssignmentNode::OP_BIT_OR:
			push_text("|");
			break;
		case AssignmentNode::OP_BIT_XOR:
			push_text("^");
			break;
		case AssignmentNode::OP_NONE:
			break;
	}
	push_text("= ");
	print_expression(p_assignment->assigned_value);
	push_line();
}

void OScriptParserUtils::Printer::print_await(AwaitNode* p_await) {
	push_text("Await ");
	print_expression(p_await->to_await);
}

void OScriptParserUtils::Printer::print_binary_op(BinaryOpNode* p_binary_op) {
	// Surround in parenthesis for disambiguation.
	push_text("(");
	print_expression(p_binary_op->left_operand);
	switch (p_binary_op->operation) {
		case BinaryOpNode::OP_ADDITION:
			push_text(" + ");
			break;
		case BinaryOpNode::OP_SUBTRACTION:
			push_text(" - ");
			break;
		case BinaryOpNode::OP_MULTIPLICATION:
			push_text(" * ");
			break;
		case BinaryOpNode::OP_DIVISION:
			push_text(" / ");
			break;
		case BinaryOpNode::OP_MODULO:
			push_text(" % ");
			break;
		case BinaryOpNode::OP_POWER:
			push_text(" ** ");
			break;
		case BinaryOpNode::OP_BIT_LEFT_SHIFT:
			push_text(" << ");
			break;
		case BinaryOpNode::OP_BIT_RIGHT_SHIFT:
			push_text(" >> ");
			break;
		case BinaryOpNode::OP_BIT_AND:
			push_text(" & ");
			break;
		case BinaryOpNode::OP_BIT_OR:
			push_text(" | ");
			break;
		case BinaryOpNode::OP_BIT_XOR:
			push_text(" ^ ");
			break;
		case BinaryOpNode::OP_LOGIC_AND:
			push_text(" AND ");
			break;
		case BinaryOpNode::OP_LOGIC_OR:
			push_text(" OR ");
			break;
		case BinaryOpNode::OP_CONTENT_TEST:
			push_text(" IN ");
			break;
		case BinaryOpNode::OP_COMP_EQUAL:
			push_text(" == ");
			break;
		case BinaryOpNode::OP_COMP_NOT_EQUAL:
			push_text(" != ");
			break;
		case BinaryOpNode::OP_COMP_LESS:
			push_text(" < ");
			break;
		case BinaryOpNode::OP_COMP_LESS_EQUAL:
			push_text(" <= ");
			break;
		case BinaryOpNode::OP_COMP_GREATER:
			push_text(" > ");
			break;
		case BinaryOpNode::OP_COMP_GREATER_EQUAL:
			push_text(" >= ");
			break;
	}
	print_expression(p_binary_op->right_operand);
	// Surround in parenthesis for disambiguation.
	push_text(")");
}

void OScriptParserUtils::Printer::print_call(CallNode* p_call) {
	if (p_call->is_super) {
		push_text("super");
		if (p_call->callee != nullptr) {
			push_text(".");
			print_expression(p_call->callee);
		}
	} else {
		print_expression(p_call->callee);
	}
	push_text("( ");
	for (int i = 0; i < p_call->arguments.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		print_expression(p_call->arguments[i]);
	}
	push_text(" )");
}

void OScriptParserUtils::Printer::print_cast(CastNode* p_cast) {
	print_expression(p_cast->operand);
	push_text(" AS ");
	print_type(p_cast->cast_type);
}

void OScriptParserUtils::Printer::print_class(ClassNode* p_class) {
	for (const AnnotationNode* E : p_class->annotations) {
		print_annotation(E);
	}
	push_text("Class ");
	if (p_class->identifier == nullptr) {
		push_text("<unnamed>");
	} else {
		print_identifier(p_class->identifier);
	}

	if (p_class->extends_used) {
		bool first = true;
		push_text(" Extends ");
		if (!p_class->extends_path.is_empty()) {
			push_text(vformat(R"("%s")", p_class->extends_path));
			first = false;
		}
		for (int i = 0; i < p_class->extends.size(); i++) {
			if (!first) {
				push_text(".");
			} else {
				first = false;
			}
			push_text(p_class->extends[i]->name);
		}
	}

	push_line(" :");

	increase_indent();

	for (int i = 0; i < p_class->members.size(); i++) {
		const ClassNode::Member &m = p_class->members[i];

		switch (m.type) {
			case ClassNode::Member::CLASS:
				print_class(m.m_class);
				break;
			case ClassNode::Member::VARIABLE:
				print_variable(m.variable);
				break;
			case ClassNode::Member::CONSTANT:
				print_constant(m.constant);
				break;
			case ClassNode::Member::SIGNAL:
				print_signal(m.signal);
				break;
			case ClassNode::Member::FUNCTION:
				print_function(m.function);
				break;
			case ClassNode::Member::ENUM:
				print_enum(m.m_enum);
				break;
			case ClassNode::Member::ENUM_VALUE:
				break; // Nothing. Will be printed by enum.
			case ClassNode::Member::GROUP:
				break; // Nothing. Groups are only used by inspector.
			case ClassNode::Member::UNDEFINED:
				push_line("<unknown member>");
				break;
		}
	}

	decrease_indent();
}

void OScriptParserUtils::Printer::print_constant(ConstantNode* p_constant) {
	push_text("Constant ");
	print_identifier(p_constant->identifier);

	increase_indent();

	push_line();
	push_text("= ");
	if (p_constant->initializer == nullptr) {
		push_text("<missing value>");
	} else {
		print_expression(p_constant->initializer);
	}
	decrease_indent();
	push_line();
}

void OScriptParserUtils::Printer::print_dictionary(DictionaryNode* p_dictionary) {
	push_line("{");
	increase_indent();
	for (int i = 0; i < p_dictionary->elements.size(); i++) {
		print_expression(p_dictionary->elements[i].key);
		if (p_dictionary->style == DictionaryNode::PYTHON_DICT) {
			push_text(" : ");
		} else {
			push_text(" = ");
		}
		print_expression(p_dictionary->elements[i].value);
		push_line(" ,");
	}
	decrease_indent();
	push_text("}");
}

void OScriptParserUtils::Printer::print_expression(ExpressionNode* p_expression) {
	if (p_expression == nullptr) {
		push_text("<invalid expression>");
		return;
	}
	switch (p_expression->type) {
		case Node::ARRAY:
			print_array(static_cast<ArrayNode* >(p_expression));
			break;
		case Node::ASSIGNMENT:
			print_assignment(static_cast<AssignmentNode* >(p_expression));
			break;
		case Node::AWAIT:
			print_await(static_cast<AwaitNode* >(p_expression));
			break;
		case Node::BINARY_OPERATOR:
			print_binary_op(static_cast<BinaryOpNode* >(p_expression));
			break;
		case Node::CALL:
			print_call(static_cast<CallNode* >(p_expression));
			break;
		case Node::CAST:
			print_cast(static_cast<CastNode* >(p_expression));
			break;
		case Node::DICTIONARY:
			print_dictionary(static_cast<DictionaryNode* >(p_expression));
			break;
		case Node::GET_NODE:
			print_get_node(static_cast<GetNodeNode* >(p_expression));
			break;
		case Node::IDENTIFIER:
			print_identifier(static_cast<IdentifierNode* >(p_expression));
			break;
		case Node::LAMBDA:
			print_lambda(static_cast<LambdaNode* >(p_expression));
			break;
		case Node::LITERAL:
			print_literal(static_cast<LiteralNode* >(p_expression));
			break;
		case Node::PRELOAD:
			print_preload(static_cast<PreloadNode* >(p_expression));
			break;
		case Node::SELF:
			print_self(static_cast<SelfNode* >(p_expression));
			break;
		case Node::SUBSCRIPT:
			print_subscript(static_cast<SubscriptNode* >(p_expression));
			break;
		case Node::TERNARY_OPERATOR:
			print_ternary_op(static_cast<TernaryOpNode* >(p_expression));
			break;
		case Node::TYPE_TEST:
			print_type_test(static_cast<TypeTestNode* >(p_expression));
			break;
		case Node::UNARY_OPERATOR:
			print_unary_op(static_cast<UnaryOpNode* >(p_expression));
			break;
		default:
			push_text(vformat("<unknown expression %d>", p_expression->type));
			break;
	}
}

void OScriptParserUtils::Printer::print_enum(EnumNode* p_enum) {
	push_text("Enum ");
	if (p_enum->identifier != nullptr) {
		print_identifier(p_enum->identifier);
	} else {
		push_text("<unnamed>");
	}

	push_line(" {");
	increase_indent();
	for (int i = 0; i < p_enum->values.size(); i++) {
		const EnumNode::Value &item = p_enum->values[i];
		print_identifier(item.identifier);
		push_text(" = ");
		push_text(itos(item.value));
		push_line(" ,");
	}
	decrease_indent();
	push_line("}");
}

void OScriptParserUtils::Printer::print_for(ForNode* p_for) {
	push_text("For ");
	print_identifier(p_for->variable);
	push_text(" IN ");
	print_expression(p_for->list);
	push_line(" :");

	increase_indent();

	print_suite(p_for->loop);

	decrease_indent();
}

void OScriptParserUtils::Printer::print_function(FunctionNode* p_function, const String &p_context) {
	for (const AnnotationNode* E : p_function->annotations) {
		print_annotation(E);
	}
	if (p_function->is_static) {
		push_text("Static ");
	}
	push_text(p_context);
	push_text(" ");
	if (p_function->identifier) {
		print_identifier(p_function->identifier);
	} else {
		push_text("<anonymous>");
	}
	push_text("( ");
	for (int i = 0; i < p_function->parameters.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		print_parameter(p_function->parameters[i]);
	}
	push_line(" ) :");
	increase_indent();
	print_suite(p_function->body);
	decrease_indent();
}

void OScriptParserUtils::Printer::print_get_node(GetNodeNode* p_get_node) {
	if (p_get_node->use_dollar) {
		push_text("$");
	}
	push_text(p_get_node->full_path);
}

void OScriptParserUtils::Printer::print_identifier(IdentifierNode* p_identifier) {
	if (p_identifier != nullptr) {
		push_text(p_identifier->name);
	} else {
		push_text("<invalid identifier>");
	}
}

void OScriptParserUtils::Printer::print_if(IfNode* p_if, bool p_is_elif) {
	if (p_is_elif) {
		push_text("Elif ");
	} else {
		push_text("If ");
	}
	print_expression(p_if->condition);
	push_line(" :");

	increase_indent();
	print_suite(p_if->true_block);
	decrease_indent();

	if (p_if->false_block != nullptr) {
		push_line("Else :");
		increase_indent();
		print_suite(p_if->false_block);
		decrease_indent();
	}
}

void OScriptParserUtils::Printer::print_lambda(LambdaNode* p_lambda) {
	print_function(p_lambda->function, "Lambda");
	push_text("| captures [ ");
	for (int i = 0; i < p_lambda->captures.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		push_text(String(p_lambda->captures[i]->name));
	}
	push_line(" ]");
}

void OScriptParserUtils::Printer::print_literal(LiteralNode* p_literal) {
	// Prefix for string types.
	switch (p_literal->value.get_type()) {
		case Variant::NODE_PATH:
			push_text("^\"");
			break;
		case Variant::STRING:
			push_text("\"");
			break;
		case Variant::STRING_NAME:
			push_text("&\"");
			break;
		default:
			break;
	}
	push_text(p_literal->value);
	// Suffix for string types.
	switch (p_literal->value.get_type()) {
		case Variant::NODE_PATH:
		case Variant::STRING:
		case Variant::STRING_NAME:
			push_text("\"");
			break;
		default:
			break;
	}
}

void OScriptParserUtils::Printer::print_match(MatchNode* p_match) {
	push_text("Match ");
	print_expression(p_match->test);
	push_line(" :");

	increase_indent();
	for (int i = 0; i < p_match->branches.size(); i++) {
		print_match_branch(p_match->branches[i]);
	}
	decrease_indent();
}

void OScriptParserUtils::Printer::print_match_branch(MatchBranchNode* p_match_branch) {
	for (int i = 0; i < p_match_branch->patterns.size(); i++) {
		if (i > 0) {
			push_text(" , ");
		}
		print_match_pattern(p_match_branch->patterns[i]);
	}

	push_line(" :");

	increase_indent();
	print_suite(p_match_branch->block);
	decrease_indent();
}

void OScriptParserUtils::Printer::print_match_pattern(PatternNode* p_match_pattern) {
	switch (p_match_pattern->pattern_type) {
		case PatternNode::PT_LITERAL:
			print_literal(p_match_pattern->literal);
			break;
		case PatternNode::PT_WILDCARD:
			push_text("_");
			break;
		case PatternNode::PT_REST:
			push_text("..");
			break;
		case PatternNode::PT_BIND:
			push_text("Var ");
			print_identifier(p_match_pattern->bind);
			break;
		case PatternNode::PT_EXPRESSION:
			print_expression(p_match_pattern->expression);
			break;
		case PatternNode::PT_ARRAY:
			push_text("[ ");
			for (int i = 0; i < p_match_pattern->array.size(); i++) {
				if (i > 0) {
					push_text(" , ");
				}
				print_match_pattern(p_match_pattern->array[i]);
			}
			push_text(" ]");
			break;
		case PatternNode::PT_DICTIONARY:
			push_text("{ ");
			for (int i = 0; i < p_match_pattern->dictionary.size(); i++) {
				if (i > 0) {
					push_text(" , ");
				}
				if (p_match_pattern->dictionary[i].key != nullptr) {
					// Key can be null for rest pattern.
					print_expression(p_match_pattern->dictionary[i].key);
					push_text(" : ");
				}
				print_match_pattern(p_match_pattern->dictionary[i].value_pattern);
			}
			push_text(" }");
			break;
	}
}

void OScriptParserUtils::Printer::print_parameter(ParameterNode* p_parameter) {
	print_identifier(p_parameter->identifier);
	if (p_parameter->datatype_specifier != nullptr) {
		push_text(" : ");
		print_type(p_parameter->datatype_specifier);
	}
	if (p_parameter->initializer != nullptr) {
		push_text(" = ");
		print_expression(p_parameter->initializer);
	}
}

void OScriptParserUtils::Printer::print_preload(PreloadNode* p_preload) {
	push_text(R"(Preload ( ")");
	push_text(p_preload->resolved_path);
	push_text(R"(" )");
}


void OScriptParserUtils::Printer::print_return(ReturnNode* p_return) {
	push_text("Return");
	if (p_return->return_value != nullptr) {
		push_text(" ");
		print_expression(p_return->return_value);
	}
	push_line();
}

void OScriptParserUtils::Printer::print_self(SelfNode* p_self) {
	push_text("Self(");
	if (p_self->current_class->identifier != nullptr) {
		print_identifier(p_self->current_class->identifier);
	} else {
		push_text("<main class>");
	}
	push_text(")");
}

void OScriptParserUtils::Printer::print_signal(SignalNode* p_signal) {
	push_text("Signal ");
	print_identifier(p_signal->identifier);
	push_text("( ");
	for (int i = 0; i < p_signal->parameters.size(); i++) {
		print_parameter(p_signal->parameters[i]);
	    if (i + 1 < p_signal->parameters.size()) {
	        push_text(", ");
	    }
	}
	push_line(" )");
}

void OScriptParserUtils::Printer::print_subscript(SubscriptNode* p_subscript) {
	print_expression(p_subscript->base);
	if (p_subscript->is_attribute) {
		push_text(".");
		print_identifier(p_subscript->attribute);
	} else {
		push_text("[ ");
		print_expression(p_subscript->index);
		push_text(" ]");
	}
}

void OScriptParserUtils::Printer::print_statement(Node* p_statement) {
	switch (p_statement->type) {
		case Node::ASSERT:
			print_assert(static_cast<AssertNode* >(p_statement));
			break;
		case Node::VARIABLE:
			print_variable(static_cast<VariableNode* >(p_statement));
			break;
		case Node::CONSTANT:
			print_constant(static_cast<ConstantNode* >(p_statement));
			break;
		case Node::IF:
			print_if(static_cast<IfNode* >(p_statement));
			break;
		case Node::FOR:
			print_for(static_cast<ForNode* >(p_statement));
			break;
		case Node::WHILE:
			print_while(static_cast<WhileNode* >(p_statement));
			break;
		case Node::MATCH:
			print_match(static_cast<MatchNode* >(p_statement));
			break;
		case Node::RETURN:
			print_return(static_cast<ReturnNode* >(p_statement));
			break;
		case Node::BREAK:
			push_line("Break");
			break;
		case Node::CONTINUE:
			push_line("Continue");
			break;
		case Node::PASS:
			push_line("Pass");
			break;
		case Node::BREAKPOINT:
			push_line("Breakpoint");
			break;
		case Node::ASSIGNMENT:
			print_assignment(static_cast<AssignmentNode* >(p_statement));
			break;
		default:
			if (p_statement->is_expression()) {
				print_expression(static_cast<ExpressionNode* >(p_statement));
				push_line();
			} else {
				push_line(vformat("<unknown statement %d>", p_statement->type));
			}
			break;
	}
}

void OScriptParserUtils::Printer::print_suite(SuiteNode* p_suite) {
	for (int i = 0; i < p_suite->statements.size(); i++) {
		print_statement(p_suite->statements[i]);
	}
}

void OScriptParserUtils::Printer::print_ternary_op(TernaryOpNode* p_ternary_op) {
	// Surround in parenthesis for disambiguation.
	push_text("(");
	print_expression(p_ternary_op->true_expr);
	push_text(") IF (");
	print_expression(p_ternary_op->condition);
	push_text(") ELSE (");
	print_expression(p_ternary_op->false_expr);
	push_text(")");
}

void OScriptParserUtils::Printer::print_type(TypeNode* p_type) {
	if (p_type->type_chain.is_empty()) {
		push_text("Void");
	} else {
		for (int i = 0; i < p_type->type_chain.size(); i++) {
			if (i > 0) {
				push_text(".");
			}
			print_identifier(p_type->type_chain[i]);
		}
	}
}

void OScriptParserUtils::Printer::print_type_test(TypeTestNode* p_test) {
	print_expression(p_test->operand);
	push_text(" IS ");
	print_type(p_test->test_type);
}

void OScriptParserUtils::Printer::print_unary_op(UnaryOpNode* p_unary_op) {
	// Surround in parenthesis for disambiguation.
	push_text("(");
	switch (p_unary_op->operation) {
		case UnaryOpNode::OP_POSITIVE:
			push_text("+");
			break;
		case UnaryOpNode::OP_NEGATIVE:
			push_text("-");
			break;
		case UnaryOpNode::OP_LOGIC_NOT:
			push_text("NOT ");
			break;
		case UnaryOpNode::OP_COMPLEMENT:
			push_text("~");
			break;
	}
	print_expression(p_unary_op->operand);
	// Surround in parenthesis for disambiguation.
	push_text(")");
}

void OScriptParserUtils::Printer::print_variable(VariableNode* p_variable) {
	for (const AnnotationNode* E : p_variable->annotations) {
		print_annotation(E);
	}

	if (p_variable->is_static) {
		push_text("Static ");
	}
	push_text("Variable ");
	print_identifier(p_variable->identifier);

	push_text(" : ");
	if (p_variable->datatype_specifier != nullptr) {
		print_type(p_variable->datatype_specifier);
	} else if (p_variable->infer_datatype) {
		push_text("<inferred type>");
	} else {
		push_text("Variant");
	}

	increase_indent();

	push_line();
	push_text("= ");
	if (p_variable->initializer == nullptr) {
		push_text("<default value>");
	} else {
		print_expression(p_variable->initializer);
	}
	push_line();

	if (p_variable->style != VariableNode::NONE) {
		if (p_variable->getter != nullptr) {
			push_text("Get");
			if (p_variable->style == VariableNode::INLINE) {
				push_line(":");
				increase_indent();
				print_suite(p_variable->getter->body);
				decrease_indent();
			} else {
				push_line(" =");
				increase_indent();
				print_identifier(p_variable->getter_pointer);
				push_line();
				decrease_indent();
			}
		}
		if (p_variable->setter != nullptr) {
			push_text("Set (");
			if (p_variable->style == VariableNode::INLINE) {
				if (p_variable->setter_parameter != nullptr) {
					print_identifier(p_variable->setter_parameter);
				} else {
					push_text("<missing>");
				}
				push_line("):");
				increase_indent();
				print_suite(p_variable->setter->body);
				decrease_indent();
			} else {
				push_line(" =");
				increase_indent();
				print_identifier(p_variable->setter_pointer);
				push_line();
				decrease_indent();
			}
		}
	}

	decrease_indent();
	// push_line();
}

void OScriptParserUtils::Printer::print_while(WhileNode* p_while) {
	push_text("While ");
	print_expression(p_while->condition);
	push_line(" :");

	increase_indent();
	print_suite(p_while->loop);
	decrease_indent();
}

String OScriptParserUtils::Printer::print_tree(ClassNode* p_class) {
	ClassNode* class_tree = p_class;
	ERR_FAIL_NULL_V_MSG(class_tree, "", "Parse the code before printing the parse tree.");

    if (class_tree->tool) {
        push_line("@tool");
    }

	if (!class_tree->icon_path.is_empty()) {
		push_text(R"(@icon (")");
		push_text(class_tree->icon_path);
		push_line("\")");
	}
	print_class(class_tree);

    return printed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Writer

#define PARSER_TYPE 1

String OScriptParserUtils::Writer::yesno(const String& p_label, bool p_value) {
    return vformat("%s: %s", p_label, p_value ? "Yes" : "No");
}

void OScriptParserUtils::Writer::write_yesno(const String& p_label, bool p_expression, bool p_default) {
    if (p_expression != p_default) {
        push_line(vformat("%s: %s", p_label, p_expression ? "Yes" : "No"));
    }
}

void OScriptParserUtils::Writer::write_string(const String& p_label, const String& p_value) {
    if (!p_value.is_empty()) {
        push_line(vformat("%s: %s", p_label, p_value));
    }
}

void OScriptParserUtils::Writer::write_method_info(const String& p_label, const MethodInfo& p_value, bool p_write_always) {
    const Dictionary mi = Dictionary(p_value);
    if (p_write_always || mi != Dictionary(MethodInfo())) {
        push_line(vformat("%s: %s", p_label, mi));
    }
}

void OScriptParserUtils::Writer::write_property_info(const String& p_label, const PropertyInfo& p_value, bool p_write_always) {
    const Dictionary pi = Dictionary(p_value);
    if (p_write_always || pi != Dictionary(PropertyInfo())) {
        push_line(vformat("%s: %s", p_label, pi));
    }
}

void OScriptParserUtils::Writer::write_dictionary(const String& p_label, const Dictionary& p_value, bool p_write_always) {
    if (p_write_always || !p_value.keys().is_empty()) {
        push_line(vformat("%s: %s", p_label, p_value));
    }
}

void OScriptParserUtils::Writer::write_datatype(const DataType& p_type) {
    push_line(vformat("Kind: %d", p_type.kind));
    push_line(vformat("Type Source: %d (Inferred %s)", p_type.type_source, !p_type.is_hard_type() ? "Yes" : "No"));
    push_line(vformat("BuiltIn Type: %d (%s)", p_type.builtin_type, Variant::get_type_name(p_type.builtin_type)));

    write_string("Enum Type", p_type.enum_type);

    write_yesno("Script Type", p_type.script_type.is_valid());
    write_string("Script Path", p_type.script_path);

    write_yesno("Const", p_type.is_constant);
    write_yesno("ReadOnly", p_type.is_read_only);
    write_yesno("MetaOnly", p_type.is_meta_type);
    write_yesno("Pseudo", p_type.is_pseudo_type);
    write_yesno("Coroutine", p_type.is_coroutine);

    write_method_info("Method", p_type.method_info);

    if (!p_type.container_element_types.is_empty()) {
        push_line("Element Types:");
        StringBuilder::IndentScope indent(buffer);
        for (int i = 0; i < p_type.container_element_types.size(); i++) {
            write_datatype(p_type.container_element_types[i]);
        }
    }

    if (!p_type.enum_values.is_empty()) {
        push_line("Enum Values:");
        StringBuilder::IndentScope indent(buffer);
        for (const KeyValue<StringName, int64_t>& E : p_type.enum_values) {
            push_line(vformat("%s: %d", E.key, E.value));
        }
    }
}

void OScriptParserUtils::Writer::write_annotations(const List<AnnotationNode*>& p_annotations) {
    if (!p_annotations.is_empty()) {
        push_line("Annotations:");
        StringBuilder::IndentScope indent(buffer);
        for (AnnotationNode* node : p_annotations) {
            push_line("Name: " + node->name);
            if (!node->arguments.is_empty()) {
                push_line("Arguments: ");
                for (ExpressionNode* argument : node->arguments) {
                    write_expression(argument);
                }
            }
            write_property_info("Export Info", node->export_info, true);
        }
    }
}

void OScriptParserUtils::Writer::write_node(Node* p_node) {
    push_line(vformat("Node Type %d (Script ID %d)", p_node->type, p_node->script_node_id));
    push_line("Node Data Type");
    {
        StringBuilder::IndentScope indent(buffer);
        write_datatype(p_node->data_type);
    }
    write_annotations(p_node->annotations);
}

void OScriptParserUtils::Writer::write_class(ClassNode* p_class) {
    push_line("Class");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_class);

    #ifdef PARSER_TYPE
    write_yesno("Tool", p_class->tool);
    #else
    write_yesno("Tool", false);
    #endif

    write_string("Icon Path", p_class->icon_path);
    write_string("Simplified Icon Path", p_class->simplified_icon_path);

    write_yesno("Extends Used", p_class->extends_used);
    write_string("Extends Path", p_class->extends_path);
    if (!p_class->extends.is_empty()) {
        push_line("Extends");
        for (int i = 0; i < p_class->extends.size(); i++) {
            StringBuilder::IndentScope s(buffer);
            write_identifier(p_class->extends[i]);
        }
    }

    write_identifier(p_class->identifier);

    write_yesno("OnReady", p_class->onready_used);
    write_yesno("Abstract", p_class->is_abstract);
    write_yesno("Static Data", p_class->has_static_data);
    write_yesno("Annotated Static Unload", p_class->annotated_static_unload);

    push_line("Data Type");
    write_datatype(p_class->base_type);

    write_string("FQCN", p_class->fqcn);

    if (!p_class->members.is_empty()) {
        push_line("Members");
        StringBuilder::IndentScope indent2(buffer);

        for (int i = 0; i < p_class->members.size(); i++) {
            const ClassNode::Member &m = p_class->members[i];
            switch (m.type) {
                case ClassNode::Member::CLASS:
                    write_class(m.m_class);
                    break;
                case ClassNode::Member::VARIABLE:
                    write_variable(m.variable);
                    break;
                case ClassNode::Member::CONSTANT:
                    write_constant(m.constant);
                    break;
                case ClassNode::Member::SIGNAL:
                    write_signal(m.signal);
                    break;
                case ClassNode::Member::FUNCTION:
                    write_function(m.function);
                    push_line("");
                    break;
                default:
                    break;
            }
        }
    }
}

void OScriptParserUtils::Writer::write_assignable(AssignableNode* p_assignable) {
    write_node(p_assignable);
    write_identifier(p_assignable->identifier);

    if (p_assignable->initializer) {
        push_line("Initializer");
        StringBuilder::IndentScope i(buffer);
        write_expression(p_assignable->initializer);
    }

    if (p_assignable->datatype_specifier) {
        push_line("Type");
        StringBuilder::IndentScope i(buffer);
        write_type(p_assignable->datatype_specifier);
    }

    write_yesno("Infer Type", p_assignable->infer_datatype);
    write_yesno("Conversion Assign", p_assignable->use_conversion_assign);
    write_string("Usages", vformat("%d", p_assignable->usages));
}

void OScriptParserUtils::Writer::write_variable(VariableNode* p_variable) {
    push_line("Variable");

    StringBuilder::IndentScope indent(buffer);
    write_assignable(p_variable);
    push_line(vformat("Style: %d", p_variable->style));
    push_line(yesno("Exported", p_variable->exported));
    push_line(yesno("OnReady", p_variable->onready));
    push_line(yesno("Static", p_variable->is_static));
    push_line(vformat("Assignments: %d", p_variable->assignments));
    write_property_info("Export Info", p_variable->export_info, true);
}

void OScriptParserUtils::Writer::write_constant(ConstantNode* p_constant) {
    push_line("Constant");
    StringBuilder::IndentScope indent(buffer);
    write_assignable(p_constant);
}

void OScriptParserUtils::Writer::write_signal(SignalNode* p_signal) {
    push_line("Signal");

    StringBuilder::IndentScope indent(buffer);
    write_identifier(p_signal->identifier);

    push_line(vformat("Indices: %s", HashMapToDictionary<StringName, int>()(p_signal->parameters_indices)));
    write_method_info("Method", p_signal->method, true);
    push_line("Usages: " + itos(p_signal->usages));
    push_line("Parameters:");

    for (int i = 0; i < p_signal->parameters.size(); i++) {
        StringBuilder::IndentScope ind(buffer);
        write_parameter(p_signal->parameters[i]);
    }
}

void OScriptParserUtils::Writer::write_parameter(ParameterNode* p_parameter) {
    push_line("Parameter");
    StringBuilder::IndentScope indent(buffer);
    write_assignable(p_parameter);
}

void OScriptParserUtils::Writer::write_function(FunctionNode* p_function) {
    push_line(vformat("Function %s", p_function->identifier->name));
    StringBuilder::IndentScope indent(buffer);

    write_node(p_function);
    write_identifier(p_function->identifier);

    write_yesno("Abstract", p_function->is_abstract);
    write_yesno("Static", p_function->is_static);
    write_yesno("Coroutine", p_function->is_coroutine);
    write_dictionary("RPC", p_function->rpc_config, true);

    push_line(vformat("Default Args: %d", p_function->default_arg_values.size()));
    #ifdef PARSER_TYPE
    write_method_info("Method", p_function->method, true);
    #else
    write_method_info("Method", p_function->info, true);
    #endif

    // push_line("Returns");
    // write_type(p_function->return_type);

    if (!p_function->parameters_indices.is_empty()) {
        push_line(vformat("Indices: %s", HashMapToDictionary<StringName, int>()(p_function->parameters_indices)));
    }

    if (!p_function->parameters.is_empty()) {
        push_line("Parameters");
        StringBuilder::IndentScope param(buffer);
        for (int i = 0; i < p_function->parameters.size(); i++) {
            write_parameter(p_function->parameters[i]);
        }
    }

    write_suite(p_function->body);
}

void OScriptParserUtils::Writer::write_suite(SuiteNode* p_suite) {
    push_line("{");
    {
        StringBuilder::IndentScope indent(buffer);
        if (p_suite == nullptr) {
            push_line("<null suite detected>");
        } else {
            for (int i = 0; i < p_suite->locals.size(); i++) {
                const SuiteNode::Local& local = p_suite->locals[i];
                push_line(vformat("Local[%d]: %s : %s", i, local.name, local.type));
            }
            for (int i = 0; i < p_suite->statements.size(); i++) {
                Node* statement = p_suite->statements[i];
                if (statement == nullptr) {
                    push_line("An unexpected null statement detected");
                }
                switch (statement->type) {
                    case Node::CALL:
                        write_call(static_cast<CallNode*>(statement));
                        break;
                    case Node::RETURN:
                        write_return(static_cast<ReturnNode*>(statement));
                        break;
                    case Node::IF:
                        write_if(static_cast<IfNode*>(statement));
                        break;
                    case Node::VARIABLE:
                        write_variable(static_cast<VariableNode*>(statement));
                        break;
                    case Node::ASSIGNMENT:
                        write_assignment(static_cast<AssignmentNode*>(statement));
                        break;
                    case Node::AWAIT:
                        write_await(static_cast<AwaitNode*>(statement));
                        break;
                    case Node::FOR:
                        write_for(static_cast<ForNode*>(statement));
                        break;
                    case Node::MATCH:
                        write_match(static_cast<MatchNode*>(statement));
                        break;
                    default:
                        push_line(vformat("<Unknown statement node type #%d>", statement->type));
                }
            }
        }
    }
    push_line("}");
}

void OScriptParserUtils::Writer::write_call(CallNode* p_call) {
    push_line("Call");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_call);

    write_yesno("Constant", p_call->is_constant);
    write_yesno("Reduced", p_call->reduced);
    write_string("Reduced Value", vformat("%s", p_call->reduced_value));

    if (p_call->callee) {
        push_line("Callee");
        StringBuilder::IndentScope callee(buffer);
        write_expression(p_call->callee);
    } else {
        push_line("Callee <null>");
    }

    write_string("Function", p_call->function_name);

    write_yesno("Super", p_call->is_super);
    write_yesno("Static", p_call->is_static);

    if (!p_call->arguments.is_empty()) {
        push_line("Arguments");
        for (int i = 0; i < p_call->arguments.size(); i++) {
            StringBuilder::IndentScope argid(buffer);
            push_line(vformat("[%d]", i));
            StringBuilder::IndentScope arg(buffer);
            write_expression(p_call->arguments[i]);
        }
    }
}

void OScriptParserUtils::Writer::write_return(ReturnNode* p_return) {
    push_line("Return");
    StringBuilder::IndentScope indent(buffer);
    push_line("Value");
    push_line(yesno("IsVoid", p_return->void_return));
    write_expression(p_return->return_value);
}

void OScriptParserUtils::Writer::write_binary_op(BinaryOpNode* p_binary_op) {
    push_line("BinaryOp");
    StringBuilder::IndentScope indent(buffer);
    write_node(p_binary_op);
    push_line(vformat("Operation: %d / %d", p_binary_op->operation, p_binary_op->variant_op));
    push_line("LHS");
    write_expression(p_binary_op->left_operand);
    push_line("RHS");
    write_expression(p_binary_op->right_operand);
}

void OScriptParserUtils::Writer::write_unary_op(UnaryOpNode* p_unary_op) {
    push_line("UnaryOp");
    StringBuilder::IndentScope indent(buffer);
    write_node(p_unary_op);
    push_line(vformat("Operation: %d / %d", p_unary_op->operation, p_unary_op->variant_op));
    push_line("Operand");
    write_expression(p_unary_op->operand);
}

void OScriptParserUtils::Writer::write_if(IfNode* p_if) {
    push_line("If");

    StringBuilder::IndentScope indent(buffer);
    push_line("Condition");
    write_expression(p_if->condition);

    push_line("True");
    write_suite(p_if->true_block);

    push_line("False");
    write_suite(p_if->false_block);
}

void OScriptParserUtils::Writer::write_expression(ExpressionNode* p_expression) {
    if (!p_expression) {
        push_line("<null>");
        return;
    }

    switch (p_expression->type) {
        case Node::LITERAL:
            write_literal(static_cast<LiteralNode*>(p_expression));
            break;
        case Node::IDENTIFIER:
            write_identifier(static_cast<IdentifierNode*>(p_expression));
            break;
        case Node::CALL:
            write_call(static_cast<CallNode*>(p_expression));
            break;
        case Node::BINARY_OPERATOR:
            write_binary_op(static_cast<BinaryOpNode*>(p_expression));
            break;
        case Node::UNARY_OPERATOR:
            write_unary_op(static_cast<UnaryOpNode*>(p_expression));
            break;
        case Node::CAST:
            write_cast(static_cast<CastNode*>(p_expression));
            break;
        case Node::SUBSCRIPT:
            write_subscript(static_cast<SubscriptNode*>(p_expression));
            break;
        case Node::SELF:
            write_self(static_cast<SelfNode*>(p_expression));
            break;
        default:
            push_line(vformat("<Unsupported Expression Node #%d>", p_expression->type));
            return;
    }

    StringBuilder::IndentScope indent(buffer);
    write_yesno("Reduced", p_expression->reduced);
    write_string("Reduced Value", vformat("%s", p_expression->reduced_value));
    write_yesno("Constant", p_expression->is_constant);
}

void OScriptParserUtils::Writer::write_literal(LiteralNode* p_literal) {
    push_line("Literal:");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_literal);
    push_line(vformat("Value: %s", p_literal->value));
}

void OScriptParserUtils::Writer::write_identifier(IdentifierNode* p_identifier) {
    if (p_identifier) {
        push_line("Identifier");

        StringBuilder::IndentScope indent(buffer);
        write_node(p_identifier);
        write_string("Name", p_identifier->name);
        write_yesno("StaticFunc", p_identifier->function_source_is_static);
        write_yesno("Constant", p_identifier->is_constant);
        write_yesno("Reduced", p_identifier->reduced);
        write_string("Value", vformat("%s", p_identifier->reduced_value));
        write_string("Source", vformat("%d", p_identifier->source));
        write_string("Usages", vformat("%d", p_identifier->usages));

        if (p_identifier->source_function) {
            if (p_identifier->source_function->identifier) {
                write_string("SourceFunc", vformat("%s", p_identifier->source_function->identifier->name));
            } else {
                write_string("SourceFunc", vformat("<unnamed function>"));
            }
        }

        write_yesno("Suite", p_identifier->suite);

    } else {
        push_line("Identifier <null>");
    }
}

void OScriptParserUtils::Writer::write_cast(CastNode* p_cast) {
    push_line("Cast");

    StringBuilder::IndentScope indent(buffer);
    push_line("Operand:");
    write_expression(p_cast->operand);
    push_line("Cast Type:");
    write_type(p_cast->cast_type);
}

void OScriptParserUtils::Writer::write_subscript(SubscriptNode* p_subscript) {
    push_line("Subscript");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_subscript);
    push_line("Base");
    write_expression(p_subscript->base);
    if (p_subscript->is_attribute) {
        push_line("Attribute");
        write_identifier(p_subscript->attribute);
    } else {
        push_line("Index");
        write_expression(p_subscript->index);
    }
}

void OScriptParserUtils::Writer::write_assignment(AssignmentNode* p_assignment) {
    push_line("Assignment");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_assignment);
    push_line(vformat("Operation %s / %s", p_assignment->operation, p_assignment->variant_op));
    push_line("Assignee");
    write_expression(p_assignment->assignee);
    push_line("Value");
    write_expression(p_assignment->assigned_value);
    push_line(yesno("Use Conversion", p_assignment->use_conversion_assign));
}

void OScriptParserUtils::Writer::write_type(TypeNode* p_type) {
    write_node(p_type);

    if (!p_type->type_chain.is_empty()) {
        push_line("Type Chain");
        StringBuilder::IndentScope chain(buffer);
        for (int i = 0; i < p_type->type_chain.size(); i++) {
            write_identifier(p_type->type_chain[i]);
        }
    }
    if (!p_type->container_types.is_empty()) {
        push_line("Container Types");
        StringBuilder::IndentScope chain(buffer);
        for (int i = 0; i < p_type->container_types.size(); i++) {
            push_line(vformat("Container Type[%d]", i));
            StringBuilder::IndentScope type(buffer);
            write_type(p_type->container_types[i]);
        }
    }
}

void OScriptParserUtils::Writer::write_self(SelfNode* p_self) {
    push_line("Self");
    StringBuilder::IndentScope indent(buffer);
    write_node(p_self);

    if (p_self->current_class->identifier) {
        push_line("Class");
        StringBuilder::IndentScope clazz(buffer);
        write_identifier(p_self->current_class->identifier);
    } else {
        push_line(vformat("Class <current-class>=%s", p_self->current_class->fqcn));
    }
}

void OScriptParserUtils::Writer::write_await(AwaitNode* p_await) {
    push_line("Await");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_await);
    push_line("ToAwait");
    write_expression(p_await->to_await);
}

void OScriptParserUtils::Writer::write_for(ForNode* p_for) {
    push_line("For");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_for);
    push_line("Variable");
    write_identifier(p_for->variable);
    push_line("Expression");
    write_expression(p_for->list);
    push_line("{");
    write_suite(p_for->loop);
    push_line("}");
}

void OScriptParserUtils::Writer::write_match(MatchNode* p_match) {
    push_line("Match");

    StringBuilder::IndentScope indent(buffer);
    write_node(p_match);
    push_line("Test");
    write_expression(p_match->test);
    push_line("Branches");
    for (int i = 0; i < p_match->branches.size(); i++) {
        write_match_branch(p_match->branches[i]);
    }
}

void OScriptParserUtils::Writer::write_match_branch(MatchBranchNode* p_match_branch) {
    push_line("Branch");
    StringBuilder::IndentScope indent(buffer);
    write_node(p_match_branch);
    write_yesno("Block", p_match_branch->block != nullptr);
    write_yesno("Guarded", p_match_branch->guard_body != nullptr);
    write_yesno("Wildcard", p_match_branch->has_wildcard);
    push_line("Patterns:");
    for (int i = 0; i < p_match_branch->patterns.size(); i++) {
        write_pattern(p_match_branch->patterns[i]);
    }
}

void OScriptParserUtils::Writer::write_pattern(PatternNode* p_pattern) {
    push_line("Pattern");
    StringBuilder::IndentScope indent(buffer);
    write_node(p_pattern);
    write_string("Type", vformat("%d", p_pattern->pattern_type));
    switch (p_pattern->pattern_type) {
        case PatternNode::PT_LITERAL:
            write_literal(p_pattern->literal);
            break;
        case PatternNode::PT_BIND:
            write_identifier(p_pattern->bind);
            break;
        case PatternNode::PT_EXPRESSION:
            write_expression(p_pattern->expression);
            break;
        default:
            push_line("<Unexpected pattern type>");
    };
}

String OScriptParserUtils::Writer::write_tree(ClassNode* p_class) {
    ClassNode* class_tree = p_class;
    ERR_FAIL_NULL_V(class_tree, "Parse the code before writing the parse tree.");

    buffer.reset();
    write_class(p_class);

    return buffer.get_buffer();
}
#endif