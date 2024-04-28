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
#include "variable_set.h"

#include "common/variant_utils.h"

class OScriptNodeVariableSetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableSet);
    StringName _variable_name;

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        Variant value = p_context.get_input(0);

        Variant current_value;
        if (_instance->get_variable(_variable_name, current_value))
        {
            // Value is currently assigned
            if (!Variant::can_convert(value.get_type(), current_value.get_type()))
            {
                p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT);
                p_context.set_invalid_argument(this, 0, value.get_type(), current_value.get_type());
                return -1;
            }
        }

        if (!_instance->set_variable(_variable_name, value))
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Variable " + _variable_name + " not found.");
            return -1;
        }

        if (!p_context.set_output(0, &value))
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Failed to set output");
            return -1;
        }

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeVariableSet::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);

    Ref<OScriptNodePin> v = create_pin(PD_Input, _variable_name, _variable->get_variable_type(), _variable->get_default_value());
    v->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);

    Ref<OScriptNodePin> value = create_pin(PD_Output, "value", _variable->get_variable_type());
    value->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::HIDE_LABEL);

    if (_variable.is_valid())
    {
        PropertyInfo pi = _variable->get_info();
        if (pi.hint == PROPERTY_HINT_FLAGS)
        {
            v->set_flags(v->get_flags() | OScriptNodePin::Flags::BITFIELD);
            v->set_target_class(pi.class_name);
            v->set_type(Variant::INT);
        }
        else if (pi.hint == PROPERTY_HINT_ENUM)
        {
            v->set_flags(v->get_flags() | OScriptNodePin::Flags::ENUM);
            v->set_target_class(pi.class_name);
            v->set_type(Variant::INT);
        }
        else if (!pi.hint_string.is_empty())
            value->set_target_class(pi.hint_string);
    }

    super::allocate_default_pins();
}

String OScriptNodeVariableSet::get_tooltip_text() const
{
    if (_variable.is_valid())
        return vformat("Set the value of variable %s", _variable->get_variable_name());
    else
        return vformat("Set the value of a variable");
}

String OScriptNodeVariableSet::get_node_title() const
{
    return vformat("Set %s", _variable->get_variable_name());
}

OScriptNodeInstance* OScriptNodeVariableSet::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeVariableSetInstance *i = memnew(OScriptNodeVariableSetInstance);
    i->_node = this;
    i->_instance = p_instance;
    i->_variable_name = _variable->get_variable_name();
    return i;
}
