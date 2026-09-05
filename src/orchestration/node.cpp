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
#include "orchestration/node.h"

#include "common/macros.h"
#include "common/variant_struct_schema.h"
#include "common/variant_utils.h"
#include "core/godot/object/enum_resolver.h"
#include "orchestration/orchestration.h"
#include "orchestration/pin_layout.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>

TypedArray<Dictionary> OScriptNode::_get_pin_data() const {
    TypedArray<Dictionary> pins;
    for (const Ref<OScriptNodePin>& pin : _input_pins) {
        pins.push_back(pin->_save());
    }
    for (const Ref<OScriptNodePin>& pin : _output_pins) {
        pins.push_back(pin->_save());
    }
    return pins;
}

void OScriptNode::_set_pin_data(const TypedArray<Dictionary>& p_pin_data) {
    for (uint32_t i = 0; i < p_pin_data.size(); i++) {
        const Dictionary& data = p_pin_data[i];
        Ref<OScriptNodePin> pin;
        pin.instantiate();
        pin->set_owning_node(this);
        pin->_load(data);
        if (pin->get_direction() == PD_Input) {
            _input_pins.push_back(pin);
        } else {
            _output_pins.push_back(pin);
        }
    }
}

void OScriptNode::_rewire_split_pins(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    int position[PD_MAX] = { 0, 0 };
    for (const Ref<OScriptNodePin>& old : p_old_pins) {
        const EPinDirection direction = old->get_direction();
        const Ref<OScriptNodePin> new_pin = rewire_old_pins_by_position()
            ? find_pin(position[direction], direction)
            : find_pin(old->get_pin_name(), direction);
        position[direction]++;

        if (old->is_split() && new_pin.is_valid()) {
            _restore_split_pin(old, new_pin);
        }
    }
}

void OScriptNode::_restore_split_pin(const Ref<OScriptNodePin>& p_old_pin, const Ref<OScriptNodePin>& p_new_pin) {
    // A pin can only stay split if it kept its type; connections at the sub-pin ports are still present,
    // so the eligibility check in split() cannot be used here.
    if (p_new_pin->is_split() || p_new_pin->get_type() != p_old_pin->get_type() || !VariantStructSchema::is_composite(p_new_pin->get_type())) {
        return;
    }

    p_new_pin->_split_components();

    const Vector<Ref<OScriptNodePin>>& old_subs = p_old_pin->get_sub_pins();
    const Vector<Ref<OScriptNodePin>>& new_subs = p_new_pin->get_sub_pins();
    for (int i = 0; i < old_subs.size() && i < new_subs.size(); i++) {
        new_subs[i]->_default_value = old_subs[i]->_default_value;
        if (old_subs[i]->is_split()) {
            _restore_split_pin(old_subs[i], new_subs[i]);
        }
    }
}

void OScriptNode::_slot_pins_changed() {
    _cache_pin_indices();
    emit_changed();
    emit_signal("pins_changed");
}

bool OScriptNode::_is_in_editor() {
    return OS::get_singleton()->has_feature("editor");
}

void OScriptNode::_queue_reconstruct() {
    if (_reconstruction_queued) {
        return;
    }
    callable_mp_this(reconstruct_node).call_deferred();
}

Ref<OScriptGraph> OScriptNode::get_owning_graph() const {
    return _orchestration->find_graph(Ref<OScriptNode>(this));
}

void OScriptNode::set_id(int p_id) {
    _id = p_id;
}

void OScriptNode::set_size(const Vector2& p_size) {
    _size = p_size;
}

void OScriptNode::set_position(const Vector2& p_position) {
    if (!_position.is_equal_approx(p_position)) {
        _position = p_position;
        emit_changed();
    }
}

void OScriptNode::set_breakpoint_flag(BreakpointFlags p_flag) {
    if (_breakpoint_flag != p_flag) {
        _breakpoint_flag = p_flag;
        emit_changed();
    }
}

void OScriptNode::set_flags(BitField<ScriptNodeFlags> p_flags) {
    _flags = p_flags;
    emit_changed();
}

void OScriptNode::get_actions(List<Ref<OScriptAction>>& p_action_list) {
}

void OScriptNode::pre_save() {
}

void OScriptNode::pre_remove() {
    // During node removal, there is no need for pin reconstruction to fire and there may
    // be situations, such as in AssignLocalVariable, that could trigger reconstruction
    // when pins are unlinked. By preemptively setting reconstructing to true, this will
    // block pin reconstruction when nodes are being removed.
    _reconstructing = true;
}

