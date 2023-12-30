// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "graph_node_pin_struct.h"

#include "api/extension_db.h"
#include "common/variant_utils.h"

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>

OrchestratorGraphNodePinStruct::OrchestratorGraphNodePinStruct(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinStruct::_bind_methods()
{
}

int OrchestratorGraphNodePinStruct::_get_grid_columns(Variant::Type p_type) const
{
    // Handle unique layouts for specific types
    switch(p_type)
    {
        case Variant::TRANSFORM3D:
        case Variant::PROJECTION:
            return 8;
        case Variant::TRANSFORM2D:
        case Variant::AABB:
        case Variant::BASIS:
            return 6;
        default:
            // use default
            return -1;
    }
}

bool OrchestratorGraphNodePinStruct::_is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property) const
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

void OrchestratorGraphNodePinStruct::_get_ui_value_by_property_path(const String& p_path, int p_index, Variant& r_value)
{
    PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1)
    {
        r_value = _edits[p_index]->get_text().to_float();
        return;
    }

    Variant part_value = r_value.get(parts[1]);
    _get_ui_value_by_property_path(p_path.substr(p_path.find(".") + 1), p_index, part_value);
    r_value.set(parts[1], part_value);
}

void OrchestratorGraphNodePinStruct::_set_ui_value_by_property_path(const String& p_path, int p_index, const Variant& p_value)
{
    PackedStringArray parts = p_path.split(".");
    if (parts.size() == 1)
    {
        _edits[p_index]->set_text(p_value);
        return;
    }

    Variant part_value = p_value.get(parts[1]);
    _set_ui_value_by_property_path(p_path.substr(p_path.find(".") + 1), p_index, part_value);
}

PackedStringArray OrchestratorGraphNodePinStruct::_get_property_paths(Variant::Type p_type) const
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
            }
            else
            {
                for (const String& part : _get_property_paths(property.type))
                    results.push_back(vformat("%s.%s", property.name, part));
            }
        }
    }

    return results;
}

void OrchestratorGraphNodePinStruct::_on_focus_entered(int p_index)
{
    // Intentionally deferred and selects all when focus is received
    if (p_index >= 0 && p_index < _edits.size())
        _edits[p_index]->call_deferred("select_all");
}

void OrchestratorGraphNodePinStruct::_on_default_value_changed()
{
    // this works if the sub-parts are variant based types, i.e. floats
    // what about when there are sub-component types, i.e. basis or transform2d

    Variant pin_value = _pin->get_default_value();
    if (pin_value.get_type() == Variant::NIL)
        pin_value = VariantUtils::make_default(_pin->get_type());

    const PackedStringArray property_paths = _get_property_paths(_pin->get_type());
    for (int i = 0; i < property_paths.size(); i++)
    {
        const String& property_path = property_paths[i];
        PackedStringArray property_name_parts = property_path.split(".");

        Variant value = pin_value.get(property_name_parts[0]);
        _get_ui_value_by_property_path(property_path, i, value);
        pin_value.set(property_name_parts[0], value);
    }
    _pin->set_default_value(pin_value);
}

Control* OrchestratorGraphNodePinStruct::_get_default_value_widget()
{
    const PackedStringArray property_paths = _get_property_paths(_pin->get_type());

    GridContainer* container = memnew(GridContainer);
    container->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    // Handle unique layouts for specific types
    const int grid_columns = _get_grid_columns(_pin->get_type());
    if (grid_columns != -1)
        container->set_columns(grid_columns);
    else
        container->set_columns(int(property_paths.size()) * 2);

    Variant pin_value = _pin->get_default_value();
    if (pin_value.get_type() == Variant::NIL)
        pin_value = VariantUtils::make_default(_pin->get_type());

    for (int i = 0; i < property_paths.size(); i++)
    {
        const String& property_path = property_paths[i];

        PackedStringArray property_name_parts = property_path.split(".");
        String label_text;
        for (const String& property_name_part : property_name_parts)
            label_text += property_name_part.substr(0, 1).capitalize();

        Label* label = memnew(Label);
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        label->set_text(label_text);
        container->add_child(label);

        LineEdit* line_edit = memnew(LineEdit);
        line_edit->set_expand_to_text_length_enabled(true);
        line_edit->add_theme_constant_override("minimum_character_width", 0);
        line_edit->connect("focus_entered", callable_mp(this, &OrchestratorGraphNodePinStruct::_on_focus_entered).bind(i));
        line_edit->connect("focus_exited", callable_mp(this, &OrchestratorGraphNodePinStruct::_on_default_value_changed));
        line_edit->connect("text_submitted", callable_mp(this, &OrchestratorGraphNodePinStruct::_on_default_value_changed));
        container->add_child(line_edit);

        _edits.push_back(line_edit);

        Variant part_value = pin_value.get(property_name_parts[0]);
        _set_ui_value_by_property_path(property_path, i, part_value);
    }

    if (_pin->get_type() == Variant::TRANSFORM3D)
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
    else if (_pin->get_type() == Variant::TRANSFORM2D)
    {
        // Rework layout for TRANSFORM2D so that the fields are
        // XX XY OX
        // YX YY OY
        container->move_child(container->get_child(8), 4);
        container->move_child(container->get_child(9), 5);
    }

    return container;
}

