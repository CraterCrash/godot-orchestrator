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
#include "graph_node_pin_factory.h"

#include "editor/graph/pins/graph_node_pins.h"

OrchestratorGraphNodePin* OrchestratorGraphNodePinFactory::create_pin(OrchestratorGraphNode* p_node, Ref<OScriptNodePin> p_pin)
{
    if (p_pin->get_flags().has_flag(OScriptNodePin::Flags::EXECUTION))
        return memnew(OrchestratorGraphNodePinExec(p_node, p_pin));

    else if (p_pin->get_flags().has_flag(OScriptNodePin::Flags::FILE))
        return memnew(OrchestratorGraphNodePinFile(p_node, p_pin));

    else if (p_pin->get_flags().has_flag(OScriptNodePin::Flags::ENUM))
        return memnew(OrchestratorGraphNodePinEnum(p_node, p_pin));

    else if (p_pin->get_flags().has_flag(OScriptNodePin::Flags::BITFIELD))
        return memnew(OrchestratorGraphNodePinBitField(p_node, p_pin));

    switch (p_pin->get_type())
    {
        case Variant::STRING:
        case Variant::STRING_NAME:
            return memnew(OrchestratorGraphNodePinString(p_node, p_pin));

        case Variant::FLOAT:
        case Variant::INT:
            return memnew(OrchestratorGraphNodePinNumeric(p_node, p_pin));

        case Variant::BOOL:
            return memnew(OrchestratorGraphNodePinBool(p_node, p_pin));

        case Variant::COLOR:
            return memnew(OrchestratorGraphNodePinColor(p_node, p_pin));

        case Variant::OBJECT:
            return memnew(OrchestratorGraphNodePinObject(p_node, p_pin));

        case Variant::NODE_PATH:
            return memnew(OrchestratorGraphNodePinNodePath(p_node, p_pin));

        // Composite/Struct types
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::TRANSFORM2D:
        case Variant::TRANSFORM3D:
        case Variant::PLANE:
        case Variant::QUATERNION:
        case Variant::PROJECTION:
        case Variant::AABB:
        case Variant::BASIS:
            return memnew(OrchestratorGraphNodePinStruct(p_node, p_pin));

        default:
            return memnew(OrchestratorGraphNodePin(p_node, p_pin));
    }
}

