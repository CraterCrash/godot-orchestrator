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
#include "script/nodes/scene/instantiate_scene.h"

#include "common/property_utils.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

class OScriptNodeInstantiateSceneInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeInstantiateScene);
    Ref<PackedScene> _scene;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (!_scene.is_valid())
        {
            _scene = ResourceLoader::get_singleton()->load(p_context.get_input(0));
            if (!_scene.is_valid())
            {
                p_context.set_error(vformat("Failed to load scene: %s", p_context.get_input(0)));
                return -1;
            }
        }

        Node* scene_node = _scene->instantiate();
        p_context.set_output(0, scene_node);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeInstantiateScene::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "scene", PROPERTY_HINT_FILE, "*.scn,*.tscn"));
}

bool OScriptNodeInstantiateScene::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("scene"))
    {
        r_value = _scene;
        return true;
    }
    return false;
}

bool OScriptNodeInstantiateScene::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("scene"))
    {
        _scene = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeInstantiateScene::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - make sure that the root scene node class name is encoded in the pin
        const Ref<OScriptNodePin> scene_root = find_pin("scene_root", PD_Output);
        if (scene_root.is_valid() && scene_root->get_property_info().class_name.is_empty())
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

Node *OScriptNodeInstantiateScene::_instantiate_scene() const
{
    if (!_scene.is_empty())
    {
        Ref<PackedScene> packed_scene = ResourceLoader::get_singleton()->load(_scene);
        if (packed_scene.is_valid())
        {
            if (packed_scene->can_instantiate())
                return packed_scene->instantiate();
        }
    }
    return nullptr;
}

void OScriptNodeInstantiateScene::post_initialize()
{
    Ref<OScriptNodePin> scene = find_pin("scene", PD_Input);
    if (scene.is_valid())
        _scene = scene->get_effective_default_value();

    super::post_initialize();
}

void OScriptNodeInstantiateScene::allocate_default_pins()
{
    String scene_root_class = Node::get_class_static();
    if (!_scene.is_empty())
    {
        if (Node* root = _instantiate_scene())
        {
            scene_root_class = root->get_class();
            memdelete(root);
        }
    }

    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_file("scene", "*.scn,*.tscn"), _scene);

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("scene_root", scene_root_class));

    super::allocate_default_pins();
}

String OScriptNodeInstantiateScene::get_tooltip_text() const
{
    return "Instantiates the specified scene.";
}

String OScriptNodeInstantiateScene::get_node_title() const
{
    return "Instantiate Scene";
}

String OScriptNodeInstantiateScene::get_icon() const
{
    return "PackedScene";
}

void OScriptNodeInstantiateScene::pin_default_value_changed(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->get_pin_name().match("scene"))
    {
        const String new_scene_name = p_pin->get_default_value();
        if (_scene != new_scene_name)
        {
            _scene = p_pin->get_default_value();
            _queue_reconstruct();
        }
    }

    super::pin_default_value_changed(p_pin);
}

StringName OScriptNodeInstantiateScene::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        if (Node* root = _instantiate_scene())
        {
            String class_name = root->get_class();
            memdelete(root);
            return class_name;
        }
    }
    return super::resolve_type_class(p_pin);
}

Ref<OScriptTargetObject> OScriptNodeInstantiateScene::resolve_target(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        if (Node* root = _instantiate_scene())
            return memnew(OScriptTargetObject(root, true));
    }
    return super::resolve_target(p_pin);
}

OScriptNodeInstance* OScriptNodeInstantiateScene::instantiate()
{
    OScriptNodeInstantiateSceneInstance* i = memnew(OScriptNodeInstantiateSceneInstance);
    i->_node = this;
    return i;
}
