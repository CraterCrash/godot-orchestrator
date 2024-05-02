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
#include "for_each.h"

#include "common/dictionary_utils.h"

class OScriptNodeForEachInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeForEach);

public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptNodeExecutionContext& p_context) override
    {
        // Break triggers node input port 1, check if that caused the step
        // If so we're done and we should exit.
        if (p_context.get_current_node_port() == 1)
            return 1;

        if (p_context.get_step_mode() == STEP_MODE_BEGIN)
            p_context.set_working_memory(0, 0);
        else
            p_context.set_working_memory(0, int(p_context.get_working_memory()) + 1);

        const Variant& array = p_context.get_input(0);
        const int index = p_context.get_working_memory();
        const int size = Array(array).size();

        if (index >= size)
        {
            // Nothing to iterate
            p_context.set_output(1, index);
            return 1;
        }

        const Variant element = Array(array)[index];

        p_context.set_output(0, element);
        p_context.set_output(1, index);

        return 0 | STEP_FLAG_PUSH_STACK_BIT;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeForEach::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::BOOL, "with_break", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeForEach::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("with_break"))
    {
        r_value = _with_break;
        return true;
    }
    return false;
}

bool OScriptNodeForEach::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("with_break"))
    {
        _with_break = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}


void OScriptNodeForEach::post_initialize()
{
    // Automatically coerces old element pins to using NIL for Any rather than OBJECT
    Ref<OScriptNodePin> element = find_pin("element", PD_Output);
    if (element.is_valid() && element->get_type() == Variant::OBJECT)
        element->set_type(Variant::NIL);

    super::post_initialize();
}

void OScriptNodeForEach::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::IGNORE_DEFAULT);

    if (_with_break)
        create_pin(PD_Input, "break")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);

    create_pin(PD_Output, "loop_body")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
    create_pin(PD_Output, "element", Variant::NIL)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "completed")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
}

String OScriptNodeForEach::get_tooltip_text() const
{
    return "Executes the 'Loop Body' for each element in the array.";
}

String OScriptNodeForEach::get_node_title() const
{
    return vformat("For Each%s", _with_break ? " With Break" : "");
}

String OScriptNodeForEach::get_icon() const
{
    return "Loop";
}

void OScriptNodeForEach::get_actions(List<Ref<OScriptAction>>& p_action_list)
{
    if (_with_break)
    {
        Callable callable = callable_mp(this, &OScriptNodeForEach::_set_with_break).bind(false);
        p_action_list.push_back(memnew(OScriptAction("Remove break pin", "Remove", callable)));
    }
    else
    {
        Callable callable = callable_mp(this, &OScriptNodeForEach::_set_with_break).bind(true);
        p_action_list.push_back(memnew(OScriptAction("Add break pin", "Add", callable)));
    }
    super::get_actions(p_action_list);
}

OScriptNodeInstance* OScriptNodeForEach::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeForEachInstance * i = memnew(OScriptNodeForEachInstance);
    i->_node = this;
    i->_instance = p_instance;
    return i;
}

void OScriptNodeForEach::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("with_break"))
            _with_break = data["with_break"];
    }

    super::initialize(p_context);
}

void OScriptNodeForEach::_set_with_break(bool p_break_status)
{
    _with_break = p_break_status;
    reconstruct_node();
}