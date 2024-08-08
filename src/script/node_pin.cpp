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
#include "script/node_pin.h"

#include "common/property_utils.h"
#include "common/settings.h"
#include "common/variant_utils.h"
#include "nodes/functions/event.h"
#include "script/node.h"
#include "script/nodes/data/coercion_node.h"
#include "script_server.h"

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

Ref<OScriptNodePin> OScriptNodePin::create(OScriptNode* p_owning_node, const PropertyInfo& p_property)
{
    Ref<OScriptNodePin> pin(memnew(OScriptNodePin));
    pin->_owning_node = p_owning_node;
    pin->_property = p_property;

    #if GODOT_VERSION < 0x040300
    if (pin->_property.usage == 7)
        pin->_property.usage = PROPERTY_USAGE_DEFAULT;
    #endif

    if (PropertyUtils::is_enum(p_property))
    {
        pin->_flags.set_flag(ENUM);
        if (p_property.usage & PROPERTY_USAGE_CLASS_IS_ENUM)
            pin->_target_class = p_property.class_name;
    }
    else if (PropertyUtils::is_bitfield(p_property))
    {
        pin->_flags.set_flag(BITFIELD);
        if (p_property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD)
            pin->_target_class = p_property.class_name;
    }

    if (p_property.hint == PROPERTY_HINT_FILE)
        pin->_flags.set_flag(FILE);
    else if (p_property.hint == PROPERTY_HINT_MULTILINE_TEXT)
        pin->_flags.set_flag(MULTILINE);

    if (pin->_target_class.is_empty() && !p_property.class_name.is_empty())
        if (p_property.hint == PROPERTY_HINT_RESOURCE_TYPE || p_property.type == Variant::OBJECT)
            pin->_target_class = p_property.class_name;

    // This will trigger an error during the validation/build, asking user to create the node
    if (pin->_target_class != p_property.class_name)
        pin->_valid = false;

    return pin;
}

void OScriptNodePin::_clear_flag(Flags p_flag)
{
    if (_flags.has_flag(p_flag))
    {
        _flags = _flags & ~p_flag;
        emit_changed();
    }
}

bool OScriptNodePin::_load(const Dictionary& p_data)
{
    // These are required fields for a pin
    if (!p_data.has("pin_name"))
        return false;

    _property.name = p_data["pin_name"];

    if (p_data.has("type"))
        _property.type = VariantUtils::to_type(p_data["type"]);

    if (p_data.has("dir"))
        _direction = EPinDirection(int(p_data["dir"]));

    if (p_data.has("flags"))
        _flags = int(p_data["flags"]);

    if (p_data.has("label"))
        _label = p_data["label"];

    if (p_data.has("target_class"))
    {
        _target_class = p_data["target_class"];
        _property.class_name = _target_class;
    }

    if (p_data.has("dv"))
        _default_value = p_data["dv"];

    if (p_data.has("gdv"))
        _generated_default_value = p_data["gdv"];
    else
        _generated_default_value = VariantUtils::make_default(_property.type);

    if (p_data.has("hint"))
        _property.hint = p_data["hint"];

    if (p_data.has("hint_string"))
        _property.hint_string = p_data["hint_string"];

    if (p_data.has("usage"))
        _property.usage = p_data["usage"];

    #if GODOT_VERSION < 0x040300
    if (_property.usage == 7)
        _property.usage = PROPERTY_USAGE_DEFAULT;
    #endif

    return true;
}

Dictionary OScriptNodePin::_save()
{
    Dictionary data;
    data["pin_name"] = _property.name;

    if (_property.type != Variant::NIL)
        data["type"] = _property.type;

    if (_direction != PD_Input)
        data["dir"] = _direction;

    if (_flags > 0)
        data["flags"] = _flags;

    if (!_label.is_empty())
        data["label"] = _label;

    if (!_target_class.is_empty())
        data["target_class"] = _target_class;

    if (_default_value.get_type() != Variant::NIL)
    {
        if (_default_value != Variant())
            data["dv"] = _default_value;
    }

    // This fixes any potential data issues and guarantees that a generated default value exists.
    if (_generated_default_value.get_type() == Variant::NIL)
        _generated_default_value = VariantUtils::make_default(_property.type);

    if (VariantUtils::make_default(_property.type) != _generated_default_value)
        data["gdv"] = _generated_default_value;

    if (_property.hint != PROPERTY_HINT_NONE)
        data["hint"] = _property.hint;

    if (!_property.hint_string.is_empty())
        data["hint_string"] = _property.hint_string;

    #if GODOT_VERSION < 0x040300
    if (_property.usage == 7)
        _property.usage = PROPERTY_USAGE_DEFAULT;
    #endif

    if (_property.usage != PROPERTY_USAGE_DEFAULT)
        data["usage"] = _property.usage;

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
    pin->_property.type = Variant::NIL;
    pin->_property.hint = PROPERTY_HINT_NONE;
    pin->_property.usage = PROPERTY_USAGE_DEFAULT;
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
    return _property.name;
}

void OScriptNodePin::set_pin_name(const StringName& p_pin_name)
{
    if (!_property.name.match(p_pin_name))
    {
        _property.name = p_pin_name;
        emit_changed();
    }
}

Variant::Type OScriptNodePin::get_type() const
{
    return _property.type;
}

