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
#include "editor/inspector/property_info_container_property.h"

#include "common/dictionary_utils.h"
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/variant_utils.h"
#include "editor/search/variable_classification_dialog.h"

#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorPropertyInfoContainerEditorProperty::_get_properties()
{
    _properties.clear();

    const TypedArray<Dictionary> properties = get_edited_object()->get(get_edited_property());
    for (int index = 0; index < properties.size(); ++index)
        _properties.push_back(DictionaryUtils::to_property(properties[index]));
}

void OrchestratorPropertyInfoContainerEditorProperty::_set_properties()
{
    TypedArray<Dictionary> properties;
    for (int index = 0; index < _properties.size(); ++index)
        properties.push_back(DictionaryUtils::from_property(_properties[index]));

    emit_changed(get_edited_property(), properties);
}

void OrchestratorPropertyInfoContainerEditorProperty::_update_pass_by_details(int p_index, const PropertyInfo& p_property)
{
    Button* pass_by = Object::cast_to<Button>(_slots[p_index].button_group->get_child(0));
    if (!pass_by)
        return;

    if (PropertyUtils::is_passed_by_reference(p_property))
    {
        pass_by->set_button_icon(SceneUtils::get_icon("CircleReference"));
        pass_by->set_tooltip_text("Property is passed by reference");
    }
    else
    {
        pass_by->set_button_icon(SceneUtils::get_icon("CircleValue"));
        pass_by->set_tooltip_text("Property is passed by value");
    }
}

void OrchestratorPropertyInfoContainerEditorProperty::_update_move_buttons(bool p_force_disable)
{
    for (int index = 0; index < _slots.size(); ++index)
    {
        Button* move_up = Object::cast_to<Button>(_slots[index].button_group->get_child(2));
        Button* move_down = Object::cast_to<Button>(_slots[index].button_group->get_child(3));

        if (_allow_rearrange)
        {
            move_up->set_disabled(index == 0 || p_force_disable);
            move_down->set_disabled(index == (_slots.size() - 1) || p_force_disable);
        }
        else
        {
            move_up->set_disabled(true);
            move_down->set_disabled(true);
        }
    }
}

void OrchestratorPropertyInfoContainerEditorProperty::_add_property()
{
    PackedStringArray existing_names;
    for (const PropertyInfo& pi : _properties)
        existing_names.push_back(pi.name);

    PropertyInfo property;
    property.name = NameUtils::create_unique_name(_args ? "NewParam" : "return_value", existing_names);
    property.type = Variant::NIL;
    property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT;
    property.hint = PROPERTY_HINT_NONE;

    _properties.push_back(property);

    _set_properties();
}

void OrchestratorPropertyInfoContainerEditorProperty::_rename_property(const String& p_name, int p_index)
{
    _properties.write[p_index].name = p_name;
    _set_properties();
}

void OrchestratorPropertyInfoContainerEditorProperty::_remove_property(int p_index)
{
    _properties.remove_at(p_index);
    _set_properties();
}

void OrchestratorPropertyInfoContainerEditorProperty::_argument_type_selected(int p_index)
{
    // The dialog outputs selected values in encoded formats:
    //  type:<basic_type>
    //  class:<class_name>
    //  enum:<enum_name>
    //  bitfield:<bitfield_name>
    //  class_enum:<class_name>.<enum_name>
    //  class_bitfield:<class_name>.<bitfield_name>

    const String selected_type = _dialog->get_selected_type();
    if (!selected_type.contains(":"))
        return;

    const PackedStringArray parts = selected_type.split(":");

    const String& classification = parts[0];
    if (classification.match("type"))
    {
        for (int i = 0; i < Variant::VARIANT_MAX; i++)
        {
            Variant::Type type = VariantUtils::to_type(i);
            if (Variant::get_type_name(type).match(parts[1]))
            {
                uint32_t usage = PROPERTY_USAGE_DEFAULT;
                if (type == Variant::NIL)
                    usage |= PROPERTY_USAGE_NIL_IS_VARIANT;

                _properties.write[p_index].type = type;
                _properties.write[p_index].class_name = "";
                _properties.write[p_index].usage = usage;

                break;
            }
        }
    }
    else if (classification.match("class"))
    {
        _properties.write[p_index].type = Variant::OBJECT;
        _properties.write[p_index].class_name = parts[1];
        _properties.write[p_index].usage = PROPERTY_USAGE_DEFAULT;
    }
    else if (classification.match("enum") || classification.match("class_enum"))
    {
        _properties.write[p_index].type = Variant::INT;
        _properties.write[p_index].class_name = parts[1];
        _properties.write[p_index].usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM;
    }
    else if (classification.match("bitfield") || classification.match("class_bitfield"))
    {
        _properties.write[p_index].type = Variant::INT;
        _properties.write[p_index].class_name = parts[1];
        _properties.write[p_index].usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD;
    }

    _cleanup_selection();
    _set_properties();
}

void OrchestratorPropertyInfoContainerEditorProperty::_show_type_selection(int p_index, const String& p_value)
{
    _dialog = memnew(OrchestratorVariableTypeSearchDialog);
    _dialog->set_title(_args ? "Select argument type" : "Select return type");
    _dialog->connect("selected", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_argument_type_selected).bind(p_index));
    _dialog->connect("canceled", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_cleanup_selection));
    add_child(_dialog);

    _dialog->popup_create(true, true, p_value, p_value);
}

