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
#include "editor/component_panels/component_panel.h"

#include "common/callable_lambda.h"
#include "common/name_utils.h"
#include "common/scene_utils.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void OrchestratorScriptComponentPanel::_toggle()
{
    _expanded = !_expanded;
    _update_collapse_button_icon();
    _tree->set_visible(_expanded);
}

void OrchestratorScriptComponentPanel::_tree_add_item()
{
    const String new_name = _create_unique_name_with_prefix(_get_unique_name_prefix());
    if (_handle_add_new_item(new_name))
    {
        update();
        _find_child_and_activate(new_name, true, true);
    }
}

void OrchestratorScriptComponentPanel::_tree_item_activated()
{
    TreeItem* item = _tree->get_selected();
    ERR_FAIL_COND_MSG(!item, "Cannot activate when no item selected");

    _handle_item_activated(item);
}

void OrchestratorScriptComponentPanel::_tree_item_edited()
{
    TreeItem* item = _tree->get_selected();
    ERR_FAIL_COND_MSG(!item, "Cannot edit item when no item selected");

    const String old_name = item->get_meta("__name");
    String new_name = item->get_text(0);

    // Nothing to edit if they're identical
    if (old_name.match(new_name))
    {
        item->set_text(0, item->get_meta("__rollback_name"));
        return;
    }

    new_name = NameUtils::create_unique_name(new_name, _get_existing_names());
    if (!_handle_item_renamed(old_name, new_name))
    {
        item->set_text(0, item->get_meta("__rollback_name"));
        return;
    }

    update();
}

void OrchestratorScriptComponentPanel::_tree_item_mouse_selected(const Vector2& p_position, int p_button)
{
    if (p_button != MOUSE_BUTTON_RIGHT)
        return;

    TreeItem* item = _tree->get_selected();
    if (!item)
        return;

    _context_menu->clear();
    _context_menu->reset_size();

    if (_populate_context_menu(item))
    {
        _context_menu->set_position(_tree->get_screen_position() + p_position);
        _context_menu->reset_size();
        _context_menu->popup();
    }
}

void OrchestratorScriptComponentPanel::_remove_confirmed()
{
    if (_tree->get_selected())
    {
        EditorInterface::get_singleton()->inspect_object(nullptr);
        _handle_remove(_tree->get_selected());

        update();
    }
}

void OrchestratorScriptComponentPanel::_tree_item_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button)
{
    _handle_button_clicked(p_item, p_column, p_id, p_mouse_button);
}

Variant OrchestratorScriptComponentPanel::_tree_drag_data(const Vector2& p_position)
{
    const Dictionary data = _handle_drag_data(p_position);
    if (data.keys().is_empty())
        return {};

    PanelContainer* container = memnew(PanelContainer);
    container->set_anchors_preset(PRESET_TOP_LEFT);
    container->set_v_size_flags(SIZE_SHRINK_BEGIN);

    HBoxContainer* hbc = memnew(HBoxContainer);
    hbc->set_v_size_flags(SIZE_SHRINK_CENTER);
    container->add_child(hbc);

    TextureRect* rect = memnew(TextureRect);
    rect->set_texture(_tree->get_selected()->get_icon(0));
    rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    rect->set_h_size_flags(SIZE_SHRINK_CENTER);
    rect->set_v_size_flags(SIZE_SHRINK_CENTER);
    hbc->add_child(rect);

    Label* label = memnew(Label);
    label->set_text(_get_tree_item_name(_tree->get_selected()));
    hbc->add_child(label);

    set_drag_preview(container);
    return data;
}

void OrchestratorScriptComponentPanel::_iterate_tree_item(TreeItem* p_item, const Callable& p_callable)
{
    p_callable.call(p_item);

    TreeItem* item = p_item->get_first_child();
    while (item)
    {
        _iterate_tree_item(item, p_callable);
        item = item->get_next();
    }
}

void OrchestratorScriptComponentPanel::_iterate_tree_items(const Callable& p_callback)
{
    if (TreeItem* root = _tree->get_root())
        _iterate_tree_item(root, p_callback);
}

void OrchestratorScriptComponentPanel::_disconnect_slot(TreeItem* p_item)
{
    const Ref<OScript> script = _orchestration->get_self();
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);

    const String method_name = _get_tree_item_name(p_item);

    for (Node* node : nodes)
    {
        TypedArray<Dictionary> connections = node->get_incoming_connections();
        for (int i = 0; i < connections.size(); i++)
        {
            const Dictionary& dict = connections[i];
            const Callable& callable = dict["callable"];
            if (callable.get_method() != method_name)
                continue;

            const Signal& signal = dict["signal"];

            if (Node* source = Object::cast_to<Node>(ObjectDB::get_instance(signal.get_object_id())))
            {
                source->disconnect(signal.get_name(), callable);

                // When disconnecting a slot from within Orchestrator, the SceneTreeDock and ConnectionsDock
                // editor windows need to be updated, so they redraw the new state of the connections.
                if (Node* editor_node = get_tree()->get_root()->get_child(0))
                {
                    if (Node* signals_node = editor_node->find_child("Signals", true, false))
                        signals_node->call("update_tree");

                    if (Node* scene_editor = editor_node->find_child("*SceneTreeEditor*", true, false))
                        scene_editor->call("update_tree");
                }

                update();
                return;
            }
        }
    }
}

