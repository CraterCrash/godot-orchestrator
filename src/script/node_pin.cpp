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
#include "script/node_pin.h"

#include "common/settings.h"
#include "common/variant_utils.h"
#include "script/node.h"
#include "script/nodes/data/coercion_node.h"

void OScriptNodePin::_bind_methods()
{
    BIND_ENUM_CONSTANT(EPinDirection::PD_Input)
    BIND_ENUM_CONSTANT(EPinDirection::PD_Output)
    BIND_ENUM_CONSTANT(EPinDirection::PD_MAX)

    BIND_ENUM_CONSTANT(Flags::NONE)
    BIND_ENUM_CONSTANT(Flags::DATA)
    BIND_ENUM_CONSTANT(Flags::EXECUTION)
    BIND_ENUM_CONSTANT(Flags::IGNORE_DEFAULT)
    BIND_ENUM_CONSTANT(Flags::READ_ONLY)
    BIND_ENUM_CONSTANT(Flags::HIDDEN)
    BIND_ENUM_CONSTANT(Flags::ORPHANED)
    BIND_ENUM_CONSTANT(Flags::ADVANCED)
    BIND_ENUM_CONSTANT(Flags::NO_CONNECTION)
    BIND_ENUM_CONSTANT(Flags::SHOW_LABEL)
    BIND_ENUM_CONSTANT(Flags::HIDE_LABEL)
    BIND_ENUM_CONSTANT(Flags::CONST)
    BIND_ENUM_CONSTANT(Flags::REFERENCE)
    BIND_ENUM_CONSTANT(Flags::OBJECT)
    BIND_ENUM_CONSTANT(Flags::FILE)
    BIND_ENUM_CONSTANT(Flags::MULTILINE)
    BIND_ENUM_CONSTANT(Flags::ENUM)
}

bool OScriptNodePin::_load(const Dictionary& p_data)
{
    // These are required fields for a pin
    if (!p_data.has("pin_name") || !p_data.has("type") || !p_data.has("dir"))
        return false;

    _pin_name = p_data["pin_name"];
    _type = VariantUtils::to_type(p_data["type"]);
    _direction = EPinDirection(int(p_data["dir"]));

    if (p_data.has("flags"))
        _flags = int(p_data["flags"]);

    if (p_data.has("label"))
        _label = p_data["label"];

    if (p_data.has("target_class"))
        _target_class = p_data["target_class"];

    if (p_data.has("dv"))
        _default_value = p_data["dv"];

    if (p_data.has("gdv"))
        _generated_default_value = p_data["gdv"];
    else
        _generated_default_value = VariantUtils::make_default(_type);

    return true;
}

Dictionary OScriptNodePin::_save()
{
    Dictionary data;
    data["pin_name"] = _pin_name;
    data["type"] = _type;
    data["dir"] = _direction;

    if (_flags > 0)
        data["flags"] = _flags;

    if (!_label.is_empty())
        data["label"] = _label;

    if (!_target_class.is_empty())
        data["target_class"] = _target_class;

    if (_default_value.get_type() != Variant::NIL)
        data["dv"] = _default_value;

    // This fixes any potential data issues and guarantees that a generated default value exists.
    if (_generated_default_value.get_type() == Variant::NIL)
        _generated_default_value = VariantUtils::make_default(_type);

    data["gdv"] = _generated_default_value;

    return data;
}

Vector2 OScriptNodePin::_calculate_midpoint_between_nodes(const Ref<OScriptNode>& p_source,
                                                          const Ref<OScriptNode>& p_target) const
{
    const Rect2 source_rect(p_source->get_position(), p_source->get_size());
    const Rect2 target_rect(p_target->get_position(), p_target->get_size());

    const Vector2 source_position = source_rect.get_center();
    const Vector2 target_position = target_rect.get_center();

    const float mid_x = (source_position.x + target_position.x) / 2.f;
    const float mid_y = (source_position.y + target_position.y) / 2.f;

    // This is the average size of a connected Intermediate Coercion node.
    const Vector2 avg_size(110, 60);
    const Vector2 mid_point = Vector2(mid_x, mid_y) - (avg_size / 2);

    Orchestration* orchestration = get_owning_node()->get_orchestration();
    if (orchestration)
    {
        Ref<OScriptGraph> graph = orchestration->find_graph(p_source);
        if (graph.is_valid())
            return mid_point / graph->get_viewport_zoom();
    }

    // Use fallback without zoom calculated
    return mid_point;
}

