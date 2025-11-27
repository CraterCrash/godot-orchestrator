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
#include "resource_path.h"

#include "common/property_utils.h"

#include <godot_cpp/classes/file_access.hpp>

class OScriptNodeResourcePathInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeResourcePath);
    Variant _path;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, &_path);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeResourcePath::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "path", PROPERTY_HINT_FILE));
}

bool OScriptNodeResourcePath::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("path"))
    {
        r_value = _path;
        return true;
    }
    return false;
}

bool OScriptNodeResourcePath::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("path"))
    {
        _path = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeResourcePath::allocate_default_pins()
{
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("path", Variant::STRING))->set_label(_path, false);

    super::allocate_default_pins();
}

String OScriptNodeResourcePath::get_tooltip_text() const
{
    return "Get the file path of an existing resource.";
}

String OScriptNodeResourcePath::get_node_title() const
{
    return "Get Resource Path";
}

String OScriptNodeResourcePath::get_icon() const
{
    return "ResourcePreloader";
}

OScriptNodeInstance* OScriptNodeResourcePath::instantiate()
{
    OScriptNodeResourcePathInstance *i = memnew(OScriptNodeResourcePathInstance);
    i->_node = this;
    i->_path = _path;
    return i;
}

void OScriptNodeResourcePath::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.resource_path)
        _path = p_context.resource_path.value();

    super::initialize(p_context);
}

void OScriptNodeResourcePath::validate_node_during_build(BuildLog& p_log) const
{
    if (!FileAccess::file_exists(_path))
        p_log.error(this, "Resource path '" + _path + "' no longer exists.");

    super::validate_node_during_build(p_log);
}