void OScriptNode::post_initialize() {
    for (const Ref<OScriptNodePin>& pin : _input_pins) {
        pin->post_initialize();
    }
    for (const Ref<OScriptNodePin>& pin : _output_pins) {
        pin->post_initialize();
    }
    _cache_pin_indices();
    _initialized = true;
}

void OScriptNode::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    allocate_default_pins();
    _cache_pin_indices();
}

void OScriptNode::reconstruct_node() {
    if (_reconstructing) {
        return;
    }

    // Set reconstruction flag
    _reconstructing = true;

    Vector<Ref<OScriptNodePin>> old_pins = get_all_pins();
    _input_pins.clear();
    _output_pins.clear();

    reallocate_pins_during_reconstruction(old_pins);
    rewire_old_pins_to_new_pins(old_pins, get_all_pins());
    _rewire_split_pins(old_pins);
    _cache_pin_indices();

    post_reconstruct_node();

    emit_changed();

    // Clear reconstruction flag
    _reconstructing = false;
    _reconstruction_queued = false;
}

void OScriptNode::post_placed_new_node() {
    _cache_pin_indices();
}

void OScriptNode::rewire_old_pins_to_new_pins(const Vector<Ref<OScriptNodePin>>& p_old_pins, const Vector<Ref<OScriptNodePin>>&  p_new_pins) {
    int position = 0;

    for (const Ref<OScriptNodePin>& old : p_old_pins) {
        if (old->is_input()) {
            Ref<OScriptNodePin> new_pin;
            if (rewire_old_pins_by_position()) {
                new_pin = find_pin(position, old->get_direction());
            } else {
                new_pin = find_pin(old->get_pin_name(), old->get_direction());
            }
            position++;

            if (new_pin.is_valid()) {
                // If new pin has a default value set that isn't the default, skip.
                if (new_pin->get_default_value().get_type() != Variant::NIL
                        && new_pin->get_default_value() != new_pin->get_generated_default_value()) {
                    continue;
                }

                // If the two pins have different generated default values, skip.
                if (new_pin->get_generated_default_value() != old->get_generated_default_value()) {
                    continue;
                }

                // If the two pins have different types, skip.
                if (new_pin->get_type() != old->get_type()) {
                    continue;
                }

                // If the old pin's default value equals the generated default value, skip.
                if (old->get_default_value() == old->get_generated_default_value()) {
                    continue;
                }

                new_pin->set_default_value(old->get_default_value());
            }
        }
    }
}

void OScriptNode::get_pin_layout(OScriptPinLayout& r_layout) const {
    r_layout.add_default_rows(_input_pins, _output_pins);
}

void OScriptNode::initialize(const OScriptNodeInitContext& p_context) {
    _initialized = true;
    allocate_default_pins();
}

String OScriptNode::get_help_topic() const {
    return vformat("class:%s", get_class());
}

Ref<OScriptNodePin> OScriptNode::create_pin(EPinDirection p_direction, EPinType p_pin_type, const PropertyInfo& p_property, const Variant& p_default_value) {
    Ref<OScriptNodePin> pin = OScriptNodePin::create(this, p_property);
    if (pin.is_valid()) {
        if (p_pin_type == PT_Execution) {
            pin->set_flag(OScriptNodePin::Flags::EXECUTION);
        } else {
            pin->set_flag(OScriptNodePin::Flags::DATA);
        }

        pin->set_direction(p_direction);
        pin->set_default_value(p_default_value);

        Variant::Type type = p_default_value.get_type() != Variant::NIL ? p_default_value.get_type() : p_property.type;
        pin->set_generated_default_value(VariantUtils::make_default(type));

        // For enums when the node spawns in the editor, set its generated default value to the first element
        if (_is_in_editor() && p_default_value.get_type() == Variant::NIL && pin->is_enum()) {
            const List<EnumResolver::EnumItem> items = EnumResolver::resolve(p_property);
            if (!items.is_empty()) {
                pin->set_generated_default_value(items.front()->get().value);
            }
        }

        if (p_direction == PD_Input) {
            _input_pins.push_back(pin);
        } else {
            _output_pins.push_back(pin);
        }
    }
    return pin;
}

Ref<OScriptNodePin> OScriptNode::find_pin(const String& p_pin_name, EPinDirection p_direction) const {
    if (p_direction != PD_Output) {
        for (const Ref<OScriptNodePin>& pin : _input_pins) {
            if (pin->get_pin_name().match(p_pin_name)) {
                return pin;
            }
        }
    }
    if (p_direction != PD_Input) {
        for (const Ref<OScriptNodePin>& pin : _output_pins) {
            if (pin->get_pin_name().match(p_pin_name)) {
                return pin;
            }
        }
    }
    return {};
}