Ref<OScriptNodePin> OScriptNodePin::create(OScriptNode* p_owning_node)
{
    Ref<OScriptNodePin> pin(memnew(OScriptNodePin));
    pin->_owning_node = p_owning_node;
    pin->_type = Variant::NIL;
    return pin;
}

void OScriptNodePin::post_initialize()
{
    _set_type_resets_default = true;
}

OScriptNode* OScriptNodePin::get_owning_node() const
{
    return _owning_node;
}

void OScriptNodePin::set_owning_node(OScriptNode* p_owning_node)
{
    if (_owning_node != p_owning_node)
    {
        _owning_node = p_owning_node;
        emit_changed();
    }
}

int32_t OScriptNodePin::get_pin_index() const
{
    #ifdef _DEBUG
    CRASH_COND_MSG(_cached_pin_index == -1, "OScriptNodePin index is not cached.");
    #else
    ERR_FAIL_COND_V_MSG(_cached_pin_index == -1, -1,
                        vformat("OScriptNodePin index not yet cached in Node %s with ID %d, possible bug?!",
                                _owning_node->get_class(), _owning_node->get_id()));
    #endif
    return _cached_pin_index;
}

StringName OScriptNodePin::get_pin_name() const
{
    return _pin_name;
}

void OScriptNodePin::set_pin_name(const StringName& p_pin_name)
{
    if (!_pin_name.match(p_pin_name))
    {
        _pin_name = p_pin_name;
        emit_changed();
    }
}

Variant::Type OScriptNodePin::get_type() const
{
    return _type;
}

void OScriptNodePin::set_type(Variant::Type p_type)
{
    if (_type != p_type)
    {
        _type = p_type;

        if (_set_type_resets_default)
            reset_default_value();

        emit_changed();
    }
}

String OScriptNodePin::get_pin_type_name() const
{
    if (_type == Variant::NIL)
        return "Variant";

    return Variant::get_type_name(_type);
}

StringName OScriptNodePin::get_target_class() const
{
    return _target_class;
}

void OScriptNodePin::set_target_class(const StringName& p_target_class)
{
    if (_target_class != p_target_class)
    {
        _target_class = p_target_class;

        if (!_target_class.is_empty())
            _type = Variant::OBJECT;

        if (_set_type_resets_default)
            reset_default_value();

        emit_changed();
    }
}

Variant OScriptNodePin::get_default_value() const
{
    return _default_value;
}

void OScriptNodePin::set_default_value(const Variant& p_default_value)
{
    if (_default_value != p_default_value)
    {
        _default_value = p_default_value;

        // Notify node the pin value changed
        Ref<OScriptNode> node = get_owning_node();
        if (node.is_valid())
        {
            set_block_signals(true);
            node->pin_default_value_changed(Ref<OScriptNodePin>(this));
            set_block_signals(true);
        }
        emit_changed();
    }
}

void OScriptNodePin::reset_default_value()
{
    _default_value = Variant();
    _generated_default_value = _target_class.is_empty() ? VariantUtils::make_default(_type) : Variant();
}

Variant OScriptNodePin::get_generated_default_value() const
{
    return _generated_default_value;
}

void OScriptNodePin::set_generated_default_value(const Variant& p_default_value)
{
    if (_generated_default_value != p_default_value)
    {
        _generated_default_value = p_default_value;
        emit_changed();
    }
}

