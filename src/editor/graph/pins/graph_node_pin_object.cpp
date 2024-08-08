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
#include "graph_node_pin_object.h"

#include "script/node.h"
#include "script/nodes/functions/call_function.h"
#include "script/nodes/variables/variable.h"

OrchestratorGraphNodePinObject::OrchestratorGraphNodePinObject(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinObject::_bind_methods()
{
}

void OrchestratorGraphNodePinObject::_update_label()
{
    if (Object::cast_to<OScriptNodeCallFunction>(_pin->get_owning_node()))
    {
        if (_pin->get_pin_name().match("target") && !_pin->has_any_connections())
        {
            const String target_class = _pin->get_property_info().class_name;
            const String base_type = _pin->get_owning_node()->get_orchestration()->get_base_type();
            if (!target_class.is_empty() && ClassDB::is_parent_class(base_type, target_class))
            {
                if (!_pin->has_any_connections() || !Object::cast_to<OScriptNodeVariable>(_pin->get_owning_node()))
                {
                    _label->set_text("[Self]");
                    return;
                }
            }
        }
    }

    OrchestratorGraphNodePin::_update_label();
}
