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
#include "editor/editor_component_view.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/tree_utils.h"
#include "editor.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/theme.hpp>

Popup* OrchestratorEditorComponentView::_get_tree_editor_popup() {
    for (const Variant& value : find_children("*", "Popup", true, false)) {
        Popup* popup = cast_to<Popup>(value);
        if (Popup::get_class_static() == popup->get_class()) {
            return popup;
        }
    }
    return nullptr;
}

Variant OrchestratorEditorComponentView::_tree_drag(const Vector2& p_position) {
    TreeItem* item = _tree->get_selected();
    if (!item || !_tree_drag_handler.is_valid()) {
        return Variant();
    }
    return _tree_drag_handler.call(item, p_position);
}

void OrchestratorEditorComponentView::_tree_gui_input(const Ref<InputEvent>& p_event) {
    if (_tree_gui_handler.is_valid() && _tree->get_selected()) {
        _tree_gui_handler.call(_tree->get_selected(), p_event);
    }
}

void OrchestratorEditorComponentView::_tree_item_collapsed(TreeItem* p_item) {
    // Make sure the tree control minimizes its height based on its content
    _tree->update_minimum_size();
}

void OrchestratorEditorComponentView::_tree_item_mouse_selected(const Vector2& p_position, int p_button) {
    if (p_button != MOUSE_BUTTON_RIGHT) {
        return;
    }

    TreeItem* selected = get_tree_selected_item();
    if (!selected) {
        return;
    }

    emit_signal("context_menu_requested", this, selected, _tree->get_screen_position() + p_position);
}

void OrchestratorEditorComponentView::_tree_item_selected() {
    TreeItem* selected = get_tree_selected_item();
    if (!selected) {
        return;
    }

    emit_signal("item_selected", this, selected);
}

void OrchestratorEditorComponentView::_tree_item_activated() {
    TreeItem* selected = get_tree_selected_item();
    if (!selected) {
        return;
    }

    emit_signal("item_activated", this, selected);
}

void OrchestratorEditorComponentView::_tree_item_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_button) {
    emit_signal("item_button_clicked", this, p_item, p_column, p_id, p_button);
}

void OrchestratorEditorComponentView::_reset_tree_size() {
    if (_tree && _tree->is_visible()) {
        _tree->set_visible(false);
        _tree->set_visible(true);
    }
}

void OrchestratorEditorComponentView::_update_collapse_button() {
    const String icon_name = _collapsed ? "GuiTreeArrowRight" : "GuiTreeArrowDown";
    _collapse_button->set_button_icon(SceneUtils::get_editor_icon(icon_name));
}

void OrchestratorEditorComponentView::_add_button_pressed() {
    if (_editing) {
        return;
    }

    if (_collapsed) {
        // When collapsed, we expand it before we add.
        // This prevents the editor popup being placed in obscure places
        toggle_collapse();
        call_deferred("emit_signal", "add_requested");
        return;
    }
    emit_signal("add_requested");
}

void OrchestratorEditorComponentView::_context_menu_id_pressed(int p_index) {
    emit_signal("context_menu_id_pressed", p_index);
}

void OrchestratorEditorComponentView::_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        toggle_collapse();

        get_viewport()->set_input_as_handled();
    }
}

bool OrchestratorEditorComponentView::is_collapsed() const {
    return _collapsed;
}

void OrchestratorEditorComponentView::toggle_collapse() {
    set_collapsed(!_collapsed);
}

void OrchestratorEditorComponentView::set_collapsed(bool p_collapsed) {
    _collapsed = p_collapsed;

    _update_collapse_button();
    _tree->set_visible(!_collapsed);
}

void OrchestratorEditorComponentView::set_title(const String& p_title) {
    _title->set_text(p_title);
}

void OrchestratorEditorComponentView::set_panel_tooltip(const String& p_tooltip_text) {
    _panel_hbox->set_tooltip_text(p_tooltip_text);
}

void OrchestratorEditorComponentView::set_add_button_icon(const Ref<Texture2D>& p_texture) {
    _add_button->set_button_icon(p_texture);
}

void OrchestratorEditorComponentView::set_add_button_tooltip(const String& p_tooltip_text) {
    _add_button->set_tooltip_text(p_tooltip_text);
}

void OrchestratorEditorComponentView::set_add_button_visible(bool p_visible) {
    _add_button->set_visible(p_visible);
}

void OrchestratorEditorComponentView::set_add_button_disabled(bool p_disabled) {
    _add_button->set_disabled(p_disabled);
}

void OrchestratorEditorComponentView::add_button(Button* p_button, int32_t p_index) {
    _panel_hbox->add_child(p_button);
    if (p_index != -1) {
        _panel_hbox->move_child(p_button, p_index);
    }
}

