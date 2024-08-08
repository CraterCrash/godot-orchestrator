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
#include "switch.h"

#include "api/extension_db.h"
#include "common/property_utils.h"

class OScriptNodeSwitchInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSwitch);
    int _case_count{ 0 };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (p_context.get_step_mode() == STEP_MODE_CONTINUE)
            return 0;

        Variant& value = p_context.get_input(0);

        for (int i = 1; i <= _case_count; i++)
            if (p_context.get_input(i) == value)
                return (i + 1) | STEP_FLAG_PUSH_STACK_BIT;

        // Default is always execution index 1
        return 1 | STEP_FLAG_PUSH_STACK_BIT;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeSwitchStringInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSwitchString);
    PackedStringArray _values;
    bool _case_sensitive{ false };
    bool _has_default{ false };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        String value = p_context.get_input(0);
        for (int i = 0; i < _values.size(); i++)
        {
            if ((_case_sensitive && value == _values[i])
                || (!_case_sensitive && value.to_lower() == _values[i].to_lower()))
                return i;
        }
        return _has_default ? _values.size() : -1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeSwitchIntegerInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSwitchInteger);
    std::vector<int> _values;
    bool _has_default{ false };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        int value = p_context.get_input(0);
        auto it = std::lower_bound(_values.begin(), _values.end(), value);
        if (it != _values.end() && *it == value)
            return static_cast<int>(std::distance(_values.begin(), it));

        // Default is always execution index 1
        return _has_default ? _values.size() : -1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class OScriptNodeSwitchEnumInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSwitchEnum);
public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant& value = p_context.get_input(0);

        int index = 0;
        for (const Ref<OScriptNodePin>& pin : _node->find_pins(PD_Output))
        {
            if (pin->get_generated_default_value() == value)
                return index;
            index++;
        }

        return -1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSwitch::_get_property_list(List<PropertyInfo> *r_list) const
{
    r_list->push_back(PropertyInfo(Variant::INT, "cases", PROPERTY_HINT_RANGE, "0,32", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeSwitch::_get(const StringName &p_name, Variant &r_value) const
{
    if (p_name.match("cases"))
    {
        r_value = _cases;
        return true;
    }
    return false;
}

bool OScriptNodeSwitch::_set(const StringName &p_name, const Variant &p_value)
{
    if (p_name.match("cases"))
    {
        int new_cases = (int) p_value;
        if (_cases != new_cases)
        {
            if (new_cases < _cases)
                _unlink_pins(new_cases);

            _cases = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

void OScriptNodeSwitch::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - Make sure pins encode the variant flag
        Ref<OScriptNodePin> value = find_pin("value", PD_Input);
        if (value.is_valid() && PropertyUtils::is_nil_no_variant(value->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeSwitch::_unlink_pins(int p_new_cases)
{
    // Case 0 index is 2
    int break_index = p_new_cases + 2;
    Vector<Ref<OScriptNodePin>> removals;
    for (const Ref<OScriptNodePin>& pin : get_all_pins())
    {
        if (pin->get_pin_index() >= break_index)
            removals.push_back(pin);
    }

    for (const Ref<OScriptNodePin>& pin : removals)
    {
        pin->unlink_all();
        remove_pin(pin);
    }
}

void OScriptNodeSwitch::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"))->set_label("value_is:");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_variant("value"));

    for (int i = 0; i < _cases; i++)
        create_pin(PD_Input, PT_Data, PropertyUtils::make_variant(_get_pin_name_given_index(i)));

    // Push output ports down to align with input cases.
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("Done"))->show_label();
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("default"))->show_label();

    for (int i = 0; i < _cases; i++)
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec(_get_pin_name_given_index(i) + "_out"));
}

String OScriptNodeSwitch::get_tooltip_text() const
{
    return "Selects an output that matches the input value.";
}

String OScriptNodeSwitch::get_node_title() const
{
    return "Switch";
}

String OScriptNodeSwitch::get_icon() const
{
    return "ClassList";
}

OScriptNodeInstance* OScriptNodeSwitch::instantiate()
{
    OScriptNodeSwitchInstance* i = memnew(OScriptNodeSwitchInstance);
    i->_node = this;
    i->_case_count = _cases;
    return i;
}

void OScriptNodeSwitch::add_dynamic_pin()
{
    _cases++;
    reconstruct_node();
}

bool OScriptNodeSwitch::can_add_dynamic_pin() const
{
    return _cases < 32;
}

bool OScriptNodeSwitch::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->get_pin_name().begins_with(get_pin_prefix()))
        return true;

    return super::can_remove_dynamic_pin(p_pin);
}

void OScriptNodeSwitch::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (!p_pin.is_valid())
        return;

    String input_name;
    if (p_pin->is_output())
        input_name = p_pin->get_pin_name().substr(0, p_pin->get_pin_name().length() - 4);  // 4 = "_out"
    else
        input_name = p_pin->get_pin_name();

    String output_name = input_name + "_out";

    Ref<OScriptNodePin> other;
    if (p_pin->is_output())
        other = find_pin(input_name, PD_Input);
    else
        other = find_pin(output_name, PD_Output);

    if (!other.is_valid())
        return;

    // Always base offset on input pin, although they should be identical
    int pin_offset = p_pin->is_input() ? p_pin->get_pin_index() : other->get_pin_index();

    // Unlink and remove pins
    p_pin->unlink_all(true);
    other->unlink_all(true);
    remove_pin(p_pin);
    remove_pin(other);

    // Adjust pins on Input and Output
    _adjust_connections(pin_offset, -1);

    _cases--;
    reconstruct_node();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSwitchEditablePin::_get_property_list(List<PropertyInfo>* p_list) const
{
    int flags = _can_pin_names_be_edited() ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_STORAGE;

    for (int i = 1; i <= _pin_names.size(); i++)
        p_list->push_back(PropertyInfo(Variant::STRING, "pin_names/name_" + itos(i), PROPERTY_HINT_NONE, "", flags));

    if (_supports_case_sensitive_pins())
        p_list->push_back(PropertyInfo(Variant::BOOL, "case_sensitive"));

    p_list->push_back(PropertyInfo(Variant::BOOL, "has_default_pin"));
}

bool OScriptNodeSwitchEditablePin::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.begins_with("pin_names/name_"))
    {
        const int64_t index = p_name.get_slicec('_', 2).to_int() - 1;
        ERR_FAIL_INDEX_V(index, _pin_names.size(), false);
        r_value = _pin_names[index];
        return true;
    }
    else if (p_name.match("case_sensitive"))
    {
        r_value = _case_sensitive;
        return true;
    }
    else if (p_name.match("has_default_pin"))
    {
        r_value = _has_default_value;
        return true;
    }
    return false;
}

