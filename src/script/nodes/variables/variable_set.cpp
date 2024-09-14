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
#include "variable_set.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"

class OScriptNodeVariableSetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableSet);
    StringName _variable_name;
    bool _constant{ false };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_constant)
        {
            p_context.set_error(vformat("Cannot modify a constant variable '%s'", _variable_name));
            return -1 | STEP_FLAG_END;
        }

        Variant value = p_context.get_input(0);

        Variant current_value;
        if (p_context.get_runtime()->get_variable(_variable_name, current_value))
        {
            // Value is currently assigned
            if (!Variant::can_convert(value.get_type(), current_value.get_type()))
            {
                p_context.set_expected_type_error(0, value.get_type(), current_value.get_type());
                return -1;
            }
        }

        if (!p_context.get_runtime()->set_variable(_variable_name, value))
        {
            p_context.set_error(vformat("Variable '%s' not found.", _variable_name));
            return -1;
        }

        if (!p_context.set_output(0, &value))
        {
            p_context.set_error("Failed to set variable value on output stack.");
            return -1;
        }

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeLocalVariableSetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeLocalVariableSet);
    StringName _variable_name;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        OScriptVirtualMachine::Function* function = reinterpret_cast<OScriptVirtualMachine::Function*>(p_context.get_function());
        if (!function)
        {
            p_context.set_error("Failed to resolve current executing function.");
            return -1 | STEP_FLAG_END;
        }

        function->_variables[_variable_name] = p_context.get_input(0);
        p_context.set_output(0, function->_variables[_variable_name]);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeVariableSet::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - makes sure that stored property matches variable, if not reconstructs
        if (_variable.is_valid())
        {
            const Ref<OScriptNodePin> input = find_pin(_variable->get_variable_name(), PD_Input);
            if (input.is_valid() && !PropertyUtils::are_equal(_variable->get_info(), input->get_property_info()))
                reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeVariableSet::_variable_changed()
{
    if (_is_in_editor())
    {
        Ref<OScriptNodePin> input = find_pin(1, PD_Input);
        if (input.is_valid() && input->has_any_connections())
        {
            Ref<OScriptNodePin> source = input->get_connections()[0];
            if (!input->can_accept(source))
                input->unlink_all();
        }

        Ref<OScriptNodePin> output = find_pin("value", PD_Output);
        if (output.is_valid() && output->has_any_connections())
        {
            Ref<OScriptNodePin> target = output->get_connections()[0];
            if (!target->can_accept(output))
                output->unlink_all();
        }
    }

    super::_variable_changed();
}

void OScriptNodeVariableSet::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, _variable->get_info())->no_pretty_format();

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _variable->get_info()))->hide_label();

    super::allocate_default_pins();
}

String OScriptNodeVariableSet::get_tooltip_text() const
{
    if (_variable.is_valid())
    {
        String tooltip_text = vformat("Set the value of variable %s", _variable->get_variable_name());
        if (!_variable->get_description().is_empty())
            tooltip_text += "\n\nDescription:\n" + _variable->get_description();

        return tooltip_text;
    }

    return vformat("Set the value of a variable");
}

String OScriptNodeVariableSet::get_node_title() const
{
    return vformat("Set %s", _variable_name);
}

void OScriptNodeVariableSet::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // Keep old default value if one was set that differs from the variable's default value
    for (const Ref<OScriptNodePin>& old_pin : p_old_pins)
    {
        if (old_pin->is_input() && !old_pin->is_execution())
        {
            if (old_pin->get_effective_default_value() != _variable->get_default_value())
            {
                Ref<OScriptNodePin> value_pin = find_pin(_variable->get_variable_name(), PD_Input);
                if (value_pin.is_valid() && !value_pin->has_any_connections())
                    value_pin->set_default_value(VariantUtils::convert(old_pin->get_effective_default_value(), value_pin->get_type()));

                break;
            }
        }
    }
}

void OScriptNodeVariableSet::validate_node_during_build(BuildLog& p_log) const
{
    if (_variable.is_valid() && _variable->is_constant())
        p_log.error(this, "Cannot modify constant variable value.");

    super::validate_node_during_build(p_log);
}

OScriptNodeInstance* OScriptNodeVariableSet::instantiate()
{
    OScriptNodeVariableSetInstance *i = memnew(OScriptNodeVariableSetInstance);
    i->_node = this;
    i->_variable_name = _variable->get_variable_name();
    i->_constant = _variable->is_constant();
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeLocalVariableSet::_variable_changed()
{
    if (_is_in_editor())
    {
        Ref<OScriptNodePin> input = find_pin(1, PD_Input);
        if (input.is_valid() && input->has_any_connections())
        {
            Ref<OScriptNodePin> source = input->get_connections()[0];
            if (!input->can_accept(source))
                input->unlink_all();
        }

        Ref<OScriptNodePin> output = find_pin("value", PD_Output);
        if (output.is_valid() && output->has_any_connections())
        {
            // todo: shouldn't this support multiple?
            Ref<OScriptNodePin> target = output->get_connections()[0];
            if (!target->can_accept(output))
                output->unlink_all();
        }
    }

    super::_variable_changed();
}

void OScriptNodeLocalVariableSet::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, _variable->get_info())->no_pretty_format();

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _variable->get_info()))->hide_label();

    super::allocate_default_pins();
}

String OScriptNodeLocalVariableSet::get_tooltip_text() const
{
    if (_variable.is_valid())
    {
        String tooltip_text = vformat("Set the value of local variable %s", _variable->get_variable_name());
        if (!_variable->get_description().is_empty())
            tooltip_text += "\n\nDescription:\n" + _variable->get_description();

        return tooltip_text;
    }

    return vformat("Set the value of a local variable");
}

String OScriptNodeLocalVariableSet::get_node_title() const
{
    return vformat("Set %s", _variable_name);
}

void OScriptNodeLocalVariableSet::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // Keep old default value if one was set that differs from the variable's default value
    for (const Ref<OScriptNodePin>& old_pin : p_old_pins)
    {
        if (old_pin->is_input() && !old_pin->is_execution())
        {
            if (old_pin->get_effective_default_value() != _variable->get_default_value())
            {
                Ref<OScriptNodePin> value_pin = find_pin(_variable->get_variable_name(), PD_Input);
                if (value_pin.is_valid() && !value_pin->has_any_connections())
                    value_pin->set_default_value(VariantUtils::convert(old_pin->get_effective_default_value(), value_pin->get_type()));

                break;
            }
        }
    }
}

OScriptNodeInstance* OScriptNodeLocalVariableSet::instantiate()
{
    OScriptNodeLocalVariableSetInstance *i = memnew(OScriptNodeLocalVariableSetInstance);
    i->_node = this;
    i->_variable_name = _variable_name;
    return i;
}