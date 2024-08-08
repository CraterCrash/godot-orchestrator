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
#include "preload.h"

#include "common/property_utils.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

class OScriptNodePreloadInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodePreload);
    Ref<Resource> _resource;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant out = _resource;
        p_context.set_output(0, &out);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodePreload::_get_property_list(List<PropertyInfo> *r_list) const
{
    r_list->push_back(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource", PROPERTY_USAGE_EDITOR));
    r_list->push_back(PropertyInfo(Variant::STRING, "path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodePreload::_get(const StringName &p_name, Variant &r_value) const
{
    if (p_name.match("resource"))
    {
        r_value = _resource;
        return true;
    }
    else if (p_name.match("path"))
    {
        r_value = _resource_path;
        return true;
    }
    return false;
}

bool OScriptNodePreload::_set(const StringName &p_name, const Variant& p_value)
{
    if (p_name.match("resource"))
    {
        _resource = p_value;
        if (_resource.is_valid())
            _resource_path = _resource->get_path();
        else
            _resource_path = "";
        notify_property_list_changed();
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("path"))
    {
        _resource_path = p_value;
        _resource = ResourceLoader::get_singleton()->load(_resource_path);
        notify_property_list_changed();
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodePreload::post_initialize()
{
    // Fixup resource pin attributes
    if (!_resource.is_valid() && !_resource_path.is_empty())
        _resource = ResourceLoader::get_singleton()->load(_resource_path);

    reconstruct_node();
    super::post_initialize();
}

void OScriptNodePreload::allocate_default_pins()
{
    const String class_name = !_resource.is_valid() ? "Resource" : _resource->get_class();
    const String path = StringUtils::default_if_empty(_resource_path, "No Resource");
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("path", class_name), _resource_path)->set_label(path, false);

    super::allocate_default_pins();
}

String OScriptNodePreload::get_tooltip_text() const
{
    return "Asynchronously loads the specified resource and returns the resource if the load succeeds.";
}

String OScriptNodePreload::get_node_title() const
{
    return "Preload Resource";
}

String OScriptNodePreload::get_icon() const
{
    return "ResourcePreloader";
}

StringName OScriptNodePreload::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        // If resource is invalid, attempt to load it
        Ref<Resource> resource = _resource;
        if (!resource.is_valid())
            resource = ResourceLoader::get_singleton()->load(_resource_path);

        // If resource is valid, if scene, get root node type; otherwise resource type
        if (resource.is_valid())
        {
            const Ref<PackedScene> scene = resource;
            if (scene.is_valid() && scene->can_instantiate())
                return scene->instantiate()->get_class();

            return resource->get_class();
        }
    }
    return super::resolve_type_class(p_pin);
}

OScriptNodeInstance* OScriptNodePreload::instantiate()
{
    OScriptNodePreloadInstance* i = memnew(OScriptNodePreloadInstance);
    i->_node = this;
    i->_resource = _resource;
    return i;
}

void OScriptNodePreload::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.resource_path)
    {
        _resource_path = p_context.resource_path.value();
        _resource = ResourceLoader::get_singleton()->load(_resource_path);
    }
    super::initialize(p_context);
}

void OScriptNodePreload::validate_node_during_build(BuildLog& p_log) const
{
    if (_resource_path.is_empty())
        p_log.error(this, "No resource specified.");
    else if (!FileAccess::file_exists(_resource_path))
        p_log.error(this, "Resource no longer exists.");

    super::validate_node_during_build(p_log);
}
