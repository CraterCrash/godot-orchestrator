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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H

#include "editor/graph/graph_node_pin.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/script.hpp>

/// Forward declarations
class OrchestratorPropertySelector;
class OrchestratorSceneNodeSelector;

/// An implementation of OrchestratorGraphNodePin for node-path pin types.
class OrchestratorGraphNodePinNodePath : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinNodePath, OrchestratorGraphNodePin);
    static void _bind_methods();

    struct MethodDescriptor
    {
        String class_name;
        String method_name;
        String pin_name;
        String dependency_pin_name;

        bool is_property_selection{ false };

        bool is_node_and_property_selection{ false };
        bool is_property_optional{ false };
    };

    static Vector<MethodDescriptor> _descriptors;

protected:
    const String DEFAULT_TEXT{ "Assign..." };
    OrchestratorPropertySelector* _property_selector{ nullptr };
    OrchestratorSceneNodeSelector* _node_selector{ nullptr };
    MethodDescriptor* _descriptor{ nullptr };
    Button* _button{ nullptr };  //! The button widget
    Button* _reset_button{ nullptr };
    NodePath _sequence_node_path;

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Checks whether this pin has a descriptor, setting it if applicable.
    void _resolve_descriptor();

    /// Checks whether this pin has a descriptor assigned
    /// @return true if there is a descriptor, false otherwise
    _FORCE_INLINE_ bool _has_descriptor() const { return _descriptor != nullptr; }

    /// There are several dialog sequences that a pin node can execute, which includes showing just
    /// the node selection, the property selection, or both in sequential order.  This method is
    /// responsible for setting up that sequence context.
    void _start_dialog_sequence();

    /// Show a dialog popup to select a given scene node
    void _show_node_dialog();

    /// Handles when the user selects a given node in the node dialog
    /// @param p_path the node path of the selected node; empty if canceled
    void _node_selected(const NodePath& p_path);

    /// Show a dialog popup to select properties
    void _show_property_dialog();

    /// Handles a property selection in the property dialog
    /// @param p_name the property name
    void _property_selected(const String& p_name);

    /// Displays a property list dialog for the given object, with optional selected value
    /// @param p_object the object to display properties for
    /// @param p_selected_value the current selected property
    void _show_property_dialog_for_object(Object* p_object, const String& p_selected_value = "");

    /// Resets the pin's state to its default
    void _reset();

    /// Sets the pin value
    /// @param p_pin_value the new pin value
    void _set_pin_value(const Variant& p_pin_value);

    /// Called when a pin is connected on the owning node
    /// @param p_type the pin type
    /// @param p_index the pin index
    void _pin_connected(int p_type, int p_index);

    /// Called when a pin is disconnected on the owning node
    /// @param p_type the pin type
    /// @param p_index the pin index
    void _pin_disconnected(int p_type, int p_index);

    OrchestratorGraphNodePinNodePath() = default;

public:
    OrchestratorGraphNodePinNodePath(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_NODE_PATH_H
