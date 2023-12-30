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
#include "preload.h"

#include <godot_cpp/classes/resource_loader.hpp>

class OScriptNodePreloadInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodePreload);
    Ref<Resource> _resource;

public:
    int step(OScriptNodeExecutionContext& p_context) override
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

void OScriptNodePreload::allocate_default_pins()
{
    Ref<OScriptNodePin> path = create_pin(PD_Output, "path", Variant::OBJECT, _resource_path);
    path->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::SHOW_LABEL);
    path->set_label(_resource_path);
    path->set_target_class("Resource");

    super::allocate_default_pins();
}

void OScriptNodePreload::post_initialize()
{
    reconstruct_node();
    super::post_initialize();
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

OScriptNodeInstance* OScriptNodePreload::instantiate(OScriptInstance* p_instance)
{
    OScriptNodePreloadInstance* i = memnew(OScriptNodePreloadInstance);
    i->_node = this;
    i->_instance = p_instance;
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