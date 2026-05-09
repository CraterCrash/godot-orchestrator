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
#include "orchestration/nodes/reroute.h"

#include "common/property_utils.h"
#include "orchestration/node_pin.h"
#include "orchestration/orchestration.h"

void OScriptNodeReroute::_apply_type(ERerouteType p_new_type) {
    if (_reroute_type == p_new_type) {
        return;
    }
    _reroute_type = p_new_type;
    _notify_pins_changed();
}

Ref<OScriptNodePin> OScriptNodeReroute::_resolve_target_pin() const {
    const Ref<OScriptNodePin> out = find_pin("output", PD_Output);
    if (!out.is_valid()) {
        return {};
    }

    for (const Ref<OScriptNodePin>& connected : out->get_connections()) {
        OScriptNodeReroute* downstream = cast_to<OScriptNodeReroute>(connected->get_owning_node());
        if (downstream) {
            Ref<OScriptNodePin> result = downstream->_resolve_target_pin();
            if (result.is_valid()) {
                return result;
            }
        } else {
            return connected;
        }
    }

    return {};
}

void OScriptNodeReroute::_cascade_resolve_from_target() {
    // Uses Orchestration connections directly.
    // Pin objects on this node may be stale (cleared and not yet repopulated) if the _notify_pins_changed
    // is triggered from a synchronous rebuild.
    for (const OScriptConnection& C : get_orchestration()->get_connections()) {
        if (static_cast<int>(C.from_node) != _id || C.from_port != 0) {
            continue;
        }

        const Ref<OScriptNode> dst = get_orchestration()->get_node(C.to_node);
        if (!dst.is_valid()) {
            continue;
        }

        OScriptNodeReroute* downstream = cast_to<OScriptNodeReroute>(dst.ptr());
        if (!downstream) {
            continue;
        }

        const Ref<OScriptNodePin> target = downstream->_resolve_target_pin();
        if (target.is_valid()) {
            downstream->_reroute_type = target->is_execution() ? REROUTE_CONTROL : REROUTE_DATA;
            downstream->_notify_pins_changed();
        } else {
            downstream->_apply_type(REROUTE_ANY);
        }

        downstream->_cascade_resolve_from_target();
    }
}

void OScriptNodeReroute::_cascade_type_downstream() {
    const Ref<OScriptNodePin> out = find_pin("output", PD_Output);
    if (!out.is_valid()) {
        return;
    }

    for (const Ref<OScriptNodePin>& connected : out->get_connections()) {
        OScriptNodeReroute* downstream = cast_to<OScriptNodeReroute>(connected->get_owning_node());
        if (!downstream) {
            continue;
        }

        if (downstream->_reroute_type == REROUTE_ANY) {
            downstream->_apply_type(_reroute_type);
        } else if (downstream->_reroute_type == _reroute_type) {
            // Same reroute type, so rebuild using allocate_default_pins, which picks up the data type.
            downstream->_notify_pins_changed();
        }
        downstream->_cascade_type_downstream();
    }
}

void OScriptNodeReroute::force_reroute_type(ERerouteType p_type) {
    if (_reroute_type != p_type) {
        _reroute_type = p_type;
        reconstruct_node();
    }
}

void OScriptNodeReroute::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_get_reroute_type"), &OScriptNodeReroute::_get_reroute_type);
    ClassDB::bind_method(D_METHOD("_set_reroute_type", "type"), &OScriptNodeReroute::_set_reroute_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reroute_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "_set_reroute_type", "_get_reroute_type");
}