void OrchestratorPropertyInfoContainerEditorProperty::_cleanup_selection()
{
    _dialog->queue_free();
    _dialog = nullptr;
}

void OrchestratorPropertyInfoContainerEditorProperty::_move_up(int p_index)
{
    if (p_index > 0)
    {
        _update_move_buttons(true);
        emit_signal("move_up", p_index);
    }
}

void OrchestratorPropertyInfoContainerEditorProperty::_move_down(int p_index)
{
    if (p_index < _properties.size())
    {
        _update_move_buttons(true);
        emit_signal("move_down", p_index);
    }
}

void OrchestratorPropertyInfoContainerEditorProperty::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040300
    EditorProperty::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
    {
        _margin = memnew(MarginContainer);
        _margin->set_theme_type_variation("MarginContainer4px");
        set_bottom_editor(_margin);
        add_child(_margin);

        _container = memnew(GridContainer);
        _container->set_columns(3);
        _container->add_theme_constant_override("separation", 5);

        VBoxContainer* outer = memnew(VBoxContainer);
        outer->add_theme_constant_override("separation", 5);
        outer->add_child(_container);

        _add_button = memnew(Button);
        _add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
        _add_button->set_text(vformat("Add %s", get_label()));
        _add_button->set_theme_type_variation("InspectorActionButton");
        _add_button->set_h_size_flags(SIZE_SHRINK_CENTER);
        _add_button->set_focus_mode(FOCUS_NONE);
        _add_button->set_disabled(is_read_only());
        _add_button->connect("pressed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_add_property));
        outer->add_child(_add_button);

        _margin->add_child(outer);
    }
}

void OrchestratorPropertyInfoContainerEditorProperty::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("move_up", PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("move_down", PropertyInfo(Variant::INT, "index")));
}

void OrchestratorPropertyInfoContainerEditorProperty::_update_property()
{
    _get_properties();

    for (int index = 0; index < _properties.size(); ++index)
    {
        const PropertyInfo& property = _properties[index];

        String type_name;
        if (property.usage & PROPERTY_USAGE_CLASS_IS_ENUM || property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD)
            type_name = property.class_name;
        else
            type_name = PropertyUtils::get_property_type_name(property);

        if (index >= _slots.size())
        {
            Slot new_slot;
            new_slot.name = memnew(LineEdit);
            new_slot.name->set_h_size_flags(SIZE_EXPAND_FILL);
            new_slot.name->connect("text_changed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_rename_property).bind(index));
            new_slot.name->set_editable(!is_read_only());
            add_focusable(new_slot.name);

            new_slot.type = memnew(Button);
            new_slot.type->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
            new_slot.type->set_custom_minimum_size(Vector2(100, 0));
            new_slot.type->set_tooltip_text("Set property type");
            new_slot.type->connect("pressed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_show_type_selection).bind(index, type_name));
            new_slot.type->set_disabled(is_read_only());
            add_focusable(new_slot.type);

            new_slot.button_group = memnew(HBoxContainer);

            Button* pass_by = memnew(Button);
            pass_by->set_flat(true);
            pass_by->set_disabled(false);
            pass_by->set_focus_mode(FOCUS_NONE);
            new_slot.button_group->add_child(pass_by);

            Button* remove = memnew(Button);
            remove->set_button_icon(SceneUtils::get_editor_icon("Remove"));
            remove->set_tooltip_text("Remove this property");
            remove->set_disabled(is_read_only());
            remove->connect("pressed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_remove_property).bind(index));
            new_slot.button_group->add_child(remove);

            Button* move_up = memnew(Button);
            move_up->set_button_icon(SceneUtils::get_editor_icon("ArrowUp"));
            move_up->set_tooltip_text("Move this property up");
            move_up->set_disabled(true);
            move_up->connect("pressed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_move_up).bind(index));
            new_slot.button_group->add_child(move_up);

            Button* move_down = memnew(Button);
            move_down->set_button_icon(SceneUtils::get_editor_icon("ArrowDown"));
            move_down->set_tooltip_text("Move this property down");
            move_down->set_disabled(true);
            move_down->connect("pressed", callable_mp(this, &OrchestratorPropertyInfoContainerEditorProperty::_move_down).bind(index));
            new_slot.button_group->add_child(move_down);

            _container->add_child(new_slot.name);
            _container->add_child(new_slot.type);
            _container->add_child(new_slot.button_group);

            _slots.push_back(new_slot);
        }

        if (_slots[index].name->has_focus())
            continue;

        if (!_args)
        {
            _slots[index].name->set_text("Return Value");
            _slots[index].name->set_editable(false);
        }
        else
            _slots[index].name->set_text(property.name);

        _slots[index].type->set_text(type_name);
        _slots[index].type->set_button_icon(SceneUtils::get_class_icon(PropertyUtils::get_property_type_name(property)));

        _update_pass_by_details(index, property);
    }

    while (_slots.size() > _properties.size())
    {
        // Cleanup grid container
        for (int i = 0; i < _container->get_columns(); i++)
        {
            Node* child = _container->get_child(_container->get_child_count() - 1);
            _container->remove_child(child);
            child->queue_free();
        }

        _slots.remove_at(_slots.size() - 1);
    }

    _add_button->set_disabled(_properties.size() == _max_entries || is_read_only());
    _update_move_buttons();
}

void OrchestratorPropertyInfoContainerEditorProperty::setup(bool p_inputs, int p_max_entries)
{
    _args = p_inputs;
    _max_entries = p_max_entries;
}