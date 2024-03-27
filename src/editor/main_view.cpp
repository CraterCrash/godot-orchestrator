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
#include "main_view.h"

#include "common/scene_utils.h"
#include "common/version.h"
#include "editor/about_dialog.h"
#include "editor/graph/graph_edit.h"
#include "editor/updater.h"
#include "editor/window_wrapper.h"
#include "plugin/plugin.h"
#include "plugin/settings.h"
#include "script/language.h"
#include "script_view.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/file_system_dock.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/menu_bar.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script_create_dialog.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/v_split_container.hpp>

#define SKEY(m,k) Key(static_cast<int>(m) | static_cast<int>(k))

OrchestratorMainView::OrchestratorMainView(OrchestratorPlugin* p_plugin, OrchestratorWindowWrapper* p_window_wrapper)
{
    _plugin = p_plugin;
    _wrapper = p_window_wrapper;
}

void OrchestratorMainView::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("toggle_component_panel", PropertyInfo(Variant::BOOL, "visible")));
}

void OrchestratorMainView::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_anchors_preset(PRESET_FULL_RECT);
        set_h_size_flags(SIZE_EXPAND_FILL);
        set_v_size_flags(SIZE_EXPAND_FILL);

        MarginContainer* margin = memnew(MarginContainer);
        margin->set_anchors_preset(PRESET_FULL_RECT);
        margin->add_theme_constant_override("margin_left", 4);
        margin->add_theme_constant_override("margin_top", 0);
        margin->add_theme_constant_override("margin_right", 5);
        margin->add_theme_constant_override("margin_bottom", 5);
        add_child(margin);

        VBoxContainer* vbox = memnew(VBoxContainer);
        vbox->add_theme_constant_override("separation", 0);
        margin->add_child(vbox);

        HBoxContainer* toolbar = memnew(HBoxContainer);
        toolbar->set_custom_minimum_size(Vector2i(0, 32.0 * _plugin->get_editor_interface()->get_editor_scale()));
        toolbar->add_theme_constant_override("separation", 0);
        vbox->add_child(toolbar);

        MenuBar* left_menu = memnew(MenuBar);
        left_menu->set_h_size_flags(SIZE_EXPAND_FILL);
        toolbar->add_child(left_menu);

        HBoxContainer* left_menu_container = memnew(HBoxContainer);
        left_menu_container->add_theme_constant_override("separation", 0);
        left_menu->add_child(left_menu_container);

        _file_menu = memnew(MenuButton);
        _file_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
        _file_menu->set_text("File");
        _file_menu->get_popup()->clear();
        _file_menu->get_popup()->add_item("New Orchestration...", AccelMenuIds::NEW, SKEY(KEY_MASK_CTRL, KEY_N));
        _file_menu->get_popup()->add_item("Open...", AccelMenuIds::OPEN);
        _file_menu->get_popup()->add_separator();
        _file_menu->get_popup()->add_item("Save", AccelMenuIds::SAVE, SKEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_S));
        _file_menu->get_popup()->add_item("Save As...", AccelMenuIds::SAVE_AS);
        _file_menu->get_popup()->add_item("Save All", AccelMenuIds::SAVE_ALL, SKEY(KEY_MASK_SHIFT | KEY_MASK_ALT, KEY_S));
        _file_menu->get_popup()->add_separator();
        _file_menu->get_popup()->add_item("Show in Filesystem", AccelMenuIds::SHOW_IN_FILESYSTEM);
        _file_menu->get_popup()->add_separator();
        _file_menu->get_popup()->add_item("Close", AccelMenuIds::CLOSE, SKEY(KEY_MASK_CTRL, KEY_W));
        _file_menu->get_popup()->add_item("Close All", AccelMenuIds::CLOSE_ALL);
        _file_menu->get_popup()->add_separator();
        _file_menu->get_popup()->add_item("Run", AccelMenuIds::RUN, SKEY(KEY_MASK_SHIFT | KEY_MASK_CTRL, KEY_X));
        _file_menu->get_popup()->add_separator();
        _file_menu->get_popup()->add_item("Toggle Orchestrator Panel", AccelMenuIds::TOGGLE_LEFT_PANEL, SKEY(KEY_MASK_CTRL, KEY_BACKSLASH));
        _file_menu->get_popup()->add_item("Toggle Component Panel", AccelMenuIds::TOGGLE_RIGHT_PANEL, SKEY(KEY_MASK_CTRL, KEY_SLASH));
        _file_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorMainView::_on_menu_option));
        _file_menu->get_popup()->connect("about_to_popup",
                                         callable_mp(this, &OrchestratorMainView::_on_prepare_file_menu));
        _file_menu->get_popup()->connect("popup_hide", callable_mp(this, &OrchestratorMainView::_on_file_menu_closed));
        left_menu_container->add_child(_file_menu);

        _goto_menu = memnew(MenuButton);
        _goto_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
        _goto_menu->set_text("Goto");
        _goto_menu->get_popup()->clear();
        _goto_menu->get_popup()->add_item("Goto Node", AccelMenuIds::GOTO_NODE, SKEY(KEY_MASK_CTRL, KEY_L));
        _goto_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorMainView::_on_menu_option));
        _goto_menu->get_popup()->connect("about_to_popup", callable_mp(this, &OrchestratorMainView::_on_prepare_goto_menu));
        left_menu_container->add_child(_goto_menu);

        _help_menu = memnew(MenuButton);
        _help_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
        _help_menu->set_text("Help");
        _help_menu->get_popup()->clear();
        _help_menu->get_popup()->add_icon_item(SceneUtils::get_icon(this, "ExternalLink"), "Online Documentation", AccelMenuIds::ONLINE_DOCUMENTATION);
        _help_menu->get_popup()->add_icon_item(SceneUtils::get_icon(this, "ExternalLink"), "Community", AccelMenuIds::COMMUNITY);
        _help_menu->get_popup()->add_separator();
        _help_menu->get_popup()->add_icon_item(SceneUtils::get_icon(this, "ExternalLink"), "Report a Bug", AccelMenuIds::GITHUB_ISSUES);
        _help_menu->get_popup()->add_icon_item(SceneUtils::get_icon(this, "ExternalLink"), "Suggest a Feature", AccelMenuIds::GITHUB_FEATURE);
        _help_menu->get_popup()->add_separator();
        _help_menu->get_popup()->add_item("About " VERSION_NAME, AccelMenuIds::ABOUT);
        _help_menu->get_popup()->add_icon_item(SceneUtils::get_icon(this, "Heart"), "Support " VERSION_NAME, AccelMenuIds::SUPPORT);
        _help_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorMainView::_on_menu_option));
        left_menu_container->add_child(_help_menu);

        HBoxContainer* right_menu_container = memnew(HBoxContainer);
        right_menu_container->add_theme_constant_override("separation", 0);
        right_menu_container->set_alignment(BoxContainer::ALIGNMENT_END);
        right_menu_container->set_anchors_preset(PRESET_FULL_RECT);
        toolbar->add_child(right_menu_container);

        Button* open_documentation = memnew(Button);
        open_documentation->set_text("Online Docs");
        open_documentation->set_button_icon(SceneUtils::get_icon(this, "ExternalLink"));
        open_documentation->set_flat(true);
        open_documentation->set_focus_mode(FOCUS_NONE);
        open_documentation->connect(
            "pressed",
            callable_mp(this, &OrchestratorMainView::_on_menu_option).bind(AccelMenuIds::ONLINE_DOCUMENTATION));
        right_menu_container->add_child(open_documentation);

        VSeparator* vs = memnew(VSeparator);
        vs->set_v_size_flags(SIZE_SHRINK_CENTER);
        vs->set_custom_minimum_size(Vector2i(0, 24));
        right_menu_container->add_child(vs);

        Label* version = memnew(Label);
        version->set_text(VERSION_NAME " v" VERSION_NUMBER);
        version->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
        right_menu_container->add_child(version);

        _updater = memnew(OrchestratorUpdater);
        right_menu_container->add_child(_updater);

        if (_wrapper->is_window_available())
        {
            vs = memnew(VSeparator);
            vs->set_v_size_flags(SIZE_SHRINK_CENTER);
            vs->set_custom_minimum_size(Vector2i(0, 24));
            right_menu_container->add_child(vs);
            _select_separator = vs;

            _select = memnew(OrchestratorScreenSelect);
            _select->set_flat(true);
            _select->set_tooltip_text("Make the Orchestration editor floating.");
            _select->connect("request_open_in_screen", callable_mp(_wrapper, &OrchestratorWindowWrapper::enable_window_on_screen).bind(true));
            right_menu_container->add_child(_select);
            _wrapper->connect("window_visibility_changed", callable_mp(this, &OrchestratorMainView::_on_window_changed));
        }

        HSplitContainer* main_view_container = memnew(HSplitContainer);
        main_view_container->set_v_size_flags(SIZE_EXPAND_FILL);
        vbox->add_child(main_view_container);

        VSplitContainer* left_panel = memnew(VSplitContainer);
        main_view_container->add_child(left_panel);
        _left_panel = left_panel;

        VBoxContainer* files_container = memnew(VBoxContainer);
        files_container->set_anchors_preset(PRESET_FULL_RECT);
        files_container->set_v_size_flags(SIZE_EXPAND_FILL);
        left_panel->add_child(files_container);

        LineEdit* file_filters = memnew(LineEdit);
        file_filters->set_placeholder("Filter orchestrations");
        file_filters->set_clear_button_enabled(true);
        file_filters->set_right_icon(SceneUtils::get_icon(this, "Search"));
        files_container->add_child(file_filters);

        _file_list = memnew(ItemList);
        _file_list->set_custom_minimum_size(Vector2i(165, 0));
        _file_list->set_allow_rmb_select(true);
        _file_list->set_focus_mode(FOCUS_NONE);
        _file_list->set_v_size_flags(SIZE_EXPAND_FILL);
        files_container->add_child(_file_list);

        file_filters->connect("text_changed", callable_mp(this, &OrchestratorMainView::_on_file_filters_changed));
        _file_list->connect("item_selected", callable_mp(this, &OrchestratorMainView::_on_file_list_selected));

        _script_editor_container = memnew(VBoxContainer);
        _script_editor_container->set_v_size_flags(SIZE_EXPAND_FILL);

        main_view_container->add_child(_script_editor_container);

        _about_window = memnew(OrchestratorAboutDialog);
        add_child(_about_window);

        _open_dialog = memnew(FileDialog);
        _open_dialog->set_min_size(Vector2(700, 400));
        _open_dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);
        _open_dialog->set_title("Open Orchestration Script");
        _open_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
        _open_dialog->add_filter("*.os", "Orchestrator Scripts");
        _open_dialog->connect("file_selected", callable_mp(this, &OrchestratorMainView::_on_open_script_file));
        add_child(_open_dialog);

        _save_dialog = memnew(FileDialog);
        _save_dialog->set_min_size(Vector2(700, 400));
        _save_dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);
        _save_dialog->set_title("Save As Orchestration Script");
        _save_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
        _save_dialog->add_filter("*.os", "Orchestrator Scripts");
        _save_dialog->connect("file_selected", callable_mp(this, &OrchestratorMainView::_on_save_script_file));
        add_child(_save_dialog);

        // Create edited close confirmation dialog
        _close_confirm = memnew(ConfirmationDialog);
        _close_confirm->set_ok_button_text("Save");
        _close_confirm->add_button("Discard", DisplayServer::get_singleton()->get_swap_cancel_ok(), "discard");
        _close_confirm->connect("confirmed", callable_mp(this, &OrchestratorMainView::_on_close_current_tab).bind(true));
        _close_confirm->connect("custom_action", callable_mp(this, &OrchestratorMainView::_on_close_discard_current_tab));
        add_child(_close_confirm);

        _goto_dialog = memnew(ConfirmationDialog);
        _goto_dialog->set_title("Go to Node");

        VBoxContainer* container = memnew(VBoxContainer);
        _goto_dialog->add_child(container);

        Label* label = memnew(Label);
        label->set_text("Node Number:");
        container->add_child(label);

        LineEdit *node_number = memnew(LineEdit);
        node_number->set_select_all_on_focus(true);
        _goto_dialog->register_text_enter(node_number);
        container->add_child(node_number);

        _goto_dialog->connect("visibility_changed", callable_mp(this, &OrchestratorMainView::_on_goto_node_visibility_changed).bind(node_number));
        _goto_dialog->connect("confirmed", callable_mp(this, &OrchestratorMainView::_on_goto_node).bind(node_number));
        _goto_dialog->connect("canceled", callable_mp(this, &OrchestratorMainView::_on_goto_node_closed).bind(node_number));

        add_child(_goto_dialog);
    }
}

