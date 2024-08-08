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
#include "select.h"

#include "common/property_utils.h"
#include "common/variant_utils.h"

class OScriptNodeSelectInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSelect);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant pick_a = p_context.get_input(2);
        if (pick_a.booleanize())
            p_context.set_output(0, &p_context.get_input(0));
        else
            p_context.set_output(0, &p_context.get_input(1));

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSelect::_get_property_list(List<PropertyInfo>* r_list) const
{
    static String types = VariantUtils::to_enum_list(false);
    r_list->push_back(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, types, PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeSelect::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("type"))
    {
        r_value = (_type - 1); // any is skipped
        return true;
    }
    return false;
}

bool OScriptNodeSelect::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("type"))
    {
        _type = p_value;
        _type += 1; // any is skipped

        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeSelect::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - reconstruct if node is NIL without variant flag
        Ref<OScriptNodePin> a = find_pin("a", PD_Input);
        if (!a.is_valid() || PropertyUtils::is_nil_no_variant(a->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeSelect::allocate_default_pins()
{
    const Variant::Type type = VariantUtils::to_type(_type);
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("a", type, true));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("b", type, true));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("pick_a", Variant::BOOL), false);
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("result", type, true));

    super::allocate_default_pins();
}

String OScriptNodeSelect::get_tooltip_text() const
{
    return "If 'Pick A' is true, A is returned, otherwise B is.";
}

String OScriptNodeSelect::get_node_title() const
{
    return "Select";
}

String OScriptNodeSelect::get_icon() const
{
    return "ClassList";
}

void OScriptNodeSelect::change_pin_types(Variant::Type p_type)
{
    _type = p_type;
    reconstruct_node();
}

Vector<Variant::Type> OScriptNodeSelect::get_possible_pin_types() const
{
    Vector<Variant::Type> types;
    for (int i = 0; i < Variant::VARIANT_MAX; i++)
    {
        if (i >= int(Variant::RID))
            break;

        types.push_back(VariantUtils::to_type(i));
    }
    return types;
}

OScriptNodeInstance* OScriptNodeSelect::instantiate()
{
    OScriptNodeSelectInstance* i = memnew(OScriptNodeSelectInstance);
    i->_node = this;
    return i;
}