Variant OScriptNodePin::get_effective_default_value() const
{
    Variant value = get_default_value();
    if (value.get_type() == Variant::NIL)
        value = get_generated_default_value();
    return value;
}

EPinDirection OScriptNodePin::get_direction() const
{
    return _direction;
}

void OScriptNodePin::set_direction(EPinDirection p_direction)
{
    if (_direction != p_direction)
    {
        _direction = p_direction;
        emit_changed();
    }
}

EPinDirection OScriptNodePin::get_complimentary_direction() const
{
    return _direction == PD_Input ? PD_Output : PD_Input;
}

BitField<OScriptNodePin::Flags> OScriptNodePin::get_flags() const
{
    return _flags;
}

void OScriptNodePin::set_flags(BitField<OScriptNodePin::Flags> p_flags)
{
    if (_flags != p_flags)
    {
        _flags = p_flags;
        emit_changed();
    }
}

String OScriptNodePin::get_label() const
{
    return _label;
}

void OScriptNodePin::set_label(const String& p_label)
{
    if (_label != p_label)
    {
        _label = p_label;
        emit_changed();
    }
}

bool OScriptNodePin::can_accept(const Ref<OScriptNodePin>& p_pin) const
{
    // Cannot match inputs to inputs and outputs to outputs
    if (get_direction() == p_pin->get_direction())
        return false;

    // Short-circuit execution ports, if both are executions, allow
    if (is_execution() && p_pin->is_execution())
        return true;

    // Cannot mix data and execution ports
    if ((!is_execution() && p_pin->is_execution()) || (is_execution() && !p_pin->is_execution()))
        return false;

    // If the types match, short-circuit
    if (_type == p_pin->get_type())
        return true;

    // Coercion is allowed here
    if (_type == Variant::STRING || p_pin->get_type() == Variant::STRING)
        return true;

    // Numeric conversions allows
    if (_type == Variant::INT || _type == Variant::FLOAT)
        if (p_pin->get_type() == Variant::INT || p_pin->get_type() == Variant::FLOAT)
            return true;

    if (_type == Variant::NIL || p_pin->get_type() == Variant::NIL)
        return true;

    return false;
}

void OScriptNodePin::link(const Ref<OScriptNodePin>& p_pin)
{
    ERR_FAIL_COND_MSG(p_pin.is_null(), "Connection link failed, target pin is not valid.");

    Orchestration* orchestration = get_owning_node()->get_orchestration();

    OScriptNode* source = nullptr;
    OScriptNode* target = nullptr;
    Ref<OScriptNodePin> source_pin;
    Ref<OScriptNodePin> target_pin;

    if (get_direction() == PD_Input) // Treat the incoming node as an output (target) pin
    {
        source = p_pin->get_owning_node();
        source_pin = p_pin;

        target = get_owning_node();
        target_pin = Ref<OScriptNodePin>(this);
    }
    else // Treat the incoming node as an input (source) pin
    {
        source = get_owning_node();
        source_pin = Ref<OScriptNodePin>(this);

        target = p_pin->get_owning_node();
        target_pin = p_pin;
    }

    // Data input pins can only have a single incoming connection
    if (!target_pin->is_execution())
        target_pin->unlink_all();

    // An execution output pin can only have a single outgoing connection
    if (source_pin->is_execution())
        source_pin->unlink_all();

    bool intermediate_required = false;
    if (!source_pin->is_execution() && !target_pin->is_execution())
        intermediate_required = get_type() != target_pin->get_type();

    Ref<OScriptNode> intermediate;

    bool use_coercion_nodes = OrchestratorSettings::get_singleton()->get_setting("ui/nodes/show_conversion_nodes");
    if (intermediate_required && use_coercion_nodes)
    {
        Ref<OScriptGraph> owning_graph = orchestration->find_graph(get_owning_node());
        ERR_FAIL_COND_MSG(!owning_graph.is_valid(), "Failed to locate owning graph, connection link failed.");

        const Vector2 position = _calculate_midpoint_between_nodes(source, target);

        Dictionary data;
        data["left_type"] = source_pin->get_type();
        data["right_type"] = target_pin->get_type();

        OScriptNodeInitContext context;
        context.user_data = data;

        intermediate = owning_graph->create_node<OScriptNodeCoercion>(context, position);

        owning_graph->link(source->get_id(), source_pin->get_pin_index(), intermediate->get_id(), 0);
        owning_graph->link(intermediate->get_id(), 0, target->get_id(), target_pin->get_pin_index());

        source->on_pin_connected(source_pin);
        intermediate->on_pin_connected(intermediate->find_pin(0, PD_Input));
        intermediate->on_pin_connected(intermediate->find_pin(0, PD_Output));
        target->on_pin_connected(target_pin);
    }
    else
    {
        Ref<OScriptGraph> owning_graph = orchestration->find_graph(get_owning_node());
        ERR_FAIL_COND_MSG(!owning_graph.is_valid(), "Failed to locate owning graph, connection link failed.");

        owning_graph->link(source->get_id(), source_pin->get_pin_index(), target->get_id(), target_pin->get_pin_index());

        source->on_pin_connected(source_pin);
        target->on_pin_connected(target_pin);
    }

    source->emit_changed();

    if (intermediate.is_valid())
        intermediate->emit_changed();

    target->emit_changed();
}