TreeItem* OrchestratorScriptComponentPanel::_create_item(TreeItem* p_parent, const String& p_text, const String& p_item_name, const String& p_icon_name)
{
    TreeItem* item = p_parent->create_child();
    item->set_text(0, p_text);
    item->set_meta("__name", p_item_name);
    item->set_meta("__rollback_name", p_text);
    if (!p_icon_name.is_empty())
        item->set_icon(0, SceneUtils::get_editor_icon(p_icon_name));

    return item;
}

String OrchestratorScriptComponentPanel::_get_tree_item_name(TreeItem* p_item)
{
    return p_item ? p_item->get_meta("__name") : "";
}

void OrchestratorScriptComponentPanel::_update_theme()
{
    if (!_theme_changing)
        return;

    Ref<Theme> theme = EditorInterface::get_singleton()->get_editor_theme();
    if (theme.is_valid() && _panel)
    {
        Ref<StyleBoxFlat> sb = theme->get_stylebox("panel", "ItemList")->duplicate();
        if (sb.is_valid())
        {
            sb->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
            sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
            _panel->add_theme_stylebox_override("panel", sb);
        }
    }

    if (theme.is_valid() && _tree)
    {
        Ref<StyleBoxFlat> sb = theme->get_stylebox("panel", "Tree")->duplicate();
        if (sb.is_valid())
        {
            sb->set_corner_radius(CORNER_TOP_LEFT, 0);
            sb->set_corner_radius(CORNER_TOP_RIGHT, 0);
            _tree->add_theme_stylebox_override("panel", sb);
        }
    }

    _add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
    _update_collapse_button_icon();

    update();

    _theme_changing = false;
}

void OrchestratorScriptComponentPanel::_clear_tree()
{
    _tree->clear();
    _tree->create_item();
}

void OrchestratorScriptComponentPanel::_edit_selected_tree_item()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        const String item_name = _get_tree_item_name(selected);
        _tree->get_selected()->set_text(0, item_name);
        _tree->edit_selected(true);
    }
}

void OrchestratorScriptComponentPanel::_update_collapse_button_icon()
{
    const String icon_name = _expanded ? "GuiTreeArrowDown" : "GuiTreeArrowRight";
    _collapse_button->set_button_icon(SceneUtils::get_editor_icon(icon_name));
}

void OrchestratorScriptComponentPanel::_show_notification(const String& p_message)
{
    _notify->set_text(p_message);
    _notify->reset_size();
    _notify->popup_centered();
}

void OrchestratorScriptComponentPanel::_show_invalid_name(const String& p_type, bool p_supports_friendly_names)
{
    String message = vformat("The %s name is not valid. Names must follow these requirements:\n\n", p_type);
    message += "* Must start with a letter (A-Z, a-z) or an underscore ('_')\n";
    message += "* Can include letters (A-Z, a-z), numbers (0-9), and underscores ('_')\n";
    message += "* Should not start with a number (0-9)\n";
    message += "* Cannot contain spaces or special characters\n";

    if (p_supports_friendly_names)
    {
        message += vformat("\nIf you want a space to appear in the %s name, please use camel-case (MyName).\n", p_type);
        message += "With friendly names enabled, the name will be rendered as 'My Name' automatically.";
    }

    _show_notification(message);
}

void OrchestratorScriptComponentPanel::_confirm_removal(TreeItem* p_item)
{
    _confirm->set_text(_get_remove_confirm_text(p_item) + "\n\nDo you want to continue?");
    _confirm->set_ok_button_text("Yes");
    _confirm->set_cancel_button_text("No");
    _confirm->reset_size();
    _confirm->popup_centered();
}

String OrchestratorScriptComponentPanel::_create_unique_name_with_prefix(const String& p_prefix)
{
    return NameUtils::create_unique_name(p_prefix, _get_existing_names());
}