Ref<OScriptNodePin> OScriptNode::find_pin(int p_index, EPinDirection p_direction) const {
    const Vector<Ref<OScriptNodePin>>& pins = p_direction == PD_Input ? _input_pins : _output_pins;
    if (p_index >= 0 && p_index < pins.size()) {
        return pins[p_index];
    }
    return {};
}

const Vector<Ref<OScriptNodePin>>& OScriptNode::find_pins(EPinDirection p_direction) const {
    ERR_FAIL_COND_V_MSG(p_direction == PD_MAX, _input_pins, "find_pins requires PD_Input or PD_Output.");
    return p_direction == PD_Input ? _input_pins : _output_pins;
}

Vector<Ref<OScriptNodePin>> OScriptNode::get_slot_pins(EPinDirection p_direction) const {
    ERR_FAIL_COND_V_MSG(p_direction == PD_MAX, {}, "get_slot_pins requires PD_Input or PD_Output.");

    // Each logical pin stands for itself, or for its sub-pins when split
    Vector<Ref<OScriptNodePin>> slot_pins;
    for (const Ref<OScriptNodePin>& pin : (p_direction == PD_Input ? _input_pins : _output_pins)) {
        slot_pins.append_array(pin->get_slot_pins());
    }
    return slot_pins;
}

Ref<OScriptNodePin> OScriptNode::find_slot_pin(int p_port, EPinDirection p_direction) const {
    // Ports are assigned over visible slot pins only, mirroring _cache_pin_indices
    int port = 0;
    for (const Ref<OScriptNodePin>& pin : get_slot_pins(p_direction)) {
        if (pin->is_hidden()) {
            continue;
        }
        if (port == p_port) {
            return pin;
        }
        port++;
    }
    return {};
}

Ref<OScriptNodePin> OScriptNode::find_slot_pin(const String& p_name, EPinDirection p_direction) const {
    for (const Ref<OScriptNodePin>& pin : get_slot_pins(p_direction)) {
        if (pin->get_pin_name().match(p_name)) {
            return pin;
        }
    }
    return {};
}

bool OScriptNode::split_pin(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null() || p_pin->get_owning_node() != this, false);
    ERR_FAIL_COND_V_MSG(!p_pin->can_split(), false, vformat("Pin '%s' cannot be split.", p_pin->get_pin_name()));

    const EPinDirection direction = p_pin->get_direction();
    const int64_t added = VariantStructSchema::get_components(p_pin->get_type()).size() - 1;

    // Ports are 8 bits wide in connections
    int64_t port_count = 0;
    for (const Ref<OScriptNodePin>& pin : get_slot_pins(direction)) {
        if (!pin->is_hidden()) {
            port_count++;
        }
    }
    ERR_FAIL_COND_V_MSG(port_count + added > 255, false, vformat("Splitting pin '%s' would exceed the 255 ports a node may have.", p_pin->get_pin_name()));

    // The pin's own port has no connections, so only the pins after it move
    const int port = p_pin->get_pin_index();
    if (!p_pin->split()) {
        return false;
    }

    if (_orchestration && added > 0) {
        _orchestration->adjust_connections(this, port + 1, added, direction);
    }

    _slot_pins_changed();
    return true;
}

bool OScriptNode::recombine_pin(const Ref<OScriptNodePin>& p_pin) {
    ERR_FAIL_COND_V(p_pin.is_null() || p_pin->get_owning_node() != this, false);

    const Ref<OScriptNodePin> parent = p_pin->is_split() ? p_pin : Ref<OScriptNodePin>(p_pin->get_parent_pin());
    ERR_FAIL_COND_V_MSG(parent.is_null() || !parent->can_recombine(), false, vformat("Pin '%s' cannot be recombined.", p_pin->get_pin_name()));

    const EPinDirection direction = parent->get_direction();
    const Vector<Ref<OScriptNodePin>> slot_pins = parent->get_slot_pins();
    const int first_port = slot_pins[0]->get_pin_index();
    const int64_t removed = slot_pins.size() - 1;

    if (!parent->recombine()) {
        return false;
    }

    // The recombined pin takes the first sub-pin's port; the pins after the last sub-pin move up
    if (_orchestration && removed > 0) {
        _orchestration->adjust_connections(this, first_port + removed + 1, -removed, direction);
    }

    _slot_pins_changed();
    return true;
}

bool OScriptNode::remove_pin(const Ref<OScriptNodePin>& p_pin) {
    if (_input_pins.has(p_pin)) {
        _input_pins.erase(p_pin);
        return true;
    }
    if (_output_pins.has(p_pin)) {
        _output_pins.erase(p_pin);
        return true;
    }
    return false;
}

