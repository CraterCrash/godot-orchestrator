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
#include "editor/graph/pins/struct_pin.h"

#include "api/extension_db.h"
#include "common/callable_lambda.h"
#include "common/variant_utils.h"

#include <string>
#include <godot_cpp/classes/grid_container.hpp>

int OrchestratorEditorGraphPinStruct::_get_grid_columns_for_type(Variant::Type p_type)
{
    switch (p_type)
    {
        case Variant::TRANSFORM3D:
        case Variant::PROJECTION:
            return 8;
        case Variant::TRANSFORM2D:
        case Variant::AABB:
        case Variant::BASIS:
            return 6;
        default:
            return -1;
    }
}

bool OrchestratorEditorGraphPinStruct::_is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property)
{
    switch (p_type)
    {
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::AABB:
            return p_property.name.match("end");
        case Variant::PLANE:
            return p_property.name.match("normal");
        default:
            return false;
    }
}

PackedStringArray OrchestratorEditorGraphPinStruct::_get_property_paths(Variant::Type p_type)
{
    PackedStringArray results;

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    if (!type.properties.is_empty())
    {
        for (const PropertyInfo& property : type.properties)
        {
            if (_is_property_excluded(p_type, property))
                continue;

            PackedStringArray sub_parts = _get_property_paths(property.type);
            if (sub_parts.is_empty())
            {
                results.push_back(property.name);
                continue;
            }

            for (const String& sub_part :sub_parts)
                results.push_back(vformat("%s.%s", property.name, sub_part));
        }
    }

    return results;
}

void OrchestratorEditorGraphPinStruct::_update_control_value_part(const String& p_path, int p_index, const Variant& p_value)
{
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1)
    {
        _controls[p_index]->set_text(p_value);
        return;
    }

    Variant part_value = p_value.get(parts[1]);
    _update_control_value_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);
}

void OrchestratorEditorGraphPinStruct::_read_control_value_part(const String& p_path, int p_index, Variant& r_value)
{
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1)
    {
        if (!_controls[p_index]->get_text().is_valid_float())
            _controls[p_index]->set_text("0.0");

        r_value = std::stof(_controls[p_index]->get_text().utf8().get_data());
        return;
    }

    Variant part_value = r_value.get(parts[1]);
    _read_control_value_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);

    r_value.set(parts[1], part_value);
}

void OrchestratorEditorGraphPinStruct::_update_control_value(const Variant& p_value)
{
    const PropertyInfo property = get_property_info();
    const PackedStringArray property_paths = _get_property_paths(property.type);

    Variant value = p_value;

    // If the default value hasn't been set, these pins expect there to be a reasonable value
    // for the given pin type, so we construct the actual value here.
    // todo: could we rely on the generated value by chance?
    if (value.get_type() == Variant::NIL)
        value = VariantUtils::make_default(property.type);

    for (int i = 0; i < property_paths.size(); i++)
    {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        Variant part_value = value.get(property_path_parts[0]);
        _update_control_value_part(property_path, i, part_value);
    }
}

Variant OrchestratorEditorGraphPinStruct::_read_control_value()
{
    const PropertyInfo property = get_property_info();

    Variant pin_value = _get_default_value();
    if (property.type == Variant::NIL)
        pin_value = VariantUtils::make_default(property.type);

    const PackedStringArray property_paths = _get_property_paths(property.type);
    for (int i = 0; i < property_paths.size(); i++)
    {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        Variant value = pin_value.get(property_path_parts[0]);
        _read_control_value_part(property_path, i, value);
        pin_value.set(property_path_parts[0], value);
    }

    return pin_value;
}

