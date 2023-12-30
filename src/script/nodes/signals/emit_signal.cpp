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
#include "emit_signal.h"

#include "common/variant_utils.h"

#include <godot_cpp/classes/os.hpp>

class OScriptNodeEmitSignalInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeEmitSignal);

    MethodInfo _signal;

public:
    int step(OScriptNodeExecutionContext& p_context) override
    {
        if (_signal.name.is_empty())
        {
            ERR_PRINT("Emit signal has no signal detail.");
            return 0;
        }

        std::vector<Variant> args;
        for (int i = 0; i < _signal.arguments.size(); i++)
            args.push_back(p_context.get_input(i));

        dispatch(_signal.name, args);

        return 0;
    }


    void dispatch(const StringName& p_name, const std::vector<Variant>& p_args)
    {
        switch (p_args.size())
        {
            case 0:
                dispatch(p_name);
                break;
            case 1:
                dispatch(p_name, p_args[0]);
                break;
            case 2:
                dispatch(p_name, p_args[0], p_args[1]);
                break;
            case 3:
                dispatch(p_name, p_args[0], p_args[1], p_args[2]);
                break;
            case 4:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3]);
                break;
            case 5:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4]);
                break;
            case 6:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5]);
                break;
            case 7:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6]);
                break;
            case 8:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7]);
                break;
            case 9:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7], p_args[8]);
                break;
            case 10:
                dispatch(p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7], p_args[8], p_args[9]);
                break;
            default:
                ERR_PRINT("Too many signal arguments, no signal dispatched");
        }
    }

    template<typename... Args>
    void dispatch(const StringName& p_name, const Args&... p_args)
    {
        _instance->get_owner()->emit_signal(p_name, p_args...);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeEmitSignal::_get_property_list(List<PropertyInfo>* r_list) const
{
    int32_t read_only_serialize = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY;

    r_list->push_back(PropertyInfo(Variant::STRING, "signal_name", PROPERTY_HINT_NONE, "", read_only_serialize));

    if (_signal.is_valid())
    {
        int32_t usage = PROPERTY_USAGE_EDITOR;
        r_list->push_back(PropertyInfo(Variant::INT, "argument_count", PROPERTY_HINT_RANGE, "0,32", usage));

        static String types = VariantUtils::to_enum_list();
        const MethodInfo& mi = _signal->get_method_info();
        for (int i = 1; i <= mi.arguments.size(); i++)
        {
            r_list->push_back(PropertyInfo(Variant::INT, "argument_" + itos(i) + "/type", PROPERTY_HINT_ENUM, types, usage));
            r_list->push_back(PropertyInfo(Variant::STRING, "argument_" + itos(i) + "/name", PROPERTY_HINT_NONE, "", usage));
        }
    }

}

bool OScriptNodeEmitSignal::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("signal_name"))
    {
        r_value = _signal_name;
        return true;
    }
    else if (p_name.match("argument_count"))
    {
        r_value = _signal.is_valid() ? _signal->get_argument_count() : 0;
        return true;
    }
    else if (p_name.begins_with("argument_"))
    {
        if (_signal.is_valid())
        {
            const MethodInfo &mi = _signal->get_method_info();

            const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
            ERR_FAIL_INDEX_V(index, mi.arguments.size(), false);

            const String what = p_name.get_slicec('/', 1);
            if (what == "type")
            {
                r_value = mi.arguments[index].type;
                return true;
            }
            else if (what == "name")
            {
                r_value = mi.arguments[index].name;
                return true;
            }
        }
    }
    return false;
}

bool OScriptNodeEmitSignal::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("signal_name"))
    {
        if (_signal_name != p_value)
        {
            _signal_name = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    else if (p_name.match("argument_count"))
    {
        if (_signal.is_valid())
        {
            if (_signal->resize_argument_list(p_value))
                notify_property_list_changed();
            return true;
        }
    }
    else if (p_name.begins_with("argument_"))
    {
        if (_signal.is_valid())
        {
            const MethodInfo &mi = _signal->get_method_info();

            const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
            ERR_FAIL_INDEX_V(index, mi.arguments.size(), false);

            const String what = p_name.get_slicec('/', 1);
            if (what == "type")
            {
                _signal->set_argument_type(index, VariantUtils::to_type(p_value));
                return true;
            }
            else if (what == "name")
            {
                _signal->set_argument_name(index, p_value);
                return true;
            }
        }
    }
    return false;
}

void OScriptNodeEmitSignal::_on_signal_changed()
{
    _signal_name = _signal->get_signal_name();
    reconstruct_node();
}

void OScriptNodeEmitSignal::post_initialize()
{
    _signal = get_owning_script()->find_custom_signal(_signal_name);
    if (_signal.is_valid() && _is_in_editor())
        _signal->connect("changed", callable_mp(this, &OScriptNodeEmitSignal::_on_signal_changed));

    super::post_initialize();
}

void OScriptNodeEmitSignal::post_placed_new_node()
{
    super::post_placed_new_node();

    if (_signal.is_valid() && _is_in_editor())
        _signal->connect("changed", callable_mp(this, &OScriptNodeEmitSignal::_on_signal_changed));
}

void OScriptNodeEmitSignal::allocate_default_pins()
{
    // Single input exec pin
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);

    // Create output exec pin
    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);

    if (_signal.is_valid())
    {
        const MethodInfo& mi = _signal->get_method_info();
        for (const PropertyInfo& pi : mi.arguments)
        {
            Ref<OScriptNodePin> arg = create_pin(PD_Input, pi.name, pi.type);
            arg->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);
        }
    }

    super::allocate_default_pins();
}

String OScriptNodeEmitSignal::get_tooltip_text() const
{
    if (_signal.is_valid())
        return vformat("Emit the signal '%s'", _signal->get_signal_name());
    else
        return "Emits a Godot signal with optional arguments";
}

String OScriptNodeEmitSignal::get_node_title() const
{
    if (_signal.is_valid())
        return vformat("Emit %s", _signal->get_signal_name());

    return super::get_node_title();
}

bool OScriptNodeEmitSignal::validate_node_during_build() const
{
    if (!super::validate_node_during_build())
        return false;

    if (!_signal.is_valid())
    {
        ERR_PRINT("There is no signal defined for the signal emit node.");
        return false;
    }
    return true;
}

bool OScriptNodeEmitSignal::can_inspect_node_properties() const
{
    return _signal.is_valid() & !_signal->get_signal_name().is_empty();
}

OScriptNodeInstance* OScriptNodeEmitSignal::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeEmitSignalInstance* i = memnew(OScriptNodeEmitSignalInstance);
    i->_node = this;
    i->_instance = p_instance;

    Ref<OScriptSignal> signal = get_owning_script()->get_custom_signal(_signal_name);
    if (signal.is_valid())
        i->_signal = signal->get_method_info();

    return i;
}

void OScriptNodeEmitSignal::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to initialize an EmitSignal without a MethodInfo");

    const MethodInfo& mi = p_context.method.value();
    _signal_name = mi.name;
    _signal = get_owning_script()->get_custom_signal(_signal_name);

    super::initialize(p_context);
}
