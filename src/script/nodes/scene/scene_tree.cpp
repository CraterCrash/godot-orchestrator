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
#include "scene_tree.h"

#include "common/property_utils.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodeSceneTreeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSceneTree);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Node* owner = Object::cast_to<Node>(p_context.get_owner());
        if (!owner)
        {
            p_context.set_error("Orchestration owner is not a Node type");
            return 0;
        }

        SceneTree* tree = owner->get_tree();
        if (!tree)
        {
            p_context.set_error("Orchestrator owner node is not currently in the scene.");
            return 0;
        }

        Variant output = tree;
        p_context.set_output(0, &output);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSceneTree::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - make sure that the SceneTree class name is encoded in the pin
        Ref<OScriptNodePin> scene_tree = find_pin("scene_tree", PD_Output);
        if (!scene_tree.is_valid() || !scene_tree->get_property_info().class_name.is_empty())
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeSceneTree::allocate_default_pins()
{
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("scene_tree", SceneTree::get_class_static()));

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

String OScriptNodeSceneTree::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class:%s", SceneTree::get_class_static());
    #else
    return SceneTree::get_class_static();
    #endif
}

OScriptNodeInstance* OScriptNodeSceneTree::instantiate()
{
    OScriptNodeSceneTreeInstance* i = memnew(OScriptNodeSceneTreeInstance);
    i->_node = this;
    return i;
}
