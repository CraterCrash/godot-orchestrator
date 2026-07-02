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
#include "orchestration/nodes/operator_node.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "orchestration/orchestration.h"

#include <godot_cpp/core/math.hpp>

void OScriptNodeOperator::_get_property_list(List<PropertyInfo>* r_list) const {
    // OperatorInfo struct details
    r_list->push_back(PropertyInfo(Variant::INT, "op", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "code", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "left_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "left_type_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "right_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "right_type_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "return_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeOperator::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("op")) {
        r_value = _info.op;
        return true;
    } else if (p_name.match("code")) {
        r_value = _info.code;
        return true;
    } else if (p_name.match("name")) {
        r_value = _info.name;
        return true;
    } else if (p_name.match("left_type")) {
        r_value = _info.left_type;
        return true;
    } else if (p_name.match("left_type_name")) {
        r_value = _info.left_type_name;
        return true;
    } else if (p_name.match("right_type")) {
        r_value = _info.right_type;
        return true;
    } else if (p_name.match("right_type_name")) {
        r_value = _info.right_type_name;
        return true;
    } else if (p_name.match("return_type")) {
        r_value = _info.return_type;
        return true;
    }
    return false;
}

bool OScriptNodeOperator::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name.match("op")) {
        _info.op = VariantOperators::Code(int(p_value));
        return true;
    } else if (p_name.match("code")) {
        _info.code = p_value;
        return true;
    } else if (p_name.match("name")) {
        _info.name = p_value;
        return true;
    } else if (p_name.match("left_type")) {
        _info.left_type = VariantUtils::to_type(p_value);
        return true;
    } else if (p_name.match("left_type_name")) {
        _info.left_type_name = p_value;
        return true;
    } else if (p_name.match("right_type")) {
        _info.right_type = VariantUtils::to_type(p_value);
        return true;
    } else if (p_name.match("right_type_name")) {
        _info.right_type_name = p_value;
        return true;
    } else if (p_name.match("return_type")) {
        _info.return_type = VariantUtils::to_type(p_value);
        return true;
    }
    return false;
}

String OScriptNodeOperator::_get_expression() const {
    if (_is_unary()) {
        switch (_info.op) {
            case VariantOperators::OP_POSITIVE:
                return "+%s";
            case VariantOperators::OP_NEGATE:
                return "-%s";
            case VariantOperators::OP_BIT_NEGATE:
                return "~%s";
            case VariantOperators::OP_NOT:
                return "!%s";
            default:
                // we should never reach this point
                return "%s";
        }
    } else if (_info.op == VariantOperators::OP_POWER) {
        return "Power(%s, %s)";
    }
    return "%s " + _info.code.replacen("%", "%%") + " %s";
}

bool OScriptNodeOperator::_is_unary() const {
    return _info.right_type_name.is_empty();
}

bool OScriptNodeOperator::_should_expand_instead_compile() const {
    const int32_t nodes = 2;
    if (nodes >= get_owning_graph()->get_nodes().size()) {
        return true;
    }
    return false;
}

void OScriptNodeOperator::post_initialize() {
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeOperator::allocate_default_pins() {
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("a", _info.left_type));
    if (!_is_unary()) {
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("b", _info.right_type));
    }
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("result", _info.return_type));

    super::allocate_default_pins();
}

