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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_STRUCT_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_STRUCT_H

#include "editor/graph/graph_node_pin.h"

/// Forward declarations
namespace godot { class LineEdit; }

/// An implementation of OrchestratorGraphNodePin for struct-like types that are composed of components
/// that are either indexed, meaning they all have the same type, or that are not which may be
/// composed of different sub-types.
class OrchestratorGraphNodePinStruct : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinStruct, OrchestratorGraphNodePin);

    static void _bind_methods();

    Vector<LineEdit*> _edits;   //! Line edits for each sub-component

    /// Calculates the number of grid columns for the pin type
    /// @param p_type the pin type
    /// @return the number of grid columns
    int _get_grid_columns(Variant::Type p_type) const;

    /// Checks whether a specific property for a given type is excluded from the property paths
    /// @param p_type the property type
    /// @param p_property the property
    /// @return true if the property is excluded, false otherwise
    bool _is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property) const;

    /// Gets the variant's value for a given property path designation from the UI
    /// @param p_path the property name, i.e. "position.x"
    /// @param p_index the editor widget index the value will be read from post traversal
    /// @param r_value the returned value for the top-level property
    void _get_ui_value_by_property_path(const String& p_path, int p_index, Variant& r_value);

    /// Setes the variant's value for a given property path designation to the UI
    /// @param p_path the property name, i.e. "position.x"
    /// @param p_index the editor's widget index the value will be written to post traversal
    /// @param p_value the initial value passed down via traversal
    void _set_ui_value_by_property_path(const String& p_path, int p_index, const Variant& p_value);

    /// For a given type, returns an array of property paths that make up the given type.
    /// @param p_type the property type
    /// @return packed string array of all valid property paths
    PackedStringArray _get_property_paths(Variant::Type p_type) const;

protected:
    OrchestratorGraphNodePinStruct() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    bool _render_default_value_below_label() const override { return true; }
    //~ End OrchestratorGraphNodePin Interface

    /// Dispatched when the edit control receives focus
    /// @param p_index the line edit control index
    void _on_focus_entered(int p_index);

    /// Dispatched when the edit control looses focus
    /// @param p_index the line edit control index
    void _on_focus_exited(int p_index);

    /// Dispatched when the text is submitted in a field
    void _on_text_submitted();

    /// Sets the default value from the all collective line edit controls
    void _set_default_value_from_line_edits();

public:
    /// Constructs the pin object
    /// @param p_node the editor graph node, should never be null
    /// @param p_pin the script pin object, should never be null
    OrchestratorGraphNodePinStruct(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif // ORCHESTRATOR_EDITOR_SCRIPT_NODE_PIN_STRUCT_H