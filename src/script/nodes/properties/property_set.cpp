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
#include "property_set.h"

#include "common/property_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodePropertySetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodePropertySet);

    OScriptNodeProperty::CallMode _call_mode;
    String _target_class;
    PropertyInfo _property;
    NodePath _node_path;

    Node* _get_node_path_target(OScriptExecutionContext& p_context)
    {
        if (Node* owner = Object::cast_to<Node>(p_context.get_owner()))
            return owner->get_tree()->get_current_scene()->get_node_or_null(_node_path);
        return nullptr;
    }

public:
    int step(OScriptExecutionContext& p_context)
    {
        const Variant& input = p_context.get_input(0);
        switch (_call_mode)
        {
            case OScriptNodeProperty::CALL_SELF:
                p_context.get_owner()->set(_property.name, input);
                break;

            case OScriptNodeProperty::CALL_NODE_PATH:
                if (Node* target = _get_node_path_target(p_context))
                    target->set(_property.name, input);
                break;

            case OScriptNodeProperty::CALL_INSTANCE:
                Variant instance = p_context.get_input(0);
                if (instance.get_type() == Variant::OBJECT)
                {
                    Object* obj = Object::cast_to<Object>(instance);
                    obj->set(_property.name, p_context.get_input(1));
                }
                break;
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodePropertySet::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    if (_call_mode == CALL_INSTANCE)
    {
        Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target", _base_type));
        target->set_label(_base_type);
        target->no_pretty_format();
    }

    create_pin(PD_Input, PT_Data, _property);
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
}

String OScriptNodePropertySet::get_tooltip_text() const
{
    if (!_property.name.is_empty())
        return vformat("Set the value for the property '%s'", _property.name);

    return "Sets the value for a given property";
}

String OScriptNodePropertySet::get_node_title() const
{
    return vformat("Set %s%s", _property.name.capitalize(), _call_mode == CALL_SELF ? " (Self)" : "");
}

OScriptNodeInstance* OScriptNodePropertySet::instantiate()
{
    OScriptNodePropertySetInstance* i = memnew(OScriptNodePropertySetInstance);
    i->_node = this;
    i->_call_mode = _call_mode;
    i->_target_class = _base_type;
    i->_property = _property;
    i->_node_path = _node_path;
    return i;
}

void OScriptNodePropertySet::set_default_value(const Variant& p_default_value)
{
    find_pin(1, PD_Input)->set_default_value(p_default_value);
}