Control* OrchestratorEditorGraphPinStruct::_create_default_value_widget()
{
    const PropertyInfo property = get_property_info();
    const PackedStringArray property_paths = _get_property_paths(property.type);

    GridContainer* container = memnew(GridContainer);
    container->set_h_size_flags(SIZE_SHRINK_BEGIN);

    // Specific data types have different layouts
    const int grid_columns = _get_grid_columns_for_type(property.type);
    container->set_columns(grid_columns != -1 ? grid_columns : static_cast<int>(property_paths.size()) * 2);

    for (int i = 0; i < property_paths.size(); i++)
    {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        String label_text;
        for (const String& part : property_path_parts)
            label_text += part.substr(0, 1).capitalize();

        Label* label = memnew(Label);
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        label->set_text(label_text);
        container->add_child(label);

        LineEdit* line_edit = memnew(LineEdit);
        line_edit->set_expand_to_text_length_enabled(true);
        line_edit->set_select_all_on_focus(true);
        line_edit->add_theme_constant_override("minimum_character_width", 0);
        line_edit->connect("focus_exited", callable_mp_lambda(this, [&] { _default_value_changed(); }));
        line_edit->connect("text_submitted", callable_mp_lambda(this, [&] { _default_value_changed(); }));
        container->add_child(line_edit);

        _controls.push_back(line_edit);
    }

    if (property.type == Variant::TRANSFORM3D)
    {
        // Rework layout for TRANSFORM3D so that the fields are
        // BXX BXY BXZ OX
        // BYX BYY BYZ OY
        // BZX BZY BZZ OZ
        container->move_child(container->get_child(18), 6);
        container->move_child(container->get_child(19), 7);
        container->move_child(container->get_child(20), 14);
        container->move_child(container->get_child(21), 15);
    }
    else if (property.type == Variant::TRANSFORM2D)
    {
        // Rework layout for TRANSFORM2D so that the fields are
        // XX XY OX
        // YX YY OY
        container->move_child(container->get_child(8), 4);
        container->move_child(container->get_child(9), 5);
    }

    return container;
}

