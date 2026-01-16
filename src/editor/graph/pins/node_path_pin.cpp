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
#include "editor/graph/pins/node_path_pin.h"

#include "common/macros.h"
#include "common/string_utils.h"
#include "editor/dialogs_helper.h"
#include "editor/graph/graph_node.h"
#include "editor/property_selector.h"
#include "editor/scene_node_selector.h"
#include "script/nodes/functions/call_member_function.h"
#include "script/nodes/properties/property_set.h"
#include "script/nodes/utilities/self.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

Vector<OrchestratorEditorGraphPinNodePath::DependencyDescriptor> OrchestratorEditorGraphPinNodePath::_descriptors;

OrchestratorEditorGraphPinNodePath::DependencyDescriptor* OrchestratorEditorGraphPinNodePath::_resolve_descriptor() {
    // Note: The parent type's pin is not yet set, so don't use any parent methods here
    ERR_FAIL_COND_V(!_owning_pin.is_valid(), nullptr);
    OScriptNode* owning_node = _owning_pin->get_owning_node();

    if (OScriptNodeCallMemberFunction* cmf = cast_to<OScriptNodeCallMemberFunction>(owning_node)) {
        const String target_class = cmf->get_target_class();
        const MethodInfo method = cmf->get_function();

        for (DependencyDescriptor& descriptor : _descriptors) {
            if (!target_class.match(descriptor.class_name)) {
                continue;
            }
            if (!method.name.match(descriptor.method_name)) {
                continue;
            }
            if (!_owning_pin->get_pin_name().match(descriptor.method_argument_name)) {
                continue;
            }
            return &descriptor;
        }
    } else if (OScriptNodePropertySet* ps = cast_to<OScriptNodePropertySet>(owning_node)) {
        if (ps->get_call_mode() == OScriptNodePropertySet::CALL_INSTANCE) {
            for (DependencyDescriptor& descriptor : _descriptors) {
                if (!ps->get_base_type().match(descriptor.class_name)) {
                    continue;
                }
                if (!ps->get_property().name.match(descriptor.property_name)) {
                    continue;
                }
                return &descriptor;
            }
        }
    }

    return nullptr;
}

void OrchestratorEditorGraphPinNodePath::_configure_descriptor(DependencyDescriptor* p_descriptor) {
    _descriptor = p_descriptor;

    if (_descriptor) {
        OScriptNode* owning_node = _owning_pin->get_owning_node();
        const Ref<OScriptNodePin> dep_pin = owning_node->find_pin(_descriptor->dependency_pin_name, PD_Input);
        if (dep_pin.is_valid() && !dep_pin->has_any_connections()) {
            _set_button_state(true);
        }
        owning_node->connect("pin_connected", callable_mp_this(_pin_connected));
        owning_node->connect("pin_disconnected", callable_mp_this(_pin_disconnected));
    }
}

bool OrchestratorEditorGraphPinNodePath::_is_only_node_selection_required() const {
    return !_descriptor || (_descriptor && !_descriptor->is_node_and_property_selection); // NOLINT
}

Ref<OrchestrationGraphPin> OrchestratorEditorGraphPinNodePath::_get_dependency_object_pin() {
    if (!_descriptor || _owning_pin.is_null()) {
        return {};
    }
    return _owning_pin->get_owning_node()->find_pin(_descriptor->dependency_pin_name, PD_Input);
}

void OrchestratorEditorGraphPinNodePath::_set_button_state(bool p_disabled, bool p_reset) {
    _set_button_disabled(p_disabled);

    const String tooltip = p_disabled
        ? vformat("Connect the %s pin to pick a %s",
            _descriptor->dependency_pin_name,
            StringUtils::default_if_empty(_descriptor->method_argument_name, _descriptor->property_name))
        : "";
    _set_button_tooltip(tooltip);

    if (p_reset) {
        _set_default_value(Variant());
    }
}

void OrchestratorEditorGraphPinNodePath::_pin_connected(int p_type, int p_index) {
    ERR_FAIL_COND(!_owning_pin.is_valid());

    OScriptNode* node = _owning_pin->get_owning_node();
    ERR_FAIL_NULL(node);

    const Ref<OScriptNodePin> pin = node->find_pin(p_index, static_cast<EPinDirection>(p_type));
    if (_descriptor && pin.is_valid() && pin->get_pin_name().match(_descriptor->dependency_pin_name)) {
        _set_button_state(false);
    }
}

void OrchestratorEditorGraphPinNodePath::_pin_disconnected(int p_type, int p_index) {
    ERR_FAIL_COND(!_owning_pin.is_valid());

    OScriptNode* node = _owning_pin->get_owning_node();
    ERR_FAIL_NULL(node);

    const Ref<OScriptNodePin> pin = node->find_pin(p_index, static_cast<EPinDirection>(p_type));
    if (_descriptor && pin.is_valid() && pin->get_pin_name().match(_descriptor->dependency_pin_name)) {
        _set_button_state(true, true);
    }
}