TreeItem* OrchestratorEditorComponentView::get_tree_selected_item() {
    return _tree->get_selected();
}

TreeItem* OrchestratorEditorComponentView::add_tree_item(const String& p_label, const Ref<Texture2D>& p_texture, TreeItem* p_parent) {
    TreeItem* parent = p_parent == nullptr ? _tree->get_root() : p_parent;

    TreeItem* item = _tree->create_item(parent);
    if (item) {
        item->set_text(0, p_label);
        item->set_icon(0, p_texture);
        item->set_meta("__name", p_label);

        _reset_tree_size();
    }

    return item;
}

TreeItem* OrchestratorEditorComponentView::add_tree_fancy_item(const String& p_fancy_name, const String& p_name, const Ref<Texture2D>& p_texture, TreeItem* p_parent) {
    TreeItem* item = add_tree_item(p_fancy_name, p_texture, p_parent);
    if (item) {
        item->set_meta("__name", p_name);
    }
    return item;
}

void OrchestratorEditorComponentView::edit_tree_item(TreeItem* p_item, const Callable& p_success, const Callable& p_canceled) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot edit a null tree item");

    _editing = true;

    const String real_name = p_item->get_meta("__name", "");
    if (!real_name.is_empty()) {
        p_item->set_meta("__fancy_name", p_item->get_text(0));
        p_item->set_meta("__original_name", p_item->get_meta("__name"));
        p_item->set_text(0, real_name);
    }

    _tree->set_selected(p_item, 0);

    Popup* popup = _get_tree_editor_popup();
    if (popup && (p_success.is_valid() || p_canceled.is_valid())) {
        TreeEditState* state = memnew(TreeEditState);
        state->popup = popup;

        if (p_success.is_valid()) {
            // Defines an 'item_edited' callback that will call the user provided callable and pass it the
            // raw TreeItem after the user has edited the item, so that the data can be used for any sort
            // of user processing. It is expected on the user edit callback that if there is any sort of
            // fancy name behavior that the item is modified or the tree repopulated. The new changed raw
            // name will be stashed into the item automatically.
            state->edited = callable_mp_lambda(this, [this, p_item, p_success, state] {
                const String text = p_item->get_text(0);
                if (p_item->has_meta("__name")) {
                    p_item->set_meta("__name", text);
                }

                p_success.call(p_item);

                if (state->canceled != Callable()) {
                    state->popup->disconnect("window_input", state->canceled);
                }

                _tree->disconnect("item_edited", state->edited);
                memdelete(state);

                _editing = false;
            });

            _tree->connect("item_edited", state->edited);
        }

        if (p_canceled.is_valid()) {
            // Defines a 'window_input' callback that handles checking whether the input was ESC.
            // If we detect ESC, the edit is canceled, and we dispatch the canceled callback.
            state->canceled = callable_mp_lambda(this, [this, p_item, p_canceled, state](const Ref<InputEvent>& event) {
                if (event.is_valid() && event->is_action_pressed("ui_cancel")) {
                    p_canceled.call(p_item);

                    if (state->edited != Callable()) {
                        _tree->disconnect("item_edited", state->edited);
                    }

                    state->popup->disconnect("window_input", state->canceled);
                    memdelete(state);

                    _reset_tree_size();

                    _editing = false;
                }
            });

            popup->connect("window_input", state->canceled);
        }
    }

    // This makes sure that the edit popup is always positioned properly
    const Ref<SceneTreeTimer> timer = get_tree()->create_timer(0.1);
    if (timer.is_valid()) {
        timer->connect("timeout", callable_mp_lambda(this, [this] {
            _tree->edit_selected(true);
        }));
    }
}

void OrchestratorEditorComponentView::rename_tree_item(TreeItem* p_item, const Callable& p_success) {
    const String old_name = p_item->get_text(0);
    edit_tree_item(p_item, p_success, callable_mp_lambda(this, [=] (TreeItem* item) { item->set_text(0, old_name); }));
}

void OrchestratorEditorComponentView::remove_tree_item(TreeItem* p_item) {
    memdelete(p_item);
}

void OrchestratorEditorComponentView::clear_tree() {
    _tree->clear();
    _tree->create_item()->set_text(0, "Root");

    _reset_tree_size();
}

void OrchestratorEditorComponentView::add_tree_empty_item(const String& p_label) {
    if (_tree->get_root() == nullptr) {
        clear_tree();
    }

    if (_tree->get_root()->get_child_count() == 0) {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, p_label);
        item->set_selectable(0, false);

        _reset_tree_size();
    }
}