// #include "common/defs.h"
// #include "editor/graph/editor_graph_pin_factory.h"
//
// #include <godot_cpp/classes/label.hpp>
//
// void OrchestratorEditorGraphPinStruct::_update_control_value()
// {
//     for (int i = 0; i < _descriptor.fields.size(); i++)
//     {
//         Variant field_value = _descriptor.fields[i].getter(_value);
//         _editors[i]->set_default_value(field_value);
//     }
// }
//
// Variant OrchestratorEditorGraphPinStruct::_read_control_value()
// {
//     Variant current_value = _value;
//     for (int i = 0; i < _descriptor.fields.size(); i++)
//     {
//         Variant field_value = _editors[i]->get_default_value();
//         _descriptor.fields[i].setter(current_value, field_value);
//     }
//     return current_value;
// }
//
// void OrchestratorEditorGraphPinStruct::set_descriptor(EResolvedOrchestrationGraphPinType p_type)
// {
//     for (int i = _control->get_child_count() - 1; i >= 0; i--)
//     {
//         Node* child = _control->get_child(i);
//         _control->remove_child(child);
//         child->queue_free();
//     }
//
//     _editors.clear();
//
//     auto it = _descriptors.find(p_type);
//     if (it == _descriptors.end())
//         CRASH_NOW_MSG("No struct pin definition defined for pin type " + itos(static_cast<int>(p_type)));
//
//     _descriptor = it->second;
//     _type = p_type;
//
//     for (int i = 0; i < _descriptor.fields.size(); i++)
//     {
//         const StructField& field = _descriptor.fields[i];
//
//         Label* label = memnew(Label);
//         label->set_text(field.label);
//         label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
//         _control->add_child(label);
//
//         OrchestratorEditorGraphPinBase* widget = OrchestratorEditorGraphPinWidgetFactory::make_widget(field.type);
//         _control->add_child(widget);
//         _editors.push_back(widget);
//
//         widget->set_default_value(field.getter(_value));
//         widget->connect("default_value_changed", callable_mp_parent(_default_value_changed));
//     }
// }
//
// OrchestratorEditorGraphPinStruct::OrchestratorEditorGraphPinStruct()
// {
//     _control = memnew(GridContainer);
//     _control->set_h_size_flags(SIZE_EXPAND_FILL);
//     _type = GPT_Unknown;
// }
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector2Desc = { {
//     { "X", GPT_Float, [](auto& v) { return Vector2(v).x; }, [](auto& v, auto& c) { c = Vector2(v).x; } },
//     { "Y", GPT_Float, [](auto& v) { return Vector2(v).y; }, [](auto& v, auto& c) { c = Vector2(v).y; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector2iDesc = { {
//     { "X", GPT_Integer, [](auto& v) { return Vector2i(v).x; }, [](auto& v, auto& c) { c = Vector2i(v).x; } },
//     { "Y", GPT_Integer, [](auto& v) { return Vector2i(v).y; }, [](auto& v, auto& c) { c = Vector2i(v).y; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector3Desc = { {
//     { "X", GPT_Float, [](auto& v) { return Vector3(v).x; }, [](auto& v, auto& c) { c = Vector3(v).x; } },
//     { "Y", GPT_Float, [](auto& v) { return Vector3(v).y; }, [](auto& v, auto& c) { c = Vector3(v).y; } },
//     { "Z", GPT_Float, [](auto& v) { return Vector3(v).z; }, [](auto& v, auto& c) { c = Vector3(v).z; } }
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector3iDesc = { {
//     { "X", GPT_Integer, [](auto& v) { return Vector3i(v).x; }, [](auto& v, auto& c) { c = Vector3i(v).x; } },
//     { "Y", GPT_Integer, [](auto& v) { return Vector3i(v).y; }, [](auto& v, auto& c) { c = Vector3i(v).y; } },
//     { "Z", GPT_Integer, [](auto& v) { return Vector3i(v).z; }, [](auto& v, auto& c) { c = Vector3i(v).z; } }
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector4Desc = { {
//     { "X", GPT_Float, [](auto& v) { return Vector4(v).x; }, [](auto& v, auto& c) { c = Vector4(v).x; } },
//     { "Y", GPT_Float, [](auto& v) { return Vector4(v).y; }, [](auto& v, auto& c) { c = Vector4(v).y; } },
//     { "Z", GPT_Float, [](auto& v) { return Vector4(v).z; }, [](auto& v, auto& c) { c = Vector4(v).z; } },
//     { "W", GPT_Float, [](auto& v) { return Vector4(v).w; }, [](auto& v, auto& c) { c = Vector4(v).w; } }
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Vector4iDesc = { {
//     { "X", GPT_Integer, [](auto& v) { return Vector4i(v).x; }, [](auto& v, auto& c) { c = Vector4i(v).x; } },
//     { "Y", GPT_Integer, [](auto& v) { return Vector4i(v).y; }, [](auto& v, auto& c) { c = Vector4i(v).y; } },
//     { "Z", GPT_Integer, [](auto& v) { return Vector4i(v).z; }, [](auto& v, auto& c) { c = Vector4i(v).z; } },
//     { "W", GPT_Integer, [](auto& v) { return Vector4i(v).w; }, [](auto& v, auto& c) { c = Vector4(v).w; } }
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Rect2Desc = { {
//     { "Position", GPT_Vector2, [](auto& v) { return Rect2(v).position; }, [](auto& v, auto& c) { c = Rect2(v).position; } },
//     { "Size", GPT_Vector2, [](auto& v) { return Rect2(v).size; }, [](auto& v, auto& c) { c = Rect2(v).size; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Rect2iDesc = { {
//     { "Position", GPT_Vector2i, [](auto& v) { return Rect2i(v).position; }, [](auto& v, auto& c) { c = Rect2i(v).position; } },
//     { "Size", GPT_Vector2i, [](auto& v) { return Rect2i(v).size; }, [](auto& v, auto& c) { c = Rect2i(v).size; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Transform2dDesc = { {
//     { "X Axis", GPT_Vector2, [](auto& v) { return Transform2D(v)[0]; }, [](auto& v, auto& c) { c = Transform2D(v)[0]; } },
//     { "Y Axis", GPT_Vector2, [](auto& v) { return Transform2D(v)[1]; }, [](auto& v, auto& c) { c = Transform2D(v)[1]; } },
//     { "Origin", GPT_Vector2, [](auto& v) { return Transform2D(v)[2]; }, [](auto& v, auto& c) { c = Transform2D(v)[2]; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::Transform3dDesc = { {
//     { "Basis", GPT_Basis, [](auto& v) { return Transform3D(v).get_basis(); }, [](auto& v, auto& c) { c = Transform3D(v).get_basis(); } },
//     { "Origin", GPT_Vector3, [](auto& v) { return Transform3D(v).get_origin(); }, [](auto& v, auto& c) { c = Transform3D(v).get_origin(); } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::PlaneDesc = { {
//     { "Normal", GPT_Vector3, [](auto& v) { return Plane(v).get_normal(); }, [](auto& v, auto& c) { c = Plane(v).get_normal(); } },
//     { "Distance", GPT_Float, [](auto& v) { return Plane(v).d(); }, [](auto& v, auto& c) { c = Plane(v).d(); } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::QuaternionDesc = { {
//     { "X", GPT_Float, [](auto& v) { return Quaternion(v).x; }, [](auto& v, auto& c) { c = Quaternion(v).x; } },
//     { "Y", GPT_Float, [](auto& v) { return Quaternion(v).y; }, [](auto& v, auto& c) { c = Quaternion(v).y; } },
//     { "Z", GPT_Float, [](auto& v) { return Quaternion(v).z; }, [](auto& v, auto& c) { c = Quaternion(v).z; } },
//     { "W", GPT_Float, [](auto& v) { return Quaternion(v).w; }, [](auto& v, auto& c) { c = Quaternion(v).w; } }
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::AabbDesc = { {
//     { "Position", GPT_Vector3, [](auto& v) { return AABB(v).position; }, [](auto& v, auto& c) { c = AABB(v).position; } },
//     { "Size", GPT_Vector3, [](auto& v) { return AABB(v).size; }, [](auto& v, auto& c) { c = AABB(v).size; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::BasisDesc = { {
//     { "X Axis", GPT_Vector3, [](auto& v) { return Basis(v)[0]; }, [](auto& v, auto& c) { c = Basis(v)[0]; } },
//     { "Y Axis", GPT_Vector3, [](auto& v) { return Basis(v)[1]; }, [](auto& v, auto& c) { c = Basis(v)[1]; } },
//     { "Z Axis", GPT_Vector3, [](auto& v) { return Basis(v)[2]; }, [](auto& v, auto& c) { c = Basis(v)[2]; } },
// } };
//
// const OrchestratorEditorGraphPinStruct::StructDescriptor OrchestratorEditorGraphPinStruct::ProjectionDesc = { {
//     { "X Axis", GPT_Vector4, [](auto& v) { return Projection(v)[0]; }, [](auto& v, auto& c) { c = Projection(v)[0]; } },
//     { "Y Axis", GPT_Vector4, [](auto& v) { return Projection(v)[1]; }, [](auto& v, auto& c) { c = Projection(v)[1]; } },
//     { "Z Axis", GPT_Vector4, [](auto& v) { return Projection(v)[2]; }, [](auto& v, auto& c) { c = Projection(v)[2]; } },
//     { "W Axis", GPT_Vector4, [](auto& v) { return Projection(v)[3]; }, [](auto& v, auto& c) { c = Projection(v)[3]; } },
// } };
//
// const std::unordered_map<int, OrchestratorEditorGraphPinStruct::StructDescriptor>
//     OrchestratorEditorGraphPinStruct::_descriptors = {
//         { GPT_Vector2, Vector2Desc },
//         { GPT_Vector2i, Vector2iDesc },
//         { GPT_Vector3, Vector3Desc },
//         { GPT_Vector3i, Vector3iDesc },
//         { GPT_Vector4, Vector4Desc },
//         { GPT_Vector4i, Vector4iDesc },
//         { GPT_Rect2, Rect2Desc },
//         { GPT_Rect2i, Rect2iDesc },
//         { GPT_Transform2d, Transform2dDesc },
//         { GPT_Transform3d, Transform3dDesc },
//         { GPT_Plane, PlaneDesc },
//         { GPT_Quaternion, QuaternionDesc },
//         { GPT_Aabb, AabbDesc },
//         { GPT_Basis, BasisDesc },
//         { GPT_Projection, ProjectionDesc }
//     };