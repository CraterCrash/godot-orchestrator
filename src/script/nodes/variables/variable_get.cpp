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
#include "variable_get.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"

class OScriptNodeVariableGetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeVariableGet);
    StringName _variable_name;
    bool _validated{ false };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        OScriptVirtualMachine::Variable* variable = p_context.get_runtime()->get_variable(_variable_name);
        if (!variable)
        {
            p_context.set_error(vformat("Variable '%s' not found.", _variable_name));
            return -1;
        }

        p_context.set_output(0, &variable->value);
        if (_validated)
        {

            if (variable->type == Variant::OBJECT && !Object::cast_to<Object>(variable->value))
                return 1;
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeVariableGet::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::BOOL, "validated", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeVariableGet::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("validated"))
    {
        r_value = _validated;
        return true;
    }

    // todo: GodotCPP expects this to be done by the developer, Wrapped::get_bind doesn't do this
    // see https://github.com/godotengine/godot-cpp/pull/1539
    return OScriptNodeVariable::_get(p_name, r_value);
}

bool OScriptNodeVariableGet::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("validated"))
    {
        _validated = p_value;
        return true;
    }

    // todo: GodotCPP expects this to be done by the developer, Wrapped::set_bind doesn't do this
    // see https://github.com/godotengine/godot-cpp/pull/1539
    return OScriptNodeVariable::_set(p_name, p_value);
}

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

void OScriptNodeVariableGet::_variable_changed()
{
    if (_is_in_editor())
    {
        Ref<OScriptNodePin> output = find_pin("value", PD_Output);
        if (output.is_valid() && output->has_any_connections())
        {
            Ref<OScriptNodePin> target = output->get_connections()[0];
            if (target.is_valid() && !target->can_accept(output))
                output->unlink_all();
        }
    }

    super::_variable_changed();
}

void OScriptNodeVariableGet::allocate_default_pins()
{
    if (_validated)
    {
        create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("is_valid"))->set_label("Is Valid");
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("is_invalid"))->set_label("Is Invalid");
    }

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
    i->_validated = _validated;
    return i;
}

void OScriptNodeVariableGet::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("validation"))
            _validated = data["validation"];
    }

    super::initialize(p_context);
}

bool OScriptNodeVariableGet::can_be_validated()
{
    PropertyInfo property = _variable->get_info();
    if (property.type == Variant::OBJECT)
        return true;
    return false;
}

void OScriptNodeVariableGet::set_validated(bool p_validated)
{
    if (_validated != p_validated)
    {
        _validated = p_validated;

        if (!_validated)
        {
            // Disconnect any control flow pins, if they exist
            Ref<OScriptNodePin> exec_in = find_pin("ExecIn", PD_Input);
            if (exec_in.is_valid())
                exec_in->unlink_all();

            Ref<OScriptNodePin> is_valid = find_pin("is_valid", PD_Output);
            if (is_valid.is_valid())
                is_valid->unlink_all();

            Ref<OScriptNodePin> is_not_valid = find_pin("is_not_valid", PD_Output);
            if (is_not_valid.is_valid())
                is_not_valid->unlink_all();
        }

        // Record the connection before the change
        Ref<OScriptNodePin> connection;
        Ref<OScriptNodePin> value = find_pin("value", PD_Output);
        if (value.is_valid() && value->has_any_connections())
        {
            connection = value->get_connections()[0];
            value->unlink_all();
        }

        _notify_pins_changed();

        if (connection.is_valid())
        {
            // Relink connection on change
            value = find_pin("value", PD_Output);
            if (value.is_valid())
                value->link(connection);
        }
    }
}
