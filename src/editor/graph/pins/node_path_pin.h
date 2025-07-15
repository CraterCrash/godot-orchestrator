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

    struct MethodDescriptor
    {
        String class_name;
        String method_name;
        String pin_name;
        String dependency_pin_name;
        bool is_property_selection = false;
        bool is_node_and_property_selection = false;
        bool is_property_optional = false;
    };

    static Vector<MethodDescriptor> _descriptors;

    Ref<OrchestrationGraphPin> _owning_pin;
    OrchestratorPropertySelector* _property_selector = nullptr;
    OrchestratorSceneNodeSelector* _node_selector = nullptr;
    MethodDescriptor* _descriptor = nullptr;
    NodePath _node_path;

    void _open_node_selector();
    void _node_selected(const NodePath& p_path);

    void _open_property_selector(Object* p_object = nullptr, const String& p_selected = String());
    void _property_selected(const String& p_property);

protected:
    static void _bind_methods();

    //~ Begin OrchestratorEditorGraphPinButtonBase Interface
    void _handle_selector_button_pressed() override;
    //~ End OrchestratorEditorGraphPinButtonBase Interface

public:

    void set_owning_pin(const Ref<OrchestrationGraphPin>& p_pin);

    OrchestratorEditorGraphPinNodePath();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_NODE_PATH_H