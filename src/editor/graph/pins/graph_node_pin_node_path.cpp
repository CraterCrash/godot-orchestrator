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
#include "graph_node_pin_node_path.h"

#include "common/callable_lambda.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/property_selector.h"
#include "editor/scene_node_selector.h"
#include "script/nodes/functions/call_function.h"
#include "script/nodes/functions/call_member_function.h"
#include "script/nodes/utilities/self.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

#define EDSCALE OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_scale()

Vector<OrchestratorGraphNodePinNodePath::MethodDescriptor> OrchestratorGraphNodePinNodePath::_descriptors;

Control* OrchestratorGraphNodePinNodePath::_get_default_value_widget()
{
    HBoxContainer* container = memnew(HBoxContainer);

    // Create button
    _button = memnew(Button);
    _button->set_focus_mode(FOCUS_NONE);
    _button->set_custom_minimum_size(Vector2(28, 0));
    _button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), DEFAULT_TEXT));
    _button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinNodePath::_start_dialog_sequence));
    container->add_child(_button);

    _reset_button = memnew(Button);
    _reset_button->set_focus_mode(FOCUS_NONE);
    _reset_button->set_button_icon(SceneUtils::get_editor_icon("Reload"));
    _reset_button->connect("pressed", callable_mp_lambda(this, [=, this]() { _reset(); }));
    _reset_button->set_visible(!_button->get_text().match("Assign..."));
    container->add_child(_reset_button);

    return container;
}

void OrchestratorGraphNodePinNodePath::_resolve_descriptor()
{
    if (OScriptNodeCallMemberFunction* node = Object::cast_to<OScriptNodeCallMemberFunction>(_pin->get_owning_node()))
    {
        const MethodInfo& mi = node->get_function();
        for (MethodDescriptor& descriptor : _descriptors)
        {
            if (!node->get_target_class().match(descriptor.class_name))
                continue;

            if (!descriptor.method_name.match(mi.name))
                continue;

            if (!descriptor.pin_name.match(_pin->get_pin_name()))
                continue;

            _descriptor = &descriptor;
            break;
        }
    }
}

void OrchestratorGraphNodePinNodePath::_start_dialog_sequence()
{
    // Reset sequence node path
    _sequence_node_path = "";

    if (!_has_descriptor() || _descriptor->is_node_and_property_selection)
    {
        // In this case, this NodePath is simply asking the user to select a scene node only.
        _show_node_dialog();
    }
    else if (_descriptor->is_property_selection)
    {
        // This requires only a property path.
        // The dependency for this is based on the descriptor
        _show_property_dialog();
    }
}

void OrchestratorGraphNodePinNodePath::_show_node_dialog()
{
    // Resolve the selected node
    // When Godot's implementation eventually supports this (https://github.com/godotengine/godot/pull/94323),
    // then we can use the default Godot API
    Node* selected = nullptr;
    const NodePath path = _pin->get_effective_default_value();
    if (!path.is_empty())
    {
        if (Node* root = get_tree()->get_edited_scene_root())
            selected = root->get_node_or_null(path);
    }

    _node_selector = memnew(OrchestratorSceneNodeSelector);
    _node_selector->set_selected(selected);
    _node_selector->connect("node_selected", callable_mp(this, &OrchestratorGraphNodePinNodePath::_node_selected));
    add_child(_node_selector);

    _node_selector->popup_centered_clamped(Size2(350, 700) * EDSCALE);
}

void OrchestratorGraphNodePinNodePath::_node_selected(const NodePath& p_path)
{
    if (p_path.is_empty())
    {
        // Nothing should be done, leave unchanged.
        _node_selector->queue_free();
        _node_selector = nullptr;
        return;
    }

    if (!_has_descriptor() || (_has_descriptor() && !_descriptor->is_node_and_property_selection))
    {
        // Only a node selection is required
        _set_pin_value(p_path);
        return;
    }

    // User is selecting a property from a given node path
    Node* node = get_tree()->get_edited_scene_root()->get_node_or_null(p_path);
    if (!node)
        return;

    String value = _pin->get_effective_default_value();
    if (!value.is_empty() && value.contains(":"))
        value = value.substr(value.find(":") + 1);
    else
        value = "";

    _sequence_node_path = p_path;
    _show_property_dialog_for_object(node, value);
}

