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

#include "common/property_utils.h"
#include "common/variant_utils.h"

#include <godot_cpp/classes/os.hpp>

class OScriptNodeEmitSignalInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeEmitSignal);

    MethodInfo _signal;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_signal.name.is_empty())
        {
            ERR_PRINT("Emit signal has no signal detail.");
            return 0;
        }

        std::vector<Variant> args;
        for (size_t i = 0; i < _signal.arguments.size(); i++)
            args.push_back(p_context.get_input(i));

        dispatch(p_context, _signal.name, args);

        return 0;
    }


    void dispatch(OScriptExecutionContext& p_context, const StringName& p_name, const std::vector<Variant>& p_args)
    {
        switch (p_args.size())
        {
            case 0:
                dispatch(p_context, p_name);
                break;
            case 1:
                dispatch(p_context, p_name, p_args[0]);
                break;
            case 2:
                dispatch(p_context, p_name, p_args[0], p_args[1]);
                break;
            case 3:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2]);
                break;
            case 4:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3]);
                break;
            case 5:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4]);
                break;
            case 6:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5]);
                break;
            case 7:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6]);
                break;
            case 8:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7]);
                break;
            case 9:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7], p_args[8]);
                break;
            case 10:
                dispatch(p_context, p_name, p_args[0], p_args[1], p_args[2], p_args[3], p_args[4], p_args[5], p_args[6], p_args[7], p_args[8], p_args[9]);
                break;
            default:
                ERR_PRINT("Too many signal arguments, no signal dispatched");
        }
    }

    template<typename... Args>
    void dispatch(OScriptExecutionContext& p_context, const StringName& p_name, const Args&... p_args)
    {
        p_context.get_owner()->emit_signal(p_name, p_args...);
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
        for (size_t i = 1; i <= mi.arguments.size(); i++)
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
        r_value = _signal.is_valid() ? static_cast<int64_t>(_signal->get_argument_count()) : 0;
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
            if (_signal->resize_argument_list(static_cast<int64_t>(p_value)))
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

void OScriptNodeEmitSignal::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - makes sure that full property attributes are encoded in the pins
        if (_signal.is_valid())
        {
            const MethodInfo& mi = _signal->get_method_info();
            for (const PropertyInfo& pi : mi.arguments)
            {
                const Ref<OScriptNodePin> pin = find_pin(pi.name, PD_Input);
                if (!pin.is_valid())
                {
                    reconstruct_node();
                    break;
                }

                const PropertyInfo pin_pi = pin->get_property_info();
                if (!PropertyUtils::are_equal(pi, pin_pi))
                {
                    reconstruct_node();
                    break;
                }

                if (!pin->use_pretty_labels())
                {
                    reconstruct_node();
                    break;
                }
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeEmitSignal::_on_signal_changed()
{
    _signal_name = _signal->get_signal_name();
    reconstruct_node();
}

void OScriptNodeEmitSignal::post_initialize()
{
    _signal = get_orchestration()->find_custom_signal(_signal_name);
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
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    // Create output exec pin
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    if (_signal.is_valid())
    {
        const MethodInfo& mi = _signal->get_method_info();
        for (const PropertyInfo& pi : mi.arguments)
            create_pin(PD_Input, PT_Data, pi);
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

bool OScriptNodeEmitSignal::can_inspect_node_properties() const
{
    return _signal.is_valid() && !_signal->get_signal_name().is_empty();
}

OScriptNodeInstance* OScriptNodeEmitSignal::instantiate()
{
    OScriptNodeEmitSignalInstance* i = memnew(OScriptNodeEmitSignalInstance);
    i->_node = this;

    Ref<OScriptSignal> signal = get_orchestration()->get_custom_signal(_signal_name);
    if (signal.is_valid())
        i->_signal = signal->get_method_info();

    return i;
}

void OScriptNodeEmitSignal::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to initialize an EmitSignal without a MethodInfo");

    const MethodInfo& mi = p_context.method.value();
    _signal_name = mi.name;
    _signal = get_orchestration()->get_custom_signal(_signal_name);

    super::initialize(p_context);
}

void OScriptNodeEmitSignal::validate_node_during_build(BuildLog& p_log) const
{
    if (!_signal.is_valid())
        p_log.error(this, "No signal is defined.");

    super::validate_node_during_build(p_log);
}