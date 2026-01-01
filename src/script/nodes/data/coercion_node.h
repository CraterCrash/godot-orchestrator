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
#ifndef ORCHESTRATOR_SCRIPT_NODE_COERCION_NODE_H
#define ORCHESTRATOR_SCRIPT_NODE_COERCION_NODE_H

#include "script/script.h"

/// A class that supports coercion of one data type to another.
/// @deprecated scheduled for removal
class OScriptNodeCoercion : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCoercion, OScriptNode);

    Variant::Type _left = Variant::NIL;
    Variant::Type _right = Variant::NIL;

    Ref<OScriptNodePin> _get_input_pin();
    Ref<OScriptNodePin> _get_output_pin();

    Ref<OScriptNodePin> _get_source_node_pin();
    Ref<OScriptNodePin> _get_target_node_pin();

    void _add_source_target_listeners();

    void _on_source_pin_changed(const Ref<OScriptNodePin>& p_pin);
    void _on_target_pin_changed(const Ref<OScriptNodePin>& p_pin);

protected:
    static void _bind_methods() {}

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    bool should_draw_as_bead() const override { return true; }
    void initialize(const OScriptNodeInitContext &p_context) override;
    //~ End OScriptNode Interface

    OScriptNodeCoercion();
};

#endif // ORCHESTRATOR_SCRIPT_NODE_COERCION_NODE_H