void OrchestratorMainView::edit(const Ref<OScript>& p_script)
{
    _open_script(p_script);
}

void OrchestratorMainView::apply_changes()
{
    for (const ScriptFile& file : _script_files)
        file.editor->apply_changes();
}

void OrchestratorMainView::get_window_layout(const Ref<ConfigFile>& p_configuration)
{
    PackedStringArray open_files;
    for (const ScriptFile& file : _script_files)
        open_files.push_back(file.file_name);

    p_configuration->set_value("Orchestrator", "open_files", open_files);

    if (_has_open_script())
        p_configuration->set_value("Orchestrator", "open_files_selected", _script_files[_current_index].file_name);
    else if (p_configuration->has_section_key("Orchestrator", "open_files_selected"))
        p_configuration->erase_section_key("Orchestrator", "open_files_selected");
}

void OrchestratorMainView::set_window_layout(const Ref<ConfigFile>& p_configuration)
{
    if (_plugin->restore_windows_on_load() && p_configuration->has_section_key("Orchestrator", "open_files"))
    {
        PackedStringArray open_files = p_configuration->get_value("Orchestrator", "open_files", {});
        for (const String& file_name : open_files)
        {
            const Ref<OScript> script = ResourceLoader::get_singleton()->load(file_name);
            if (script.is_valid())
                _open_script(script);
        }

        String open_selected_file = p_configuration->get_value("Orchestrator", "open_files_selected", "");
        if (!open_selected_file.is_empty())
        {
            for (int i = 0; i < _file_list->get_item_count(); i++)
            {
                if (_script_files[i].file_name == open_selected_file)
                {
                    // Selecting the item in the ItemList does not raise the signal
                    _file_list->select(i);
                    _show_script_editor_view(open_selected_file);
                    break;
                }
            }
        }
    }
}

