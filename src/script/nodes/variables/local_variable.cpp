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
#include "local_variable.h"

#include "common/property_utils.h"
#include "common/variant_utils.h"

class OScriptNodeLocalVariableInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeLocalVariable)
    Variant::Type _type{ Variant::NIL };
    bool _initialized{ false };

public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        if (!_initialized)
        {
            // First time a Local Variable node is accessed, if it's not a reference type, it
            // automatically generate its default value and store it in working memory.
            if (_type != Variant::NIL && _type != Variant::OBJECT)
                p_context.set_working_memory(0, VariantUtils::make_default(_type));

            _initialized = true;
        }

        p_context.set_output(0, p_context.get_working_memory());
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeAssignLocalVariableInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeAssignLocalVariable)
public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_working_memory(0, p_context.get_input(1));
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeLocalVariable::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "guid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "variable_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR));
    r_list->push_back(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT));
}

bool OScriptNodeLocalVariable::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("guid"))
    {
        r_value = _guid.to_string();
        return true;
    }
    else if (p_name.match("variable_name"))
    {
        Ref<OScriptNodePin> variable = find_pin("variable", PD_Output);
        if (variable.is_valid())
        {
            r_value = variable->get_label();
            return true;
        }
    }
    else if (p_name.match("description"))
    {
        r_value = _description;
        return true;
    }
    return false;
}

bool OScriptNodeLocalVariable::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("guid"))
    {
        _guid = Guid(p_value);
        return true;
    }
    else if (p_name.match("variable_name"))
    {
        Ref<OScriptNodePin> variable = find_pin("variable", PD_Output);
        if (variable.is_valid())
        {
            if (p_value)
                variable->set_label(p_value);
            else
                variable->set_label("");

            emit_changed();
            return true;
        }
    }
    else if (p_name.match("description"))
    {
        _description = p_value;
        emit_changed();
        return true;
    }
    return false;
}

void OScriptNodeLocalVariable::post_initialize()
{
    _type = find_pin("variable", PD_Output)->get_type();

    super::post_initialize();
}

void OScriptNodeLocalVariable::allocate_default_pins()
{
    // todo: handle complex types like objects, enums, bitfields, etc.
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("variable", _type));
    super::allocate_default_pins();
}

String OScriptNodeLocalVariable::get_node_title() const
{
    return vformat("Local %s", VariantUtils::get_friendly_type_name(_type, true));
}

String OScriptNodeLocalVariable::get_icon() const
{
    return "MemberProperty";
}

String OScriptNodeLocalVariable::get_tooltip_text() const
{
    if (_type != Variant::NIL)
        return vformat("A local temporary %s variable", VariantUtils::get_friendly_type_name(_type));
    else
        return vformat("A local temporary variable of a given type");
}

bool OScriptNodeLocalVariable::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const
{
    return p_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION);
}

OScriptNodeInstance* OScriptNodeLocalVariable::instantiate()
{
    OScriptNodeLocalVariableInstance* i = memnew(OScriptNodeLocalVariableInstance);
    i->_node = this;
    i->_type = _type;
    return i;
}

void OScriptNodeLocalVariable::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "A local variable node requires a type argument.");

    _type = Variant::NIL;

    const Dictionary data = p_context.user_data.value();
    if (data.has("type"))
        _type = VariantUtils::to_type(data["type"]);

    _guid = Guid::create_guid();
    super::initialize(p_context);
}

OScriptNodeLocalVariable::OScriptNodeLocalVariable()
{
    _flags = ScriptNodeFlags::DEPRECATED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeAssignLocalVariable::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - make sure variant is encoded for nils
        Ref<OScriptNodePin> variable = find_pin("variable", PD_Input);
        if (variable.is_valid() && PropertyUtils::is_nil_no_variant(variable->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeAssignLocalVariable::post_initialize()
{
    _type = find_pin("variable", PD_Input)->get_type();

    super::post_initialize();
}

void OScriptNodeAssignLocalVariable::post_placed_new_node()
{
    Ref<OScriptNodePin> variable = find_pin("variable");
    if (variable.is_valid() && _type != Variant::NIL && !variable->has_any_connections())
    {
        _type = Variant::NIL;
        reconstruct_node();
    }
    super::post_placed_new_node();
}

void OScriptNodeAssignLocalVariable::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    // todo: handle complex types like objects, enums, bitfields, etc.
    if (_type == Variant::NIL)
    {
        // Should be variants
        create_pin(PD_Input, PT_Data, PropertyUtils::make_variant("variable"))->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);
        create_pin(PD_Input, PT_Data, PropertyUtils::make_variant("value"));
    }
    else
    {
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("variable", _type))->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("value", _type));
    }

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodeAssignLocalVariable::get_node_title() const
{
    return "Assign";
}

String OScriptNodeAssignLocalVariable::get_tooltip_text() const
{
    return "Assigns a value to a local variable.";
}

bool OScriptNodeAssignLocalVariable::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const
{
    return p_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION);
}

OScriptNodeInstance* OScriptNodeAssignLocalVariable::instantiate()
{
    OScriptNodeAssignLocalVariableInstance* i = memnew(OScriptNodeAssignLocalVariableInstance);
    i->_node = this;
    return i;
}

void OScriptNodeAssignLocalVariable::validate_node_during_build(BuildLog& p_log) const
{
    Ref<OScriptNodePin> variable = find_pin("variable", PD_Input);
    if (variable.is_valid())
    {
        if (!variable->has_any_connections())
            p_log.error(this, variable, "Requires a connection.");
        else
        {
            Ref<OScriptNodePin> source_pin = variable->get_connections()[0];
            if (source_pin.is_valid())
            {
                if (!Object::cast_to<OScriptNodeLocalVariable>(source_pin->get_owning_node()))
                    p_log.error(this, variable, "Connection expected with a Local Variable node.");
            }
        }
    }
}

void OScriptNodeAssignLocalVariable::on_pin_connected(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->is_input() && p_pin->get_pin_name().match("variable"))
    {
        Vector<Ref<OScriptNodePin>> pin_connections = p_pin->get_connections();
        Variant::Type pin_type = pin_connections[0]->get_type();
        if (pin_type != _type)
        {
            _type = pin_connections[0]->get_type();
            _notify_pins_changed();
        }
    }
    super::on_pin_connected(p_pin);
}

void OScriptNodeAssignLocalVariable::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->is_input() && p_pin->get_pin_name().match("variable"))
    {
        // Check if any inputs remain connected
        bool still_connected = false;
        for (const Ref<OScriptNodePin>& input : find_pins(PD_Input))
            if (!input->is_execution() && input->has_any_connections())
                still_connected = true;

        // If there are no connections to Variable and Value ports, reset the type
        if (!still_connected)
        {
            _type = Variant::NIL;
            _notify_pins_changed();
        }
    }
    super::on_pin_disconnected(p_pin);
}

String OScriptNodeAssignLocalVariable::get_variable_guid() const
{
    Ref<OScriptNodePin> variable = find_pin("variable", PD_Input);
    if (variable.is_valid())
    {
        const Vector<Ref<OScriptNodePin>> conns = variable->get_connections();
        if (!conns.is_empty())
        {
            const Ref<OScriptNodePin>& first = conns[0];
            Ref<OScriptNodeLocalVariable> node = Object::cast_to<OScriptNodeLocalVariable>(first->get_owning_node());
            if (node.is_valid())
                return node->get("guid");
        }
    }
    return {};
}

OScriptNodeAssignLocalVariable::OScriptNodeAssignLocalVariable()
{
    _flags = ScriptNodeFlags::DEPRECATED;
}