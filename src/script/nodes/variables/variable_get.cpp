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
#include "common/property_utils.h"

class OScriptNodeVariableGetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableGet);
    StringName _variable_name;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant value;
        if (!p_context.get_runtime()->get_variable(_variable_name, value))
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

void OScriptNodeVariableGet::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 and p_current_version >= 2)
    {
        // Fixup - makes sure that stored property matches variable, if not reconstructs
        if (_variable.is_valid())
        {
            const Ref<OScriptNodePin> output = find_pin("value", PD_Output);
            if (output.is_valid() && !PropertyUtils::are_equal(_variable->get_info(), output->get_property_info()))
                reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeVariableGet::allocate_default_pins()
{
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _variable->get_info()))->set_label(_variable_name, false);
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

OScriptNodeInstance* OScriptNodeVariableGet::instantiate()
{
    OScriptNodeVariableGetInstance *i = memnew(OScriptNodeVariableGetInstance);
    i->_node = this;
    i->_variable_name = _variable->get_variable_name();
    return i;
}
