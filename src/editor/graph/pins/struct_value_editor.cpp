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
#include "editor/graph/pins/struct_value_editor.h"

#include "api/extension_db.h"
#include "common/callable_lambda.h"
#include "common/variant_utils.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"

#include <string>

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/label.hpp>

int OrchestratorEditorGraphPinValueEditorStruct::_get_grid_columns_for_type(Variant::Type p_type) {
    switch (p_type) {
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

bool OrchestratorEditorGraphPinValueEditorStruct::_is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property) {
    switch (p_type) {
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::AABB:
            return p_property.name.match(CoreStringName(end));
        case Variant::PLANE:
            return p_property.name.match(CoreStringName(normal));
        default:
            return false;
    }
}

PackedStringArray OrchestratorEditorGraphPinValueEditorStruct::_get_property_paths(Variant::Type p_type) {
    PackedStringArray results;
    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    if (!type.properties.is_empty()) {
        for (const PropertyInfo& property : type.properties) {
            if (_is_property_excluded(p_type, property)) {
                continue;
            }
            PackedStringArray sub_parts = _get_property_paths(property.type);
            if (sub_parts.is_empty()) {
                results.push_back(property.name);
                continue;
            }
            for (const String& sub_part : sub_parts) {
                results.push_back(vformat("%s.%s", property.name, sub_part));
            }
        }
    }
    return results;
}

void OrchestratorEditorGraphPinValueEditorStruct::_update_control_part(const String& p_path, int p_index, const Variant& p_value) {
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1) {
        _controls[p_index]->set_text(p_value);
        return;
    }
    Variant part_value = p_value.get(parts[1]);
    _update_control_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);
}

void OrchestratorEditorGraphPinValueEditorStruct::_read_control_part(const String& p_path, int p_index, Variant& r_value) {
    const PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1) {
        if (!_controls[p_index]->get_text().is_valid_float()) {
            _controls[p_index]->set_text("0.0");
        }
        r_value = std::stof(_controls[p_index]->get_text().utf8().get_data());
        return;
    }
    Variant part_value = r_value.get(parts[1]);
    _read_control_part(p_path.substr(p_path.find(".") + 1), p_index, part_value);
    r_value.set(parts[1], part_value);
}

void OrchestratorEditorGraphPinValueEditorStruct::_commit() {
    const PackedStringArray property_paths = _get_property_paths(_property.type);

    Variant value = VariantUtils::make_default(_property.type);
    for (int i = 0; i < property_paths.size(); i++) {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");

        Variant part = value.get(property_path_parts[0]);
        _read_control_part(property_path, i, part);
        value.set(property_path_parts[0], part);
    }
    _emit_value_changed(value);
}

void OrchestratorEditorGraphPinValueEditorStruct::configure(const PropertyInfo& p_property) {
    if (!_controls.is_empty()) {
        return;
    }

    _property = p_property;
    const PackedStringArray property_paths = _get_property_paths(_property.type);

    GridContainer* container = memnew(GridContainer);
    container->set_h_size_flags(SIZE_SHRINK_BEGIN);
    const int grid_columns = _get_grid_columns_for_type(_property.type);
    container->set_columns(grid_columns != -1 ? grid_columns : static_cast<int>(property_paths.size()) * 2);

    for (const String& property_path : property_paths) {
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
        line_edit->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [this] { _commit(); }));
        line_edit->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [this](const String&) { _commit(); }));
        container->add_child(line_edit);
        _controls.push_back(line_edit);
    }

    if (_property.type == Variant::TRANSFORM3D) {
        // Rework layout for TRANSFORM3D so that the fields are
        // BXX BXY BXZ OX
        // BYX BYY BYZ OY
        // BZX BZY BZZ OZ
        container->move_child(container->get_child(18), 6);
        container->move_child(container->get_child(19), 7);
        container->move_child(container->get_child(20), 14);
        container->move_child(container->get_child(21), 15);
    } else if (_property.type == Variant::TRANSFORM2D) {
        // Rework layout for TRANSFORM2D so that the fields are
        // XX XY OX
        // YX YY OY
        container->move_child(container->get_child(8), 4);
        container->move_child(container->get_child(9), 5);
    }

    add_child(container);
}

void OrchestratorEditorGraphPinValueEditorStruct::set_value(const Variant& p_value) {
    if (_controls.is_empty()) {
        return;
    }

    const PackedStringArray property_paths = _get_property_paths(_property.type);
    Variant value = p_value;
    if (value.get_type() == Variant::NIL) {
        value = VariantUtils::make_default(_property.type);
    }

    for (int i = 0; i < property_paths.size(); i++) {
        const String& property_path = property_paths[i];
        const PackedStringArray property_path_parts = property_path.split(".");
        Variant part_value = value.get(property_path_parts[0]);
        _update_control_part(property_path, i, part_value);
    }
}
