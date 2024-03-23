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
#include "graph_node_pin_enum.h"

#include "api/extension_db.h"
#include <godot_cpp/classes/option_button.hpp>

OrchestratorGraphNodePinEnum::OrchestratorGraphNodePinEnum(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinEnum::_bind_methods()
{
}

void OrchestratorGraphNodePinEnum::_on_item_selected(int p_index, OptionButton* p_button)
{
    const String enum_class = _pin->get_target_class();
    if (ExtensionDB::get_global_enum_names().has(enum_class))
    {
        const EnumInfo& ei = ExtensionDB::get_global_enum(enum_class);
        _pin->set_default_value(ei.values[p_index].value);
        p_button->release_focus();
    }
}

Control* OrchestratorGraphNodePinEnum::_get_default_value_widget()
{
    OptionButton* button = memnew(OptionButton);
    button->connect("item_selected", callable_mp(this, &OrchestratorGraphNodePinEnum::_on_item_selected).bind(button));

    const String enum_class = _pin->get_target_class();
    if (!enum_class.is_empty() && ExtensionDB::get_global_enum_names().has(enum_class))
    {
        int effective_default = _pin->get_effective_default_value();

        const EnumInfo &ei = ExtensionDB::get_global_enum(enum_class);
        for (const EnumValue& value : ei.values)
        {
            if (!value.friendly_name.is_empty())
            {
                button->add_item(value.friendly_name);
                if (effective_default == value.value)
                    button->select(button->get_item_count() - 1);
            }
        }
    }

    return button;
}