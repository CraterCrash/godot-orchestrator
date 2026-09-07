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

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// Presentation metadata for an annotation shown in the editor.
///
/// Rules live in OScriptAnnotationRegistry. This table only supplies what the inspector displays:
/// a label, a description, and dropdown choices for arguments that are enumerations.
///
struct OrchestratorEditorAnnotationPresentation {
    String label;
    String description;

    /// Looks up the presentation for an annotation, falling back to its name when unlisted.
    static OrchestratorEditorAnnotationPresentation get(const StringName& p_name);

    /// Dropdown choices for a signature argument.
    /// @param p_name the annotation name
    /// @param p_argument_index the signature argument index
    /// @param r_labels the labels to display
    /// @param r_values the stored values, parallel to the labels
    /// @return true when the argument is rendered as a dropdown
    static bool get_argument_choices(const StringName& p_name, int p_argument_index, PackedStringArray& r_labels, Array& r_values);
};