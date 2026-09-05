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

#include "common/callable_lambda.h"
#include "common/variant_struct_schema.h"
#include "common/variant_utils.h"
#include "core/godot/scene_string_names.h"

#include <string>

#include <godot_cpp/classes/grid_container.hpp>

int OrchestratorEditorGraphPinStruct::_get_grid_columns_for_type(Variant::Type p_type) {
    switch (p_type) {
        case Variant::TRANSFORM3D:
        case Variant::PROJECTION: {
            return 8;
        }
        case Variant::TRANSFORM2D:
        case Variant::AABB:
        case Variant::BASIS: {
            return 6;
        }
        default: {
            return -1;
        }
    }
}

void OrchestratorEditorGraphPinStruct::_update_control_value_part(const String& p_path, int p_index, const Variant& p_value) {
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1) {
        _controls[p_index]->set_text(p_value);
        return;
    }

    Variant part_value = p_value.get(parts[1]);
    _update_control_value_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);
}

void OrchestratorEditorGraphPinStruct::_read_control_value_part(const String& p_path, int p_index, Variant& r_value) {
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1) {
        if (!_controls[p_index]->get_text().is_valid_float()) {
            _controls[p_index]->set_text("0.0");
        }
        r_value = std::stof(_controls[p_index]->get_text().utf8().get_data());
        return;
    }

    Variant part_value = r_value.get(parts[1]);
    _read_control_value_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);

    r_value.set(parts[1], part_value);
}

void OrchestratorEditorGraphPinStruct::_update_control_value(const Variant& p_value) {
    const PropertyInfo property = get_property_info();
    const PackedStringArray property_paths = VariantStructSchema::get_component_paths(property.type);

    Variant value = p_value;

    // If the default value hasn't been set, these pins expect there to be a reasonable value
    // for the given pin type, so we construct the actual value here.
    // todo: could we rely on the generated value by chance?
    if (value.get_type() == Variant::NIL) {
        value = VariantUtils::make_default(property.type);
    }

    for (int i = 0; i < property_paths.size(); i++) {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        Variant part_value = value.get(property_path_parts[0]);
        _update_control_value_part(property_path, i, part_value);
    }
}

Variant OrchestratorEditorGraphPinStruct::_read_control_value() {
    const PropertyInfo property = get_property_info();

    Variant pin_value = _get_default_value();
    if (property.type == Variant::NIL) {
        pin_value = VariantUtils::make_default(property.type);
    }

    const PackedStringArray property_paths = VariantStructSchema::get_component_paths(property.type);
    for (int i = 0; i < property_paths.size(); i++) {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        Variant value = pin_value.get(property_path_parts[0]);
        _read_control_value_part(property_path, i, value);
        pin_value.set(property_path_parts[0], value);
    }

    return pin_value;
}

Control* OrchestratorEditorGraphPinStruct::_create_default_value_widget() {
    const PropertyInfo property = get_property_info();
    const PackedStringArray property_paths = VariantStructSchema::get_component_paths(property.type);

    GridContainer* container = memnew(GridContainer);
    container->set_h_size_flags(SIZE_SHRINK_BEGIN);

    // Specific data types have different layouts
    const int grid_columns = _get_grid_columns_for_type(property.type);
    container->set_columns(grid_columns != -1 ? grid_columns : static_cast<int>(property_paths.size()) * 2);

    for (int i = 0; i < property_paths.size(); i++) {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        String label_text;
        for (const String& part : property_path_parts) {
            label_text += part.substr(0, 1).capitalize();
        }

        Label* label = memnew(Label);
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        label->set_text(label_text);
        container->add_child(label);

        LineEdit* line_edit = memnew(LineEdit);
        line_edit->set_expand_to_text_length_enabled(true);
        line_edit->set_select_all_on_focus(true);
        line_edit->add_theme_constant_override("minimum_character_width", 0);
        line_edit->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [&] { _default_value_changed(); }));
        line_edit->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [&] (const String& value) { _default_value_changed(); }));
        container->add_child(line_edit);

        _controls.push_back(line_edit);
    }

    if (property.type == Variant::TRANSFORM3D) {
        // Rework layout for TRANSFORM3D so that the fields are
        // BXX BXY BXZ OX
        // BYX BYY BYZ OY
        // BZX BZY BZZ OZ
        container->move_child(container->get_child(18), 6);
        container->move_child(container->get_child(19), 7);
        container->move_child(container->get_child(20), 14);
        container->move_child(container->get_child(21), 15);
    } else if (property.type == Variant::TRANSFORM2D) {
        // Rework layout for TRANSFORM2D so that the fields are
        // XX XY OX
        // YX YY OY
        container->move_child(container->get_child(8), 4);
        container->move_child(container->get_child(9), 5);
    }

    return container;
}
