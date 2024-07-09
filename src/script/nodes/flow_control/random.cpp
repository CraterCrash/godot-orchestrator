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
#include "random.h"

#include "common/property_utils.h"

class OScriptNodeRandomInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeRandom);

    Ref<RandomNumberGenerator> _random;
    int _possibilities{ 0 };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_possibilities == 0)
            return -1;

        if (!_random.is_valid())
            _random.instantiate();

        return _random->randi_range(0, _possibilities - 1);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeRandom::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::INT, "possibilities", PROPERTY_HINT_RANGE, "1,10", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeRandom::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("possibilities"))
    {
        r_value = _possibilities;
        return true;
    }
    return false;
}

bool OScriptNodeRandom::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("possibilities"))
    {
        int new_possibilities = p_value;
        if (new_possibilities != _possibilities)
        {
            if (_possibilities > new_possibilities)
            {
                for (const Ref<OScriptNodePin>& pin : get_all_pins())
                    if (pin.is_valid() && pin->is_output() && pin->get_pin_index() >= new_possibilities)
                        pin->unlink_all();
            }
            _possibilities = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

void OScriptNodeRandom::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    for (int i = 0; i < _possibilities; i++)
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec(_get_pin_name_given_index(i)))->show_label();

    super::allocate_default_pins();
}

String OScriptNodeRandom::get_tooltip_text() const
{
    return "Picks a random output where each output has equal chance.";
}

String OScriptNodeRandom::get_node_title() const
{
    return "Random";
}

void OScriptNodeRandom::pin_default_value_changed(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->get_pin_name().match("possibilities"))
    {
        _possibilities = p_pin->get_effective_default_value();
        reconstruct_node();
    }

    super::pin_default_value_changed(p_pin);
}

OScriptNodeInstance* OScriptNodeRandom::instantiate()
{
    OScriptNodeRandomInstance* i = memnew(OScriptNodeRandomInstance);
    i->_node = this;
    i->_possibilities = _possibilities;
    return i;
}

void OScriptNodeRandom::add_dynamic_pin()
{
    _possibilities++;
    reconstruct_node();
}

bool OScriptNodeRandom::can_add_dynamic_pin() const
{
    return _possibilities <= 10;
}

bool OScriptNodeRandom::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    return _possibilities > 1 && !p_pin->is_input();
}

void OScriptNodeRandom::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin.is_valid())
    {
        int pin_offset = p_pin->get_pin_index();

        p_pin->unlink_all(true);
        remove_pin(p_pin);

        _adjust_connections(pin_offset, -1, PD_Output);

        _possibilities--;
        reconstruct_node();
    }
}