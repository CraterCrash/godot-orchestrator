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
#include "scene_node.h"

#include "common/scene_utils.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodeSceneNodeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSceneNode);
    NodePath _node_path;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Node* owner = Object::cast_to<Node>(p_context.get_owner());
        if (!owner)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, "Orchestration owner is not a Node");
            return 0;
        }

        Node* scene_node = SceneUtils::get_relative_scene_root(owner)->get_node_or_null(_node_path);
        if (!scene_node)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, "Node path does not exist");
            return 0;
        }

        Variant variant = scene_node;
        p_context.set_output(0, &variant);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSceneNode::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::NODE_PATH, "node_path"));
}

bool OScriptNodeSceneNode::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("node_path"))
    {
        r_value = _node_path;
        return true;
    }
    return false;
}

bool OScriptNodeSceneNode::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("node_path"))
    {
        _node_path = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeSceneNode::allocate_default_pins()
{
    Ref<OScriptNodePin> path_pin = create_pin(PD_Output, PT_Data, _node_path, Variant::OBJECT);
    path_pin->set_flag(OScriptNodePin::OBJECT);
    path_pin->no_pretty_format();
    path_pin->set_target_class("Node");

    if (_is_in_editor() && !_node_path.is_empty())
    {
        Node* root = ((SceneTree*)Engine::get_singleton()->get_main_loop())->get_edited_scene_root();
        if (Node* target_node = root->get_node_or_null(_node_path))
            path_pin->set_target_class(target_node->get_class());
    }

    super::allocate_default_pins();
}

String OScriptNodeSceneNode::get_tooltip_text() const
{
    return "Return the specified scene node.";
}

String OScriptNodeSceneNode::get_node_title() const
{
    return "Get Scene Node";
}

String OScriptNodeSceneNode::get_icon() const
{
    return "NodeInfo";
}

Ref<OScriptTargetObject> OScriptNodeSceneNode::resolve_target(const Ref<OScriptNodePin>& p_pin) const
{
    if (_is_in_editor() && p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        SceneTree* st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (st && st->get_edited_scene_root())
        {
            Node* root = st->get_edited_scene_root();
            if (root)
                return memnew(OScriptTargetObject(root->get_node_or_null(_node_path), false));
        }
    }
    return super::resolve_target(p_pin);
}

OScriptNodeInstance* OScriptNodeSceneNode::instantiate()
{
    OScriptNodeSceneNodeInstance* i = memnew(OScriptNodeSceneNodeInstance);
    i->_node = this;
    i->_node_path = _node_path;
    return i;
}

void OScriptNodeSceneNode::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.node_path)
        _node_path = p_context.node_path.value();

    super::initialize(p_context);
}
