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
#include "editor/graph/pins/option_picker_pin.h"

#include "common/macros.h"

#include <godot_cpp/classes/popup_menu.hpp>

void OrchestratorEditorGraphPinOptionPicker::_option_item_selected(int p_index)
{
    _control->release_focus();
    _default_value_changed();
}

void OrchestratorEditorGraphPinOptionPicker::_update_control_value(const Variant& p_value)
{
    for (int i = 0; i < _control->get_item_count(); i++)
    {
        const Variant value = _control->get_item_metadata(i);
        if (p_value == value)
        {
            _control->select(i);
            return;
        }
    }

    if (_control->get_item_count() > 0)
        _control->select(0);
}

Variant OrchestratorEditorGraphPinOptionPicker::_read_control_value()
{
    if (_control->has_selectable_items())
        return _control->get_item_metadata(_control->get_selected_id());

    return Variant();
}

Control* OrchestratorEditorGraphPinOptionPicker::_create_default_value_widget()
{
    _control = memnew(OptionButton);
    _control->set_allow_reselect(true);
    _control->get_popup()->set_max_size(Vector2(32768, 400));
    _control->connect("item_selected", callable_mp_this(_option_item_selected));

    return _control;
}

void OrchestratorEditorGraphPinOptionPicker::add_item(const String& p_item, const Variant& p_value, bool p_selected)
{
    _control->add_item(p_item);
    _control->set_item_metadata(-1, p_value);

    if (p_selected)
        _control->select(_control->get_item_count() - 1);
}

void OrchestratorEditorGraphPinOptionPicker::clear()
{
    _control->clear();
}

void OrchestratorEditorGraphPinOptionPicker::set_tooltip_text(const String& p_tooltip_text)
{
    _control->set_tooltip_text(p_tooltip_text);
}