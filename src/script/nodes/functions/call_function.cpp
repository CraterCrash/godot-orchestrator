// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "call_function.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/node.hpp>

class OScriptNodeCallFunctionInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeCallFunction);

    OScriptFunctionReference _reference;
    StringName _function_name;
    int _argument_count{ 0 };
    int _argument_offset{ 2 };
    bool _pure{ false };
    bool _chained{ false };
    Array _args;

    int _do_pure(OScriptExecutionContext& p_context) const
    {
        // Pure function calls use the Godot Expression class to evaluate the function call.
        // This requires that we bind the arguments using a variant Array.
        // Additionally, we need to generate argument name bindings for the expression.
        Array args;
        PackedStringArray arg_names;
        for (int i = 0; i < _argument_count; i++)
        {
            args.push_back(p_context.get_input(i + _argument_offset));
            arg_names.push_back(vformat("x%d", i));
        }

        // Create the expression to be parsed
        const String expression = vformat("%s(%s)", _reference.name, StringUtils::join(",", arg_names));

        Ref<Expression> parser;
        parser.instantiate();

        Error err = parser->parse(expression, arg_names);
        if (err == Error::OK)
        {
            // Execute the expression with the provided arguments.
            // This requires an instance object, we use the script owner.
            Variant result = parser->execute(args, p_context.get_owner());
            if (!parser->has_execute_failed())
            {
                // Execution was successful, set output if applicable.
                if (MethodUtils::has_return_value(_reference.method))
                    p_context.set_output(0, result);

                return 0;
            }

            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, parser->get_error_text());
            return -1 | STEP_FLAG_END;
        }

        p_context.set_error(GDExtensionCallErrorType(err), "Failed to parse expression: " + expression);
        return -1 | STEP_FLAG_END;
    }

    int _do_target_type(OScriptExecutionContext& p_context) const
    {
        const Variant** pargs = _argument_count > 0 ? p_context.get_input_ptr() + 1 : nullptr;
        Variant target = p_context.get_input(0);

        GDExtensionCallError err;
        Variant result;
        target.callp(_reference.name, pargs, _argument_count, result, err);
        if (err.error != GDEXTENSION_CALL_OK)
        {
            p_context.set_error(err.error, "Failed executing method " + _reference.name);
            return -1 | STEP_FLAG_END;
        }

        if (MethodUtils::has_return_value(_reference.method))
            p_context.set_output(0, result);

        return 0;
    }

    Object* _get_call_instance(OScriptExecutionContext& p_context)
    {
        if (_argument_offset == 0)
            return p_context.get_owner();

        Variant target = p_context.get_input(0);
        return !target ? p_context.get_owner() : Object::cast_to<Object>(target);
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        // Check if function call is pure
        if (_pure)
            return _do_pure(p_context);

        // Check if the function call is on a specific target type
        if (_reference.target_type != Variant::NIL)
            return _do_target_type(p_context);

        Object* instance = _get_call_instance(p_context);
        if (!instance)
        {
            ERR_PRINT("Cannot call function " + _reference.name + " on null target");
            return -1 | STEP_FLAG_END;
        }

        // Handle instanced function calls
        if (_argument_count > 0)
        {
            if (_args.size() != _argument_count)
                _args.resize(_argument_count);

            for (int i = 0; i < _argument_count; i++)
                _args[i] = p_context.get_input(i + _argument_offset);
        }

        if (MethodUtils::has_return_value(_reference.method))
        {
            Variant result = instance->callv(_function_name, _args);
            p_context.set_output(0, result);
            if (_chained)
                p_context.set_output(1, p_context.get_input(0));
        }
        else
        {
            instance->callv(_function_name, _args);
            if (_chained)
                p_context.set_output(0, p_context.get_input(0));
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeCallFunction::_bind_methods()
{
    BIND_ENUM_CONSTANT(FF_NONE)
    BIND_ENUM_CONSTANT(FF_PURE)
    BIND_ENUM_CONSTANT(FF_CONST)
    BIND_ENUM_CONSTANT(FF_IS_BEAD)
    BIND_ENUM_CONSTANT(FF_IS_SELF)
}

void OScriptNodeCallFunction::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "guid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "target_class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "target_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    const String flags = "Pure,Const,Is Bead,Is Self,Virtual,VarArg,Static,Object Core,Editor";
    r_list->push_back(PropertyInfo(Variant::INT, "flags", PROPERTY_HINT_FLAGS, flags, PROPERTY_USAGE_STORAGE));

    if (_is_method_info_serialized())
        r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    if (_reference.method.flags & METHOD_FLAG_VARARG)
        r_list->push_back(PropertyInfo(Variant::INT, "variable_arg_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    if (_chainable)
        r_list->push_back(PropertyInfo(Variant::BOOL, "chain"));
}

bool OScriptNodeCallFunction::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("guid"))
    {
        r_value = _reference.guid;
        return true;
    }
    else if (p_name.match("function_name"))
    {
        r_value = _reference.method.name;
        return true;
    }
    else if (p_name.match("target_class_name"))
    {
        r_value = _reference.target_class_name;
        return true;
    }
    else if (p_name.match("target_type"))
    {
        r_value = _reference.target_type;
        return true;
    }
    else if (p_name.match("flags"))
    {
        r_value = _function_flags;
        return true;
    }
    else if (p_name.match("method"))
    {
        r_value = DictionaryUtils::from_method(_reference.method);
        return true;
    }
    else if (p_name.match("variable_arg_count"))
    {
        r_value = _vararg_count;
        return true;
    }
    else if (p_name.match("chain"))
    {
        r_value = _chain;
        return true;
    }
    return false;
}

bool OScriptNodeCallFunction::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("guid"))
    {
        _reference.guid = Guid(p_value);
        return true;
    }
    else if (p_name.match("target_class_name"))
    {
        _reference.target_class_name = p_value;
        return true;
    }
    else if (p_name.match("target_type"))
    {
        _reference.target_type = VariantUtils::to_type(p_value);
        return true;
    }
    else if (p_name.match("flags"))
    {
        _function_flags = int(p_value);
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("method"))
    {
        _reference.method = DictionaryUtils::to_method(p_value);
        return true;
    }
    else if (p_name.match("variable_arg_count"))
    {
        _vararg_count = int(p_value);
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("chain"))
    {
        _chain = p_value;
        if (!_chain)
        {
            Ref<OScriptNodePin> pin = find_pin("return_target", PD_Output);
            if (pin.is_valid() && pin->has_any_connections())
                pin->unlink_all();
        }
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeCallFunction::_create_pins_for_method(const MethodInfo& p_method)
{
    if (_has_execution_pins(p_method))
    {
        create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
        create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    }

    _chainable = false;
    if (get_argument_offset() != 0)
    {
        Variant::Type target_type = _reference.target_type != Variant::NIL ? _reference.target_type : Variant::OBJECT;
        Ref<OScriptNodePin> target = create_pin(PD_Input, "target", target_type);
        target->set_flags(OScriptNodePin::Flags::DATA);
        if (_reference.target_type == Variant::NIL)
        {
            if (_function_flags.has_flag(FF_IS_SELF))
                target->set_target_class(get_orchestration()->get_base_type());
            else
                target->set_target_class("Object");

            _chainable = true;
            notify_property_list_changed();
        }
    }

    const size_t default_start_index = p_method.arguments.empty()
        ? 0
        : p_method.arguments.size() - p_method.default_arguments.size();

    size_t argument_index = 0;
    size_t default_index = 0;
    for (const PropertyInfo& pi : p_method.arguments)
    {
        Ref<OScriptNodePin> pin = create_pin(PD_Input, pi.name, pi.type);
        if (pin.is_valid())
        {
            BitField<OScriptNodePin::Flags> flags(OScriptNodePin::Flags::DATA | OScriptNodePin::NO_CAPITALIZE);
            if (pi.usage & PROPERTY_USAGE_CLASS_IS_ENUM)
            {
                flags.set_flag(OScriptNodePin::Flags::ENUM);
                pin->set_target_class(pi.class_name);
                pin->set_type(pi.type);
            }
            else if (pi.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD)
            {
                flags.set_flag(OScriptNodePin::Flags::BITFIELD);
                pin->set_target_class(pi.class_name);
                pin->set_type(pi.type);
            }
            pin->set_flags(flags);

            if (argument_index >= default_start_index)
                pin->set_default_value(p_method.default_arguments[default_index++]);
        }
        argument_index++;
    }

    if (_reference.method.flags & METHOD_FLAG_VARARG)
    {
        const int base_arg_count = _reference.method.arguments.size() + 1;
        for (int i = 0; i < _vararg_count; i++)
        {
            Ref<OScriptNodePin> vararg_pin = create_pin(PD_Input, "arg" + itos(base_arg_count + i), Variant::NIL);
            vararg_pin->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);
        }
    }

    if (MethodUtils::has_return_value(p_method))
    {
        Ref<OScriptNodePin> rv = create_pin(PD_Output, "return_value", p_method.return_val.type);
        rv->set_flags(OScriptNodePin::Flags::DATA);
        rv->set_target_class(p_method.return_val.class_name);
    }

    if (_chainable && _chain)
    {
        Ref<OScriptNodePin> chain = create_pin(PD_Output, "return_target", Variant::OBJECT);
        chain->set_flags(OScriptNodePin::Flags::DATA);
        chain->set_label("Target");
        chain->set_target_class(find_pin("target", PD_Input)->get_target_class());
    }
}