bool OScriptNodeSwitchEditablePin::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.begins_with("pin_names/name_"))
    {
        const int64_t index = p_name.get_slicec('_', 2).to_int() - 1;

        const int64_t current_size = _pin_names.size();
        if (index >= current_size)
        {
            _pin_names.resize(index + 1);
            notify_property_list_changed();
        }

        ERR_FAIL_INDEX_V(index, _pin_names.size(), false);
        _pin_names[index] = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("case_sensitive"))
    {
        _case_sensitive = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("has_default_pin"))
    {
        _has_default_value = p_value;
        if (!_has_default_value)
        {
            Ref<OScriptNodePin> pin = find_pin("default", PD_Output);
            if (pin.is_valid() && pin->has_any_connections())
                pin->unlink_all(true);
        }
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeSwitchEditablePin::_recompute_pin_names(int p_index)
{
    for (int i = p_index; i < _pin_names.size() - 1; i++)
        _pin_names[i] = _pin_names[i + 1];
}

void OScriptNodeSwitchEditablePin::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("value", _get_input_pin_type()));

    for (int i = 0; i < _pin_names.size(); i++)
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec(_get_pin_name_given_index(i)))->set_label(_pin_names[i], false);

    if (_has_default_value)
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("default"))->set_label("Default");
}

String OScriptNodeSwitchEditablePin::get_tooltip_text() const
{
    return "Switches an output that matches the input.";
}

bool OScriptNodeSwitchEditablePin::can_add_dynamic_pin() const
{
    return _pin_names.size() < 32;
}

bool OScriptNodeSwitchEditablePin::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && p_pin->get_pin_name().begins_with(get_pin_prefix()))
        return true;

    return super::can_remove_dynamic_pin(p_pin);
}

void OScriptNodeSwitchEditablePin::add_dynamic_pin()
{
    _pin_names.resize(_pin_names.size() + 1);
    _pin_names[_pin_names.size() - 1] = _get_new_pin_name();

    reconstruct_node();

    if (_has_default_value)
    {
        // We simply attempt to adjust connections up by 1 for new next-to-last index
        const int default_index = find_pin("default", PD_Output)->get_pin_index();
        _adjust_connections(default_index - 1, 1, PD_Output);
    }

    notify_property_list_changed();
}

