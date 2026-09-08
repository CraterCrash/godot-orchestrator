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

#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

/// Strategy that shapes the polyline drawn between two connected ports in the graph panel.
///
/// The <code>GraphEdit</code> control asks for a polyline through <code>_get_connection_line</code> and uses
/// the returned points for the wire, its hover hit-testing, rubber-band selection, the minimap and the
/// drag preview. A style therefore only needs to produce points; the engine renders them.
///
/// Endpoints arrive in zoom-scaled screen space for the wire and drag preview, but in graph space for the
/// minimap. Any fixed pixel distances a style uses should be multiplied by <code>Context::zoom</code>.
///
class OrchestratorEditorGraphConnectionLineStyle {
public:
    /// Per-connection details that influence the generated polyline.
    struct Context {
        float zoom = 1.f;
        bool source_is_reroute = false;
        bool target_is_reroute = false;
    };

    static constexpr const char* STYLE_DEFAULT = "Default";         // Default Godot style
    static constexpr const char* STYLE_STRAIGHT = "Straight";
    static constexpr const char* STYLE_ANGLED_45 = "45 Degrees";
    static constexpr const char* STYLE_ANGLED_90 = "90 Degrees";
    static constexpr const char* STYLE_CUSTOM = "Custom";           // user-specified curvature

protected:
    /// Produces the polyline for the given endpoints; implemented by each concrete style.
    virtual PackedVector2Array _build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const = 0;

public:
    /// Produces the polyline for the given endpoints.
    ///
    /// A wire that runs between two reroute nodes is always a straight segment, regardless of style, so that
    /// reroutes remain the user's tool for placing bends.
    PackedVector2Array build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const;

    /// Creates the style that corresponds to a <code>connection_line_style</code> setting value.
    ///
    /// @param p_style the setting value, one of the <code>STYLE_*</code> constants
    /// @param p_custom_curvature the curvature used by the custom style
    /// @param p_default_curvature the curvature used by the default style, taken from the graph edit control
    /// @return the style, never <code>null</code>; unknown values fall back to the default style
    static OrchestratorEditorGraphConnectionLineStyle* create(const String& p_style, float p_custom_curvature, float p_default_curvature);

    virtual ~OrchestratorEditorGraphConnectionLineStyle() = default;
};