void OrchestratorGraphNodePinNodePath::_show_property_dialog()
{
    const Ref<OScriptNodePin> owner_pin = _pin->get_owning_node()->find_pin(_descriptor->dependency_pin_name, PD_Input);
    if (!owner_pin.is_valid() || !owner_pin->has_any_connections())
        return;

    const Ref<OScriptTargetObject> target = owner_pin->get_connections()[0]->resolve_target();
    if (!target.is_valid() || !target->has_target())
    {
        // In the event that the self node cannot be resolved, inform user about the edited scene needing to
        // include the reference to the node with the Orchestration for this lookup to resolve correctly.
        OScriptNodeSelf* self = Object::cast_to<OScriptNodeSelf>(owner_pin->get_connections()[0]->get_owning_node());
        if (self)
        {
            AcceptDialog* accept = memnew(AcceptDialog);
            accept->set_text("This Orchestration is not attached to any node in the current edited\nscene, so the reference cannot be resolved and no properties selected.");
            accept->set_title("Error");
            accept->set_exclusive(true);
            add_child(accept);

            accept->connect("canceled", callable_mp_lambda(this, [&] { accept->queue_free(); }));
            accept->connect("confirmed", callable_mp_lambda(this, [&] { accept->queue_free(); }));
            accept->popup_centered();
        }
        return;
    }

    String value = _pin->get_effective_default_value();
    if (!value.is_empty() && value.begins_with(":"))
        value = value.substr(1);

    _show_property_dialog_for_object(target->get_target(), value);
}

void OrchestratorGraphNodePinNodePath::_property_selected(const String& p_name)
{
    _set_pin_value(vformat("%s:%s", _sequence_node_path, p_name));
}

void OrchestratorGraphNodePinNodePath::_show_property_dialog_for_object(Object* p_object, const String& p_selected_value)
{
    // When Godot's implementation eventually supports this (https://github.com/godotengine/godot/pull/94323),
    // then we can use the default Godot API

    _property_selector = memnew(OrchestratorPropertySelector);
    _property_selector->connect("selected", callable_mp(this, &OrchestratorGraphNodePinNodePath::_property_selected));
    add_child(_property_selector);

    _property_selector->select_property_from_instance(p_object, p_selected_value);
}

void OrchestratorGraphNodePinNodePath::_reset()
{
    _set_pin_value(Variant());
}

void OrchestratorGraphNodePinNodePath::_set_pin_value(const Variant& p_pin_value)
{
    _pin->set_default_value(p_pin_value);
    _button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), DEFAULT_TEXT));
    _reset_button->set_visible(!_button->get_text().match(DEFAULT_TEXT));
}

void OrchestratorGraphNodePinNodePath::_pin_connected(int p_type, int p_index)
{
    const Ref<OScriptNodePin> pin = _pin->get_owning_node()->find_pin(p_index, EPinDirection(p_type));
    if (_has_descriptor() && pin.is_valid() && pin->get_pin_name().match(_descriptor->dependency_pin_name))
        _button->set_visible(true);
}

void OrchestratorGraphNodePinNodePath::_pin_disconnected(int p_type, int p_index)
{
    const Ref<OScriptNodePin> pin = _pin->get_owning_node()->find_pin(p_index, EPinDirection(p_type));
    if (_has_descriptor() && pin.is_valid() && pin->get_pin_name().match(_descriptor->dependency_pin_name))
    {
        _reset();
        _button->set_visible(false);
    }
}

void OrchestratorGraphNodePinNodePath::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040300
    OrchestratorGraphNodePin::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
    {
        if (_has_descriptor())
        {
            OScriptNode* owner = _pin->get_owning_node();

            const Ref<OScriptNodePin> target = owner->find_pin(_descriptor->dependency_pin_name, PD_Input);
            if (target.is_valid() && !target->has_any_connections())
                _button->set_visible(false);

            owner->connect("pin_connected", callable_mp(this, &OrchestratorGraphNodePinNodePath::_pin_connected));
            owner->connect("pin_disconnected", callable_mp(this, &OrchestratorGraphNodePinNodePath::_pin_disconnected));
        }
    }
}

void OrchestratorGraphNodePinNodePath::_bind_methods()
{
    // Register descriptors
    _descriptors.push_back({ "Tween", "tween_property", "property", "object", true, false });
    _descriptors.push_back({ "AnimationMixer", "set_root_motion_track", "path", "", false, true, true });
}

OrchestratorGraphNodePinNodePath::OrchestratorGraphNodePinNodePath(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
    _resolve_descriptor();
}