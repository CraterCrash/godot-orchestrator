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

#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "script/script_server.h"

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
    r_list->push_back(PropertyInfo(Variant::STRING, "target_class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeSceneNode::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("node_path"))
    {
        r_value = _node_path;
        return true;
    }
    if (p_name.match("target_class_name"))
    {
        r_value = _class_name;
        return true;
    }
    return false;
}

bool OScriptNodeSceneNode::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("node_path"))
    {
        _node_path = p_value;

        if (_initialized)
        {
            if (Node* node = _get_referenced_node())
            {
                Ref<Script> script = node->get_script();

                String global_class;
                if (script.is_valid())
                    global_class = ScriptServer::get_global_name(script);

                _class_name = StringUtils::default_if_empty(global_class, node->get_class());
            }
        }

        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("target_class_name"))
    {
        _class_name = p_value;
        return true;
    }
    return false;
}

void OScriptNodeSceneNode::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - class name to be encoded on output pin
        if (_class_name.is_empty())
        {
            const Ref<OScriptNodePin> output = find_pin(_node_path, PD_Output);
            if (output.is_valid() && !output->get_property_info().class_name.is_empty())
            {
                _class_name = output->get_property_info().class_name;
                reconstruct_node();
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

Node* OScriptNodeSceneNode::_get_referenced_node() const
{
    if (_is_in_editor() && !_node_path.is_empty())
    {
        if (SceneTree* st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop()))
        {
            if (st->get_edited_scene_root())
            {
                if (Node* root = st->get_edited_scene_root())
                    return root->get_node_or_null(_node_path);
            }
        }
    }
    return nullptr;
}

void OScriptNodeSceneNode::allocate_default_pins()
{
    const String class_name = StringUtils::default_if_empty(_class_name, "Node");
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object(_node_path, class_name))->no_pretty_format();

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
        if (Node* scene_node = _get_referenced_node())
            return memnew(OScriptTargetObject(scene_node, false));
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
    if (p_context.class_name)
        _class_name = p_context.class_name.value();

    super::initialize(p_context);
}

void OScriptNodeSceneNode::validate_node_during_build(BuildLog& p_log) const
{
    if (_node_path.is_empty())
        p_log.error(this, "No NodePath specified.");

    super::validate_node_during_build(p_log);
}