void OrchestratorEditorGraphPinNodePath::_open_node_selector() {
    // Resolve the selected node
    // When Godot's implementation eventually supports this (https://github.com/godotengine/godot/pull/94323),
    // then we can use the default Godot API
    Node* selected = nullptr;

    const NodePath path = _read_control_value();
    if (!path.is_empty()) {
        if (Node* root = get_tree()->get_edited_scene_root()) {
            selected = root->get_node_or_null(path);
        }
    }

    _node_selector = memnew(OrchestratorSceneNodeSelector);
    _node_selector->set_selected(selected);
    _node_selector->connect("node_selected", callable_mp_this(_node_selected));

    EI->popup_dialog_centered_clamped(_node_selector, Size2(350, 700) * EDSCALE);
}

void OrchestratorEditorGraphPinNodePath::_node_selected(const NodePath& p_path) {
    if (p_path.is_empty()) {
        // Nothing should be done, leave unchanged.
        _node_selector->queue_free();
        _node_selector = nullptr;
        return;
    }

    if (_is_only_node_selection_required()) {
        _handle_selector_button_response(p_path);
        return;
    }

    // User is selecting a property from a given node path
    Node* node = get_tree()->get_edited_scene_root()->get_node_or_null(p_path);
    if (!node) {
        return;
    }

    String value = _read_control_value();
    if (!value.is_empty() && value.contains(":")) {
        value = value.substr(value.find(":") + 1);
    } else {
        value = "";
    }

    _node_path = p_path;
    _open_property_selector(node, value);
}

void OrchestratorEditorGraphPinNodePath::_open_property_selector(Object* p_object, const String& p_selected) {
    String value = p_selected;

    if (p_object == nullptr) {
        const Ref<OrchestrationGraphPin> pin = _get_dependency_object_pin();
        if (!pin.is_valid() || !pin->has_any_connections()) {
            return;
        }

        const Vector<Ref<OScriptNodePin>> connections = pin->get_connections();
        const Ref<OScriptNodePin> connection = !connections.is_empty() ? connections[0] : Ref<OScriptNodePin>();
        if (!connection.is_valid()) {
            return;
        }

        const Ref<OScriptTargetObject> target = connection->resolve_target();
        if (!target.is_valid() || !target->has_target()) {
            // In the event that the self node cannot be resolved, inform user about the edited scene needing to
            // include the reference to the node with the Orchestration for this lookup to resolve correctly.
            if (cast_to<OScriptNodeSelf>(connection->get_owning_node())) {
                OrchestratorEditorDialogs::error(
                    "This orchestration is not attached to any node in the current edited\n"
                    "scene, so the reference cannot be resolved and no properties selected.");
            }
            return;
        }

        p_object = target->get_target();

        value = _read_control_value();
        if (!value.is_empty() && value.begins_with(":")) {
            value = value.substr(1);
        }
    }

    _property_selector = memnew(OrchestratorPropertySelector);
    _property_selector->connect("selected", callable_mp_this(_property_selected));
    add_child(_property_selector);

    _property_selector->select_property_from_instance(p_object, value);
}

void OrchestratorEditorGraphPinNodePath::_property_selected(const String& p_property) {
    _handle_selector_button_response(vformat("%s:%s", _node_path, p_property));
}

void OrchestratorEditorGraphPinNodePath::_handle_selector_button_pressed() {
    _node_path = "";

    if (!_descriptor || _descriptor->is_node_and_property_selection) {
        _open_node_selector();
    } else if (_descriptor->is_property_selection) {
        _open_property_selector();
    }
}

void OrchestratorEditorGraphPinNodePath::set_pin(const Ref<OrchestrationGraphPin>& p_pin) {
    _owning_pin = p_pin;

    // This needs to be resolved before setting parent pin type
    _descriptor = _resolve_descriptor();
    parent_type::set_pin(p_pin);

    // Now configure the descriptor behavior
    _configure_descriptor(_descriptor);
}

void OrchestratorEditorGraphPinNodePath::_bind_methods() {
    // Method Overrides
    _descriptors.push_back({ "Tween", "tween_property", "property", "", "object", true, false });
    _descriptors.push_back({ "AnimationMixer", "set_root_motion_track", "path", "", "", false, true, true });

    // Property Overrides
    _descriptors.push_back({ "AnimationMixer", "", "", "root_motion_track", "", false, true, true });
}

OrchestratorEditorGraphPinNodePath::OrchestratorEditorGraphPinNodePath() {
    set_default_text("Assign...");
}