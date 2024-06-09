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
#include "engine_singleton.h"

#include "common/string_utils.h"

#include <godot_cpp/classes/engine.hpp>

class OScriptNodeEngineSingletonInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeEngineSingleton);
    Object* _value{ nullptr };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeEngineSingleton::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String singleton_names = StringUtils::join(",", Engine::get_singleton()->get_singleton_list());
    r_list->push_back(PropertyInfo(Variant::STRING, "singleton", PROPERTY_HINT_ENUM, singleton_names));
}

bool OScriptNodeEngineSingleton::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("singleton"))
    {
        r_value = _singleton;
        return true;
    }
    return false;
}

bool OScriptNodeEngineSingleton::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("singleton"))
    {
        _singleton = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeEngineSingleton::allocate_default_pins()
{
    Ref<OScriptNodePin> pin = create_pin(PD_Output, "singleton", Variant::OBJECT);
    pin->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);
    pin->set_label(_singleton);

    super::allocate_default_pins();
}

String OScriptNodeEngineSingleton::get_tooltip_text() const
{
    return "Obtain a reference to an engine singleton";
}

String OScriptNodeEngineSingleton::get_node_title() const
{
    return vformat("Get %s", _singleton);
}

String OScriptNodeEngineSingleton::get_icon() const
{
    return "GodotMonochrome";
}

PackedStringArray OScriptNodeEngineSingleton::get_keywords() const
{
    return Engine::get_singleton()->get_singleton_list();
}

StringName OScriptNodeEngineSingleton::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    return _singleton;
}

OScriptNodeInstance* OScriptNodeEngineSingleton::instantiate()
{
    OScriptNodeEngineSingletonInstance* i = memnew(OScriptNodeEngineSingletonInstance);
    i->_node = this;

    if (!_singleton.is_empty() && Engine::get_singleton()->get_singleton_list().has(_singleton))
        i->_value = Engine::get_singleton()->get_singleton(_singleton);

    return i;
}