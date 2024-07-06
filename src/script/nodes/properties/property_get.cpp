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
#include "property_get.h"

#include "script/script.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodePropertyGetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodePropertyGet);

    OScriptNodeProperty::CallMode _call_mode{ OScriptNodeProperty::CallMode::CALL_SELF };
    String _target_class;
    String _property_name;
    NodePath _node_path;

    Node* _get_node_path_target(OScriptExecutionContext& p_context)
    {
        if (Node* owner = Object::cast_to<Node>(p_context.get_owner()))
            return owner->get_tree()->get_current_scene()->get_node_or_null(_node_path);
        return nullptr;
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        switch (_call_mode)
        {
            case OScriptNodeProperty::CALL_SELF:
            {
                Variant value = p_context.get_owner()->get(_property_name);
                p_context.set_output(0, &value);
                break;
            }

            case OScriptNodeProperty::CALL_INSTANCE:
            {
                Variant instance = p_context.get_input(0);
                if (instance.get_type() == Variant::OBJECT)
                {
                    Object* obj = Object::cast_to<Object>(instance);
                    Variant value = obj->get(_property_name);
                    p_context.set_output(0, &value);
                }
                break;
            }

            case OScriptNodeProperty::CALL_NODE_PATH:
            {
                if (Node* target = _get_node_path_target(p_context))
                {
                    Variant value = target->get(_property_name);
                    p_context.set_output(0, &value);
                }
                break;
            }
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodePropertyGet::allocate_default_pins()
{
    if (_call_mode == CALL_INSTANCE)
        create_pin(PD_Input, PT_Data, "target", Variant::OBJECT);

    create_pin(PD_Output, PT_Data, _property_name, _property_type);
}

String OScriptNodePropertyGet::get_tooltip_text() const
{
    if (!_property_name.is_empty())
        return vformat("Returns the value the property '%s'", _property_name);
    else
        return "Returns the value of a given property";
}

String OScriptNodePropertyGet::get_node_title() const
{
    return vformat("Get %s%s", _property_name.capitalize(), _call_mode == CALL_SELF ? " (Self)" : "");
}

StringName OScriptNodePropertyGet::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output())
    {
        if (!_property_hint.is_empty())
            return _property_hint;
        if (!_base_type.is_empty())
            return _base_type;
    }
    return super::resolve_type_class(p_pin);
}

OScriptNodeInstance* OScriptNodePropertyGet::instantiate()
{
    OScriptNodePropertyGetInstance* i = memnew(OScriptNodePropertyGetInstance);
    i->_node = this;
    i->_call_mode = _call_mode;
    i->_target_class = _base_type;
    i->_property_name = _property_name;
    i->_node_path = _node_path;
    return i;
}

void OScriptNodePropertyGet::initialize(const OScriptNodeInitContext& p_context)
{
    super::initialize(p_context);
}