bool OrchestratorMainView::build()
{
    for (const ScriptFile& file : _script_files)
    {
        if (!file.editor->build())
            return false;
    }
    return true;
}

bool OrchestratorMainView::_has_open_script() const
{
    return _current_index >= 0 && _current_index < _script_files.size();
}

int OrchestratorMainView::_get_script_file_index_by_file_name(const String& p_file_name) const
{
    for (int i = 0; i < _script_files.size(); i++)
    {
        if (_script_files[i].file_name == p_file_name)
            return i;
    }
    return -1;
}

bool OrchestratorMainView::_is_current_script_unsaved() const
{
    return _has_open_script() && _script_files[_current_index].editor->is_modified();
}

void OrchestratorMainView::_ask_close_current_unsaved_script()
{
    if (_has_open_script())
    {
        const ScriptFile& file = _script_files[_current_index];
        _close_confirm->set_text("Close and save changes to " + file.file_name);
        _close_confirm->popup_centered();
    }
}

void OrchestratorMainView::_open_script(const Ref<OScript>& p_script)
{
    if (p_script->get_path().is_empty())
    {
        ERR_PRINT("Script has no path, cannot be opened.");
        return;
    }

    // Before we open the new file, an existing editors need to be hidden.
    // Unlike GDScript, we don't use tabs but rather control which editor is visible manually.
    for (const ScriptFile& file : _script_files)
        file.editor->hide();

    // Now check whether the file is already open in the editor.
    // If so, use that editor rather than creating a new one.
    for (int i = 0; i < _script_files.size(); i++)
    {
        const ScriptFile& file = _script_files[i];
        if (file.file_name == p_script->get_path())
        {
            _current_index = i;
            file.editor->show();

            _update_files_list();
            _on_prepare_file_menu();
            return;
        }
    }

    // This is a new file opened
    OrchestratorScriptView* editor = memnew(OrchestratorScriptView(_plugin, this, p_script));
    _script_editor_container->add_child(editor);

    ScriptFile file;
    file.file_name = p_script->get_path();
    file.editor = editor;

    _current_index = _script_files.size();
    _script_files.push_back(file);

    _update_files_list();
    _on_prepare_file_menu();

    _show_script_editor_view(file.file_name);

    // Since the editor's ready callback needs to fire, we defer this call
    call_deferred("emit_signal", "toggle_component_panel", _right_panel_visible);
}