String OScriptNodeOperator::get_tooltip_text() const {
    // If the operator structure isn't populated, return no tooltip.
    // This is currently used by the actions menu
    if (_info.code.is_empty()) {
        return "";
    }

    switch (_info.op) {
        case VariantOperators::OP_EQUAL:
            return "Returns true if A is equal to B (A == B)";
        case VariantOperators::OP_NOT_EQUAL:
            return "Returns true if A is not equal to B (A != B)";
        case VariantOperators::OP_LESS:
            return "Returns true if A is less-than B (A < B)";
        case VariantOperators::OP_LESS_EQUAL:
            return "Returns true if A is less-than or equal-to B (A <= B)";
        case VariantOperators::OP_GREATER:
            return "Returns true if A is greater-than B (A > B)";
        case VariantOperators::OP_GREATER_EQUAL:
            return "Returns true if A is greater-than or equal-to B (A >= B)";
        case VariantOperators::OP_ADD:
            return "Adds two values.";
        case VariantOperators::OP_SUBTRACT:
            return "Subtracts two values.";
        case VariantOperators::OP_MULTIPLY:
            return "Multiplies two values.";
        case VariantOperators::OP_DIVIDE:
            return "Divides two values.";
        case VariantOperators::OP_NEGATE:
            return "Negates a value by multiplying it by -1 (-A)";
        case VariantOperators::OP_POSITIVE:
            return "Returns the unary positive value of A (+A)";
        case VariantOperators::OP_MODULE:
            return "Modulo (A % B)";
        case VariantOperators::OP_POWER:
            return "Returns the power of A raised to B; Power(A, B)";
        case VariantOperators::OP_SHIFT_LEFT:
            return "Bitwise Shift-Left";
        case VariantOperators::OP_SHIFT_RIGHT:
            return "Bitwise Shift-Right";
        case VariantOperators::OP_BIT_AND:
            return "Bitwise AND (A & B)";
        case VariantOperators::OP_BIT_OR:
            return "Bitwise OR (A | B)";
        case VariantOperators::OP_BIT_XOR:
            return "Bitwise XOR (A ^ B)";
        case VariantOperators::OP_BIT_NEGATE:
            return "Bitwise NOT (~ A)";
        case VariantOperators::OP_AND:
            return "Returns the logical AND of two values (A AND B)";
        case VariantOperators::OP_OR:
            return "Returns the logical OR of two values (A OR B)";
        case VariantOperators::OP_XOR:
            return "Returns the logical eX-clusive OR of two values (A XOR B)";
        case VariantOperators::OP_NOT:
            return "Returns the logical complement of the boolean value (NOT A)";
        case VariantOperators::OP_IN:
            return "Returns true if A is in B (A IN B)";
        default:
            return super::get_tooltip_text();
    }
}

String OScriptNodeOperator::get_node_title() const {
    if (_is_unary()) {
        return vformat(_get_expression(), "A");
    } else {
        return vformat(_get_expression(), "A", "B");
    }
}

void OScriptNodeOperator::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "No data provided to create an Operator node");

    const Dictionary &data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("op"), "An operation node requires specifying an 'op' value.");
    ERR_FAIL_COND_MSG(!data.has("type"), "An operation node requires specifying a 'type' value.");

    _info.op = VariantOperators::Code(int(data["op"]));
    _info.code = data["code"];
    _info.name = data["name"];
    _info.left_type = VariantUtils::to_type(data["left_type"]);
    _info.left_type_name = data["left_type_name"];
    _info.right_type = VariantUtils::to_type(data["right_type"]);
    _info.right_type_name = data["right_type_name"];
    _info.return_type = VariantUtils::to_type(data["return_type"]);

    super::initialize(p_context);
}

bool OScriptNodeOperator::is_pure() const {
    return !_should_expand_instead_compile();
}

bool OScriptNodeOperator::is_supported(Variant::Type p_type) {
    return p_type != Variant::NIL && p_type < Variant::PACKED_BYTE_ARRAY;
}

bool OScriptNodeOperator::is_operator_supported(const OperatorInfo& p_operator) {
    return p_operator.right_type < Variant::PACKED_BYTE_ARRAY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodePromotableOperator
///

void OScriptNodePromotableOperator::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::INT, "op", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::PACKED_INT32_ARRAY, "operand_types", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "result_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodePromotableOperator::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.nocasecmp_to("op") == 0) {
        r_value = _op;
        return true;
    } else if (p_name.nocasecmp_to("operand_types") == 0) {
        PackedInt32Array array;
        for (const Variant::Type& type : _operands) {
            array.push_back(type);
        }
        r_value = array;
        return true;
    } else if (p_name.nocasecmp_to("result_type") == 0) {
        r_value = _result;
        return true;
    }
    return false;
}