bool OScriptNode::has_any_connections() const {
    for (const Ref<OScriptNodePin>& pin : _input_pins) {
        if (pin->has_any_connections()) {
            return true;
        }
    }
    for (const Ref<OScriptNodePin>& pin : _output_pins) {
        if (pin->has_any_connections()) {
            return true;
        }
    }
    return false;
}

Vector<Ref<OScriptNodePin>> OScriptNode::get_all_pins() const {
    Vector<Ref<OScriptNodePin>> pins;
    pins.resize(_input_pins.size() + _output_pins.size());
    Ref<OScriptNodePin>* dst = pins.ptrw();
    for (const Ref<OScriptNodePin>& pin : _input_pins) {
        *dst++ = pin;
    }
    for (const Ref<OScriptNodePin>& pin : _output_pins) {
        *dst++ = pin;
    }
    return pins;
}

Vector<Ref<OScriptNodePin>> OScriptNode::get_eligible_autowire_pins(const Ref<OScriptNodePin>& p_pin) const {
    // Autowire targets connectable pins, so sub-pins stand in for their split parents
    Vector<Ref<OScriptNodePin>> candidates = get_slot_pins(PD_Input);
    candidates.append_array(get_slot_pins(PD_Output));

    Vector<Ref<OScriptNodePin>> eligible_pins;
    for (const Ref<OScriptNodePin>& pin : candidates) {
        // Invalid or hidden pins are skipped
        if (!pin.is_valid() || pin->is_hidden()) {
            continue;
        }

        // Skip pins that are specifically flagged as non-autowirable
        if (!pin->can_autowire()) {
            continue;
        }

        // Cannot connect input to input or output to output
        if (p_pin->get_direction() == pin->get_direction()) {
            continue;
        }

        // Match execution or data state
        if ((p_pin->is_execution() && !pin->is_execution()) || (!p_pin->is_execution() && pin->is_execution())) {
            continue;
        }

        if (!p_pin->is_execution() && !pin->is_execution()) {
            // Data flow pins must match types
            if (pin->get_type() != p_pin->get_type()) {
                continue;
            }
        }

        eligible_pins.push_back(pin);
    }
    return eligible_pins;
}

void OScriptNode::on_pin_connected(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid()) {
        p_pin->reset_default_value();
    }
    emit_signal("pin_connected", p_pin->get_direction(), p_pin->get_pin_index());
}

void OScriptNode::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) {
    emit_signal("pin_disconnected", p_pin->get_direction(), p_pin->get_pin_index());
}

bool OScriptNode::has_execution_pins() const {
    for (const Ref<OScriptNodePin>& pin : find_pins(PD_Input)) {
        if (pin->is_execution()) {
            return true;
        }
    }
    return false;
}

void OScriptNode::_validate_input_default_values() {
}

void OScriptNode::_notify_pins_changed() {
    if (_initialized) {
        reconstruct_node();
        emit_signal("pins_changed");
    }
}

void OScriptNode::_cache_pin_indices() {
    // A pin's index is its port: its ordinal among visible pins in slot order. find_slot_pin(int) is the inverse.
    int input_index = 0;
    for (const Ref<OScriptNodePin>& pin : get_slot_pins(PD_Input)) {
        if (!pin->is_hidden()) {
            pin->_cached_pin_index = input_index++;
        }
    }
    int output_index = 0;
    for (const Ref<OScriptNodePin>& pin : get_slot_pins(PD_Output)) {
        if (!pin->is_hidden()) {
            pin->_cached_pin_index = output_index++;
        }
    }
}

OScriptNode::OScriptNode()
    : _flags(CATALOGABLE)
    , _breakpoint_flag(BREAKPOINT_NONE) {
}

void OScriptNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_id", "id"), &OScriptNode::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &OScriptNode::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_id", "get_id");

    ClassDB::bind_method(D_METHOD("set_size", "size"), &OScriptNode::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &OScriptNode::get_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_size", "get_size");

    ClassDB::bind_method(D_METHOD("set_position", "position"), &OScriptNode::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &OScriptNode::get_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("_set_pin_data", "pin_data"), &OScriptNode::_set_pin_data);
    ClassDB::bind_method(D_METHOD("_get_pin_data"), &OScriptNode::_get_pin_data);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pin_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_pin_data", "_get_pin_data");

    ADD_SIGNAL(MethodInfo("pin_connected", PropertyInfo(Variant::INT, "pin_type"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("pin_disconnected", PropertyInfo(Variant::INT, "pin_type"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("pins_changed"));
}