void OScriptNodePin::unlink(const Ref<OScriptNodePin>& p_pin)
{
    ERR_FAIL_COND_MSG(!p_pin.is_valid(), "Connection unlink failed, pin is not valid.");

    // todo: tried delegating to node graph; however, the find_graph method failed to return a graph instance
    Orchestration* orchestration = get_owning_node()->get_orchestration();

    if (get_direction() == PD_Input)  // Treat the incoming node as an output (target) pin
    {
        OScriptNode* source = p_pin->get_owning_node();
        OScriptNode* target = get_owning_node();

        orchestration->disconnect_nodes(source->get_id(), p_pin->get_pin_index(), target->get_id(), get_pin_index());

        source->on_pin_disconnected(p_pin);
        target->on_pin_disconnected(this);
    }
    else  // Treat the incoming node as an input (source) pin
    {
        OScriptNode* source = get_owning_node();
        OScriptNode* target = p_pin->get_owning_node();

        orchestration->disconnect_nodes(source->get_id(), get_pin_index(), target->get_id(), p_pin->get_pin_index());

        source->on_pin_disconnected(this);
        target->on_pin_disconnected(p_pin);
    }

    emit_changed();
    p_pin->emit_changed();
}

void OScriptNodePin::unlink_all(bool p_notify_nodes)
{
    Orchestration* orchestration = get_owning_node()->get_orchestration();

    Vector<Ref<OScriptNode>> affected_nodes;
    Vector<Ref<OScriptNodePin>> connections = orchestration->get_connections(this);
    for (const Ref<OScriptNodePin>& pin : connections)
    {
        OScriptNode* pin_node = pin->get_owning_node();
        unlink(pin);
        if (p_notify_nodes && !affected_nodes.has(pin_node))
            affected_nodes.push_back(pin_node);
    }

    // LEAVE THIS DEFERRED
    // This addresses a race concern when deleting Coercion nodes and it also
    // deals with misalignment of connections when nodes are deleted but are
    // still connected to nodes.
    for (const Ref<OScriptNode>& node : affected_nodes)
        node->call_deferred("emit_changed");
}

bool OScriptNodePin::has_any_connections() const
{
    return !get_connections().is_empty();
}

Vector<Ref<OScriptNodePin>> OScriptNodePin::get_connections() const
{
    return get_owning_node()->get_orchestration()->get_connections(this);
}

Ref<OScriptTargetObject> OScriptNodePin::resolve_target()
{
    return get_owning_node()->resolve_target(this);
}