void OrchestratorMainView::_save_script()
{
    if (_has_open_script())
    {
        const ScriptFile& file = _script_files[_current_index];
        file.editor->apply_changes();
    }
}

void OrchestratorMainView::_save_all_scripts()
{
    for (const ScriptFile& file : _script_files)
        file.editor->apply_changes();
}

void OrchestratorMainView::_close_script(bool p_save)
{
    if (_has_open_script())
    {
        // Get the current open file
        const ScriptFile& file = _script_files[_current_index];

        if (p_save)
            file.editor->apply_changes();

        // Hide the current editor and remove it
        file.editor->queue_free();

        // Remove the entry from the list
        _script_files.remove_at(_current_index);

        if (_script_files.size() > 0)
        {
            // In this case we removed the last entry, go the one previous
            if (_current_index == _script_files.size())
                _current_index--;

            const ScriptFile& new_file = _script_files[_current_index];
            _show_script_editor_view(new_file.file_name);
        }
        else
        {
            // There are no files left, set to -1
            _current_index = -1;
        }

        _update_files_list();
    }
}

void OrchestratorMainView::_close_all_scripts()
{
    for (const ScriptFile& file : _script_files)
        _script_close_queue.push_back(file);

    _queue_close_scripts();
}

void OrchestratorMainView::_queue_close_scripts()
{
    while (!_script_close_queue.is_empty())
    {
        const ScriptFile file = _script_close_queue.front()->get();
        _script_close_queue.pop_front();
        _show_script_editor_view(file.file_name);

        if (file.editor->is_modified())
        {
            file.editor->connect("tree_exited", callable_mp(this, &OrchestratorMainView::_queue_close_scripts), CONNECT_ONE_SHOT);
            _ask_close_current_unsaved_script();
            break;
        }
        _close_script(false);
    }

    _update_files_list();
}

