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
#include "editor/graph/graph_connection_line_style.h"

#include <godot_cpp/classes/curve2d.hpp>
#include <godot_cpp/core/memory.hpp>

namespace {

/// Matches GraphEdit's MAX_CONNECTION_LINE_CURVE_TESSELATION_STAGES.
constexpr int BEZIER_TESSELLATION_STAGES = 5;
/// Matches the tolerance GraphEdit passes to Curve2D::tessellate.
constexpr float BEZIER_TESSELLATION_TOLERANCE = 2.0f;

/// Unscaled horizontal distance an orthogonal wire travels away from a port before it may turn.
constexpr float ORTHOGONAL_STUB_LENGTH = 16.0f;

PackedVector2Array make_straight(const Vector2& p_from, const Vector2& p_to) {
    PackedVector2Array points;
    points.push_back(p_from);
    points.push_back(p_to);
    return points;
}

/// A single straight segment between the two ports.
class StraightStyle : public OrchestratorEditorGraphConnectionLineStyle {
protected:
    PackedVector2Array _build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const override {
        return make_straight(p_from, p_to);
    }
};

/// A cubic bezier whose horizontal control-point offset is a fraction of the horizontal distance; this is the
/// same construction GraphEdit uses internally, so a curvature of 0.5 reproduces the engine's default wire.
class BezierStyle : public OrchestratorEditorGraphConnectionLineStyle {
    float _curvature;

protected:
    PackedVector2Array _build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const override {
        const float x_diff = p_to.x - p_from.x;
        float cp_offset = x_diff * _curvature;
        if (x_diff < 0) {
            cp_offset *= -1;
        }

        Ref<Curve2D> curve;
        curve.instantiate();
        curve->add_point(p_from);
        curve->set_point_out(0, Vector2(cp_offset, 0));
        curve->add_point(p_to);
        curve->set_point_in(1, Vector2(-cp_offset, 0));

        if (_curvature > 0) {
            return curve->tessellate(BEZIER_TESSELLATION_STAGES, BEZIER_TESSELLATION_TOLERANCE);
        }
        return curve->tessellate(1);
    }

public:
    explicit BezierStyle(float p_curvature) : _curvature(p_curvature) { }
};

/// Routes the wire along horizontal and vertical runs, in the manner of a circuit-board trace.
///
/// When the target lies far enough to the right, the wire leaves the source horizontally, turns vertically at
/// the horizontal midpoint and enters the target horizontally. Otherwise the wire leaves by a short stub, runs
/// back at the vertical midpoint and enters the target through a matching stub.
///
/// With chamfered corners, each bend is cut by up to half the length of the shorter adjacent run, so a short
/// vertical collapses into a single 45 degree diagonal while longer runs keep a vertical between two diagonals.
class OrthogonalStyle : public OrchestratorEditorGraphConnectionLineStyle {
    bool _chamfer_corners;

    static PackedVector2Array _route(const Vector2& p_from, const Vector2& p_to, float p_stub) {
        PackedVector2Array points;
        points.push_back(p_from);

        const Vector2 delta = p_to - p_from;
        if (delta.x >= 2 * p_stub) {
            const float mid_x = p_from.x + delta.x * 0.5f;
            points.push_back(Vector2(mid_x, p_from.y));
            points.push_back(Vector2(mid_x, p_to.y));
        } else {
            const float mid_y = p_from.y + delta.y * 0.5f;
            points.push_back(Vector2(p_from.x + p_stub, p_from.y));
            points.push_back(Vector2(p_from.x + p_stub, mid_y));
            points.push_back(Vector2(p_to.x - p_stub, mid_y));
            points.push_back(Vector2(p_to.x - p_stub, p_to.y));
        }

        points.push_back(p_to);
        return points;
    }

    static PackedVector2Array _chamfer(const PackedVector2Array& p_points) {
        const int64_t count = p_points.size();
        if (count < 3) {
            return p_points;
        }

        PackedVector2Array points;
        points.push_back(p_points[0]);

        for (int64_t i = 1; i < count - 1; i++) {
            const Vector2 previous = p_points[i - 1];
            const Vector2 current = p_points[i];
            const Vector2 next = p_points[i + 1];

            const Vector2 to_previous = previous - current;
            const Vector2 to_next = next - current;
            const float cut = MIN(to_previous.length(), to_next.length()) * 0.5f;
            if (Math::is_zero_approx(cut)) {
                points.push_back(current);
                continue;
            }

            points.push_back(current + to_previous.normalized() * cut);
            points.push_back(current + to_next.normalized() * cut);
        }

        points.push_back(p_points[count - 1]);
        return points;
    }

    static PackedVector2Array _remove_duplicates(const PackedVector2Array& p_points) {
        PackedVector2Array points;
        for (const Vector2& point : p_points) {
            if (points.is_empty() || !points[points.size() - 1].is_equal_approx(point)) {
                points.push_back(point);
            }
        }
        return points;
    }

protected:
    PackedVector2Array _build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const override {
        PackedVector2Array points = _remove_duplicates(_route(p_from, p_to, ORTHOGONAL_STUB_LENGTH * p_context.zoom));
        if (_chamfer_corners) {
            points = _remove_duplicates(_chamfer(points));
        }
        return points.size() >= 2 ? points : make_straight(p_from, p_to);
    }

public:
    explicit OrthogonalStyle(bool p_chamfer_corners) : _chamfer_corners(p_chamfer_corners) { }
};

}

PackedVector2Array OrchestratorEditorGraphConnectionLineStyle::build(const Vector2& p_from, const Vector2& p_to, const Context& p_context) const {
    if (p_context.source_is_reroute && p_context.target_is_reroute) {
        return make_straight(p_from, p_to);
    }
    return _build(p_from, p_to, p_context);
}

OrchestratorEditorGraphConnectionLineStyle* OrchestratorEditorGraphConnectionLineStyle::create(const String& p_style, float p_custom_curvature, float p_default_curvature) {
    if (p_style == STYLE_STRAIGHT) {
        return memnew(StraightStyle);
    }
    if (p_style == STYLE_ANGLED_45) {
        return memnew(OrthogonalStyle(true));
    }
    if (p_style == STYLE_ANGLED_90) {
        return memnew(OrthogonalStyle(false));
    }
    if (p_style == STYLE_CUSTOM) {
        return memnew(BezierStyle(p_custom_curvature));
    }
    return memnew(BezierStyle(p_default_curvature));
}