bool OScriptNodeCallFunction::_has_execution_pins(const MethodInfo& p_method) const
{
    if (MethodUtils::has_return_value(p_method) && p_method.arguments.empty())
    {
        const String method_name = p_method.name.capitalize();
        if (method_name.begins_with("Is ") || method_name.begins_with("Get "))
            return false;
    }
    return true;
}

void OScriptNodeCallFunction::_set_function_flags(const MethodInfo& p_method)
{
    if (p_method.flags & METHOD_FLAG_CONST)
        _function_flags.set_flag(FF_CONST);

    if (p_method.flags & METHOD_FLAG_VIRTUAL)
        _function_flags.set_flag(FF_IS_VIRTUAL);

    if (p_method.flags & METHOD_FLAG_STATIC)
        _function_flags.set_flag(FF_STATIC);

    if (p_method.flags & METHOD_FLAG_VARARG)
        _function_flags.set_flag(FF_VARARG);
}

int OScriptNodeCallFunction::get_argument_count() const
{
    int arg_count = _reference.method.arguments.size();
    if (_reference.method.flags & METHOD_FLAG_VARARG)
        arg_count += _vararg_count;

    return arg_count;
}

void OScriptNodeCallFunction::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // Specifying default values on the call function node take precedence over the values defined on the
    // FunctionEntry node, and therefore since the create pins method does not set the default values on
    // the input pins, this allows us to copy the values from the old pins to the new ones created when
    // the node is reconstructed, so those values are not lost across load/save operations.
    //
    // This allows the pins to restore the default values without having to serialize those as any other
    // value held by this node.
    Vector<Ref<OScriptNodePin>> inputs = find_pins(PD_Input);
    for (int i = 2; i < inputs.size(); i++)
    {
        const Ref<OScriptNodePin>& input = inputs[i];
        for (const Ref<OScriptNodePin>& old_pin : p_old_pins)
        {
            if (old_pin->get_direction() == input->get_direction() && old_pin->get_pin_name() == input->get_pin_name())
            {
                input->set_generated_default_value(old_pin->get_generated_default_value());
                input->set_default_value(old_pin->get_default_value());
            }
        }
    }
}