bool OScriptNodePromotableOperator::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.nocasecmp_to("op") == 0) {
        _op = CAST_INT_TO_ENUM(VariantOperators::Code, p_value);
        return true;
    } else if (p_name.nocasecmp_to("operand_types") == 0) {
        const PackedInt32Array array = p_value;
        for (int i = 0; i < array.size(); i++) {
            _operands.push_back(CAST_INT_TO_ENUM(Variant::Type, array[i]));
        }
        return true;
    } else if (p_name.nocasecmp_to("result_type") == 0) {
        _result = CAST_INT_TO_ENUM(Variant::Type, p_value);
        return true;
    }
    return false;
}

Variant::Type OScriptNodePromotableOperator::_get_result_type(Variant::Type p_left, Variant::Type p_right) const {
    for (const OperatorInfo& oi : ExtensionDB::get_builtin_type(p_left).operators) {
        if (oi.op == _op && oi.right_type == p_right) {
            return oi.return_type;
        }
    }
    return Variant::NIL; // inconsistent
}

Variant::Type OScriptNodePromotableOperator::_get_result_type() const {
    switch (_op) {
        // Comparisons
        case VariantOperators::OP_LESS:
        case VariantOperators::OP_LESS_EQUAL:
        case VariantOperators::OP_GREATER:
        case VariantOperators::OP_GREATER_EQUAL:
        case VariantOperators::OP_EQUAL:
        case VariantOperators::OP_NOT_EQUAL:
        // Logic
        case VariantOperators::OP_AND:
        case VariantOperators::OP_OR:
        case VariantOperators::OP_XOR:
        case VariantOperators::OP_NOT:
        // Containment
        case VariantOperators::OP_IN: {
            return Variant::BOOL;
        }
        default: {
            if (!_operands.is_empty()) {
                Variant::Type left = _operands[0];
                for (int i = 1; i < _operands.size(); i++) {
                    left = _get_result_type(left, _operands[i]);
                }
                return left;
            }
            return Variant::NIL;
        }
    }
}

void OScriptNodePromotableOperator::_reset_to_variant() {
    for (int i = 0; i < _operands.size(); i++) {
        _operands.write[i] = Variant::NIL;
    }

    _result = _get_result_type();

    for (const Ref<OScriptNodePin>& input : find_pins(PD_Input)) {
        if (input.is_valid() && input->has_any_connections()) {
            input->unlink_all();
        }
    }

    _queue_reconstruct();
}

void OScriptNodePromotableOperator::allocate_default_pins() {
    for (int i = 0; i < _operands.size(); i++) {
        const Variant default_value = VariantUtils::make_default(_operands[i]);

        // Use A to Z names, caps at 25 possible pins
        String pin_name;
        if (is_string_format_using_modulo()) {
            if (i == 0) {
                pin_name = "Format";
            } else {
                pin_name = vformat("%c", 'A' + i - 1);
            }
        } else {
            pin_name = vformat("%c", 'A' + i);
        }

        const PropertyInfo pi = _operands[i] == Variant::NIL
            ? PropertyUtils::make_variant(pin_name)
            : PropertyUtils::make_typed(pin_name, _operands[i]);

        create_pin(PD_Input, PT_Data, pi, default_value);
    }

    const PropertyInfo pi = _result == Variant::NIL
            ? PropertyUtils::make_variant("result")
            : PropertyUtils::make_typed("result", _result);

    create_pin(PD_Output, PT_Data, pi, VariantUtils::make_default(_result))->hide_label();

    super::allocate_default_pins();
}

void OScriptNodePromotableOperator::post_placed_new_node() {
    if (!is_in_object() && !is_string_format_using_modulo()) {
        _reset_to_variant();
    }
    super::post_placed_new_node();
}