bool OrchestratorScriptComponentPanel::_find_child_and_activate(const String& p_name, bool p_edit, bool p_activate)
{
    TreeItem* root = _tree->get_root();

    for (int i = 0; i < root->get_child_count(); i++)
    {
        if (TreeItem* child = Object::cast_to<TreeItem>(root->get_child(i)))
        {
            if (_get_tree_item_name(child).match(p_name))
            {
                emit_signal("scroll_to_item", child);

                if (p_activate)
                {
                    _tree->set_selected(child, 0);
                    _tree->emit_signal("item_activated");
                    _tree->set_selected(child, 0);
                }
                else
                    _tree->set_selected(child, 0);

                if (p_edit)
                {
                    Ref<SceneTreeTimer> timer = get_tree()->create_timer(0.1f);
                    if (timer.is_valid())
                        timer->connect("timeout", callable_mp(this, &OrchestratorScriptComponentPanel::_edit_selected_tree_item));
                }

                return true;
            }
        }
    }
    return false;
}

void OrchestratorScriptComponentPanel::_gui_input(const Ref<InputEvent>& p_event)
{
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_LEFT)
    {
        _toggle();
        get_viewport()->set_input_as_handled();
    }
}

void OrchestratorScriptComponentPanel::update()
{
    if (_expanded)
    {
        // A simple hack to redraw the tree based on content height
        _tree->set_visible(false);
        _tree->set_visible(true);
    }
}

void OrchestratorScriptComponentPanel::find_and_edit(const String& p_item_name)
{
    _find_child_and_activate(p_item_name, true);
}

void OrchestratorScriptComponentPanel::set_collapsed(bool p_collapsed)
{
    _expanded = p_collapsed;
    _toggle();
}

void OrchestratorScriptComponentPanel::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        _panel_hbox = memnew(HBoxContainer);
        _panel_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
        _panel_hbox->set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(_get_tooltip_text()));

        _collapse_button = memnew(Button);
        _collapse_button->set_focus_mode(FOCUS_NONE);
        _collapse_button->set_flat(true);
        _update_collapse_button_icon();
        _panel_hbox->add_child(_collapse_button);

        Label* label = memnew(Label);
        label->set_text(_title);
        label->set_h_size_flags(SIZE_EXPAND_FILL);
        _panel_hbox->add_child(label);

        _add_button = memnew(Button);
        _add_button->set_focus_mode(FOCUS_NONE);
        _add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
        _add_button->set_tooltip_text("Add a new " + _get_item_name());
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
        _tree->set_focus_mode(FOCUS_NONE);
        _tree->create_item()->set_text(0, "Root"); // creates the root item
        add_child(_tree);

        _context_menu = memnew(PopupMenu);
        add_child(_context_menu);

        _confirm = memnew(ConfirmationDialog);
        _confirm->set_title("Please confirm...");
        _confirm->get_label()->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        add_child(_confirm);

        _notify = memnew(AcceptDialog);
        _notify->set_title("Message");
        add_child(_notify);

        // For handling friendly name changes
        ProjectSettings* settings = ProjectSettings::get_singleton();
        settings->connect("settings_changed", callable_mp(this, &OrchestratorScriptComponentPanel::update));

        // Connections
        _collapse_button->connect("pressed", callable_mp(this, &OrchestratorScriptComponentPanel::_toggle));
        _add_button->connect("pressed", callable_mp(this, &OrchestratorScriptComponentPanel::_tree_add_item));
        _tree->connect("item_activated", callable_mp(this, &OrchestratorScriptComponentPanel::_tree_item_activated));
        _tree->connect("item_edited", callable_mp(this, &OrchestratorScriptComponentPanel::_tree_item_edited));
        _tree->connect("item_selected", callable_mp_lambda(this, [&] { _handle_item_selected(); }));
        _tree->connect("item_mouse_selected", callable_mp(this, &OrchestratorScriptComponentPanel::_tree_item_mouse_selected));
        _tree->connect("item_collapsed", callable_mp_lambda(this, [&]([[maybe_unused]] TreeItem* i) { _tree->update_minimum_size(); }));
        _tree->connect("button_clicked", callable_mp(this, &OrchestratorScriptComponentPanel::_tree_item_button_clicked));
        _context_menu->connect("id_pressed", callable_mp_lambda(this, [&](int id) { _handle_context_menu(id); }));
        _confirm->connect("confirmed", callable_mp(this, &OrchestratorScriptComponentPanel::_remove_confirmed));

        _tree->set_drag_forwarding(callable_mp(this, &OrchestratorScriptComponentPanel::_tree_drag_data), Callable(), Callable());
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        _theme_changing = true;
        callable_mp(this, &OrchestratorScriptComponentPanel::_update_theme).call_deferred();
    }
}

void OrchestratorScriptComponentPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("scroll_to_item", PropertyInfo(Variant::OBJECT, "item")));
}

OrchestratorScriptComponentPanel::OrchestratorScriptComponentPanel(const String& p_title, Orchestration* p_orchestration)
    : _title(p_title)
    , _orchestration(p_orchestration)
{
    set_v_size_flags(SIZE_SHRINK_BEGIN);
    set_h_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 0);
    set_custom_minimum_size(Vector2i(165, 0));
}