void OrchestratorMainView::_show_create_new_script_dialog()
{
    const String inherits = OrchestratorSettings::get_singleton()->get_setting("settings/default_type");

    ScriptCreateDialog* dialog = _plugin->get_script_create_dialog();

    // Cache existing position and change it for our pop-out
    const Window::WindowInitialPosition initial_position = dialog->get_initial_position();
    dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);

    // Finds the LanguageMenu option button and forces the Orchestrator choice
    // This must be done before calling "config" to make sure that all the dialog logic for templates
    // and the language choice align properly.
    const String language_name = OScriptLanguage::get_singleton()->_get_name();
    TypedArray<Node> nodes = dialog->find_children("*", "OptionButton", true, false);
    if (!nodes.is_empty())
    {
        OptionButton* language_menu = Object::cast_to<OptionButton>(nodes[0]);
        for (int i = 0; i < language_menu->get_item_count(); i++)
        {
            if (language_menu->get_item_text(i).match(language_name))
            {
                language_menu->select(i);
                break;
            }
        }
    }

    dialog->set_title("Create Orchestration");
    dialog->config(inherits, "new_orchestration.os", false, false);

    // Save reference of the change
    Ref<EditorSettings> settings = _plugin->get_editor_interface()->get_editor_settings();
    settings->set_project_metadata("script_setup", "last_selected_language", language_name);

    if (!dialog->is_connected("script_created", callable_mp(this, &OrchestratorMainView::_on_script_file_created)))
        dialog->connect("script_created", callable_mp(this, &OrchestratorMainView::_on_script_file_created));

    dialog->popup_centered();

    // Restore old position
    dialog->set_initial_position(initial_position);
}