void OScriptNodeReroute::allocate_default_pins() {

    auto make_reroute_pin = [&](EPinDirection p_dir, EPinType p_type, const PropertyInfo& p_prop) {
        Ref<OScriptNodePin> pin = create_pin(p_dir, p_type, p_prop);
        pin->hide_label();
        pin->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);
        return pin;
    };

    if (_reroute_type == REROUTE_CONTROL) {
        make_reroute_pin(PD_Input,  PT_Execution, PropertyUtils::make_exec("input"));
        make_reroute_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("output"));
    } else {
        // REROUTE_DATA: Derive the data type from the source connected to input port 0
        // REROUTE_ANY: Leave data_type as NIL and use make_variant so the pin acts as a wildcard
        Variant::Type data_type = Variant::NIL;
        if (_reroute_type == REROUTE_DATA) {
            // During node reconstruction, the _pins array is cleared so query connections by node ID and port index.
            // Precedence: Try the input (Source) first, its authoritative when connected.
            for (const OScriptConnection& C : get_orchestration()->get_connections()) {
                if (static_cast<int>(C.to_node) == _id && C.to_port == 0) {
                    const Ref<OScriptNode> src = get_orchestration()->get_node(C.from_node);
                    if (src.is_valid()) {
                        const Ref<OScriptNodePin> src_pin = src->find_pin(C.from_port, PD_Output);
                        if (src_pin.is_valid()) {
                            data_type = src_pin->get_type();
                        }
                    }
                    break;
                }
            }

            // There was no input source, so walk the output chain to derive the type at the target
            if (data_type == Variant::NIL) {
                int walk_node = _id;
                while (true) {
                    bool advanced = false;
                    for (const OScriptConnection& C : get_orchestration()->get_connections()) {
                        if (static_cast<int>(C.from_node) != walk_node || C.from_port != 0) {
                            continue;
                        }

                        const Ref<OScriptNode> dst = get_orchestration()->get_node(C.to_node);
                        if (!dst.is_valid()) {
                            break;
                        }

                        if (cast_to<OScriptNodeReroute>(dst.ptr())) {
                            walk_node = C.to_node;
                            advanced = true;
                        } else {
                            const Ref<OScriptNodePin> dst_pin = dst->find_pin(C.to_port, PD_Input);
                            if (dst_pin.is_valid()) {
                                data_type = dst_pin->get_type();
                            }
                        }
                        break;
                    }

                    if (!advanced) {
                        break;
                    }
                }
            }
        }

        if (data_type == Variant::NIL) {
            // Variant pin: accepts any data type (PROPERTY_USAGE_NIL_IS_VARIANT)
            make_reroute_pin(PD_Input,  PT_Data, PropertyUtils::make_variant("input"));
            make_reroute_pin(PD_Output, PT_Data, PropertyUtils::make_variant("output"));
        } else {
            make_reroute_pin(PD_Input,  PT_Data, PropertyUtils::make_typed("input",  data_type));
            make_reroute_pin(PD_Output, PT_Data, PropertyUtils::make_typed("output", data_type));
        }
    }
}

void OScriptNodeReroute::on_pin_connected(const Ref<OScriptNodePin>& p_pin) {
    super::on_pin_connected(p_pin);

    // Type is determined by the INPUT side only.
    // Output connections are downstream consumers.
    if (!p_pin->is_input()) {
        return;
    }

    const Vector<Ref<OScriptNodePin>> conns = p_pin->get_connections();
    if (conns.is_empty()) {
        return;
    }

    if (_reroute_type == REROUTE_ANY) {
        _apply_type(conns[0]->is_execution() ? REROUTE_CONTROL : REROUTE_DATA);
    } else if (_reroute_type == REROUTE_DATA) {
        // Rebuild so allocate_default_pins picks up the new source data type.
        _notify_pins_changed();
    }

    // Propagate the (possibly changed) type to all downstream reroutes.
    _cascade_type_downstream();
}

void OScriptNodeReroute::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) {
    super::on_pin_disconnected(p_pin);

    // Only the input side determines type.
    // When the last input connection is gone, resolve the type from the target end of the chain
    // rather than blindly reverting to ANY. Only revert to ANY (and cascade) when there is no target.
    if (p_pin->is_input() && !p_pin->has_any_connections()) {
        const Ref<OScriptNodePin> target = _resolve_target_pin();
        if (target.is_valid()) {
            _reroute_type = target->is_execution() ? REROUTE_CONTROL : REROUTE_DATA;
            _notify_pins_changed();
        } else {
            _apply_type(REROUTE_ANY);
        }

        // Always cascade
        // Any downstream reroutes must always independently resolve their type from the target,
        // and we use the orchestration-level connections to avoid stale pin objects post rebuild.
        _cascade_resolve_from_target();
    }
}

String OScriptNodeReroute::get_tooltip_text() const {
    switch (_reroute_type) {
        case REROUTE_CONTROL:
            return "Reroutes an execution connection.";
        case REROUTE_DATA:
            return "Reroutes a data connection, fanning out to multiple targets.";
        default:
            return "Reroute node. Connect a data or execution pin to define its type.";
    }
}

OScriptNodeReroute::OScriptNodeReroute() {
    _flags = CATALOGABLE;
}