String OScriptNodePromotableOperator::get_tooltip_text() const {
    String text = get_operator_tooltip_text(_op);

    switch (_op) {
        case VariantOperators::OP_LESS:
        case VariantOperators::OP_LESS_EQUAL:
        case VariantOperators::OP_GREATER:
        case VariantOperators::OP_GREATER_EQUAL: {
            if (_operands.size() == 2 && _operands[0] == Variant::ARRAY && _operands[1] == Variant::ARRAY) {
                text += "\nFor Array types, performs an element-by-element comparison.";
            }
            break;
        }
        case VariantOperators::OP_MODULE: {
            if (is_string_format_using_modulo()) {
                text = vformat("Formats %s placeholders using modulo operator.", Variant::get_type_name(_operands[0]));
                text += "\n\nSee documentation for placeholder options.";
            }
            break;
        }
        case VariantOperators::OP_NOT: {
            if (!_operands.is_empty() && _operands[0] >= Variant::PACKED_BYTE_ARRAY) {
                text += "\n" + Variant::get_type_name(_operands[0]) + " returns true if empty, otherwise false.";
            }
            break;
        }
        case VariantOperators::OP_IN: {
            if (is_in_object()) {
                text = "Check if a given property, method, or signal exists by name in the Object.";
            }
            break;
        }
    }

    return text;
}

String OScriptNodePromotableOperator::get_node_title() const {
    if (is_string_format_using_modulo()) {
        return vformat("%s Format Using Modulo", Variant::get_type_name(_operands[0]));
    } else if (is_in_object()) {
        return vformat("Member Exists in Object");
    }

    return get_operator_name(_op);
}

String OScriptNodePromotableOperator::get_help_topic() const {
    if (!_operands.is_empty()) {
        if (_op == VariantOperators::OP_IN) {
            if (_operands.size() == 2) {
                switch (_operands[1]) {
                    case Variant::ARRAY:
                    case Variant::DICTIONARY: {
                        return vformat("class_method:%s:has", Variant::get_type_name(_operands[1]));
                    }
                    case Variant::STRING:
                    case Variant::STRING_NAME: {
                        return vformat("class_method:%s:contains", Variant::get_type_name(_operands[1]));
                    }
                    case Variant::OBJECT: {
                        return vformat("class_name:Object");
                    }
                }
            }
        }

        const String lexical_operator = get_operator_lexical_code(_op);
        return vformat("class_method:%s:operator %s", Variant::get_type_name(_operands[0]), lexical_operator);
    }
    return super::get_help_topic();
}

void OScriptNodePromotableOperator::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "No data provided to create an Operator node");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("op"), "An operation node requires specify an 'op' value.");

    _op = CAST_INT_TO_ENUM(VariantOperators::Code, data["op"]);
    if (is_unary()) {
        _operands.push_back(Variant::NIL);
    } else {
        _operands.push_back(Variant::NIL);
        _operands.push_back(Variant::NIL);
    }

    super::initialize(p_context);
}

bool OScriptNodePromotableOperator::is_pure() const {
    return true;
}

bool OScriptNodePromotableOperator::can_change_pin_type(const Ref<OScriptNodePin>& p_pin) const {
    return p_pin.is_valid() && p_pin->is_input();
}