void OrchestratorMainView::_update_files_list()
{
    _file_list->clear();

    HashSet<String> file_stems;
    HashSet<String> duplicate_stems;
    for (const ScriptFile& file : _script_files)
    {
        const String file_stem = file.file_name.get_file();
        if (!file_stems.has(file_stem))
            file_stems.insert(file_stem);
        else
            duplicate_stems.insert(file_stem);
    }

    for (int i = 0; i < _script_files.size(); i++)
    {
        const ScriptFile& file = _script_files[i];
        if (_file_name_filter.is_empty() || file.file_name.contains(_file_name_filter))
        {
            const String stem = file.file_name.get_file();
            const String base = file.file_name.get_base_dir().replace("res://", "");
            const String full = base.is_empty() ? stem : vformat("%s/%s", base, stem);

            String item_text = duplicate_stems.has(stem) ? full : stem;
            int32_t index = _file_list->add_item(item_text, SceneUtils::get_icon(this, "GDScript"));

            if (i == _current_index)
                _file_list->select(index);
        }
    }
}

void OrchestratorMainView::_navigate_to_current_path()
{
    if (_has_open_script())
    {
        const ScriptFile& file = _script_files[_current_index];
        _plugin->get_editor_interface()->get_file_system_dock()->navigate_to_path(file.file_name);
    }
}

void OrchestratorMainView::_show_script_editor_view(const String& p_file_name)
{
    // Hide all editors
    for (const ScriptFile& file : _script_files)
        file.editor->hide();

    // Clear the inspector
    _plugin->get_editor_interface()->inspect_object(nullptr);

    _current_index = _get_script_file_index_by_file_name(p_file_name);
    _script_files[_current_index].editor->show();
}

void OrchestratorMainView::_on_prepare_file_menu()
{
    PopupMenu* popup = _file_menu->get_popup();

    bool no_open_file = !_has_open_script();
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::SAVE), no_open_file);
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::SAVE_AS), no_open_file);
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::SAVE_ALL), no_open_file);
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::SHOW_IN_FILESYSTEM), no_open_file);
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::CLOSE), no_open_file);
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::CLOSE_ALL), no_open_file);

    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::RUN), true);  // always disabled, likely remove
}

void OrchestratorMainView::_on_file_menu_closed()
{
    PopupMenu* popup = _file_menu->get_popup();
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::RUN), false);
}

