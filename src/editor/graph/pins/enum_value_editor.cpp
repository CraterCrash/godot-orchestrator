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
#include "editor/graph/pins/enum_value_editor.h"

#include "common/macros.h"
#include "core/godot/object/enum_resolver.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinValueEditorEnum::_item_selected(int p_index) {
    _selected_index = p_index;
    _button->release_focus();

    if (_selected_index >= 0 && _selected_index < _button->get_item_count()) {
        _emit_value_changed(_button->get_item_metadata(_selected_index));
    }
}

void OrchestratorEditorGraphPinValueEditorEnum::configure(const PropertyInfo& p_property) {
    if (_button) {
        return;
    }

    _property = p_property;

    _button = memnew(OptionButton);
    _button->connect(SceneStringName(item_selected), callable_mp_this(_item_selected));
    add_child(_button);
}

void OrchestratorEditorGraphPinValueEditorEnum::set_value(const Variant& p_value) {
    GUARD_NULL(_button);

    using EnumItem = EnumResolver::EnumItem;

    _button->set_block_signals(true);
    _button->select(-1);

    if (!_generated) {
        const List<EnumItem>& items = EnumResolver::resolve(_property);
        for (const EnumItem& item : items) {
            const Variant item_value = item.value;
            const int32_t index = _button->get_item_count();
            _button->add_item(item.friendly_name);
            _button->set_item_metadata(index, item.value);

            if (item_value == p_value) {
                _button->select(index);
            }
        }
        _generated = true;
    } else {
        for (int index = 0; index < _button->get_item_count(); index++) {
            if (_button->get_item_metadata(index) == p_value) {
                _button->select(index);
                break;
            }
        }
    }

    if (_button->get_item_count() > 0 && _button->get_selected_id() == -1) {
        _button->select(0);
    }

    _selected_index = _button->get_selected();
    _button->set_block_signals(false);
}