Vector<Variant::Type> OScriptNodePromotableOperator::get_possible_pin_types(const Ref<OScriptNodePin>& p_pin) const {
    if (!can_change_pin_type(p_pin)) {
        return {};
    }

    const Vector<Ref<OScriptNodePin>> input_pins = find_pins(PD_Input);
    const int pin_offset = input_pins.find(p_pin);
    if (pin_offset < 0) {
        return {};
    }

    Vector<Variant::Type> possible_types;
    possible_types.push_back(Variant::NIL); // Any pin can be reset to Any

    // Rules guarantee that pins are all-wildcard or all-concrete, so a single check is enough.
    const bool all_any = input_pins[0]->get_property_info().type == Variant::NIL;

    // When every operand is a wildcard the selection is unconstrained; offer every type that
    // can start a chain for this operator.
    if (all_any) {
        for (const BuiltInType& builtin_type : ExtensionDB::get_builtin_types()) {
            for (const OperatorInfo& oi : builtin_type.operators) {
                if (oi.op == _op && is_operator_supported(oi) && !possible_types.has(oi.left_type)) {
                    possible_types.push_back(oi.left_type);
                }
            }
        }
        return possible_types;
    }

    // Fold the operands ABOVE the target into the left operand feeding it.
    Variant::Type left = input_pins[0]->get_property_info().type;
    for (int i = 1; i < pin_offset; i++) {
        left = _get_result_type(left, input_pins[i]->get_property_info().type);
    }

    // Build the candidate set for the target pin from the operator's ordered pairing (the
    // constraint imposed by the operands above it).
    Vector<Variant::Type> candidates;
    if (pin_offset == 0) {
        // First operand: any type that appears as a left operand for this operator.
        for (const BuiltInType& builtin_type : ExtensionDB::get_builtin_types()) {
            for (const OperatorInfo& oi : builtin_type.operators) {
                if (oi.op == _op && is_operator_supported(oi) && !candidates.has(oi.left_type)) {
                    candidates.push_back(oi.left_type);
                }
            }
        }
    } else {
        // Later operand: only valid right operands for (_op, left).
        for (const OperatorInfo& oi : ExtensionDB::get_builtin_type(left).operators) {
            if (oi.op == _op && is_operator_supported(oi) && !candidates.has(oi.right_type)) {
                candidates.push_back(oi.right_type);
            }
        }
    }

    // Keep only candidates that also leave the operands BELOW the target valid, with every
    // other pin held fixed. This is what enforces the operator's ordered pairing, so the list
    // never offers a type whose neighbor would become an invalid right operand.
    for (const Variant::Type& candidate : candidates) {
        Variant::Type running = pin_offset == 0 ? candidate : _get_result_type(left, candidate);

        bool valid = true;
        for (int i = pin_offset + 1; i < input_pins.size() && valid; i++) {
            running = _get_result_type(running, input_pins[i]->get_property_info().type);
            valid = running != Variant::NIL;
        }

        if (valid && !possible_types.has(candidate)) {
            possible_types.push_back(candidate);
        }
    }

    return possible_types;
}

void OScriptNodePromotableOperator::change_pin_types(const Ref<OScriptNodePin>& p_pin, Variant::Type p_type) {
    if (!can_change_pin_type(p_pin)) {
        return;
    }

    const Vector<Ref<OScriptNodePin>> input_pins = find_pins(PD_Input);
    const int pin_offset = input_pins.find(p_pin);
    if (pin_offset < 0) {
        return;
    }

    if (_operands[0] == Variant::NIL || p_type == Variant::NIL) {
        if (p_type == Variant::NIL) {
            _reset_to_variant();
        } else {
            for (int i = 0; i < _operands.size(); i++) {
                _operands.write[i] = p_type;
            }
        }
    } else {
        _operands.write[pin_offset] = p_type;
    }

    _result = _get_result_type();

    reconstruct_node();
}

PackedStringArray OScriptNodePromotableOperator::get_keywords() const {
    switch (_op) {
        case VariantOperators::OP_ADD: {
            return Array::make("add", "+", "addition");
        }
        case VariantOperators::OP_SUBTRACT: {
            return Array::make("minus", "-", "subtract", "subtraction");
        }
        case VariantOperators::OP_MULTIPLY: {
            return Array::make("multiply", "*", "multiplication");
        }
        case VariantOperators::OP_DIVIDE: {
            return Array::make("divide", "/", "division");
        }
        default: {
            return {};
        }
    }
}

void OScriptNodePromotableOperator::get_actions(List<Ref<OScriptAction>>& p_action_list) {

}

