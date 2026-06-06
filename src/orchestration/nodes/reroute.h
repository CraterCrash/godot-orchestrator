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
#pragma once

#include "orchestration/node.h"

/// Reroute nodes direct data or execution connections through a visual waypoint.
///
/// Reroute nodes begin as "ANY", allowing the user to connect either an execution or data wire.
/// The node shifts its type based on the first pin that is connected. When all connections are
/// removed, the reroute will revert to "ANY".
///
/// For control/execution reroutes, these enforce a 1-to-1 flow (1 input and only 1 output).
/// This is because control flow is not allowed to branch through a reroute node.
///
/// For data reroutes, these enforce a fan-out strategy of 1-to-N (1 input and 1 or more outputs).
/// This allows for a single data connection pipeline to route values to multiple targets.
///
class OScriptNodeReroute : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeReroute, OScriptNode);

public:
    enum ERerouteType {
        REROUTE_ANY     = 0,
        REROUTE_DATA    = 1,
        REROUTE_CONTROL = 2,
    };

private:
    ERerouteType _reroute_type = REROUTE_ANY;

    ERerouteType _get_reroute_type() const { return _reroute_type; }
    void _set_reroute_type(ERerouteType p_type) { _reroute_type = p_type; }

    // Rebuild pins for the given type.
    void _apply_type(ERerouteType p_new_type);
    // Walk downstream reroutes via connections and resolve each one's type from its own sink.
    void _cascade_resolve_from_target();
    // Walk output connections and push the current type to any downstream reroutes.
    void _cascade_type_downstream();
    // Walk the output chain and return the first non-reroute pin, or invalid if none.
    Ref<OScriptNodePin> _resolve_target_pin() const;

protected:
    static void _bind_methods();

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    void on_pin_connected(const Ref<OScriptNodePin>& p_pin) override;
    void on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) override;
    bool is_pure() const override { return false; }
    bool is_reroute() const override { return true; }
    String get_node_title() const override { return ""; }
    String get_tooltip_text() const override;
    String get_node_title_color_name() const override { return "reroute"; }
    bool can_user_delete_node() const override { return true; }
    bool can_inspect_node_properties() const override { return false; }
    //~ End OScriptNode Interface

    ERerouteType get_reroute_type() const { return _reroute_type; }

    /// Set the type directly and rebuild pins.
    /// This is used by migration code.
    /// @param p_type the reroute type
    void force_reroute_type(ERerouteType p_type);

    OScriptNodeReroute();
};

VARIANT_ENUM_CAST(OScriptNodeReroute::ERerouteType);
