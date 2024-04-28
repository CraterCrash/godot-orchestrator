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
#include "variable_get.h"

#include "common/dictionary_utils.h"

class OScriptNodeVariableGetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableGet);
    StringName _variable_name;

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        Variant value;
        if (!_instance->get_variable(_variable_name, value))
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Variable " + _variable_name + " not found.");
            return -1;
        }

        if (!p_context.set_output(0, &value))
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Unable to set output");
            return -1;
        }

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeVariableGet::allocate_default_pins()
{
    Ref<OScriptNodePin> value = create_pin(PD_Output, "value", _variable->get_variable_type());
    value->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);
    value->set_label(_variable_name);

    const PropertyInfo& pi = _variable->get_info();

    if (_variable->get_variable_type() == Variant::OBJECT)
    {
        if (!pi.hint_string.is_empty())
            value->set_target_class(pi.hint_string);
        else
            value->set_target_class(pi.class_name);
    }

    super::allocate_default_pins();
}

String OScriptNodeVariableGet::get_tooltip_text() const
{
    if (_variable.is_valid())
        return vformat("Read the value of variable %s", _variable->get_variable_name());
    else
        return "Read the value of a variable";
}

String OScriptNodeVariableGet::get_node_title() const
{
    return vformat("Get %s", _variable->get_variable_name());
}

OScriptNodeInstance* OScriptNodeVariableGet::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeVariableGetInstance *i = memnew(OScriptNodeVariableGetInstance);
    i->_node = this;
    i->_instance = p_instance;
    i->_variable_name = _variable->get_variable_name();
    return i;
}