void OScriptNodePromotableOperator::post_reconstruct_node() {
    // Check if the input pins should be disconnected due to incompatibility
    const Vector<Ref<OScriptNodePin>> inputs = find_pins(PD_Input);
    for (int i = 0; i < inputs.size(); i++) {
        if (inputs[i]->has_any_connections()) {
            const Ref<OScriptNodePin> link = inputs[i]->get_connection();
            if (!inputs[i]->can_accept(link)) {
                inputs[i]->unlink_all();
                break;
            }
        }
    }

    // Check if the output pin should be disconnected due to incompatibility
    const Ref<OScriptNodePin> output = find_pin(0, PD_Output);
    for (const Ref<OScriptNodePin>& link : output->get_connections()) {
        if (!link->can_accept(output)) {
            output->unlink_all();
            break;
        }
    }

    super::post_reconstruct_node();
}

void OScriptNodePromotableOperator::on_pin_connected(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && p_pin->is_input() && _operands.size() >= 1 && _operands[0] == Variant::NIL) {
        if (can_change_pin_type(p_pin)) {
            change_pin_types(p_pin, p_pin->get_connection()->get_property_info().type);
        }
    }
}

bool OScriptNodePromotableOperator::can_add_dynamic_pin() const {
    switch (_op) {
        case VariantOperators::OP_ADD:
        case VariantOperators::OP_SUBTRACT:
        case VariantOperators::OP_MULTIPLY:
        case VariantOperators::OP_DIVIDE: {
            return _operands.size() >= 2 && _operands.size() <= 25;
        }
        case VariantOperators::OP_MODULE: {
            return _operands.size() >= 2 && _operands.size() <= 25 && is_string_format_using_modulo();
        }
        default: {
            return false;
        }
    }
}

void OScriptNodePromotableOperator::add_dynamic_pin() {
    // Always adds a new pin based on the existing last pin type
    if (!is_unary() && !_operands.is_empty()) {
        _operands.push_back(_operands[_operands.size() - 1]);
        reconstruct_node();
    }
}

bool OScriptNodePromotableOperator::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const {
    if (p_pin.is_valid() && p_pin->is_input()) {
        if (is_unary()) {
            return find_pins(PD_Input).size() >= 2;
        }
        return find_pins(PD_Input).size() > 2;
    }
    return false;
}

void OScriptNodePromotableOperator::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) {
    if (!can_remove_dynamic_pin(p_pin)) {
        return;
    }

    const int pin_offset = p_pin->get_pin_index();

    p_pin->unlink_all();
    remove_pin(p_pin);

    _operands.remove_at(pin_offset);

    Variant::Type new_result_type = _get_result_type();
    if (_result != new_result_type) {
        _result = new_result_type;
    }

    get_orchestration()->adjust_connections(this, pin_offset, -1, PD_Input);
    reconstruct_node();
}

bool OScriptNodePromotableOperator::is_unary() const {
    switch (_op) {
        case VariantOperators::OP_NEGATE:
        case VariantOperators::OP_POSITIVE:
        case VariantOperators::OP_NOT:
            return true;
        default:
            return false;
    }
}

bool OScriptNodePromotableOperator::is_string_format_using_modulo() const {
    if (_op == VariantOperators::OP_MODULE && !_operands.is_empty()) {
        return _operands[0] == Variant::STRING || _operands[0] == Variant::STRING_NAME;
    }
    return false;
}

bool OScriptNodePromotableOperator::is_in_object() const {
    return _op == VariantOperators::OP_IN && _operands.size() == 2 && _operands[1] == Variant::OBJECT;
}

bool OScriptNodePromotableOperator::is_supported(Variant::Type p_type) {
    return p_type != Variant::NIL && p_type < Variant::PACKED_BYTE_ARRAY;
}

bool OScriptNodePromotableOperator::is_operator_supported(const OperatorInfo& p_operator) {
    return p_operator.right_type < Variant::PACKED_BYTE_ARRAY;
}

String OScriptNodePromotableOperator::get_operator_name(VariantOperators::Code p_operator) {
    for (const BuiltInType& type : ExtensionDB::get_builtin_types()) {
        for (const OperatorInfo& oi : type.operators) {
            if (oi.op == p_operator) {
                return oi.name;
                // return vformat("%s (%s)", oi.name, oi.code);
            }
        }
    }
    return vformat("Unknown (%d)", p_operator);
}