void OScriptNodeCallFunction::post_initialize()
{
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeCallFunction::allocate_default_pins()
{
    _create_pins_for_method(get_method_info());
    super::allocate_default_pins();
}

OScriptNodeInstance* OScriptNodeCallFunction::instantiate()
{
    OScriptNodeCallFunctionInstance *i = memnew(OScriptNodeCallFunctionInstance);
    i->_node = this;
    i->_argument_count = get_argument_count();
    i->_argument_offset = get_argument_offset();
    i->_reference = _reference;
    i->_function_name = _reference.name;
    i->_pure = _function_flags.has_flag(FF_PURE);
    i->_chained = _chain;
    return i;
}

void OScriptNodeCallFunction::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(_reference.name.is_empty(), "Function name not specified.");
    super::initialize(p_context);
}

void OScriptNodeCallFunction::add_dynamic_pin()
{
    _vararg_count++;
    reconstruct_node();
}

bool OScriptNodeCallFunction::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    if (is_vararg() && p_pin.is_valid())
    {
        for (const PropertyInfo& pi : _reference.method.arguments)
        {
            if (pi.name.match(p_pin->get_pin_name()))
                return false;
        }
        return true;
    }
    return false;
}

void OScriptNodeCallFunction::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin.is_valid() && can_remove_dynamic_pin(p_pin))
    {
        int pin_offset = p_pin->get_pin_index();

        p_pin->unlink_all(true);
        remove_pin(p_pin);

        // Taken from OScriptNodeEditablePin::_adjust_connections
        get_orchestration()->adjust_connections(this, pin_offset, -1, PD_Input);

        _vararg_count--;
        reconstruct_node();
    }
}
