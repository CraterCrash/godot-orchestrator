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
#include "autoload.h"

#include "common/property_utils.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/node.hpp>

class OScriptNodeAutoloadInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeAutoload);
    String _autoload;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (!OScriptLanguage::get_singleton()->has_any_global_constant(_autoload))
        {
            p_context.set_error(vformat("No autoload with name %s found.", _autoload));
            return -1;
        }

        Variant autoload = OScriptLanguage::get_singleton()->get_any_global_constant(_autoload);
        p_context.set_output(0, autoload);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeAutoload::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String autoload_names = StringUtils::join(",", OScriptLanguage::get_singleton()->get_global_constant_names());
    r_list->push_back(PropertyInfo(Variant::STRING, "autoload", PROPERTY_HINT_ENUM, autoload_names));
}

bool OScriptNodeAutoload::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("autoload"))
    {
        r_value = _autoload;
        return true;
    }
    return false;
}

bool OScriptNodeAutoload::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("autoload"))
    {
        _autoload = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeAutoload::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 and p_current_version >= 2)
    {
        // Fixup - makes sure that autoload class type is encoded in pin
        Ref<OScriptNodePin> pin = find_pin("autoload", PD_Output);
        if (pin.is_valid() && pin->get_property_info().class_name.is_empty())
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

Variant OScriptNodeAutoload::_get_autoload_instance() const
{
    Variant value = OScriptLanguage::get_singleton()->get_any_global_constant(_autoload);
    if (value && value.get_type() == Variant::OBJECT)
        return value;

    return {};
}

String OScriptNodeAutoload::_get_autoload_base_type() const
{
    const Variant instance = _get_autoload_instance();
    if (Object* autoload = Object::cast_to<Object>(instance))
        return autoload->get_class();

    return "";
}

void OScriptNodeAutoload::allocate_default_pins()
{
    // Always default to first registered autoload
    if (_autoload.is_empty())
    {
        const PackedStringArray values = OScriptLanguage::get_singleton()->get_global_constant_names();
        if (!values.is_empty())
            _autoload = values[0];
    }

    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("autoload", _get_autoload_base_type()))->set_label(_autoload, false);

    super::allocate_default_pins();
}

String OScriptNodeAutoload::get_tooltip_text() const
{
    return "Obtain a reference to a project autoload.";
}

String OScriptNodeAutoload::get_node_title() const
{
    return vformat("Get %s", _autoload);
}

String OScriptNodeAutoload::get_icon() const
{
    return "GodotMonochrome";
}

StringName OScriptNodeAutoload::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    return StringUtils::default_if_empty(_get_autoload_base_type(), _autoload);
}

Ref<OScriptTargetObject> OScriptNodeAutoload::resolve_target(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin->is_output() && p_pin->get_pin_name().match("autoload"))
    {
        Variant instance = _get_autoload_instance();
        if (instance)
            return memnew(OScriptTargetObject(instance, false));
    }
    return super::resolve_target(p_pin);
}

OScriptNodeInstance* OScriptNodeAutoload::instantiate()
{
    OScriptNodeAutoloadInstance* i = memnew(OScriptNodeAutoloadInstance);
    i->_node = this;
    i->_autoload = _autoload;
    return i;
}

void OScriptNodeAutoload::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data.has_value() && p_context.user_data.value().has("class_name"))
        _autoload = p_context.user_data.value()["class_name"];

    super::initialize(p_context);
}

void OScriptNodeAutoload::validate_node_during_build(BuildLog& p_log) const
{
    if (OScriptLanguage::get_singleton()->has_any_global_constant(_autoload))
    {
        Variant instance = _get_autoload_instance();
        if (instance && Object::cast_to<Node>(instance))
            return;
    }

    p_log.error(this, vformat("No autoload registered with name '%s' in the project settings.", _autoload));

    super::validate_node_during_build(p_log);
}