void OScriptNodeSwitchEditablePin::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    int pin_offset = p_pin->get_pin_index();

    p_pin->unlink_all(true);
    remove_pin(p_pin);

    _adjust_connections(pin_offset, -1, PD_Output);

    _recompute_pin_names(pin_offset);
    _pin_names.resize(_pin_names.size() - 1);

    reconstruct_node();

    notify_property_list_changed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String OScriptNodeSwitchString::_get_new_pin_name()
{
    for (int i = 0; i < _pin_names.size(); i++)
    {
        String name = vformat("Case_%d", i);
        if (!_pin_names.has(name))
            return name;
    }
    return {};
}

OScriptNodeInstance* OScriptNodeSwitchString::instantiate()
{
    OScriptNodeSwitchStringInstance* i = memnew(OScriptNodeSwitchStringInstance);
    i->_node = this;
    i->_values = _pin_names;
    i->_case_sensitive = _case_sensitive;
    i->_has_default = _has_default_value;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSwitchInteger::_get_property_list(List<PropertyInfo>* p_list) const
{
    p_list->push_back(PropertyInfo(Variant::INT, "start_index"));
}

bool OScriptNodeSwitchInteger::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("start_index"))
    {
        r_value = _start_index;
        return true;
    }

    // todo:    This appears to be a GodotCPP bug.
    //          So there is no need to dispatch to _get_property_list on super types, but
    //          if we don't do that here, then values are not properly fetched.
    return super::_get(p_name, r_value);
}

bool OScriptNodeSwitchInteger::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("start_index"))
    {
        _start_index = p_value;
        _recompute_pin_names(0);
        _notify_pins_changed();
        return true;
    }

    // todo:    This appears to be a GodotCPP bug.
    //          So there is no need to dispatch to _get_property_list on super types, but
    //          if we don't do that here, then values are not properly set.
    return super::_set(p_name, p_value);
}

void OScriptNodeSwitchInteger::_recompute_pin_names(int p_index)
{
    for (int i = 0; i < _pin_names.size(); i++)
        _pin_names[i] = itos(_start_index + i);
}

String OScriptNodeSwitchInteger::_get_new_pin_name()
{
    return itos(_start_index + (_pin_names.size() - 1));
}

OScriptNodeInstance* OScriptNodeSwitchInteger::instantiate()
{
    OScriptNodeSwitchIntegerInstance* i = memnew(OScriptNodeSwitchIntegerInstance);
    i->_node = this;
    i->_has_default = _has_default_value;

    for (const String& pin_name : _pin_names)
        i->_values.push_back(pin_name.to_int());

    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSwitchEnum::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - Some enum nodes were encoded as object types, which is incorrect.
        Ref<OScriptNodePin> pin = find_pin("value", PD_Input);
        if (pin.is_valid() && pin->is_enum() && pin->get_property_info().type == Variant::OBJECT)
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeSwitchEnum::post_initialize()
{
    Ref<OScriptNodePin> pin = find_pin("value", PD_Input);
    if (pin.is_valid())
    {
        if (pin->is_enum())
            _enum_name = pin->get_target_class();
    }
    super::post_initialize();
}

void OScriptNodeSwitchEnum::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"))->set_label("value_is:");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_enum_class("value", _enum_name));

    const EnumInfo& ei = ExtensionDB::get_global_enum(_enum_name);
    for (const EnumValue& ev : ei.values)
    {
        if (!ev.friendly_name.is_empty())
        {
            Ref<OScriptNodePin> out = create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("case_" + itos(ev.value) + "_out"));
            out->set_label(ev.friendly_name, false);
            out->set_generated_default_value(ev.value);
        }
    }
}

String OScriptNodeSwitchEnum::get_node_title() const
{
    return vformat("Switch on %s", _enum_name);
}

String OScriptNodeSwitchEnum::get_tooltip_text() const
{
    return "Selects an output that matches the input value.";
}

OScriptNodeInstance* OScriptNodeSwitchEnum::instantiate()
{
    OScriptNodeSwitchEnumInstance* i = memnew(OScriptNodeSwitchEnumInstance);
    i->_node = this;
    return i;
}

void OScriptNodeSwitchEnum::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("enum"))
            _enum_name = data["enum"];
    }
    super::initialize(p_context);
}