String OScriptNodePromotableOperator::get_operator_lexical_code(VariantOperators::Code p_operator) {
    for (const BuiltInType& type : ExtensionDB::get_builtin_types()) {
        for (const OperatorInfo& oi : type.operators) {
            if (oi.op == p_operator) {
                return oi.code;
            }
        }
    }
    return "";
}

String OScriptNodePromotableOperator::get_operator_tooltip_text(VariantOperators::Code p_operator) {
    switch (p_operator) {
        case VariantOperators::OP_EQUAL:
            return "Returns true if A is equal to B (A == B)";
        case VariantOperators::OP_NOT_EQUAL:
            return "Returns true if A is not equal to B (A != B)";
        case VariantOperators::OP_LESS:
            return "Returns true if A is less-than B (A < B)";
        case VariantOperators::OP_LESS_EQUAL:
            return "Returns true if A is less-than or equal-to B (A <= B)";
        case VariantOperators::OP_GREATER:
            return "Returns true if A is greater-than B (A > B)";
        case VariantOperators::OP_GREATER_EQUAL:
            return "Returns true if A is greater-than or equal-to B (A >= B)";
        case VariantOperators::OP_ADD:
            return "Adds multiple values.";
        case VariantOperators::OP_SUBTRACT:
            return "Subtracts multiple values.";
        case VariantOperators::OP_MULTIPLY:
            return "Multiplies multiple values.";
        case VariantOperators::OP_DIVIDE:
            return "Divides multiple values.";
        case VariantOperators::OP_NEGATE:
            return "Negates a value by multiplying it by -1 (-A)";
        case VariantOperators::OP_POSITIVE:
            return "Returns the unary positive value of A (+A)";
        case VariantOperators::OP_MODULE:
            return "Modulo (A % B)\nFor String or StringName bound types to argument A, this can be used to replace one or more placeholders for formatted text.";
        case VariantOperators::OP_POWER:
            return "Returns the power of A raised to B; Power(A, B)";
        case VariantOperators::OP_SHIFT_LEFT:
            return "Bitwise Shift-Left";
        case VariantOperators::OP_SHIFT_RIGHT:
            return "Bitwise Shift-Right";
        case VariantOperators::OP_BIT_AND:
            return "Bitwise AND (A & B)";
        case VariantOperators::OP_BIT_OR:
            return "Bitwise OR (A | B)";
        case VariantOperators::OP_BIT_XOR:
            return "Bitwise XOR (A ^ B)";
        case VariantOperators::OP_BIT_NEGATE:
            return "Bitwise NOT (~ A)";
        case VariantOperators::OP_AND:
            return "Returns the logical AND of two values (A AND B)";
        case VariantOperators::OP_OR:
            return "Returns the logical OR of two values (A OR B)";
        case VariantOperators::OP_XOR:
            return "Returns the logical eX-clusive OR of two values (A XOR B)";
        case VariantOperators::OP_NOT:
            return "Returns the logical complement of the boolean value (NOT A)";
        case VariantOperators::OP_IN:
            return "Returns true if A is in B (A IN B)";
        default:
            return "";
    }
}

void OScriptNodePromotableOperator::copy_pin_types(const Ref<OrchestrationGraphNode>& p_source, const Ref<OrchestrationGraphNode>& p_target) {
    if (p_source.is_null() || p_target.is_null() || !p_target->has_any_connections()) {
        return;
    }

    const Ref<OScriptNodePromotableOperator> promotable_operator = p_target;
    if (promotable_operator.is_null()) {
        return;
    }

    for (const Ref<OrchestrationGraphPin>& pin : promotable_operator->get_all_pins()) {
        if (const Ref<OrchestrationGraphPin>& source_pin = p_source->find_pin(pin->get_pin_name(), pin->get_direction()); source_pin.is_valid()) {
            promotable_operator->change_pin_types(pin, source_pin->get_type());
        }
    }
}

void OScriptNodePromotableOperator::_bind_methods() {

}