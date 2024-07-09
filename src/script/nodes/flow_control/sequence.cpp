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
#include "sequence.h"

#include "common/property_utils.h"

class OScriptNodeSequenceInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSequence);
    int _steps{ 0 };

public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        if (p_context.get_step_mode() == STEP_MODE_BEGIN)
            p_context.set_working_memory(0, 0);

        int step = p_context.get_working_memory(0);
        if (step + 1 == _steps)
            return step;

        p_context.set_working_memory(0, step + 1);
        return step | STEP_FLAG_PUSH_STACK_BIT;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSequence::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::INT, "steps", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeSequence::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("steps"))
    {
        r_value = _steps;
        return true;
    }
    return false;
}

bool OScriptNodeSequence::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("steps"))
    {
        int new_steps = Math::max(int(p_value), 2);
        if (new_steps != _steps)
        {
            if (_steps > new_steps)
            {
                for (const Ref<OScriptNodePin>& pin : get_all_pins())
                    if (pin.is_valid() && pin->is_output() && pin->get_pin_index() >= new_steps)
                        pin->unlink_all();
            }
            _steps = new_steps;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

void OScriptNodeSequence::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    for (int i = 0; i < _steps; i++)
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec(_get_pin_name_given_index(i)))->show_label();

    super::allocate_default_pins();
}

String OScriptNodeSequence::get_tooltip_text() const
{
    return "Executes a series of pins in order.";
}

String OScriptNodeSequence::get_node_title() const
{
    return "Sequence";
}

String OScriptNodeSequence::get_icon() const
{
    return "AnimationTrackList";
}

OScriptNodeInstance* OScriptNodeSequence::instantiate()
{
    OScriptNodeSequenceInstance* i = memnew(OScriptNodeSequenceInstance);
    i->_node = this;
    i->_steps = _steps;
    return i;
}

void OScriptNodeSequence::add_dynamic_pin()
{
    _steps++;
    reconstruct_node();
}

bool OScriptNodeSequence::can_add_dynamic_pin() const
{
    return _steps < 10;
}

bool OScriptNodeSequence::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    // Sequence requires a minimum of 2 output pins
    if (_steps > 2)
    {
        if (p_pin.is_valid() && p_pin->is_output() && p_pin->get_pin_name().begins_with(get_pin_prefix()))
            return true;
    }
    return super::can_remove_dynamic_pin(p_pin);
}

void OScriptNodeSequence::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin.is_valid() && p_pin->is_output())
    {
        int pin_offset = p_pin->get_pin_index();

        p_pin->unlink_all(true);
        remove_pin(p_pin);

        _adjust_connections(pin_offset, -1, PD_Output);

        _steps--;
        reconstruct_node();
    }
}