void OScriptNodePin::set_type(Variant::Type p_type)
{
    if (_property.type != p_type)
    {
        _property.type = p_type;

        if (_set_type_resets_default)
            reset_default_value();

        emit_changed();
    }
}

String OScriptNodePin::get_pin_type_name() const
{
    return PropertyUtils::get_property_type_name(_property);
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
            _property.type = Variant::OBJECT;

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
    _generated_default_value = _target_class.is_empty() ? VariantUtils::make_default(_property.type) : Variant();
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

void OScriptNodePin::set_flag(Flags p_flag)
{
    if (!_flags.has_flag(p_flag))
    {
        _flags.set_flag(p_flag);
        emit_changed();
    }
}

String OScriptNodePin::get_label() const
{
    return _label;
}

void OScriptNodePin::set_label(const String& p_label, bool p_pretty_format)
{
    if (_label != p_label)
    {
        _label = p_label;

        // To simplify the logic, setting a label for Data-type pins shows automatically.
        // For execution pins, this requires setting the SHOW_LABEL and the label text.
        // This allows simply calling set_label to have the label shown for execution pins.
        // todo: can this be done irrespective of pin types?
        if (_flags.has_flag(EXECUTION) && !_flags.has_flag(SHOW_LABEL))
            _flags.set_flag(SHOW_LABEL);

        if (!p_pretty_format && !_flags.has_flag(NO_CAPITALIZE))
            _flags.set_flag(NO_CAPITALIZE);

        emit_changed();
    }
}

void OScriptNodePin::show_label()
{
    _clear_flag(HIDE_LABEL);
    set_flag(SHOW_LABEL);
}

void OScriptNodePin::hide_label()
{
    _clear_flag(SHOW_LABEL);
    set_flag(HIDE_LABEL);
}

void OScriptNodePin::no_pretty_format()
{
    set_flag(NO_CAPITALIZE);
}

void OScriptNodePin::set_file_types(const String& p_file_types)
{
    if (_property.hint == PROPERTY_HINT_FILE || _flags.has_flag(FILE))
        _property.hint_string = p_file_types;
}

String OScriptNodePin::get_file_types() const
{
    if (_property.hint == PROPERTY_HINT_FILE || _flags.has_flag(FILE))
        return _property.hint_string;
    return "";
}

bool OScriptNodePin::can_accept(const Ref<OScriptNodePin>& p_pin) const
{
    // This should always be called from the context that "this" is the target and p_pin is the source.
    if (get_direction() != PD_Input || p_pin->get_direction() != PD_Output)
        return false;

    // Short-circuit execution ports, if both are executions, allow
    if (is_execution() && p_pin->is_execution())
        return true;

    // Cannot mix data and execution ports
    if ((!is_execution() && p_pin->is_execution()) || (is_execution() && !p_pin->is_execution()))
        return false;

    // Any pin can connect to a Boolean input pin.
    if (_property.type == Variant::BOOL)
        return true;

    // Types match
    if (_property.type == p_pin->get_type())
    {
        const String target_class = _property.class_name;
        const String source_class = p_pin->get_property_info().class_name;
        if (!target_class.is_empty() && !source_class.is_empty())
        {
            // Check inheritance of global classes
            if (ScriptServer::is_global_class(source_class) && ScriptServer::is_parent_class(source_class, target_class))
                return true;

            // Either must be the same or the target must be a super type of the source
            // The equality check is to handle enum classes that aren't registered as "classes" in Godot terms
            if (ClassDB::is_parent_class(source_class, target_class) || target_class == source_class)
                return true;

            return false;
        }
        else if (target_class.is_empty() && !source_class.is_empty())
        {
            // If the source is a derived object type of the target, thats fine
            if (_property.type == Variant::OBJECT)
                return true;

            // If source is an enum/bitfield, allow coercion
            if (!PropertyUtils::is_class_enum(p_pin->get_property_info()) && !PropertyUtils::is_class_bitfield(p_pin->get_property_info()))
                return false;
        }

        return true;
    }

    // Coercion is allowed here
    if (_property.type == Variant::STRING)
    {
        // File targets should only accept string sources
        if (_property.hint == PROPERTY_HINT_FILE)
        {
            if (!(p_pin->get_type() == Variant::STRING || PropertyUtils::is_variant(p_pin->get_property_info())))
                return false;
        }
        return true;
    }

    if (_property.type == Variant::STRING_NAME && p_pin->get_property_info().type == Variant::STRING)
        return true;

    // Numeric conversions allows
    if (_property.type == Variant::INT || _property.type == Variant::FLOAT)
        if (p_pin->get_type() == Variant::INT || p_pin->get_type() == Variant::FLOAT)
            return true;

    // Allow any-to-specific or specific-to-any
    if (PropertyUtils::is_variant(_property) || PropertyUtils::is_variant(p_pin->get_property_info()))
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

bool OScriptNodePin::is_label_visible() const
{
    if (_flags.has_flag(HIDE_LABEL) || _flags.has_flag(HIDDEN))
        return false;

    if (_flags.has_flag(EXECUTION) && !_flags.has_flag(SHOW_LABEL))
        return false;

    return true;
}

Ref<OScriptTargetObject> OScriptNodePin::resolve_target()
{
    return get_owning_node()->resolve_target(this);
}