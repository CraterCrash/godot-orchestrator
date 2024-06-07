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
#include "script/nodes/signals/emit_member_signal.h"

#include "common/dictionary_utils.h"

class OScriptNodeEmitMemberSignalInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeEmitMemberSignal);

    MethodInfo _method;
    Array _args;

    Object* _get_call_instance(OScriptNodeExecutionContext& p_context)
    {
        Variant target = p_context.get_input(0);
        return !target ? _instance->get_owner() : Object::cast_to<Object>(target);
    }

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        if (!_method.name.is_empty())
        {
            Object* instance = _get_call_instance(p_context);
            if (!instance)
            {
                ERR_PRINT("Cannot emit signal " + _method.name + " on an invalid target.");
                return -1 | STEP_FLAG_END;
            }

            if (!_method.arguments.empty())
            {
                if (uint64_t(_args.size()) != _method.arguments.size())
                    _args.resize(_method.arguments.size());

                for (int i = 0; i < _method.arguments.size(); i++)
                    _args[i] = p_context.get_input(i + 2);
            }

            instance->emit_signal(_method.name, _args);
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeEmitMemberSignal::_get_property_list(List<PropertyInfo>* r_list) const
{
    constexpr int64_t usage = PROPERTY_USAGE_STORAGE;
    r_list->push_back(PropertyInfo(Variant::STRING, "target_class", PROPERTY_HINT_TYPE_STRING, "", usage));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", usage));
}

bool OScriptNodeEmitMemberSignal::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("target_class"))
    {
        r_value = _target_class;
        return true;
    }
    else if (p_name.match("method"))
    {
        r_value = DictionaryUtils::from_method(_method);
        return true;
    }
    return false;
}

bool OScriptNodeEmitMemberSignal::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("target_class"))
    {
        if (_target_class != p_value)
        {
            _target_class = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    else if (p_name.match("method"))
    {
        _method = DictionaryUtils::to_method(p_value);
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeEmitMemberSignal::post_initialize()
{
    super::post_initialize();
}

void OScriptNodeEmitMemberSignal::post_placed_new_node()
{
    super::post_placed_new_node();
}

void OScriptNodeEmitMemberSignal::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "target", Variant::OBJECT)->set_flags(OScriptNodePin::Flags::DATA);

    // Godot signals do not support default values or varargs, no need to be concerned with those
    // They also do not support return values.
    for (const PropertyInfo& pi : _method.arguments)
    {
        Ref<OScriptNodePin> pin = create_pin(PD_Input, pi.name, pi.type);
        if (pin.is_valid())
        {
            BitField<OScriptNodePin::Flags> flags(OScriptNodePin::Flags::DATA | OScriptNodePin::NO_CAPITALIZE);
            if (pi.usage & PROPERTY_USAGE_CLASS_IS_ENUM)
            {
                flags.set_flag(OScriptNodePin::Flags::ENUM);
                pin->set_target_class(pi.class_name);
                pin->set_type(pi.type);
            }
            else if (pi.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD)
            {
                flags.set_flag(OScriptNodePin::Flags::BITFIELD);
                pin->set_target_class(pi.class_name);
                pin->set_type(pi.type);
            }
            pin->set_flags(flags);
        }
    }

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);

    super::allocate_default_pins();
}

String OScriptNodeEmitMemberSignal::get_tooltip_text() const
{
    return vformat("Emit the %s signal '%s'", _target_class, _method.name);
}

String OScriptNodeEmitMemberSignal::get_node_title() const
{
    return vformat("Emit %s", _method.name);
}

void OScriptNodeEmitMemberSignal::validate_node_during_build(BuildLog& p_log) const
{
    if (_target_class.is_empty())
    {
        p_log.error("No target class defined.");
        return;
    }

    if (_method.name.is_empty())
    {
        p_log.error("No method defined");
        return;
    }

    const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
    if (!target_pin.is_valid())
    {
        p_log.error("Failed to find target pin");
        return;
    }

    const Vector<Ref<OScriptNodePin>> connections = target_pin->get_connections();
    if (connections.is_empty())
    {
        // If the node isn't connected on its execution input pin; safe to ignore this.
        Ref<OScriptNodePin> exec_in = find_pin("ExecIn", PD_Input);
        if (!(exec_in.is_valid() && exec_in->get_connections().is_empty()))
        {
            if (!ClassDB::class_has_signal(get_orchestration()->get_base_type(), _method.name))
                p_log.error(vformat("No signal found in %s with name: %s", get_orchestration()->get_self()->get_path(), _method.name));
        }
    }
    else
    {
        const Ref<OScriptTargetObject> target = connections[0]->resolve_target();
        if (target.is_null())
            p_log.error("No target object resolved");
        else if (!target->has_signal(_method.name))
            p_log.error(vformat("No signal found on target with method name: %s", _method.name));
    }
}

OScriptNodeInstance* OScriptNodeEmitMemberSignal::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeEmitMemberSignalInstance* i = memnew(OScriptNodeEmitMemberSignalInstance);
    i->_node = this;
    i->_instance = p_instance;
    i->_method = _method;
    return i;
}

void OScriptNodeEmitMemberSignal::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize an EmitMemberSignal");
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to iniitialize an EmitMemberSignal, method info required.");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("target_class"), "Failed to initialize an EmitMemberSignal without target class.");

    _target_class = data["target_class"];
    _method = p_context.method.value();

    super::initialize(p_context);
}





