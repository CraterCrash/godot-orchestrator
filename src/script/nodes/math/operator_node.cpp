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
#include "operator_node.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"

#include <godot_cpp/core/math.hpp>

class OScriptNodeOperatorInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeOperator);
    Variant::Operator _operator{ Variant::Operator::OP_EQUAL };
    bool _unary{ false };
    Variant _result;

    int _evaluate_variant(OScriptExecutionContext& p_context, const Variant& p_arg0, const Variant& p_arg1)
    {
        bool valid = true;
        Variant::evaluate(_operator, p_arg0, p_arg1, _result, valid);
        if (!valid)
        {
            const String message = vformat("Operation type #%d failed: ({arg0=[%s,%s]}, {arg1=[%s,%s]})",
                                           _operator,
                                           Variant::get_type_name(p_arg0.get_type()), p_arg0,
                                           Variant::get_type_name(p_arg1.get_type()), p_arg1);
            p_context.set_error(message);
            return -1 | STEP_FLAG_END;
        }
        p_context.set_output(0, &_result);
        return 0;
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_unary)
            return _evaluate_variant(p_context, p_context.get_input(0), Variant());
        else
            return _evaluate_variant(p_context, p_context.get_input(0), p_context.get_input(1));
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeOperator::_bind_methods()
{
}

void OScriptNodeOperator::_get_property_list(List<PropertyInfo>* r_list) const
{
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

bool OScriptNodeOperator::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("op"))
    {
        r_value = _info.op;
        return true;
    }
    else if (p_name.match("code"))
    {
        r_value = _info.code;
        return true;
    }
    else if (p_name.match("name"))
    {
        r_value = _info.name;
        return true;
    }
    else if (p_name.match("left_type"))
    {
        r_value = _info.left_type;
        return true;
    }
    else if (p_name.match("left_type_name"))
    {
        r_value = _info.left_type_name;
        return true;
    }
    else if (p_name.match("right_type"))
    {
        r_value = _info.right_type;
        return true;
    }
    else if (p_name.match("right_type_name"))
    {
        r_value = _info.right_type_name;
        return true;
    }
    else if (p_name.match("return_type"))
    {
        r_value = _info.return_type;
        return true;
    }
    return false;
}

bool OScriptNodeOperator::_set(const StringName &p_name, const Variant &p_value)
{
    if (p_name.match("op"))
    {
        _info.op = VariantOperators::Code(int(p_value));
        return true;
    }
    else if (p_name.match("code"))
    {
        _info.code = p_value;
        return true;
    }
    else if (p_name.match("name"))
    {
        _info.name = p_value;
        return true;
    }
    else if (p_name.match("left_type"))
    {
        _info.left_type = VariantUtils::to_type(p_value);
        return true;
    }
    else if (p_name.match("left_type_name"))
    {
        _info.left_type_name = p_value;
        return true;
    }
    else if (p_name.match("right_type"))
    {
        _info.right_type = VariantUtils::to_type(p_value);
        return true;
    }
    else if (p_name.match("right_type_name"))
    {
        _info.right_type_name = p_value;
        return true;
    }
    else if (p_name.match("return_type"))
    {
        _info.return_type = VariantUtils::to_type(p_value);
        return true;
    }
    return false;
}

String OScriptNodeOperator::_get_expression() const
{
    if (_is_unary())
    {
        switch (_info.op)
        {
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
    }
    else if (_info.op == VariantOperators::OP_POWER)
    {
        return "Power(%s, %s)";
    }
    return "%s " + _info.code.replacen("%", "%%") + " %s";
}

bool OScriptNodeOperator::_is_unary() const
{
    return _info.right_type_name.is_empty();
}

void OScriptNodeOperator::post_initialize()
{
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeOperator::allocate_default_pins()
{
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("a", _info.left_type));
    if (!_is_unary())
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("b", _info.right_type));

    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("result", _info.return_type));

    super::allocate_default_pins();
}

String OScriptNodeOperator::get_tooltip_text() const
{
    // If the operator structure isn't populated, return no tooltip.
    // This is currently used by the actions menu
    if (_info.code.is_empty())
        return "";

    switch (_info.op)
    {
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

String OScriptNodeOperator::get_node_title() const
{
    if (_is_unary())
        return vformat(_get_expression(), "A");
    else
        return vformat(_get_expression(), "A", "B");
}

OScriptNodeInstance* OScriptNodeOperator::instantiate()
{
    OScriptNodeOperatorInstance* i = memnew(OScriptNodeOperatorInstance);
    i->_node = this;
    i->_unary = _is_unary();
    i->_operator = VariantOperators::to_engine(_info.op);
    return i;
}

void OScriptNodeOperator::initialize(const OScriptNodeInitContext& p_context)
{
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

void OScriptNodeOperator::validate_node_during_build(BuildLog& p_log) const
{
    Ref<OScriptNodePin> result = find_pin("result", PD_Output);
    if (!result.is_valid())
        p_log.error(this, "No result pin found, right-click node and select 'Refresh Nodes'.");
    // GH-667 Relaxed temporarily until we can introduce graph traversal to determine if the
    // node actually will be discarded at runtime.
    // else if (!result->has_any_connections())
    //     p_log.error(this, result, "Requires a connection.");

    super::validate_node_during_build(p_log);
}

bool OScriptNodeOperator::is_supported(Variant::Type p_type)
{
    return p_type != Variant::NIL && p_type < Variant::PACKED_BYTE_ARRAY;
}

bool OScriptNodeOperator::is_operator_supported(const OperatorInfo& p_operator)
{
    return p_operator.right_type < Variant::PACKED_BYTE_ARRAY;
}