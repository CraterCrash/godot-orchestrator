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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_NODE_PATH_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_NODE_PATH_H

#include "editor/graph/pins/button_base_pin.h"
#include "script/node_pin.h"

class OrchestratorSceneNodeSelector;
class OrchestratorPropertySelector;

/// An implementation of <code>OrchestratorEditorGraphPinButtonBase</code> select a <code>NodePath</code>
///
class OrchestratorEditorGraphPinNodePath : public OrchestratorEditorGraphPinButtonBase
{
    GDCLASS(OrchestratorEditorGraphPinNodePath, OrchestratorEditorGraphPinButtonBase);

    struct DependencyDescriptor
    {
        String class_name;
        String method_name;
        String method_argument_name;
        String property_name;
        String dependency_pin_name;
        bool is_property_selection = false;
        bool is_node_and_property_selection = false;
        bool is_property_optional = false;
    };

    static Vector<DependencyDescriptor> _descriptors;

    Ref<OrchestrationGraphPin> _owning_pin;
    OrchestratorPropertySelector* _property_selector = nullptr;
    OrchestratorSceneNodeSelector* _node_selector = nullptr;
    DependencyDescriptor* _descriptor = nullptr;
    NodePath _node_path;

    DependencyDescriptor* _resolve_descriptor();
    void _configure_descriptor(DependencyDescriptor* p_descriptor);

    bool _is_only_node_selection_required() const;
    Ref<OrchestrationGraphPin> _get_dependency_object_pin();

    void _set_button_state(bool p_disabled, bool p_reset = false);
    void _pin_connected(int p_type, int p_index);
    void _pin_disconnected(int p_type, int p_index);

    void _open_node_selector();
    void _node_selected(const NodePath& p_path);

    void _open_property_selector(Object* p_object = nullptr, const String& p_selected = String());
    void _property_selected(const String& p_property);

protected:
    static void _bind_methods();

    //~ Begin OrchestratorEditorGraphPinButtonBase Interface
    void _handle_selector_button_pressed() override;
    //~ End OrchestratorEditorGraphPinButtonBase Interface

    //~ Begin OrchestratorEditorGraphPin Interface
    void set_pin(const Ref<OrchestrationGraphPin>& p_pin) override;
    //~ End OrchestratorEditorGraphPin Interface

public:

    OrchestratorEditorGraphPinNodePath();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_NODE_PATH_H