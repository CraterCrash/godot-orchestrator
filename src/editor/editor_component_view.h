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
#ifndef ORCHESTRATOR_EDITOR_COMPONENT_VIEW_H
#define ORCHESTRATOR_EDITOR_COMPONENT_VIEW_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

/// A collapsable widget that provides a title bar, button holder, and a tree to store specific state.
class OrchestratorEditorComponentView : public VBoxContainer {
    GDCLASS(OrchestratorEditorComponentView, VBoxContainer);

    struct TreeEditState {
        Callable edited;
        Callable canceled;
        Popup* popup = nullptr;
    };

    Label* _title = nullptr;
    PanelContainer* _panel = nullptr;
    HBoxContainer* _panel_hbox = nullptr;
    Tree* _tree = nullptr;
    Button* _collapse_button = nullptr;
    Button* _add_button = nullptr;
    bool _collapsed = false;
    Callable _tree_drag_handler;
    Callable _tree_gui_handler;
    bool _editing = false;

    Popup* _get_tree_editor_popup();
    Variant _tree_drag(const Vector2& p_position);
    void _tree_gui_input(const Ref<InputEvent>& p_event);
    void _tree_item_collapsed(TreeItem* p_item);
    void _tree_item_mouse_selected(const Vector2& p_position, int p_button);
    void _tree_item_selected();
    void _tree_item_activated();
    void _tree_item_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_button);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _reset_tree_size();
    void _update_collapse_button();
    void _add_button_pressed();
    void _context_menu_id_pressed(int p_index);

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    // View API
    bool is_collapsed() const;
    void toggle_collapse();
    void set_collapsed(bool p_collapsed);
    void set_title(const String& p_title);
    void set_panel_tooltip(const String& p_tooltip_text);

    // Button API
    void set_add_button_icon(const Ref<Texture2D>& p_texture);
    void set_add_button_tooltip(const String& p_tooltip_text);
    void set_add_button_visible(bool p_visible);
    void set_add_button_disabled(bool p_disabled);
    void add_button(Button* p_button, int32_t p_index = -1);

    // Tree API
    TreeItem* get_tree_selected_item();
    TreeItem* add_tree_item(const String& p_label, const Ref<Texture2D>& p_texture = Ref<Texture2D>(), TreeItem* p_parent = nullptr);
    TreeItem* add_tree_fancy_item(const String& p_fancy_name, const String& p_name, const Ref<Texture2D>& p_texture = Ref<Texture2D>(), TreeItem* p_parent = nullptr);
    void edit_tree_item(TreeItem* p_item, const Callable& p_success, const Callable& p_canceled);
    void rename_tree_item(TreeItem* p_item, const Callable& p_success);
    void remove_tree_item(TreeItem* p_item);
    void clear_tree();
    void add_tree_empty_item(const String& p_label);
    void set_tree_drag_forward(const Callable& p_drag_function);
    void set_tree_gui_handler(const Callable& p_gui_handler);
    void for_each_item(const Callable& p_callback);

    TreeItem* find_item(const String& p_name);

    OrchestratorEditorComponentView();
};

#endif // ORCHESTRATOR_EDITOR_COMPONENT_VIEW_H