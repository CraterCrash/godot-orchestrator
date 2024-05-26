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
#include "graph_node_pin_object.h"

#include "script/node.h"
#include "script/nodes/functions/call_function.h"

OrchestratorGraphNodePinObject::OrchestratorGraphNodePinObject(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinObject::_bind_methods()
{
}

Control* OrchestratorGraphNodePinObject::_get_default_value_widget()
{
    // Specifically checks if the node is a CallFunction node and if the pin is not the "Target" pin.
    // In these cases, we should not render the "(self)" label.
    if (OScriptNode* node = _pin->get_owning_node())
    {
        if (Object::cast_to<OScriptNodeCallFunction>(node))
        {
            if (!_pin->get_pin_name().match("target"))
                return nullptr;
        }
    }

    Label* label = memnew(Label);
    label->set_text("(self)");
    return label;
}