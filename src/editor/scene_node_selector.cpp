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
#include "editor/scene_node_selector.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/version.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorSceneNodeSelector::_close_requested() {
    hide();
}

void OrchestratorSceneNodeSelector::_confirmed() {
    if (TreeItem* item = _tree->get_selected()) {
        emit_signal("node_selected", item->get_metadata(0));
    }
    hide();
}

void OrchestratorSceneNodeSelector::_filter_changed(const String& p_text) {
    _update_tree(_tree->get_root());
}

void OrchestratorSceneNodeSelector::_filter_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid()) {
        switch (key->get_keycode()) {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN: {
                push_and_accept_event(key, _filter, _tree);

                TreeItem* root = _tree->get_root();
                if (!root->get_first_child()) {
                    break;
                }

                TreeItem* current = _tree->get_selected();
                if (!current) {
                    break;
                }

                TreeItem* item = _tree->get_next_selected(root);
                while (item) {
                    item->deselect(0);
                    item = _tree->get_next_selected(item);
                }

                current->select(0);
                break;
            }
            default: {
                break;
            }
        }
    }
}

void OrchestratorSceneNodeSelector::_item_activated() {
    _confirmed();
}

void OrchestratorSceneNodeSelector::_item_selected() {
    get_ok_button()->set_disabled(!_tree->get_selected());
}

Node* OrchestratorSceneNodeSelector::_get_scene_node() const {
    ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
    return get_tree()->get_edited_scene_root();
}

void OrchestratorSceneNodeSelector::_update_tree(bool p_scroll_to_selected) {
    if (!is_inside_tree()) {
        return;
    }

    _tree->clear();
    if (_get_scene_node()) {
        _add_nodes(_get_scene_node(), nullptr);
    }

    if (!_filter->get_text().strip_edges().is_empty() || !_show_all_nodes) {
        _update_filter(nullptr, p_scroll_to_selected);
    } else if (_filter->get_text().is_empty()) {
        _update_filter(nullptr, p_scroll_to_selected);
    }
}

void OrchestratorSceneNodeSelector::_add_nodes(Node* p_node, TreeItem* p_parent) {
    if (!p_node) {
        return;
    }

    bool part_of_subscene = false;
    if (p_node->get_owner() != _get_scene_node() && p_node != _get_scene_node()) {
        if (p_node->get_owner() && _get_scene_node()->is_editable_instance(p_node->get_owner())) {
            part_of_subscene = true;
        } else {
            return;
        }
    }

    TreeItem *item = _tree->create_item(p_parent);
    item->set_text(0, p_node->get_name());
    item->set_selectable(0, true);

    const Ref<Texture2D> icon = SceneUtils::get_editor_icon(p_node->get_class());
    item->set_icon(0, icon);
    item->set_metadata(0, _get_scene_node()->get_path_to(p_node));

    if (p_node != _get_scene_node() && !p_node->get_scene_file_path().is_empty()) {
        item->add_button(0, SceneUtils::get_editor_icon("InstanceOptions"));
    }

    if (part_of_subscene) {
        const Color color = SceneUtils::get_editor_color("warning_color");
        item->set_custom_color(0, color);
        item->set_meta("custom_color", color);
    } else if (!p_node->can_process()) {
        const Color color = SceneUtils::get_editor_color("font_disabled_color");
        item->set_custom_color(0, color);
        item->set_meta("custom_color", color);
    }

    if (_selected == p_node) {
        item->select(0);
    }

    for (int i = 0; i < p_node->get_child_count(); i++) {
        _add_nodes(p_node->get_child(i), item);
    }
}

bool OrchestratorSceneNodeSelector::_item_matches_all_terms(TreeItem* p_item, const PackedStringArray& p_terms) {
    if (p_terms.is_empty()) {
        return true;
    }

    for (int i = 0; i < p_terms.size(); i++) {
        const String &term = p_terms[i];

        if (!p_item->get_text(0).to_lower().contains(term)) {
            return false;
        }
    }

    return true;
}

bool OrchestratorSceneNodeSelector::_update_filter(TreeItem* p_parent, bool p_scroll_to_selected) {
    if (!p_parent) {
        p_parent = _tree->get_root();
    }

    if (!p_parent) {
        // Tree is empty, nothing to do here.
        return false;
    }

    bool keep_for_children = false;
    for (TreeItem *child = p_parent->get_first_child(); child; child = child->get_next()) {
        // Always keep if at least one of the children are kept.
        keep_for_children = _update_filter(child, p_scroll_to_selected) || keep_for_children;
    }

    // Now find other reasons to keep this Node, too.
    PackedStringArray terms = _filter->get_text().to_lower().split(" ", false);
    bool keep = _item_matches_all_terms(p_parent, terms);

    bool selectable = keep;

    // Show only selectable nodes, or parents of selectable if not showing all nodes
    p_parent->set_visible(keep_for_children || (_show_all_nodes ? keep : selectable));

    if (selectable) {
        Color custom_color = p_parent->get_meta("custom_color", Color(0, 0, 0, 0));
        if (custom_color == Color(0, 0, 0, 0)) {
            p_parent->clear_custom_color(0);
        } else {
            p_parent->set_custom_color(0, custom_color);
        }
        p_parent->set_selectable(0, true);
    } else if (keep_for_children) {
        p_parent->set_custom_color(0, SceneUtils::get_editor_color("font_disabled_color"));
        p_parent->set_selectable(0, false);
        p_parent->deselect(0);
    }

    return p_parent->is_visible();
}

void OrchestratorSceneNodeSelector::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            OCONNECT(_filter, SceneStringName(text_changed), callable_mp_this(_filter_changed));
            OCONNECT(_filter, SceneStringName(gui_input), callable_mp_this(_filter_gui_input));
            OCONNECT(_tree, SceneStringName(item_activated), callable_mp_this(_item_activated));
            OCONNECT(_tree, SceneStringName(item_selected), callable_mp_this(_item_selected));
            OCONNECT(this, SceneStringName(confirmed), callable_mp_this(_confirmed));
            OCONNECT(this, SceneStringName(canceled), callable_mp_this(_close_requested));

            callable_mp_this(_update_tree).bind(false).call_deferred();
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (is_visible()) {
                _filter->grab_focus();
            }
            break;
        }
    }
}

void OrchestratorSceneNodeSelector::_bind_methods() {
    ADD_SIGNAL(MethodInfo("node_selected", PropertyInfo(Variant::NODE_PATH, "node_path")));
}

OrchestratorSceneNodeSelector::OrchestratorSceneNodeSelector() {
    VBoxContainer* vbox = memnew(VBoxContainer);
    add_child(vbox);

    HBoxContainer* container = memnew(HBoxContainer);
    vbox->add_child(container);

    _filter = memnew(LineEdit);
    _filter->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _filter->set_placeholder("Filter Nodes");
    _filter->set_clear_button_enabled(true);
    _filter->add_theme_constant_override("minimum_character_width", 0);
    _filter->set_right_icon(SceneUtils::get_editor_icon("Search"));
    container->add_child(_filter);

    _tree = memnew(Tree);
    _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    #if GODOT_VERSION >= 0x040300
    _tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    #else
    _tree->set_auto_translate(false);
    #endif
    _tree->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
    _tree->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
    _tree->set_begin(Point2(0, 0));
    _tree->set_end(Point2(0, 0));
    _tree->set_allow_reselect(true);
    _tree->add_theme_constant_override("button_margin", 0);
    vbox->add_child(_tree);

    set_title("Select a Node");
    get_ok_button()->set_disabled(!_tree->get_selected());
}
