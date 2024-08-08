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
#include "dictionary.h"

#include "common/property_utils.h"

class OScriptNodeMakeDictionaryInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeMakeDictionary);
    int _count{ 0 };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Dictionary result;
        for (int i = 0; i < _count; i += 2)
        {
            Variant key = p_context.get_input(i);
            Variant value = p_context.get_input(i + 1);
            result[key] = value;
        }
        p_context.set_output(0, result);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeDictionarySetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeDictionarySet);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Dictionary dict = p_context.get_input(0);

        Variant oldValue;
        bool replaced = false;

        Variant key = p_context.get_input(1);
        if (dict.has(key))
        {
            oldValue = dict[key];
            replaced = true;
        }
        dict[key] = p_context.get_input(2);

        p_context.set_output(0, dict);
        p_context.set_output(1, replaced);
        p_context.set_output(2, oldValue);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeMakeDictionary::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        if (_element_count > 0)
        {
            // Fixup - make sure variant is encoded in pins
            const Ref<OScriptNodePin> key = find_pin(_get_pin_name_given_index(0) + "_key");
            if (key.is_valid() && !PropertyUtils::is_nil_no_variant(key->get_property_info()))
                reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeMakeDictionary::post_initialize()
{
    _element_count = find_pins(PD_Input).size() / 2;
    super::post_initialize();
}

void OScriptNodeMakeDictionary::allocate_default_pins()
{
    for (int i = 0; i < _element_count; i++)
    {
        const String element_prefix = _get_pin_name_given_index(i);

        Ref<OScriptNodePin> key = create_pin(PD_Input, PT_Data, PropertyUtils::make_variant(vformat("%s_key", element_prefix)));
        key->set_label(vformat("Key %d", i), false);

        Ref<OScriptNodePin> value = create_pin(PD_Input, PT_Data, PropertyUtils::make_variant(vformat("%s_value", element_prefix)));
        value->set_label(vformat("Value %d", i), false);
    }

    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("dictionary", Variant::DICTIONARY));
    super::allocate_default_pins();
}

String OScriptNodeMakeDictionary::get_tooltip_text() const
{
    return "Create a dictionary from a series of key/value pairs.";
}

String OScriptNodeMakeDictionary::get_node_title() const
{
    return "Make Dictionary";
}

String OScriptNodeMakeDictionary::get_icon() const
{
    return "Dictionary";
}

OScriptNodeInstance* OScriptNodeMakeDictionary::instantiate()
{
    OScriptNodeMakeDictionaryInstance* i = memnew(OScriptNodeMakeDictionaryInstance);
    i->_node = this;
    i->_count = _element_count * 2;
    return i;
}

void OScriptNodeMakeDictionary::add_dynamic_pin()
{
    _element_count++;
    reconstruct_node();
}

bool OScriptNodeMakeDictionary::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    return _element_count > 0 && p_pin->is_input();
}

void OScriptNodeMakeDictionary::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin.is_valid() && p_pin->is_input())
    {
        int pin_offset = p_pin->get_pin_index();

        // This node manages two input pins that act as a single unit. The pin offset must be checked
        // and if it is not event, it must be adjusted by 1 so that the work done here always starts
        // with the key and collectively works on the key/value tuple for that single unit.
        const bool is_even_index = (pin_offset % 2) == 0;
        if (!is_even_index && pin_offset > 0)
            pin_offset -= 1;

        const int pair_index = pin_offset / 2;
        Ref<OScriptNodePin> key = find_pin(vformat("%s_key", _get_pin_name_given_index(pair_index)), PD_Input);
        Ref<OScriptNodePin> value = find_pin(vformat("%s_value", _get_pin_name_given_index(pair_index)), PD_Input);

        // Unlink pins and remove them
        key->unlink_all(true);
        value->unlink_all(true);
        remove_pin(key);
        remove_pin(value);

        // Adjust connections but only on the input side
        // Adjusts by 2 due to the key/value tuple!
        _adjust_connections(pin_offset, -2, PD_Input);

        _element_count--;
        reconstruct_node();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeDictionarySet::post_initialize()
{
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeDictionarySet::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("target", Variant::DICTIONARY));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_variant("key"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_variant("value"));

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("dictionary", Variant::DICTIONARY));
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("replaced", Variant::BOOL));
    create_pin(PD_Output, PT_Data, PropertyUtils::make_variant("old_value"));

    super::allocate_default_pins();
}

String OScriptNodeDictionarySet::get_tooltip_text() const
{
    return "Set a dictionary key/value pair.";
}

String OScriptNodeDictionarySet::get_node_title() const
{
    return "Set Dictionary Item";
}

String OScriptNodeDictionarySet::get_icon() const
{
    return "Dictionary";
}

OScriptNodeInstance* OScriptNodeDictionarySet::instantiate()
{
    OScriptNodeDictionarySetInstance* i = memnew(OScriptNodeDictionarySetInstance);
    i->_node = this;
    return i;
}