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
#include "chance.h"

class OScriptNodeChanceInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeChance);
    Ref<RandomNumberGenerator> _random;
    int _chance{ 0 };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (!_random.is_valid())
            _random.instantiate();

        const int _calculated_chance = _random->randi_range(0, 100);
        return _calculated_chance <= _chance ? 0 : 1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeChance::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::INT, "chance", PROPERTY_HINT_RANGE, "0,100"));
}

bool OScriptNodeChance::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("chance"))
    {
        r_value = _chance;
        return true;
    }
    return false;
}

bool OScriptNodeChance::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("chance"))
    {
        _chance = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeChance::post_initialize()
{
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeChance::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, "ExecIn");

    create_pin(PD_Output, PT_Execution, "Within")->set_label(vformat("%d%%", _chance));
    create_pin(PD_Output, PT_Execution, "Outside")->set_label(vformat("%d%%", 100 - _chance));

    super::allocate_default_pins();
}

String OScriptNodeChance::get_tooltip_text() const
{
    return "Calculates a percentage chance (0 to 100), taking the path based on the chance.";
}

String OScriptNodeChance::get_node_title() const
{
    return "Chance";
}

OScriptNodeInstance* OScriptNodeChance::instantiate()
{
    OScriptNodeChanceInstance *i = memnew(OScriptNodeChanceInstance);
    i->_node = this;
    i->_chance = _chance;
    return i;
}