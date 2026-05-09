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
#include "editor/graph/pins/option_picker_value_editor.h"

#include "common/macros.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/popup_menu.hpp>

void OrchestratorEditorGraphPinValueEditorOptionPicker::_option_item_selected(int p_index) {
    GUARD_NULL(_control);

    _control->release_focus();
    _emit_value_changed(_control->get_item_metadata(_control->get_selected_id()));
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::configure(const PropertyInfo& p_property) {
    if (_control) {
        return;
    }

    _control = memnew(OptionButton);
    _control->set_allow_reselect(true);
    _control->get_popup()->set_max_size(Vector2(32768, 400));
    _control->connect(SceneStringName(item_selected), callable_mp_this(_option_item_selected));
    add_child(_control);
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::set_value(const Variant& p_value) {
    GUARD_NULL(_control);

    _control->set_block_signals(true);
    for (int i = 0; i < _control->get_item_count(); i++) {
        if (_control->get_item_metadata(i) == p_value) {
            _control->select(i);
            _control->set_block_signals(false);
            return;
        }
    }

    if (_control->get_item_count() > 0) {
        _control->select(0);
    }

    _control->set_block_signals(false);
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::add_item(const String& p_item, bool p_selected) {
    add_item(p_item, p_item, p_selected);
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::add_item(const String& p_item, const Variant& p_value, bool p_selected) {
    GUARD_NULL(_control);

    _control->add_item(p_item);
    _control->set_item_metadata(-1, p_value);

    if (p_selected) {
        _control->select(_control->get_item_count() - 1);
    }
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::clear() {
    GUARD_NULL(_control);
    _control->clear();
}

void OrchestratorEditorGraphPinValueEditorOptionPicker::set_control_tooltip(const String& p_tooltip) {
    GUARD_NULL(_control);
    _control->set_tooltip_text(p_tooltip);
}
