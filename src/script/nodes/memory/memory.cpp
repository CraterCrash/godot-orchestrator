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
#include "script/nodes/memory/memory.h"

#include "common/version.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>

class OScriptNodeNewInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeNew);
    String _class_name;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (!_class_name.is_empty() && ClassDB::can_instantiate(_class_name))
        {
            Variant object = ClassDB::instantiate(_class_name);
            p_context.set_output(0, object);
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeFreeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeFree);
public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant object = p_context.get_input(0);
        if (object)
        {
            Object* casted = Object::cast_to<Object>(object);
            if (ClassDB::is_parent_class(casted->get_class(), "Node"))
            {
                Node* node = Object::cast_to<Node>(casted);
                node->queue_free();
            }
            else if (ClassDB::is_parent_class(casted->get_class(), "RefCounted"))
            {
                RefCounted *ref = Object::cast_to<RefCounted>(casted);
                ref->unreference();
            }
            else
            {
                memdelete(casted);
            }
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeNew::OScriptNodeNew()
{
    _flags.set_flag(OScriptNode::ScriptNodeFlags::EXPERIMENTAL);
}

void OScriptNodeNew::_bind_methods()
{
}

void OScriptNodeNew::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_TYPE_STRING, "Object"));
}

bool OScriptNodeNew::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("class_name"))
    {
        r_value = _class_name;
        return true;
    }
    return false;
}

bool OScriptNodeNew::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("class_name"))
    {
        if (_class_name != p_value)
        {
            const bool is_singleton = Engine::get_singleton()->get_singleton_list().has(p_value);
            ERR_FAIL_COND_V_MSG(is_singleton, false, vformat("Cannot create an instance of '%s', a singleton.", p_value));

            _class_name = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

void OScriptNodeNew::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "Instance", Variant::OBJECT)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeNew::get_tooltip_text() const
{
    return vformat("Creates a new instance of %s.", _class_name.is_empty() ? "a class" : _class_name);
}

String OScriptNodeNew::get_node_title() const
{
    return _class_name.is_empty() ? "Create instance" : vformat("Create a %s", _class_name);
}

String OScriptNodeNew::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class:%s", _class_name);
    #else
    return super::get_help_topic();
    #endif
}

String OScriptNodeNew::get_icon() const
{
    return "CurveCreate";
}

OScriptNodeInstance* OScriptNodeNew::instantiate()
{
    OScriptNodeNewInstance* i = memnew(OScriptNodeNewInstance);
    i->_node = this;
    i->_class_name = _class_name;
    return i;
}

void OScriptNodeNew::initialize(const OScriptNodeInitContext& p_context)
{
    _class_name = "Object";
    super::initialize(p_context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeFree::OScriptNodeFree()
{
    _flags.set_flag(OScriptNode::ScriptNodeFlags::EXPERIMENTAL);
}

void OScriptNodeFree::_bind_methods()
{
}

void OScriptNodeFree::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "Target", Variant::OBJECT)->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::IGNORE_DEFAULT);
    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);

    super::allocate_default_pins();
}

String OScriptNodeFree::get_tooltip_text() const
{
    return "Free the memory used by the specified object.";
}

String OScriptNodeFree::get_node_title() const
{
    return "Free instance";
}

String OScriptNodeFree::get_icon() const
{
    return "CurveDelete";
}

OScriptNodeInstance* OScriptNodeFree::instantiate()
{
    OScriptNodeFreeInstance* i = memnew(OScriptNodeFreeInstance);
    i->_node = this;
    return i;
}
