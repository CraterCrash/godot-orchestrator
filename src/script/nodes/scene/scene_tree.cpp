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
#include "scene_tree.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodeSceneTreeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSceneTree);

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        Node* owner = Object::cast_to<Node>(_instance->get_owner());
        if (!owner)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, "Orchestration owner is not a Node");
            return 0;
        }

        SceneTree* tree = owner->get_tree();
        if (!tree)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                                "Orchestrator owner node is not currently in a tree.");
            return 0;
        }

        Variant output = tree;
        p_context.set_output(0, &output);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSceneTree::allocate_default_pins()
{
    Ref<OScriptNodePin> pin = create_pin(PD_Output, "scene_tree", Variant::OBJECT);
    pin->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::OBJECT);
    pin->set_target_class("SceneTree");

    super::allocate_default_pins();
}

String OScriptNodeSceneTree::get_tooltip_text() const
{
    return "Return the scene tree.";
}

String OScriptNodeSceneTree::get_node_title() const
{
    return "Get Scene Tree";
}

String OScriptNodeSceneTree::get_icon() const
{
    return "NodeInfo";
}

OScriptNodeInstance* OScriptNodeSceneTree::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeSceneTreeInstance* i = memnew(OScriptNodeSceneTreeInstance);
    i->_node = this;
    i->_instance = p_instance;
    return i;
}