void OrchestratorMainView::_on_menu_option(int p_option)
{
    switch (p_option)
    {
        case NEW:
            _show_create_new_script_dialog();
            break;
        case OPEN:
            _open_dialog->popup_centered();
            break;
        case SAVE_AS:
            _save_dialog->popup_centered();
            break;
        case SAVE:
            _save_script();
            break;
        case SAVE_ALL:
            _save_all_scripts();
            break;
        case CLOSE:
        {
            if (_is_current_script_unsaved())
                _ask_close_current_unsaved_script();
            else
                _close_script(false);
            break;
        }
        case CLOSE_ALL:
            _close_all_scripts();
            break;
        case ONLINE_DOCUMENTATION:
            OS::get_singleton()->shell_open(_plugin->get_plugin_online_documentation_url());
            break;
        case COMMUNITY:
            OS::get_singleton()->shell_open(_plugin->get_community_url());
            break;
        case GITHUB_ISSUES:
        case GITHUB_FEATURE:
            OS::get_singleton()->shell_open(_plugin->get_github_issues_url());
            break;
        case SUPPORT:
            OS::get_singleton()->shell_open(_plugin->get_patreon_url());
            break;
        case ABOUT:
            _about_window->popup_centered(Size2(780, 500));
            break;
        case SHOW_IN_FILESYSTEM:
            _navigate_to_current_path();
            break;
        case TOGGLE_LEFT_PANEL:
            _left_panel->set_visible(!_left_panel->is_visible());
            break;
        case TOGGLE_RIGHT_PANEL:
            _right_panel_visible = !_right_panel_visible;
            emit_signal("toggle_component_panel", _right_panel_visible);
            break;
        case GOTO_NODE:
        {
            _goto_dialog->popup_centered();
            break;
        }
        default:
            break;
    }
}

void OrchestratorMainView::_on_script_file_created(const Ref<Script>& p_script)
{
    if (!p_script.is_valid() || p_script->get_class() != "OScript")
    {
        ERR_PRINT(vformat("The script is not an orchestration"));
        return;
    }
    edit(p_script);
}

void OrchestratorMainView::_on_open_script_file(const String& p_file_name)
{
    const Ref<OScript> script = ResourceLoader::get_singleton()->load(p_file_name);
    if (!script.is_valid())
        OS::get_singleton()->alert("The orchestration script file is not valid.", "Orchestration not valid");

    _open_script(script);
}

void OrchestratorMainView::_on_save_script_file(const String& p_file_name)
{
    const ScriptFile& file = _script_files[_current_index];
    if (file.editor->save_as(p_file_name))
    {
        _script_files.write[_current_index].file_name = p_file_name;
        _update_files_list();
    }
}

void OrchestratorMainView::_on_file_filters_changed(const String& p_text)
{
    _file_name_filter = p_text;
    _update_files_list();
}

void OrchestratorMainView::_on_file_list_selected(int p_index)
{
    _show_script_editor_view(_script_files[p_index].file_name);
}

void OrchestratorMainView::_on_close_current_tab(bool p_save)
{
    _close_script(p_save);
}

void OrchestratorMainView::_on_close_discard_current_tab([[maybe_unused]] const String &p_data)
{
    if (_has_open_script())
    {
        const ScriptFile& file = _script_files[_current_index];
        file.editor->reload_from_disk();

        _close_script(false);
    }
    _close_confirm->hide();
}

void OrchestratorMainView::_on_goto_node_visibility_changed(LineEdit* p_edit)
{
    if (_goto_dialog->is_visible())
        p_edit->grab_focus();
}

void OrchestratorMainView::_on_prepare_goto_menu()
{
    PopupMenu* popup = _goto_menu->get_popup();

    bool no_open_file = !_has_open_script();
    popup->set_item_disabled(popup->get_item_index(AccelMenuIds::GOTO_NODE), no_open_file);
}

void OrchestratorMainView::_on_goto_node(LineEdit* p_edit)
{
    if (!p_edit->get_text().is_valid_int())
        return;

    int node_id = p_edit->get_text().to_int();
    p_edit->set_text("");

    if (_has_open_script())
    {
        const ScriptFile& file = _script_files[_current_index];
        file.editor->goto_node(node_id);
    }
}

void OrchestratorMainView::_on_goto_node_closed(LineEdit* p_edit)
{
    p_edit->set_text("");
}

void OrchestratorMainView::_on_window_changed(bool p_visible)
{
    _select_separator->set_visible(!p_visible);
    _select->set_visible(!p_visible);
    _floating = p_visible;
}
