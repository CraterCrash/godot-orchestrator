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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_H

#include "script/node_pin.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

class OrchestratorEditorContextMenu;
class OrchestratorEditorGraphNode;
class OrchestratorEditorGraphPanel;

struct OrchestratorEditorGraphPinSlotInfo
{
    bool enabled = false;
    int type = 0;
    String icon;
    Color color;
};

/// A pin represents a specific port or connection point on a visual script node that identifies
/// either execution control flow for the visual code or a data value.
///
class OrchestratorEditorGraphPin : public VBoxContainer
{

    friend class OrchestratorEditorGraphPanel;

    // todo:
    //  to have UE hover on pins, we need to use a PanelContainer
    //  the Stylebox Needs to be a Gradient 2D texture
    //  It needs to use
    //      offsets -.1 0.4 0.6 1.1
    //      colors [background] [pin color] [pin color] [background]

    GDCLASS(OrchestratorEditorGraphPin, VBoxContainer);

    Ref<OrchestrationGraphPin> _pin;
    OrchestratorEditorGraphNode* _node = nullptr;
    int _index = -1;
    bool _dirty = false;

    TextureRect* _icon = nullptr;
    Label* _label = nullptr;
    Control* _default_value = nullptr;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    virtual String _get_pin_color_name() const;

    void _default_value_changed();

    Variant _get_default_value();
    void _set_default_value(const Variant& p_value);

    virtual Variant _read_control_value() { return {}; }
    void _update_control();
    virtual void _update_control_value(const Variant& p_value) { }

    void _create_pin_layout();
    virtual bool _is_default_value_below_label() const { return false; }
    virtual Control* _create_default_value_widget() { return nullptr; }

    virtual String _get_label_text();
    virtual String _get_tooltip_text();

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    OrchestratorEditorGraphPanel* get_graph();

    OrchestratorEditorGraphNode* get_graph_node() const { return _node; }
    void set_graph_node(OrchestratorEditorGraphNode* p_owner_node);

    void set_pin(const Ref<OrchestrationGraphPin>& p_pin);

    String get_pin_name() const;
    EPinDirection get_direction() const;

    bool is_execution() const;
    bool is_linked() const;
    bool is_hidden() const;
    bool is_connectable() const;
    bool is_target_self() const;
    bool is_autowire_enabled() const;

    OrchestratorEditorGraphPinSlotInfo get_slot_info() const;

    const PropertyInfo& get_property_info() const;

    void set_default_value_control_visible(bool p_visible);
    void set_icon_visible(bool p_visible);
    void set_show_advanced_tooltips(bool p_show_advanced_tooltips);

    OrchestratorEditorGraphPin();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_H