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
#include "script/node.h"

#include "common/variant_utils.h"
#include "common/version.h"
#include "script/script.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>

OScriptNode::OScriptNode()
    : _orchestration(nullptr)
    , _initialized(false)
    , _id(-1)
    , _flags(ScriptNodeFlags::CATALOGABLE)
    #if GODOT_VERSION >= 0x040300
    , _breakpoint_flag(BreakpointFlags::BREAKPOINT_NONE)
    #endif
{
}

void OScriptNode::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_id", "id"), &OScriptNode::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &OScriptNode::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_id", "get_id");

    ClassDB::bind_method(D_METHOD("set_size", "size"), &OScriptNode::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &OScriptNode::get_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_size",
                 "get_size");

    ClassDB::bind_method(D_METHOD("set_position", "position"), &OScriptNode::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &OScriptNode::get_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("_set_pin_data", "pin_data"), &OScriptNode::_set_pin_data);
    ClassDB::bind_method(D_METHOD("_get_pin_data"), &OScriptNode::_get_pin_data);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pin_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_pin_data", "_get_pin_data");

    ADD_SIGNAL(MethodInfo("pin_connected", PropertyInfo(Variant::INT, "pin_type"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(
        MethodInfo("pin_disconnected", PropertyInfo(Variant::INT, "pin_type"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("pins_changed"));
}

TypedArray<Dictionary> OScriptNode::_get_pin_data() const
{
    TypedArray<Dictionary> pins;
    for (const Ref<OScriptNodePin>& pin : _pins)
        pins.push_back(pin->_save());
    return pins;
}

void OScriptNode::_set_pin_data(const TypedArray<Dictionary>& p_pin_data)
{
    for (int i = 0; i < p_pin_data.size(); i++)
    {
        Ref<OScriptNodePin> pin;
        pin.instantiate();
        pin->set_owning_node(this);
        pin->_load(p_pin_data[i]);
        _pins.push_back(pin);
    }
}

bool OScriptNode::_is_in_editor()
{
    return OS::get_singleton()->has_feature("editor");
}

void OScriptNode::_queue_reconstruct()
{
    if (_reconstruction_queued)
        return;

    callable_mp(this, &OScriptNode::reconstruct_node).call_deferred();
}

Ref<OScriptGraph> OScriptNode::get_owning_graph()
{
    return _orchestration->find_graph(this);
}

void OScriptNode::set_id(int p_id)
{
    _id = p_id;
}

void OScriptNode::set_size(const Vector2& p_size)
{
    _size = p_size;
}

void OScriptNode::set_position(const Vector2& p_position)
{
    _position = p_position;
}

#if GODOT_VERSION >= 0x040300
void OScriptNode::set_breakpoint_flag(BreakpointFlags p_flag)
{
    if (_breakpoint_flag != p_flag)
    {
        _breakpoint_flag = p_flag;
        emit_changed();
    }
}
#endif

void OScriptNode::set_flags(BitField<ScriptNodeFlags> p_flags)
{
    _flags = p_flags;
    emit_changed();
}

void OScriptNode::get_actions(List<Ref<OScriptAction>>& p_action_list)
{
}

void OScriptNode::pre_save()
{
}

void OScriptNode::pre_remove()
{
    // During node removal, there is no need for pin reconstruction to fire and there may
    // be situations, such as in AssignLocalVariable, that could trigger reconstruction
    // when pins are unlinked. By preemptively setting reconstructing to true, this will
    // block pin reconstruction when nodes are being removed.
    _reconstructing = true;
}

void OScriptNode::post_initialize()
{
    for (const Ref<OScriptNodePin>& pin : _pins)
        pin->post_initialize();

    _cache_pin_indices();
    _initialized = true;
}

void OScriptNode::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    allocate_default_pins();
    _cache_pin_indices();
}

void OScriptNode::reconstruct_node()
{
    if (_reconstructing)
        return;

    // Set reconstruction flag
    _reconstructing = true;

    Vector<Ref<OScriptNodePin>> old_pins = _pins;
    _pins.clear();

    reallocate_pins_during_reconstruction(old_pins);
    rewire_old_pins_to_new_pins(old_pins, _pins);

    post_reconstruct_node();

    emit_changed();

    // Clear reconstruction flag
    _reconstructing = false;
    _reconstruction_queued = false;
}

void OScriptNode::post_placed_new_node()
{
    _cache_pin_indices();
}

void OScriptNode::rewire_old_pins_to_new_pins(const Vector<Ref<OScriptNodePin>>& p_old_pins,
                                              const Vector<Ref<OScriptNodePin>>&  p_new_pins)
{
    for (const Ref<OScriptNodePin>& old : p_old_pins)
    {
        if (old->is_input())
        {
            Ref<OScriptNodePin> newPin = find_pin(old->get_pin_name(), old->get_direction());
            if (newPin.is_valid())
            {
                // If new pin has a default value set that isn't the default, skip.
                if (newPin->get_default_value().get_type() != Variant::NIL
                        && (newPin->get_default_value() != newPin->get_generated_default_value()))
                    continue;

                // If the two pins have different generated default values, skip.
                if (newPin->get_generated_default_value() != old->get_generated_default_value())
                    continue;

                // If the two pins have different types, skip.
                if (newPin->get_type() != old->get_type())
                    continue;

                // If the old pin's default value equals the generated default value, skip.
                if (old->get_default_value() == old->get_generated_default_value())
                    continue;

                newPin->set_default_value(old->get_default_value());
            }
        }
    }
}

void OScriptNode::validate_node_during_build(BuildLog& p_log) const
{
    for (const Ref<OScriptNodePin>& pin : _pins)
    {
        if (pin->is_output() && pin->has_any_connections())
        {
            for (const Ref<OScriptNodePin>& connection : pin->get_connections())
            {
                if (!connection->can_accept(pin))
                {
                    p_log.error(
                        this,
                        pin,
                        vformat("Is not compatible with one of its connected input pins.\n\tTo fix, re-add the target node to the graph to fix the metadata."));
                }
            }
        }

        if (!pin->is_valid())
        {
            p_log.error(
                this,
                pin,
                "Not valid and could not be upgraded.\n\tPlease re-create the node to fix the metadata.");
        }
    }
}

OScriptNodeInstance* OScriptNode::instantiate()
{
    ERR_PRINT("A custom script node implementation did not override instantiate");
    return nullptr;
}

void OScriptNode::initialize(const OScriptNodeInitContext& p_context)
{
    _initialized = true;
    allocate_default_pins();
}

String OScriptNode::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class:%s", get_class());
    #else
    return get_class();
    #endif
}

Ref<OScriptNodePin> OScriptNode::create_pin(EPinDirection p_direction, EPinType p_pin_type, const PropertyInfo& p_property, const Variant& p_default_value)
{
    Ref<OScriptNodePin> pin = OScriptNodePin::create(this, p_property);
    if (pin.is_valid())
    {
        if (p_pin_type == PT_Execution)
            pin->set_flag(OScriptNodePin::Flags::EXECUTION);
        else
            pin->set_flag(OScriptNodePin::Flags::DATA);

        pin->set_direction(p_direction);
        pin->set_default_value(p_default_value);

        Variant::Type type = p_default_value.get_type() != Variant::NIL ? p_default_value.get_type() : p_property.type;
        pin->set_generated_default_value(VariantUtils::make_default(type));

        _pins.push_back(pin);
    }
    return pin;
}

Ref<OScriptNodePin> OScriptNode::find_pin(const String& p_pin_name, EPinDirection p_direction) const
{
    for (const Ref<OScriptNodePin>& pin : _pins)
        if ((p_direction == PD_MAX || pin->get_direction() == p_direction) && pin->get_pin_name().match(p_pin_name))
            return pin;
    return {};
}

Ref<OScriptNodePin> OScriptNode::find_pin(int p_index, EPinDirection p_direction) const
{
    int current_index = 0;
    for (const Ref<OScriptNodePin>& pin : _pins)
    {
        if (p_direction == pin->get_direction())
        {
            if (current_index == p_index)
                return pin;

            current_index++;
        }
    }

    return {};
}

bool OScriptNode::remove_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (_pins.has(p_pin))
    {
        _pins.erase(p_pin);
        return true;
    }
    return false;
}

bool OScriptNode::has_any_connections() const
{
    for (const Ref<OScriptNodePin>& pin : _pins)
        if (pin->has_any_connections())
            return true;
    return false;
}

Vector<Ref<OScriptNodePin>> OScriptNode::get_eligible_autowire_pins(const Ref<OScriptNodePin>& p_pin) const
{
    Vector<Ref<OScriptNodePin>> eligible_pins;
    for (const Ref<OScriptNodePin>& pin : get_all_pins())
    {
        // Invalid or hidden pins are skipped
        if (!pin.is_valid() || pin->is_hidden())
            continue;

        // Skip pins that are specifically flagged as non-autowirable
        if (!pin->can_autowire())
            continue;

        // Cannot connect input to input or output to output
        if (p_pin->get_direction() == pin->get_direction())
            continue;

        // Match execution or data state
        if ((p_pin->is_execution() && !pin->is_execution()) || (!p_pin->is_execution() && pin->is_execution()))
            continue;

        if (!p_pin->is_execution() && !pin->is_execution())
        {
            // Data flow pins must match types
            if (pin->get_type() != p_pin->get_type())
                continue;
        }

        eligible_pins.push_back(pin);
    }
    return eligible_pins;
}

void OScriptNode::on_pin_connected(const Ref<OScriptNodePin>& p_pin)
{
    emit_signal("pin_connected", p_pin->get_direction(), p_pin->get_pin_index());
}

void OScriptNode::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin)
{
    emit_signal("pin_disconnected", p_pin->get_direction(), p_pin->get_pin_index());
}

Vector<Ref<OScriptNodePin>> OScriptNode::find_pins(EPinDirection p_direction)
{
    if (p_direction == PD_MAX)
        return _pins;

    Vector<Ref<OScriptNodePin>> pins;
    for (const auto& _pin : _pins)
        if (_pin->get_direction() == p_direction)
            pins.push_back(_pin);
    return pins;
}

void OScriptNode::_validate_input_default_values()
{
}

void OScriptNode::_notify_pins_changed()
{
    if (_initialized)
    {
        reconstruct_node();
        emit_signal("pins_changed");
    }
}

void OScriptNode::_cache_pin_indices()
{
    // Iterate loaded pins and cache indices
    int input_index = 0;
    int output_index = 0;
    for (const Ref<OScriptNodePin>& pin : _pins)
    {
        if (pin->is_hidden())
            continue;

        if (pin->is_input())
            pin->_cached_pin_index = input_index++;
        else
            pin->_cached_pin_index = output_index++;
    }
}