void OrchestratorEditorComponentView::set_tree_drag_forward(const Callable& p_drag_function) {
    _tree_drag_handler = p_drag_function;
}

void OrchestratorEditorComponentView::set_tree_gui_handler(const Callable& p_gui_handler) {
    _tree_gui_handler = p_gui_handler;
}

void OrchestratorEditorComponentView::for_each_item(const Callable& p_callback) {
    if (TreeItem* root = _tree->get_root()) {
        TreeItem* current = root;
        while (current) {
            p_callback.call(current);

            if (current->get_first_child()) {
                current = current->get_first_child();
            } else {
                while (current && !current->get_next()) {
                    current = current->get_parent();
                }
                if (current) {
                    current = current->get_next();
                }
            }
        }
    }
}

TreeItem* OrchestratorEditorComponentView::find_item(const String& p_name) {
    if (TreeItem* root = _tree->get_root()) {
        TreeItem* current = root;
        while (current) {
            if (current->has_meta("__name") && p_name.match(current->get_meta("__name"))) {
                return current;
            }

            if (current->get_first_child()) {
                current = current->get_first_child();
            } else {
                while (current && !current->get_next()) {
                    current = current->get_parent();
                }
                if (current) {
                    current = current->get_next();
                }
            }
        }
    }

    return nullptr;
}

void OrchestratorEditorComponentView::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            Ref<Theme> theme = EI->get_editor_theme();
            if (theme.is_valid() && _panel) {
                Ref<StyleBoxFlat> style_box = theme->get_stylebox("panel", "ItemList");
                if (style_box.is_valid()) {
                    style_box = style_box->duplicate();
                    style_box->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
                    style_box->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
                    _panel->add_theme_stylebox_override("panel", style_box);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditorComponentView::_bind_methods() {
    ADD_SIGNAL(MethodInfo("add_requested"));
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::OBJECT, "item"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::OBJECT, "item")));
    ADD_SIGNAL(MethodInfo("item_activated", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::OBJECT, "item")));
    ADD_SIGNAL(MethodInfo("item_button_clicked", PropertyInfo(Variant::OBJECT, "none"), PropertyInfo(Variant::OBJECT, "item"), PropertyInfo(Variant::INT, "column"), PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::INT, "button")));
}

OrchestratorEditorComponentView::OrchestratorEditorComponentView() {
    set_v_size_flags(SIZE_SHRINK_BEGIN);
    set_h_size_flags(SIZE_EXPAND_FILL);

    add_theme_constant_override("separation", 0);
    set_custom_minimum_size(Vector2i(165, 0));

    _panel_hbox = memnew(HBoxContainer);
    _panel_hbox->set_h_size_flags(SIZE_EXPAND_FILL);

    _collapse_button = memnew(Button);
    _collapse_button->set_focus_mode(FOCUS_NONE);
    _collapse_button->set_flat(true);
    _panel_hbox->add_child(_collapse_button);
    _update_collapse_button();

    _title = memnew(Label);
    _title->set_h_size_flags(SIZE_EXPAND_FILL);
    _panel_hbox->add_child(_title);

    _add_button = memnew(Button);
    _add_button->set_focus_mode(FOCUS_NONE);
    _add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
    _add_button->connect("pressed", callable_mp_this(_add_button_pressed));
    _panel_hbox->add_child(_add_button);

    _panel = memnew(PanelContainer);
    _panel->set_mouse_filter(MOUSE_FILTER_PASS);
    _panel->add_child(_panel_hbox);
    add_child(_panel);

    _tree = memnew(Tree);
    _tree->set_columns(1);
    _tree->set_allow_rmb_select(true);
    _tree->set_allow_reselect(true);
    _tree->set_select_mode(Tree::SELECT_ROW);
    _tree->set_h_scroll_enabled(false);
    _tree->set_v_scroll_enabled(false);
    _tree->set_h_size_flags(SIZE_EXPAND_FILL);
    _tree->set_v_size_flags(SIZE_FILL);
    _tree->set_hide_root(true);
    _tree->connect("item_collapsed", callable_mp_this(_tree_item_collapsed));
    _tree->connect("item_mouse_selected", callable_mp_this(_tree_item_mouse_selected));
    _tree->connect("item_selected", callable_mp_this(_tree_item_selected));
    _tree->connect("item_activated", callable_mp_this(_tree_item_activated));
    _tree->connect("button_clicked", callable_mp_this(_tree_item_button_clicked));
    _tree->connect("gui_input", callable_mp_this(_tree_gui_input));
    _tree->set_drag_forwarding(callable_mp_this(_tree_drag), Callable(), Callable());
    add_child(_tree);

    _collapse_button->connect("pressed", callable_mp_this(toggle_collapse));
}