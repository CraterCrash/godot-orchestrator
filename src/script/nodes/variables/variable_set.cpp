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

class OScriptNodeVariableSetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableSet);
    StringName _variable_name;

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        Variant value = p_context.get_input(0);
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
