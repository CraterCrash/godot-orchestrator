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

#include "orchestration/node_pin.h"

#include <godot_cpp/classes/h_box_container.hpp>

using namespace godot;

/// Base class for all pin default-value editors.
///
/// Extends HBoxContainer so child widgets are automatically laid out side-by-side. A value
/// editor owns the UI for one pin's default value and has no knowledge of graph nodes,
/// connections, or the broader pin lifecycle: it receives the current value via set_value(),
/// presents it to the user, and emits value_changed() when the user edits it.
///
class OrchestratorEditorGraphPinValueEditor : public HBoxContainer {
    GDCLASS(OrchestratorEditorGraphPinValueEditor, HBoxContainer);

protected:
    static void _bind_methods();

    void _emit_value_changed(const Variant& p_value);

public:
    /// Called once at creation (and again when the effective type changes for Any pins).
    /// @param p_property the property information for the pin
    virtual void configure(const PropertyInfo& p_property) { }

    /// Update the displayed value to match the model.
    /// @param p_value the model's value
    virtual void set_value(const Variant& p_value) { }

    /// Return if this editor prefers rendering below the label.
    /// @return {@code true} if the editor should be rendered below the label, {@code false} otherwise.
    virtual bool is_below_label() const { return false; }

    /// Return whether this editor prefers being before the icon/label.
    /// @return {@code true} if the editor should be placed before icon/label, {@code false} otherwise.
    virtual bool prefers_leading_placement() const { return false; }

    /// Notify the editor when the owning pin's connected state changes.
    /// @param p_linked whether the owning pin was connected or disconnected
    virtual void set_linked(bool p_linked) { }

    /// Supply the model pin for editors that require direct model access (NodePath, Any).
    /// @param p_pin the pin reference, if applicable.
    virtual void set_pin_ref(const Ref<OrchestrationGraphPin>& p_pin) { }
};
