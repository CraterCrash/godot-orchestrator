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
#include "editor/editor.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/resource_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/config/project_settings_cache.h"
#include "core/godot/editor/settings/editor_settings.h"
#include "core/godot/gdextension_compat.h"
#include "core/godot/scene_string_names.h"
#include "editor/actions/registry.h"
#include "editor/editor_view.h"
#include "editor/getting_started.h"
#include "editor/gui/about_dialog.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/gui/editor_log_event_router.h"
#include "editor/gui/file_dialog.h"
#include "editor/gui/window_wrapper.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/scene/connections_dock.h"
#include "editor/updater/updater.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_system_dock.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/templates/hash_set.hpp>

int OrchestratorEditor::_editor_view_func_count = 0;
OrchestratorEditorViewFunc OrchestratorEditor::_editor_view_funcs[ORCHESTRATOR_VIEW_FUNC_MAX];
OrchestratorEditor* OrchestratorEditor::_editor;

void OrchestratorEditor::_open_in_browser(const String& p_url) {
    OS::get_singleton()->shell_open(p_url);
}

void OrchestratorEditor::_set_script_create_dialog_language(const String& p_language_name) {
    TypedArray<Node> nodes = _script_create_dialog->find_children("*", OptionButton::get_class_static(), true, false);
    if (nodes.is_empty()) {
        return;
    }

    OptionButton* menu = cast_to<OptionButton>(nodes[0]);
    for (int i = 0; i < menu->get_item_count(); i++) {
        if (menu->get_item_text(i).match(p_language_name)) {
            menu->select(i);
            break;
        }
    }
}

void OrchestratorEditor::_prepare_file_menu() {
    PopupMenu* menu = _file_menu->get_popup();
    OrchestratorEditorView* editor = _get_current_editor();
    const Ref<Resource> res = editor ? editor->get_edited_resource() : Ref<Resource>();

    const Ref<Script> current_script = _get_current_script();
    const bool current_script_tool = current_script.is_valid() ? current_script->is_tool() : false;

    menu->set_item_disabled(menu->get_item_index(FILE_REOPEN_CLOSED), _previous_scripts.is_empty());
    menu->set_item_disabled(menu->get_item_index(FILE_SOFT_RELOAD_TOOL_SCRIPT), !current_script_tool);
    menu->set_item_disabled(menu->get_item_index(FILE_SAVE), res.is_null());
    menu->set_item_disabled(menu->get_item_index(FILE_SAVE_AS), res.is_null());
    menu->set_item_disabled(menu->get_item_index(FILE_SAVE_ALL), res.is_null());
    menu->set_item_disabled(menu->get_item_index(FILE_SHOW_IN_FILESYSTEM), res.is_null());
    menu->set_item_disabled(menu->get_item_index(FILE_CLOSE), res.is_null());
    menu->set_item_disabled(menu->get_item_index(FILE_CLOSE_ALL), res.is_null());
}

void OrchestratorEditor::_file_menu_closed() {
    PopupMenu* menu = _file_menu->get_popup();

    menu->set_item_disabled(menu->get_item_index(FILE_SAVE), false);
    menu->set_item_disabled(menu->get_item_index(FILE_SAVE_AS), false);
    menu->set_item_disabled(menu->get_item_index(FILE_SAVE_ALL), false);
    menu->set_item_disabled(menu->get_item_index(FILE_SHOW_IN_FILESYSTEM), false);
    menu->set_item_disabled(menu->get_item_index(FILE_CLOSE), false);
    menu->set_item_disabled(menu->get_item_index(FILE_CLOSE_ALL), false);
}

void OrchestratorEditor::_file_dialog_action(const String& p_file) {
    switch (_file_dialog_option) {
        case FILE_OPEN: {
            open_file(p_file);
            break;
        }
        case FILE_SAVE_AS: {
            OrchestratorEditorView* current = _get_current_editor();
            if (current) {
                const Ref<Resource> resource = current->get_edited_resource();
                const String path = ProjectSettings::get_singleton()->localize_path(p_file);

                if (ResourceSaver::get_singleton()->save(resource, path) != OK) {
                    ORCHESTRATOR_ACCEPT("Error saving files");
                }

                resource->set_path(path);

                EI->get_resource_filesystem()->update_file(path);

                _update_script_names();
            }
            break;
        }
    }

    _file_dialog_option = -1;
}

void OrchestratorEditor::_menu_option(int p_option) {
    OrchestratorEditorView* current = _get_current_editor();
    switch (p_option) {
        case FILE_NEW: {
            // Find LanguageMenu option and force it to Orchestrator as the selected choice
            // Must be called before "config" to guarantee that the dialog logic for templates and language work
            const String language_name = OScriptLanguage::get_singleton()->_get_name();
            _set_script_create_dialog_language(language_name);

            const String inherits = ORCHESTRATOR_GET("settings/default_type", "Node");
            _script_create_dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);
            _script_create_dialog->set_title("Create Orchestration");
            _script_create_dialog->config(inherits, "new_script.os", false, false);

            PROJECT_SET("script_setup", "last_selected_language", language_name);
            _script_create_dialog->popup_centered();

            break;
        }
        case FILE_OPEN: {
            _file_dialog_option = FILE_OPEN;

            _file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
            _file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
            _file_dialog->set_title("Open Orchestration");

            const PackedStringArray extensions = get_recognized_extensions();

            _file_dialog->clear_filters();
            for (const String& extension : extensions) {
                _file_dialog->add_filter("*." + extension, extension.to_upper());
            }

            _file_dialog->popup_file_dialog();
            break;
        }
        case FILE_REOPEN_CLOSED: {
            if (_previous_scripts.is_empty()) {
                return;
            }

            const String path = _previous_scripts.back()->get();
            _previous_scripts.pop_back();

            const Ref<Resource> script = ResourceLoader::get_singleton()->load(path);
            if (script.is_null()) {
                OrchestratorEditorDialogs::error("Could not load file at: " + path);
                _file_dialog_option = -1;
                return;
            }

            edit(script);
            _file_dialog_option = -1;
            break;
        }
        case FILE_SAVE_ALL: {
            if (_test_script_times_on_disk()) {
                return;
            }
            save_all_scripts();
            break;
        }
        case FILE_TOGGLE_LEFT_PANEL: {
            toggle_scripts_panel();
            if (current) {
                current->update_toggle_scripts_button();
            }
            break;
        }
        case FILE_TOGGLE_RIGHT_PANEL: {
            toggle_components_panel();

            if (current) {
                current->update_toggle_components_button();
            }

            break;
        }
        case HELP_ONLINE_DOCUMENTATION: {
            _open_in_browser(OrchestratorPlugin::get_plugin_online_documentation_url());
            break;
        }
        case HELP_COMMUNITY: {
            _open_in_browser(OrchestratorPlugin::get_community_url());
            break;
        }
        case HELP_GITHUB_ISSUES:
        case HELP_GITHUB_FEATURE: {
            _open_in_browser(OrchestratorPlugin::get_github_issues_url());
            break;
        }
        case HELP_SUPPORT: {
            _open_in_browser(OrchestratorPlugin::get_patreon_url());
            break;
        }
        case HELP_ABOUT: {
            _about_dialog->popup_centered(ABOUT_DIALOG_SIZE * EDSCALE);
            break;
        }
        default: {
            break;
        }
    }

    if (current) {
        switch (p_option) {
            case FILE_SAVE: {
                save_current_script();
                break;
            }
            case FILE_SAVE_AS: {
                Ref<Resource> resource = current->get_edited_resource();
                Ref<OScript> script = resource;
                if (script.is_valid()) {
                    clear_docs_from_script(script);

                    push_item(resource.ptr());
                    save_resource_as(resource);

                    if (script.is_valid()) {
                        update_docs_from_script(script);
                    }
                }
                break;
            }
            case FILE_CLOSE: {
                if (current->is_unsaved()) {
                    _ask_close_current_unsaved_tab(current);
                } else {
                    _close_current_tab(false);
                }
                break;
            }
            case FILE_CLOSE_OTHERS: {
                _close_other_tabs();
                break;
            }
            case FILE_CLOSE_ALL: {
                _close_all_tabs();
                break;
            }
            case FILE_SOFT_RELOAD_TOOL_SCRIPT: {
                const Ref<Script> script = current->get_edited_resource();
                if (script.is_null()) {
                    ORCHESTRATOR_ERROR("Can't obtain script for reloading.");
                } else {
                    if (!script->is_tool()) {
                        ORCHESTRATOR_ERROR("Reloading only takes effect on tool orchestrations.");
                    } else {
                        script->reload(true);
                    }
                }
                break;
            }
            case FILE_COPY_PATH: {
                _copy_script_path();
                break;
            }
            case FILE_COPY_UID: {
                _copy_script_uid();
                break;
            }
            case FILE_SHOW_IN_FILESYSTEM: {
                const Ref<Resource> resource = current->get_edited_resource();
                if (resource.is_valid()) {
                    String path = resource->get_path();
                    if (!path.is_empty()) {
                        if (ResourceUtils::is_builtin(resource)) {
                            path = path.get_slice("::", 0);
                        }
                        EI->get_file_system_dock()->navigate_to_path(path);
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

void OrchestratorEditor::_tab_changed(int p_tab) {
    ensure_select_current();
}

void OrchestratorEditor::_show_getting_started() {
    _tab_container->hide();
    _tab_container->set_v_size_flags(SIZE_EXPAND);
    _getting_started->show();
}

void OrchestratorEditor::_show_tabs() {
    _tab_container->show();
    _tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    _getting_started->hide();
}

void OrchestratorEditor::_close_current_tab(bool p_save, bool p_history_back) {
    _close_tab(_tab_container->get_current_tab(), p_save, p_history_back);
}

void OrchestratorEditor::_close_tab(int p_idx, bool p_save, bool p_history_back) {
    int selected = p_idx;
    if (selected < 0 || selected >= _tab_container->get_tab_count()) {
        return;
    }

    Node* selected_node = _tab_container->get_tab_control(selected);

    OrchestratorEditorView* current = cast_to<OrchestratorEditorView>(selected_node);
    if (current) {
        const Ref<Resource> file = current->get_edited_resource();
        if (p_save && file.is_valid()) {
            if (!ResourceUtils::is_builtin(file)) {
                save_current_script();
            }
        }
        if (file.is_valid()) {
            if (!file->get_path().is_empty()) {
                // Only saved scripts can be restored
                _previous_scripts.push_back(file->get_path());
            }

            const Ref<Script> script = file;
            if (script.is_valid()) {
                notify_script_close(script);
            }
        }
    }

    int idx = _tab_container->get_current_tab();
    if (current) {
        current->clear_edit_menu();
        _save_editor_state(current);
    }
    memdelete(selected_node);

    if (_script_close_queue.is_empty()) {
        if (idx >= _tab_container->get_tab_count()) {
            idx = _tab_container->get_tab_count() - 1;
        }

        if (idx >= 0) {
            _go_to_tab(idx);
        } else {
            _update_selected_editor_menu();
            _update_online_doc();
        }

        _update_script_names();
        _save_layout();

        EI->inspect_object(nullptr);
    }

    if (_tab_container->get_tab_count() == 0) {
        _show_getting_started();
    }
}

void OrchestratorEditor::_close_discard_current_tab(const String& p_value) {
    Ref<OScript> script = _get_current_script();
    if (script.is_valid()) {
        script->reload_from_file();
    }
    _close_tab(_tab_container->get_current_tab(), false);
    _erase_tab_confirm->hide();
}

void OrchestratorEditor::_close_other_tabs() {
    int current_index = _tab_container->get_current_tab();
    for (int i = _tab_container->get_tab_count() - 1; i >= 0; i--) {
        if (i != current_index) {
            _script_close_queue.push_back(i);
        }
    }
    _queue_close_tabs();
}

void OrchestratorEditor::_close_all_tabs() {
    for (int i = _tab_container->get_tab_count() - 1; i >= 0; i--) {
        _script_close_queue.push_back(i);
    }
    _queue_close_tabs();
}

void OrchestratorEditor::_queue_close_tabs() {
    while (!_script_close_queue.is_empty()) {
        int index = _script_close_queue.front()->get();
        _script_close_queue.pop_front();

        _tab_container->set_current_tab(index);

        if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(index))) {
            if (view->is_unsaved()) {
                _ask_close_current_unsaved_tab(view);
                _erase_tab_confirm->connect(SceneStringName(visibility_changed), callable_mp_this(_queue_close_tabs), CONNECT_ONE_SHOT);
                break;
            }
        }

        _close_current_tab(false, false);
    }
}

void OrchestratorEditor::_ask_close_current_unsaved_tab(OrchestratorEditorView* p_current) {
    _erase_tab_confirm->set_text("Close and save changes?\n\"" + p_current->get_name() + "\"");
    _erase_tab_confirm->popup_centered();
}

void OrchestratorEditor::_go_to_tab(int p_idx) {
    OrchestratorEditorView* current = _get_current_editor();
    if (current && current->is_unsaved()) {
        current->apply_code();
    }

    Control* control = _tab_container->get_tab_control(p_idx);
    if (!control) {
        return;
    }

    _tab_container->set_current_tab(p_idx);
    control = _tab_container->get_current_tab_control();

    if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(control)) {
        _script_name_label->set_text(view->get_name());
        _script_icon->set_texture(view->get_theme_icon());
        if (is_visible_in_tree()) {
            view->ensure_focus();
        }

        Ref<Script> script = view->get_edited_resource();
        if (script.is_valid()) {
            notify_script_changed(script);
        }

        view->validate();
    }

    _update_script_colors();
    _update_selected_editor_menu();
    _update_online_doc();
}

void OrchestratorEditor::_copy_script_path() {
    OrchestratorEditorView* current = _get_current_editor();
    if (current) {
        const Ref<Resource> resource = current->get_edited_resource();
        DisplayServer::get_singleton()->clipboard_set(resource->get_path());
    }
}

void OrchestratorEditor::_copy_script_uid() {
    OrchestratorEditorView* current = _get_current_editor();
    if (current) {
        const Ref<Resource> resource = current->get_edited_resource();
        const int64_t uid = ResourceLoader::get_singleton()->get_resource_uid(resource->get_path());
        DisplayServer::get_singleton()->clipboard_set(ResourceUID::get_singleton()->id_to_text(uid));
    }
}

void OrchestratorEditor::_live_auto_reload_running_scripts() {
    _pending_auto_reload = false;

    #if GODOT_VERSION >= 0x040300
    if (_reload_all_scripts)
        OrchestratorEditorDebuggerPlugin::get_singleton()->reload_all_scripts();
    else
        OrchestratorEditorDebuggerPlugin::get_singleton()->reload_scripts(_script_paths_to_reload);
    #endif

    _reload_all_scripts = false;
    _script_paths_to_reload.clear();
}

void OrchestratorEditor::_filter_scripts_text_changed(const String& p_text) {
    _update_script_names();
}

struct OrchestratorEditorItemData {
    String name;
    String sort_key;
    Ref<Texture2D> icon;
    Ref<Texture2D> indicator_icon;
    bool tool = false;
    int index = 0;
    String tooltip;
    bool used = false;
    int category = 0;
    Node* ref = nullptr;

    bool operator<(const OrchestratorEditorItemData& other) const {
        if (category == other.category) {
            if (sort_key == other.sort_key) {
                return index < other.index;
            }
            #if GODOT_VERSION >= 0x040300
            return sort_key.filenocasecmp_to(other.sort_key) < 0;
            #else
            return sort_key.to_lower().casecmp_to(other.sort_key.to_lower()) < 0;
            #endif
        }
        return category < other.category;
    }
};

void OrchestratorEditor::_update_script_colors() {
    // currently a no-op
}

void OrchestratorEditor::_update_script_names() {
    if (_restoring_layout) {
        return;
    }

    HashSet<Ref<Script>> used;
    Node* edited_scene = EI->get_edited_scene_root();
    if (edited_scene && EDITOR_GET("text_editor/script_list/highlight_scene_scripts")) {
        find_scene_scripts(edited_scene, edited_scene, used);
    }

    _script_list->clear();

    ScriptSortBy sort_by = EDITOR_GET_ENUM(ScriptSortBy, "text_editor/script_list/sort_scripts_by");
    ScriptListName display_as = EDITOR_GET_ENUM(ScriptListName, "text_editor/script_list/list_script_names_as");

    Vector<OrchestratorEditorItemData> data;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i))) {
            Ref<Texture2D> icon = view->get_theme_icon();
            Ref<Texture2D> indicator_icon = view->get_indicator_icon();
            String path = view->get_edited_resource()->get_path();
            bool saved = !path.is_empty();
            String name = view->get_name();
            Ref<Script> script = view->get_edited_resource();

            OrchestratorEditorItemData item;
            item.icon = icon;
            item.indicator_icon = indicator_icon;
            item.name = name;
            item.tooltip = saved ? path : "Unsaved file.";
            item.index = i;
            item.used = used.has(view->get_edited_resource());
            item.category = 0;
            item.ref = view;
            if (script.is_valid()) {
                item.tool = script->is_tool();
            }

            switch (sort_by) {
                case SORT_BY_NAME: {
                    item.sort_key = name.to_lower();
                    break;
                }
                case SORT_BY_PATH: {
                    item.sort_key = path;
                    break;
                }
                case SORT_BY_NONE: {
                    item.sort_key = "";
                    break;
                }
            }

            switch (display_as) {
                case DISPLAY_NAME: {
                    item.name = name;
                    break;
                }
                case DISPLAY_DIR_AND_NAME: {
                    if (!path.get_base_dir().get_file().is_empty()) {
                        item.name = path.get_base_dir().get_file().path_join(name);
                    } else {
                        item.name = name;
                    }
                    break;
                }
                case DISPLAY_FULL_PATH: {
                    item.name = path;
                    break;
                }
            }

            if (!saved) {
                item.name = view->get_name();
            }

            data.push_back(item);
        }

        Vector<String> disambiguated_script_names;
        Vector<String> full_script_paths;
        for (int j = 0; j < data.size(); j++) {
            String name = data[j].name.replace("(*)", "");
            ScriptListName script_display = EDITOR_GET_ENUM(ScriptListName, "text_editor/script_list/list_script_names_as");

            switch (script_display) {
                case DISPLAY_NAME: {
                    name = name.get_file();
                    break;
                }
                case DISPLAY_DIR_AND_NAME: {
                    name = name.get_base_dir().get_file().path_join(name.get_file());
                    break;
                }
                default: {
                    break;
                }
            }
            disambiguated_script_names.append(name);
            full_script_paths.append(data[j].tooltip);
        }

        disambiguate_filenames(full_script_paths, disambiguated_script_names);

        for (int j = 0; j < data.size(); j++) {
            if (data[j].name.ends_with("(*)")) {
                data.write[j].name = vformat("%s(*)", disambiguated_script_names[j]);
            } else {
                data.write[j].name = disambiguated_script_names[j];
            }
        }
    }

    if (_sort_list_on_update && !data.is_empty()) {
        data.sort();

        // Change actual order of tab_container so that the order can be rearranged by user
        int cur_tab = _tab_container->get_current_tab();
        int prev_tab = _tab_container->get_previous_tab();
        int new_cur_tab = -1;
        int new_prev_tab = -1;
        for (int i = 0; i < data.size(); i++) {
            _tab_container->move_child(data[i].ref, i);
            if (new_prev_tab == -1 && data[i].index == prev_tab) {
                new_prev_tab = i;
            }
            if (new_cur_tab == -1 && data[i].index == cur_tab) {
                new_cur_tab = i;
            }

            // Update index of the data entries for sorted order
            OrchestratorEditorItemData item = data[i];
            item.index = i;
            data.set(i, item);
        }

        //_go_to_tab(new_prev_tab);
        //_go_to_tab(new_cur_tab);

        _sort_list_on_update = false;
    }

    Vector<OrchestratorEditorItemData> data_filtered;
    for (int i = 0; i < data.size(); i++) {
        const String filter = _filter_scripts->get_text();
        if (filter.is_empty() && filter.is_subsequence_ofn(data[i].name)) {
            data_filtered.push_back(data[i]);
        }
    }

    Color tool_color = get_theme_color("accent_color", "Editor");
    tool_color.set_s(tool_color.get_s() * 1.5f);

    for (int i = 0; i < data_filtered.size(); i++) {
        const Ref<Texture2D> icon = data_filtered[i].indicator_icon.is_valid()
            ? data_filtered[i].indicator_icon
            : data_filtered[i].icon;

        _script_list->add_item(data_filtered[i].name, icon);
        if (data_filtered[i].tool) {
            _script_list->set_item_icon_modulate(-1, tool_color);
        }

        int index = _script_list->get_item_count() - 1;
        _script_list->set_item_tooltip(index, data_filtered[i].tooltip);
        _script_list->set_item_metadata(index, data_filtered[i].index); // Script's index in the tab container

        if (data_filtered[i].used) {
            _script_list->set_item_custom_bg_color(index, Color(0.5, 0.5, 0.5, 0.125));
        }

        if (_tab_container->get_current_tab() == data_filtered[i].index) {
            _script_list->select(index);

            _script_name_label->set_text(data_filtered[i].name);
            _script_icon->set_texture(data_filtered[i].icon);

            OrchestratorEditorView* view = _get_current_editor();
            if (view) {
                view->enable_editor(this);
                _update_selected_editor_menu();
            }
        }
    }

    _waiting_update_names = false;
    _update_script_colors();
}

void OrchestratorEditor::_update_selected_editor_menu() {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        const bool current = _tab_container->get_current_tab() == i;

        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view && view->get_edit_menu()) {
            if (current) {
                view->get_edit_menu()->show();
            } else {
                view->get_edit_menu()->hide();
            }
        }
    }
}

void OrchestratorEditor::_update_online_doc() {
    _site_search->set_text("Online Docs");
    _site_search->set_tooltip_text("Open Orchestrator online documentation.");
}

void OrchestratorEditor::_script_list_clicked(int p_item, Vector2 p_local_mouse_pos, MouseButton p_button_index) {
    if (p_button_index == MOUSE_BUTTON_MIDDLE) {
        _script_list->select(p_item);
        _script_selected(p_item);
        _menu_option(FILE_CLOSE);
    }

    if (p_button_index == MOUSE_BUTTON_RIGHT) {
        _make_script_list_context_menu();
    }
}

void OrchestratorEditor::_make_script_list_context_menu() {
    _context_menu->clear();

    const int selected = _tab_container->get_current_tab();
    if (selected < 0 || selected >= _tab_container->get_tab_count()) {
        return;
    }

    OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(selected));
    if (view) {
        _context_menu->add_item("Save", FILE_SAVE);
        _context_menu->add_item("Save As...", FILE_SAVE_AS);
    }

    _context_menu->add_item("Close", FILE_CLOSE);
    _context_menu->add_item("Close All", FILE_CLOSE_ALL);
    _context_menu->add_item("Close Other Tabs", FILE_CLOSE_OTHERS);
    _context_menu->add_separator();

    if (view) {
        Ref<Resource> resource = view->get_edited_resource();
        Ref<Script> script = resource;

        if (script.is_valid() && script->is_tool()) {
            _context_menu->add_item("Soft Reload Tool Script");
            _context_menu->add_item("Run");
            _context_menu->add_separator();
        }

        _context_menu->add_item("Copy Script Path", FILE_COPY_PATH);
        _context_menu->set_item_disabled(-1, resource->get_path().is_empty());

        const int64_t uid = ResourceLoader::get_singleton()->get_resource_uid(resource->get_path());
        _context_menu->add_item("Copy Script UID", FILE_COPY_UID);
        _context_menu->set_item_disabled(-1, uid  == ResourceUID::INVALID_ID);

        _context_menu->add_item("Show in FileSystem", FILE_SHOW_IN_FILESYSTEM);
        _context_menu->add_separator();
    }

    _context_menu->add_item("Tooltip Orchestration Panel", FILE_TOGGLE_LEFT_PANEL);

    _context_menu->set_item_disabled(_context_menu->get_item_index(FILE_CLOSE_ALL), _tab_container->get_tab_count() == 0);
    _context_menu->set_item_disabled(_context_menu->get_item_index(FILE_CLOSE_OTHERS), _tab_container->get_tab_count() == 0);

    _context_menu->set_position(get_screen_position() + get_local_mouse_position());
    _context_menu->reset_size();
    _context_menu->popup();
}

void OrchestratorEditor::_script_selected(int p_index) {
    _grab_focus_block = !Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_LEFT);
    _go_to_tab(_script_list->get_item_metadata(p_index));
    _grab_focus_block = false;
}

void OrchestratorEditor::_script_changed() { // NOLINT
    if (Node* dock = get_connections_dock()) {
        dock->call("update_tree");
        return;
    }

    WARN_PRINT("Script %s changed but ConnectionsDock could not be notified.");
}

void OrchestratorEditor::_script_created(const Ref<Script>& p_script) {
    push_item(p_script.operator->());
}

void OrchestratorEditor::_add_callback(Object* p_object, const String& p_function, const PackedStringArray& p_args) {
    ERR_FAIL_NULL(p_object);

    const Ref<ScriptExtension> script = p_object->get_script();
    ERR_FAIL_COND(script.is_null());

    #if GODOT_VERSION >= 0x040300
    const ScriptLanguageExtension* language = cast_to<ScriptLanguageExtension>(script->_get_language());
    if (!language || !language->_can_make_function()) {
        return;
    }
    #endif

    cache_and_push_item(script.ptr());

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        if (view->get_edited_resource() != p_object->get_script()) {
            continue;
        }

        view->add_callback(p_function, p_args);
        _go_to_tab(i);

        // _script_list->select(_script_list->find_metadata(i));
        for (int j = 0; j < _script_list->get_item_count(); j++) {
            const int metadata = _script_list->get_item_metadata(j);
            if (metadata == i) {
                _script_list->select(j);
                break;
            }
        }

        if (!ResourceUtils::is_builtin(script)) {
            save_current_script();
        }

        break;
    }

    edit_previous_item();
}

void OrchestratorEditor::_resave_scripts(const String& p_value) {
    apply_scripts();

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        const Ref<Resource> resource = view->get_edited_resource();
        if (ResourceUtils::is_builtin(resource)) {
            continue;
        }

        save_resource(resource);

        view->tag_saved_version();
    }
    _disk_changed->hide();
}

void OrchestratorEditor::_reload_scripts(bool p_refresh_only) {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        Ref<Resource> edited_resource = view->get_edited_resource();
        if (ResourceUtils::is_builtin(edited_resource)) {
            continue;
        }

        const uint64_t modified_time = FileAccess::get_modified_time(edited_resource->get_path());

        if (p_refresh_only) {
            view->edited_file_data.last_modified_time = modified_time;
            view->reload_text();
            continue;
        }

        uint64_t last_modified_time = view->edited_file_data.last_modified_time;
        if (last_modified_time == modified_time) {
            continue;
        }

        view->edited_file_data.last_modified_time = modified_time;

        Ref<OScript> script = edited_resource;
        if (script.is_valid()) {
            // This does not work exactly like the ScriptEditor but it mimics.
            script->reload_from_file();

            // When reloaded, make sure the inspector is cleared as to avoid showing stale state.
            #if GODOT_VERSION >= 0x040400
            EI->get_inspector()->edit(nullptr);
            #else
            EI->inspect_object(nullptr);
            #endif

            update_docs_from_script(script);
        }

        view->reload_text();
    }

    _disk_changed->hide();

    _update_script_names();
}

void OrchestratorEditor::_resource_saved_callback(const Ref<Resource>& p_resource) {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            const Ref<Resource> edited_resource = view->get_edited_resource();
            if (edited_resource == p_resource) {
                view->tag_saved_version();
            }
        }
    }

    if (p_resource.is_valid()) {
        _mark_built_in_scripts_as_saved(p_resource->get_path());
    }

    _update_script_names();

    const Ref<Script> script = p_resource;
    if (script.is_valid()) {
        trigger_live_script_reload(script->get_path());
    }
}

void OrchestratorEditor::_mark_built_in_scripts_as_saved(const String& p_full_path) {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            const Ref<Resource> edited_resource = view->get_edited_resource();
            if (!ResourceUtils::is_builtin(edited_resource)) {
                continue;
            }

            if (edited_resource->get_path().get_slice("::", 0) == p_full_path) {
                view->tag_saved_version();
            }

            Ref<Script> script = edited_resource;
            if (script.is_valid()) {
                trigger_live_script_reload(script->get_path());
                if (script->is_tool()) {
                    script->reload(true);
                }
            }
        }
    }
}

PackedStringArray OrchestratorEditor::_get_recent_scripts() const { // NOLINT
    const Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    return metadata->get_value("recent_files", "orchestrations", PackedStringArray());
}

void OrchestratorEditor::_set_recent_scripts(const PackedStringArray& p_scripts) { // NOLINT
    const Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    metadata->set_value("recent_files", "orchestrations", p_scripts);
    OrchestratorPlugin::get_singleton()->save_metadata(metadata);
}

void OrchestratorEditor::_add_recent_script(const String& p_path) {
    if (p_path.is_empty()) {
        return;
    }

    PackedStringArray recents = _get_recent_scripts();
    if (recents.has(p_path)) {
        int64_t position = recents.find(p_path);
        if (position != -1) {
            recents.remove_at(position);
        }
    }

    recents.push_back(p_path);
    if (recents.size() > 10) {
        recents.resize(10);
    }

    _set_recent_scripts(recents);
    _update_recent_scripts();
}

void OrchestratorEditor::_update_recent_scripts() {
    GUARD_NULL(_recent_history);

    _recent_history->clear();

    const PackedStringArray recents = _get_recent_scripts();
    for (int i = 0; i < recents.size(); i++) {
        const String& path = recents[i];
        _recent_history->add_item(path.replace("res://", ""));
    }

    _recent_history->add_separator();
    _recent_history->add_shortcut(ED_GET_SHORTCUT("orchestrator_editor/clear_recent"));
    _recent_history->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_ALWAYS);
    _recent_history->set_item_disabled(_recent_history->get_item_id(_recent_history->get_item_count() - 1), recents.is_empty());

    _recent_history->reset_size();
}

void OrchestratorEditor::_open_recent_script(int p_index) {
    if (p_index == _recent_history->get_item_count() - 1) {
        _clear_recent_scripts();
        return;
    }

    PackedStringArray recents = _get_recent_scripts();
    ERR_FAIL_INDEX(p_index, recents.size());

    String path = recents[p_index];
    if (FileAccess::file_exists(path)) {
        const PackedStringArray extensions = get_recognized_extensions();
        if (extensions.find(path.get_extension())) {
            const Ref<Resource> script = ResourceLoader::get_singleton()->load(path);
            if (script.is_valid()) {
                edit(script, true);
                return;
            }
        }
    }

    recents.remove_at(p_index);

    _set_recent_scripts(recents);
    _update_recent_scripts();

    _error_dialog->set_text(vformat("Can't open '%s'. The file could have been moved or deleted.", path));
    _error_dialog->popup_centered();
}

void OrchestratorEditor::_clear_recent_scripts() {
    _set_recent_scripts(Array());
    callable_mp_this(_update_recent_scripts).call_deferred();
}

void OrchestratorEditor::_autosave_scripts() {
    save_all_scripts();
}

void OrchestratorEditor::_update_autosave_timer() {
    if (!_autosave_timer->is_inside_tree()) {
        return;
    }

    float autosave_time = EDITOR_GET("text_editor/behavior/files/autosave_interval_secs");
    if (autosave_time > 0) {
        _autosave_timer->set_wait_time(autosave_time);
        _autosave_timer->start();
        return;
    }

    _autosave_timer->stop();
}

bool OrchestratorEditor::_test_script_times_on_disk(const Ref<Resource>& p_for_script) {
    // See OrchestratorPlugin::_save_external_data
    // It cooperates with this method check during scene saves.

    _disk_changed_list->clear();
    TreeItem* root = _disk_changed_list->create_item();

    bool need_ask = false;
    bool need_reload = false;
    bool use_autoreload = EDITOR_GET("text_editor/behavior/files/auto_reload_scripts_on_external_change");

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            Ref<Resource> edited_resource = view->get_edited_resource();
            if (p_for_script.is_valid() && edited_resource.is_valid() && p_for_script != edited_resource) {
                continue;
            }

            if (ResourceUtils::is_builtin(edited_resource)) {
                continue;
            }

            uint64_t last_date = view->edited_file_data.last_modified_time;
            uint64_t date = FileAccess::get_modified_time(view->edited_file_data.path);
            if (last_date != date) {
                TreeItem* item = _disk_changed_list->create_item(root);
                item->set_text(0, view->edited_file_data.path.get_file());

                if (!use_autoreload || view->is_unsaved()) {
                    need_ask = true;
                }

                need_reload = true;
            }
        }
    }

    if (need_reload) {
        if (!need_ask) {
            reload_scripts();
            need_reload = false;
        }
        else {
            callable_mp_cast(_disk_changed, Window, popup_centered_ratio).call_deferred(0.3);
        }
    }

    return need_reload;
}

Ref<Script> OrchestratorEditor::_get_current_script() const{
    if (OrchestratorEditorView* current = _get_current_editor()) {
        Ref<Script> script = current->get_edited_resource();
        if (script.is_valid()) {
            return script;
        }
    }
    return nullptr;
}

TypedArray<Script> OrchestratorEditor::_get_open_scripts() const {
    const Vector<Ref<Script>> scripts = get_open_scripts();
    const int script_count = scripts.size();

    TypedArray<Script> result;
    for (int i = 0; i < script_count; i++) {
        result.push_back(scripts[i]);
    }

    return result;
}

void OrchestratorEditor::_goto_script_node(int p_node) {
    if (OrchestratorEditorView* current = _get_current_editor()) {
        current->goto_node(p_node);
    }
}

void OrchestratorEditor::_goto_script_line(const Ref<RefCounted>& p_script, int p_line) {
    const Ref<Script> script = cast_to<Script>(*p_script);
    if (script.is_valid() && ResourceUtils::is_file(script->get_path())) {
        if (edit(p_script, p_line)) {
            push_item(p_script.ptr());

            OrchestratorEditorView* current = _get_current_editor();
            current->goto_node(p_line);

            _save_history();
        }
    }
}

void OrchestratorEditor::_breaked(bool p_breaked, bool p_can_debug) {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            view->set_debugger_active(p_breaked);
        }
    }
}

PackedStringArray OrchestratorEditor::_get_breakpoints() const {
    PackedStringArray breakpoints;

    HashSet<String> loaded_scripts;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        Ref<Script> script = view->get_edited_resource();
        if (!script.is_valid()) {
            continue;
        }

        String base = script->get_path();
        loaded_scripts.insert(base);
        if (base.is_empty() || base.begins_with("local://")) {
            continue;
        }

        PackedInt32Array view_breakpoints = view->get_breakpoints();
        for (int32_t point : view_breakpoints) {
            breakpoints.push_back(vformat("%s:%d", base, point));
        }
    }

    const PackedStringArray cached_editors = _editor_cache->get_sections();
    for (const String& section : cached_editors) {
        if (loaded_scripts.has(section)) {
            continue;
        }
        const Array section_breakpoints = _get_cached_breakpoints_for_script(section);
        for (int i = 0; i < section_breakpoints.size(); i++) {
            breakpoints.push_back(vformat("%s:%d", section, section_breakpoints[i]));
        }
    }

    return breakpoints;
}

void OrchestratorEditor::_set_breakpoint(const Ref<RefCounted>& p_script, int p_node, bool p_enabled) {
    Ref<Script> script = cast_to<Script>(*p_script);
    if (script.is_valid() && ResourceUtils::is_file(script->get_path())) {
        // Update if its open
        for (int i = 0; i < _tab_container->get_tab_count(); i++) {
            OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
            if (view && view->get_edited_resource()->get_path() == script->get_path()) {
                view->set_breakpoint(p_node, p_enabled);
                return;
            }
        }

        // Handle closed.
        Dictionary state = _editor_cache->get_value(script->get_path(), "state", Dictionary());

        Array breakpoints;
        if (state.has("breakpoints")) {
            breakpoints = state["breakpoints"];
        }

        if (breakpoints.has(p_node)) {
            if (p_enabled) {
                breakpoints.erase(p_node);
            }
        } else if (p_enabled) {
            breakpoints.push_back(p_node);
        }

        state["breakpoints"] = breakpoints;
        _editor_cache->set_value(script->get_path(), "state", state);

        #if GODOT_VERSION >= 0x040300
        if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton()) {
            debugger->set_breakpoint(script->get_path(), p_node, p_enabled);
        }
        #endif
    }
}

void OrchestratorEditor::_clear_breakpoints() {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i))) {
            view->clear_breakpoints();
        }
    }

    const PackedStringArray cached_editors = _editor_cache->get_sections();
    for (const String& section : cached_editors) {
        Array breakpoints = _get_cached_breakpoints_for_script(section);

        #if GODOT_VERSION >= 0x040300
        for (int i = 0; i < breakpoints.size(); i++) {
            OrchestratorEditorDebuggerPlugin::get_singleton()->set_breakpoint(section, breakpoints[i], false);
        }
        #endif

        if (breakpoints.size() > 0) {
            Dictionary state = _editor_cache->get_value(section, "state");
            state["breakpoints"] = Array();
            _editor_cache->set_value(section, "state", state);
        }
    }
}

Array OrchestratorEditor::_get_cached_breakpoints_for_script(const String& p_path) const {
    if (!ResourceLoader::get_singleton()->exists(p_path, "Script")
        || p_path.begins_with("local://")
        || !_editor_cache->has_section_key(p_path, "state")) {
        return {};
    }

    Dictionary state = _editor_cache->get_value(p_path, "state");
    if (!state.has("breakpoints")) {
        return {};
    }

    return state["breakpoints"];
}

void OrchestratorEditor::_window_changed(bool p_visible) {
    _make_floating->set_visible(!p_visible);
    _make_floating_separator->set_visible(!p_visible);
    _floating = p_visible;
}

void OrchestratorEditor::_tree_changed() {
}

void OrchestratorEditor::_split_dragged(float p_value) {
    _save_layout();
}

void OrchestratorEditor::_apply_editor_settings() {
    _update_autosave_timer();
    _update_script_names();

    ScriptServer::set_reload_scripts_on_save(EDITOR_GET("text_editor/behavior/files/auto_reload_and_parse_scripts_on_save"));

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i))) {
            view->update_settings();
        }
    }
}

void OrchestratorEditor::_editor_settings_changed() {
    // The ScriptEditor predicates this, but we don't have access to that in godot-cpp right now
    _apply_editor_settings();
}

void OrchestratorEditor::_filesystem_changed() {
    _update_script_names();
}

void OrchestratorEditor::_file_removed(const String& p_file) {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view && view->edited_file_data.path == p_file) {
            _close_tab(i, false, false);
        }
    }

    // Check closed
    if (_editor_cache->has_section(p_file)) {
        #if GODOT_VERSION >= 0x040300
        Array breakpoints = _get_cached_breakpoints_for_script(p_file);
        for (int i = 0; i < breakpoints.size(); i++) {
            OrchestratorEditorDebuggerPlugin::get_singleton()->set_breakpoint(p_file, breakpoints[i], false);
        }

        #endif

        _editor_cache->erase_section(p_file);
    }
}

void OrchestratorEditor::_files_moved(const String& p_old_file, const String& p_new_file) {
    if (!_editor_cache->has_section(p_old_file)) {
        return;
    }

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view && view->edited_file_data.path == p_old_file) {
            view->edited_file_data.path = p_new_file;
            break;
        }
    }

    Variant state = _editor_cache->get_value(p_old_file, "state");
    _editor_cache->erase_section(p_old_file);
    _editor_cache->set_value(p_new_file, "state", state);

    #if GODOT_VERSION >= 0x040300
    Array breakpoints = _get_cached_breakpoints_for_script(p_new_file);
    for (int i = 0; i < breakpoints.size(); i++) {
        OrchestratorEditorDebuggerPlugin::get_singleton()->set_breakpoint(p_old_file, breakpoints[i], false);
        if (!p_new_file.begins_with("local://") && ResourceLoader::get_singleton()->exists(p_new_file, "Script")) {
            OrchestratorEditorDebuggerPlugin::get_singleton()->set_breakpoint(p_new_file, breakpoints[i], true);
        }
    }
    #endif
}

OrchestratorEditorView* OrchestratorEditor::_get_current_editor() const {
    const int selected = _tab_container->get_current_tab();
    if (selected < 0 || selected >= _tab_container->get_tab_count()) {
        return nullptr;
    }
    return cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(selected));
}

TypedArray<OrchestratorEditorView> OrchestratorEditor::_get_open_script_editors() const {
    TypedArray<OrchestratorEditorView> script_editors;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        if (OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i))) {
            script_editors.push_back(view);
        }
    }
    return script_editors;
}

void OrchestratorEditor::_view_layout_restored(OrchestratorEditorView* p_view) {
    for (int i = 0; i < _restore_queue.size(); i++) {
        if (_restore_queue[i] == p_view) {
            _restore_queue.remove_at(i);
            break;
        }
    }

    if (_restore_queue.is_empty()) {
        _restoring_layout = false;
        OrchestratorPlugin::get_singleton()->queue_save_layout();
    }
}

void OrchestratorEditor::_save_layout() {
    if (_restoring_layout) {
        return;
    }
    save_editor_layout_delayed();
}

void OrchestratorEditor::_save_editor_state(OrchestratorEditorView* p_editor) {
    if (_restoring_layout) {
        return;
    }

    const String& path = p_editor->get_edited_resource()->get_path();
    if (!ResourceUtils::is_file(path)) {
        return;
    }

    _editor_cache->set_value(path, "state", p_editor->get_edit_state());
}

void OrchestratorEditor::_save_previous_state([[maybe_unused]] Dictionary p_state) {
}

void OrchestratorEditor::_save_history() {
}

void OrchestratorEditor::_help_search(const String& p_text) {
    if (Node* editor_node = EditorNode) {
        editor_node->emit_signal("request_help_search", p_text);
    }
}

bool OrchestratorEditor::_is_editor_setting_script_list_visible() const { // NOLINT
    return PROJECT_GET("Orchestrator", "file_list_visibility", true);
}

void OrchestratorEditor::_project_settings_changed() {
    _update_input_actions_cache();
}

void OrchestratorEditor::_update_input_actions_cache() {
    Vector<InputAction> cache;

    const Ref<ConfigFile> project(memnew(ConfigFile));
    if (project->load("res://project.godot") == OK) {
        if (project->has_section("input")) {
            const PackedStringArray keys = project->get_section_keys("input");
            for (const String& key : keys) {
                cache.push_back({ key, true });
            }
        }
    }

    const TypedArray<StringName> action_names = InputMap::get_singleton()->get_actions();
    for (int i = 0; i < action_names.size(); i++) {
        const StringName& action_name = action_names[i];
        cache.push_back({ action_name, false });
    }

    if (_input_action_cache != cache) {
        _input_action_cache = cache;
        emit_signal("input_action_cache_updated");
    }
}

void OrchestratorEditor::_shortcut_input(const Ref<InputEvent>& p_event) {
    ERR_FAIL_COND(p_event.is_null());

    if (!is_visible_in_tree() || !p_event->is_pressed()) {
        return;
    }

    if (ED_IS_SHORTCUT("orchestrator_editor/clear_recent", p_event)) {
        _clear_recent_scripts();
        accept_event();
    }
}

Ref<OrchestratorEditorGraphNodeThemeCache> OrchestratorEditor::get_theme_cache() const {
    return _theme_cache;
}

bool OrchestratorEditor::toggle_scripts_panel() {
    _scripts_vbox->set_visible(!_scripts_vbox->is_visible());
    PROJECT_SET("Orchestrator", "file_list_visibility", _scripts_vbox->is_visible());
    return _scripts_vbox->is_visible();
}

bool OrchestratorEditor::is_scripts_panel_toggled() {
    return _scripts_vbox->is_visible();
}

void OrchestratorEditor::toggle_components_panel() { // NOLINT
    const bool visibility = PROJECT_GET("Orchestrator", "component_panel_visibility", true);
    PROJECT_SET("Orchestrator", "component_panel_visibility", !visibility);

    // This must be done because changing project metadata doesn't raise a changed signal
    // Observers like the OrchestratorEditorScriptGraphView must listen to ProjectSettings
    ProjectSettings::get_singleton()->emit_signal("settings_changed");
}

void OrchestratorEditor::apply_scripts() {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            view->apply_code();
        }
    }
}

void OrchestratorEditor::reload_scripts(bool p_refresh_only) {
    // Call deferred to make sure it runs on the main thread
    callable_mp_this(_reload_scripts).call_deferred(p_refresh_only);
}

PackedStringArray OrchestratorEditor::get_unsaved_scripts() const {
    PackedStringArray unsaved_scripts;
    if (_tab_container) {
        for (int i = 0; i < _tab_container->get_tab_count(); i++) {
            OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
            if (view && view->is_unsaved()) {
                unsaved_scripts.push_back(view->get_name());
            }
        }
    }
    return unsaved_scripts;
}

void OrchestratorEditor::save_current_script() {
    OrchestratorEditorView* view = _get_current_editor();
    if (!view || _test_script_times_on_disk()) {
        return;
    }

    Ref<OScript> script = view->get_edited_resource();
    if (script.is_valid()) {
        clear_docs_from_script(script);
        save_resource(script);
        update_docs_from_script(script);
    }
}

void OrchestratorEditor::save_all_scripts() {
    HashSet<String> scenes_to_save;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view || !view->is_unsaved()) {
            continue;
        }

        Ref<Resource> edited_resource = view->get_edited_resource();
        if (edited_resource.is_valid()) {
            view->apply_code();
        }

        if (!ResourceUtils::is_builtin(edited_resource)) {
            Ref<OScript> script = edited_resource;
            if (script.is_valid()) {
                clear_docs_from_script(script);
            }
            save_resource(edited_resource);

            if (script.is_valid()) {
                update_docs_from_script(script);
            }
        }
    }

    _update_script_names();
}

void OrchestratorEditor::update_script_times() {
    // See OrchestratorPlugin::_save_external_data
    // It cooperates with this method check during scene saves.
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (view) {
            view->edited_file_data.last_modified_time = FileAccess::get_modified_time(view->edited_file_data.path);
        }
    }
}

void OrchestratorEditor::update_docs_from_script(const Ref<Script>& p_script) { // NOLINT
    #if GODOT_VERSION >= 0x040400
    EI->get_script_editor()->update_docs_from_script(p_script);
    #endif
}

void OrchestratorEditor::clear_docs_from_script(const Ref<Script>& p_script) {
    // todo:
    //  The ScriptEditor has this method where it checks whether the provided Script has any
    //  documentation, and if so and the EditorHelp has it, removes the docs for the script.
    //  Given that ScriptEditor exposes update_docs_from_script, it makes sense that we also
    //  introduce the same for clear_docs_from_script.
    //  see https://github.com/godotengine/godot/pull/107862
    // EI->get_script_editor()->clear_docs_from_script(p_script);
}

Vector<Ref<Script>> OrchestratorEditor::get_open_scripts() const {
    Vector<Ref<Script>> scripts;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        const Ref<Script> script = view->get_edited_resource();
        if (!script.is_valid()) {
            continue;
        }

        scripts.push_back(script);
    }
    return scripts;
}

bool OrchestratorEditor::script_goto_node(const Ref<Script>& p_script, int p_node) {
    const Ref<OScript> script = p_script;
    if (script.is_valid() && script->get_orchestration().is_valid()) {
        const Ref<OScriptNode> node = script->get_orchestration()->get_node(p_node);
        if (node.is_valid()) {
            return edit(p_script, node->get_id(), true);
        }
    }
    return false;
}

bool OrchestratorEditor::script_goto_method(const Ref<Script>& p_script, const String& p_method)
{
    const Ref<OScript> script = p_script;
    if (script.is_valid() && script->get_orchestration().is_valid()) {
        const Ref<OScriptFunction> function = script->get_orchestration()->find_function(StringName(p_method));
        if (function.is_valid()) {
            int function_node_id = function->get_owning_node_id();
            return edit(p_script, function_node_id, true);
        }
    }
    return false;
}

void OrchestratorEditor::open_script_create_dialog(const String& p_base_name, const String& p_base_path) {
    _menu_option(FILE_NEW);
    _script_create_dialog->config(p_base_name, p_base_path);
}

Ref<Resource> OrchestratorEditor::open_file(const String& p_file) {
    const PackedStringArray extensions = get_recognized_extensions();
    if (!extensions.find(p_file.get_extension())) {
        return {};
    }

    const Ref<Resource> resource = ResourceLoader::get_singleton()->load(p_file);
    if (resource.is_null()) {
        WARN_PRINT_ED("Could not load file at: \n\n" + p_file);
        return {};
    }

    edit(resource);
    return resource;
}

void OrchestratorEditor::ensure_select_current() {
    if (_tab_container->get_tab_count() && _tab_container->get_current_tab() >= 0) {
        if (OrchestratorEditorView* view = _get_current_editor()) {
            view->enable_editor(this);
            if (!_grab_focus_block && is_visible_in_tree()) {
                view->ensure_focus();
            }
        }
    }
    _update_selected_editor_menu();
}

bool OrchestratorEditor::is_editor_floating() { // NOLINT
    return _floating;
}

bool OrchestratorEditor::edit(const Ref<Resource>& p_resource, int p_node, bool p_grab_focus) {
    if (!p_resource.is_valid()) {
        return false;
    }

    const Ref<OScript> script = p_resource;

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        if ((script.is_valid() && p_resource == view->get_edited_resource())
            || view->get_edited_resource()->get_path() == p_resource->get_path()) {
            view->enable_editor(this);

            if (_tab_container->get_current_tab() != i) {
                _go_to_tab(i);
            }
            if (is_visible_in_tree()) {
                view->ensure_focus();
            }
            if (p_node > 0) {
                view->goto_node(p_node);
            }

            _update_script_names();
            _script_list->ensure_current_is_visible();

            return true;
        }
    }

    OrchestratorEditorView* view = nullptr;
    for (int i = _editor_view_func_count - 1; i >= 0; i--) {
        view = _editor_view_funcs[i](p_resource);
        if (view) {
            break;
        }
    }
    ERR_FAIL_NULL_V(view, false);

    view->set_edited_resource(p_resource);

    _tab_container->add_child(view);
    if (_tab_container->get_tab_count() > 0) {
        _show_tabs();
    }

    if (p_grab_focus) {
        view->enable_editor(this);
    }

    view->edited_file_data.path = p_resource->get_path();
    view->edited_file_data.last_modified_time = FileAccess::get_modified_time(p_resource->get_path());

    if (view->get_edit_menu()) {
        view->get_edit_menu()->hide();
        _menu_hb->add_child(view->get_edit_menu());
        _menu_hb->move_child(view->get_edit_menu(), 1);
    }

    if (p_grab_focus) {
        _go_to_tab(_tab_container->get_tab_count() - 1);
        _add_recent_script(p_resource->get_path());
    }

    if (_editor_cache->has_section(p_resource->get_path())) {
        if (_restoring_layout) {
            _restore_queue.push_back(view);
            view->connect("view_layout_restored", callable_mp_this(_view_layout_restored).bind(view));
        }

        view->set_edit_state(_editor_cache->get_value(p_resource->get_path(), "state"));
        view->store_previous_state();
    } else {
        // If there is no saved state, directly call this
        _view_layout_restored(view);
    }

    _sort_list_on_update = true;
    _update_script_names();
    _save_layout();

    view->connect("name_changed", callable_mp_this(_update_script_names));
    view->connect("edited_script_changed", callable_mp_this(_script_changed));
    view->connect("request_help", callable_mp_this(_help_search));
    view->connect("request_open_script_at_line", callable_mp_this(_goto_script_line));
    view->connect("request_save_history", callable_mp_this(_save_history));
    view->connect("request_save_previous_state", callable_mp_this(_save_previous_state));
    view->connect("go_to_method", callable_mp_this(script_goto_method));

    _test_script_times_on_disk(p_resource);

    if (p_node) {
        view->goto_node(p_node);
    }

    notify_script_changed(p_resource);

    return true;
}

void OrchestratorEditor::notify_script_close(const Ref<Script>& p_script) {
    // Notifies that a script was closed
    emit_signal("script_close", p_script);
}

void OrchestratorEditor::notify_script_changed(const Ref<Script>& p_script) {
    // Notifies that a script tab was changed
    emit_signal("editor_script_changed", p_script);
}

void OrchestratorEditor::notify_scene_changed(Node* p_node) {
    // Notifies that a scene tab changed
    emit_signal("scene_changed", p_node);
}

void OrchestratorEditor::trigger_live_script_reload(const String& p_script_path) {
    if (!_script_paths_to_reload.has(p_script_path)) {
        _script_paths_to_reload.append(p_script_path);
    }

    if (!_pending_auto_reload && _auto_reload_running_scripts) {
        callable_mp_this(_live_auto_reload_running_scripts).call_deferred();
        _pending_auto_reload = true;
    }
}

void OrchestratorEditor::trigger_live_script_reload_all() {
    if (!_pending_auto_reload && _auto_reload_running_scripts) {
        callable_mp_this(_live_auto_reload_running_scripts).call_deferred();
        _pending_auto_reload = true;
        _reload_all_scripts = true;
    }
}

void OrchestratorEditor::set_live_auto_reload_running_scripts(bool p_enabled) {
    _auto_reload_running_scripts = p_enabled;
}

PackedStringArray OrchestratorEditor::get_breakpoints() {
    return _get_breakpoints();
}

void OrchestratorEditor::get_window_layout(const Ref<ConfigFile>& r_layout) {
    String selected_path;
    PackedStringArray open_files;

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(_tab_container->get_tab_control(i));
        if (!view) {
            continue;
        }

        const Ref<Resource> resource = view->get_edited_resource();
        if (!resource.is_valid()) {
            continue;
        }

        const String path = resource->get_path();
        if (!ResourceUtils::is_file(path)) {
            continue;
        }

        if (_tab_container->get_current_tab_control() == _tab_container->get_tab_control(i)) {
            selected_path = path;
        }

        _save_editor_state(view);
        open_files.push_back(path);
    }

    r_layout->set_value("Orchestrator", "open_files", open_files);

    if (selected_path.is_empty()) {
        if (r_layout->has_section_key("Orchestrator", "open_files_selected")) {
            r_layout->erase_section_key("Orchestrator", "open_files_selected");
        }
    } else {
        r_layout->set_value("Orchestrator", "open_files_selected", selected_path);
    }

    r_layout->set_value("Orchestrator", "file_list_visibility", _script_split->is_visible());
    r_layout->set_value("Orchestrator", "left_list_width", _script_split->get_split_offset());

    for (const KeyValue<String, Variant>& E : _extra_layout_values) {
        r_layout->set_value("Orchestrator", E.key, E.value);
    }

    _editor_cache->save(EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator_editor_cache.cfg"));
}

void OrchestratorEditor::set_window_layout(const Ref<ConfigFile>& p_layout) {
    _restoring_layout = true;

    if (p_layout->has_section("Orchestrator")) {
        for (const String& key : p_layout->get_section_keys("Orchestrator")) {
            if (key == "open_files" || key == "file_list_visibility" || key == "left_list_width"
                || key == "open_files" || key == "open_files_selected") {
                continue;
                }
            _extra_layout_values[key] = p_layout->get_value("Orchestrator", key);
        }
    }

    const bool restore_windows = OrchestratorPlugin::get_singleton()->restore_windows_on_load();
    if (!restore_windows && !p_layout->has_section_key("Orchestrator", "open_files")) {
        return;
    }

    _script_split->set_visible(p_layout->get_value("Orchestrator", "file_list_visibility", true));
    _script_split->set_split_offset(p_layout->get_value("Orchestrator", "left_list_width", 0));

    PackedStringArray open_files = p_layout->get_value("Orchestrator", "open_files", PackedStringArray());
    for (const String& file_name : open_files) {
        edit(ResourceLoader::get_singleton()->load(file_name), false);
    }

    const String selected_file_name = p_layout->get_value("Orchestrator", "open_files_selected", "");
    if (!selected_file_name.is_empty()) {
        edit(ResourceLoader::get_singleton()->load(selected_file_name), true);
    }

    _update_script_names();
}

bool OrchestratorEditor::can_take_away_focus() const {
    OrchestratorEditorView* current = _get_current_editor();
    if (current) {
        return current->can_lose_focus_on_node_selection();
    }

    return true;
}

Variant OrchestratorEditor::get_drag_data_fw(const Point2& p_point, Control* p_from) {
    if (_tab_container->get_tab_count() == 0) {
        return {};
    }

    Node* current_node = _tab_container->get_tab_control(_tab_container->get_current_tab());

    HBoxContainer* drag_preview = memnew(HBoxContainer);
    String preview_name = "";
    Ref<Texture2D> preview_icon;

    OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(current_node);
    if (view) {
        preview_name = view->get_name();
        preview_icon = view->get_theme_icon();
    }

    if (preview_icon.is_valid()) {
        TextureRect* tex = memnew(TextureRect);
        tex->set_texture(preview_icon);
        tex->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
        drag_preview->add_child(tex);
    }

    Label* label = memnew(Label);
    label->set_text(preview_name);
    #if GODOT_VERSION >= 0x040300
    label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    #endif
    drag_preview->add_child(label);

    set_drag_preview(drag_preview);

    Dictionary data;
    data["type"] = "orchestration_list_element";
    data["orchestration_list_element"] = current_node;

    return data;
}

bool OrchestratorEditor::can_drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) const {
    Dictionary data = p_data;
    if (!data.has("type")) {
        return false;
    }

    const String type = data["type"];
    if (type == "orchestration_list_element") {
        Node* node = cast_to<Node>(data["orchestration_list_element"]);
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(node);
        if (view) {
            return true;
        }
    }

    return false;
}

void OrchestratorEditor::drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) {
    if (!can_drop_data_fw(p_point, p_data, p_from)) {
        return;
    }

    Dictionary data = p_data;
    if (!data.has("type")) {
        return;
    }

    const String type = data["type"];
    if (type == "orchestration_list_element") {
        Node* node = cast_to<Node>(data["orchestration_list_element"]);
        OrchestratorEditorView* view = cast_to<OrchestratorEditorView>(node);
        if (view) {
            int new_index = 0;
            if (_script_list->get_item_count() > 0) {
                new_index = _script_list->get_item_metadata(_script_list->get_item_at_position(p_point));
            }

            _tab_container->move_child(node, new_index);
            _tab_container->set_current_tab(new_index);

            _update_script_names();
        }
    }
}

PackedStringArray OrchestratorEditor::get_recognized_extensions() const { // NOLINT
    PackedStringArray extensions;
    extensions.append_array(ResourceLoader::get_singleton()->get_recognized_extensions_for_type(OScript::get_class_static()));
    return extensions;
}

void OrchestratorEditor::register_create_view_function(OrchestratorEditorViewFunc p_function) {
    ERR_FAIL_COND(_editor_view_func_count == ORCHESTRATOR_VIEW_FUNC_MAX);
    _editor_view_funcs[_editor_view_func_count++] = p_function;
}

void OrchestratorEditor::find_scene_scripts(Node* p_base, Node* p_current, HashSet<Ref<Script>>& r_used) {
    if (p_current != p_base && p_current->get_owner() != p_base) {
        return;
    }

    #if GODOT_VERSION >= 0x040500
    // Was introduced in Godot 4.5
    if (GDE_INTERFACE(object_get_script_instance)(p_current, nullptr))
    #endif
    {
        Ref<Script> script = p_current->get_script();
        if (script.is_valid()) {
            r_used.insert(script);
        }
    }

    for (int i = 0; i < p_current->get_child_count(); i++) {
        find_scene_scripts(p_base, p_current->get_child(i), r_used);
    }
}

void OrchestratorEditor::push_item(Object* p_object, const String& p_property, bool p_inspector_only) {
    if (Node* editor_node = EditorNode) {
        editor_node->call("push_item", p_object, p_property, p_inspector_only);
    }
}

void OrchestratorEditor::cache_and_push_item(Object* p_object, const String& p_property, bool p_inspector_only) {
    _previous_item = EI->get_inspector()->get_edited_object();
    push_item(p_object);
}

void OrchestratorEditor::edit_previous_item() {
    // Move back to the previously edited node to reselect it in the Inspector and the NodeDock
    // We assume that the previous item is the node on which the callback was added
    EI->inspect_object(_previous_item);
    _previous_item = nullptr;
}

void OrchestratorEditor::save_resource(const Ref<Resource>& p_resource) {
    // This is taken from editor_node.cpp and is a scaled down version of what the EditorNode offers.
    if (ResourceUtils::is_builtin(p_resource)) {
        WARN_PRINT_ED("OrchestratorEditor cannot save built-in resources.");
        return;
    }

    const String path = p_resource->get_path();
    if (ResourceUtils::is_file(path) && !FileAccess::file_exists(vformat("%s.import", path))) {
        save_resource_in_path(p_resource, p_resource->get_path());
    } else {
        save_resource_as(p_resource, p_resource->get_path());
    }
}

void OrchestratorEditor::save_resource_in_path(const Ref<Resource>& p_resource, const String& p_path) {
    int flags = ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

    if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
        flags |= ResourceSaver::FLAG_COMPRESS;
    }

    const String path = ProjectSettings::get_singleton()->localize_path(p_path);
    const Error error = ResourceSaver::get_singleton()->save(p_resource, p_path, flags);
    if (error != OK) {
        ORCHESTRATOR_ACCEPT("Error saving resource!");
    }

    p_resource->set_path(p_path);
    // EI->get_resource_filesystem()->update_file(p_path);

    EditorNode->emit_signal("resource_saved", p_resource);
}

void OrchestratorEditor::save_resource_as(const Ref<Resource>& p_resource, const String& p_path) {
    _file_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
    _file_dialog_option = FILE_SAVE_AS;

    const PackedStringArray extensions = ResourceSaver::get_singleton()->get_recognized_extensions(p_resource);
    _file_dialog->clear_filters();

    List<String> preferred;
    for (const String& extension : extensions) {
        if (ClassDB::is_parent_class(p_resource->get_class(), "Script") && (extension == "tres" || extension == "res")) {
            continue;
        }

        _file_dialog->add_filter("*." + extension, extension.to_upper());
        preferred.push_back(extension);
    }

    if (!p_path.is_empty()) {
        _file_dialog->set_current_dir(p_path);
        if (ResourceUtils::is_file(p_resource->get_path())) {
            _file_dialog->set_current_file(p_resource->get_path().get_file());
        }
    } else if (!p_resource->get_path().is_empty()) {
        _file_dialog->set_current_path(p_resource->get_path());
    } else if (!preferred.is_empty()) {
        const String resource_name_snake_case = p_resource->get_class().to_snake_case();
        const String existing = "new_" + resource_name_snake_case + "." + preferred.front()->get().to_lower();
        _file_dialog->set_current_path(existing);
    }

    _file_dialog->set_title("Save Orchestration As...");
    _file_dialog->popup_file_dialog();
}

void OrchestratorEditor::save_editor_layout_delayed() {
    // As of Godot 4.4.1, the first Timer child of EditorNode should be started.
    if (Node* editor_node = EditorNode) {
        const TypedArray<Node> timers = editor_node->find_children("*", "Timer", true, false);
        #if GODOT_VERSION >= 0x040400
        if (!timers.is_empty()) {
            if (Timer* timer = cast_to<Timer>(timers.get(0))) {
                timer->start();
            }
        }
        #else
        for (int i = 0; i < timers.size(); i++) {
            Timer* timer = cast_to<Timer>(timers[0]);
            if (timer) {
                timer->start();
            }
            break;
        }
        #endif
    }
}

void OrchestratorEditor::disambiguate_filenames(const Vector<String>& p_full_paths, Vector<String>& r_filenames) {
    // Taken from editor_node.cpp
    ERR_FAIL_COND_MSG(p_full_paths.size() != r_filenames.size(),
        vformat("disambiguate_filenames requires two string vectors of same length (%d != %d).",
            p_full_paths.size(), r_filenames.size()));

	// Keep track of a list of "index sets," i.e. sets of indices
	// within disambiguated_scene_names which contain the same name.
	Vector<RBSet<int>> index_sets;
	HashMap<String, int> scene_name_to_set_index;
	for (int i = 0; i < r_filenames.size(); i++) {
		const String &scene_name = r_filenames[i];
	    if (!scene_name_to_set_index.has(scene_name)) {
			index_sets.append(RBSet<int>());
			scene_name_to_set_index.insert(r_filenames[i], index_sets.size() - 1);
		}
		index_sets.write[scene_name_to_set_index[scene_name]].insert(i);
	}

	// For each index set with a size > 1, we need to disambiguate.
	for (int i = 0; i < index_sets.size(); i++) {
		RBSet<int> iset = index_sets[i];
		while (iset.size() > 1) {
			// Append the parent folder to each scene name.
			for (const int &E : iset) {
				int set_idx = E;
				String scene_name = r_filenames[set_idx];
				String full_path = p_full_paths[set_idx];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (full_path.begins_with("res://"))
					full_path = full_path.substr(6);

				full_path = full_path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				full_path = full_path.trim_suffix("/");

				int scene_name_size = scene_name.length();
				int full_path_size = full_path.length();
				int difference = full_path_size - scene_name_size;

				// Find just the parent folder of the current path and append it.
				// If the current name is foo.tscn, and the full path is /some/folder/foo.tscn
				// then slash_idx is the second '/', so that we select just "folder", and
				// append that to yield "folder/foo.tscn".
				if (difference > 0) {
					String parent = full_path.substr(0, difference);
					int slash_idx = parent.rfind("/");
					slash_idx = parent.rfind("/", slash_idx - 1);
					parent = (slash_idx >= 0 && parent.length() > 1) ? parent.substr(slash_idx + 1) : parent;
					r_filenames.write[set_idx] = parent + r_filenames[set_idx];
				}
			}

			// Loop back through scene names and remove non-ambiguous names.
			bool can_proceed = false;
			RBSet<int>::Element *E = iset.front();
			while (E) {
				String scene_name = r_filenames[E->get()];
				bool duplicate_found = false;
				for (const int &F : iset) {
					if (E->get() == F) {
					    continue;
					}

					const String &other_scene_name = r_filenames[F];
					if (other_scene_name == scene_name) {
						duplicate_found = true;
						break;
					}
				}

				RBSet<int>::Element *to_erase = duplicate_found ? nullptr : E;

				// We need to check that we could actually append anymore names
				// if we wanted to for disambiguation. If we can't, then we have
				// to abort even with ambiguous names. We clean the full path
				// and the scene name first to remove extensions so that this
				// comparison actually works.
				String path = p_full_paths[E->get()];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (path.begins_with("res://")) {
				    path = path.substr(6);
				}

				path = path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				path = path.trim_suffix("/");

				// We can proceed if the full path is longer than the scene name,
				// meaning that there is at least one more parent folder we can
				// tack onto the name.
				can_proceed = can_proceed || (path.length() - scene_name.length()) >= 1;

				E = E->next();
				if (to_erase) {
				    iset.erase(to_erase);
				}
			}

			if (!can_proceed) {
			    break;
			}
		}
	}
}

Node* OrchestratorEditor::get_connections_dock() const {
    if (Node* editor_node = EditorNode) {
        TypedArray<Node> nodes = editor_node->find_children("*", "ConnectionsDock", true, false);
        if (nodes.size() == 1) {
            return cast_to<Node>(nodes[0]);
        }
    }
    return nullptr;
}

Node* OrchestratorEditor::get_inspector_dock() const {
    if (Node* editor_node = EditorNode) {
        TypedArray<Node> nodes = editor_node->find_children("*", "InspectorDock", true, false);
        if (nodes.size() == 1) {
            return cast_to<Node>(nodes[0]);
        }
    }
    return nullptr;
}

void OrchestratorEditor::make_inspector_visible() {
    Control* control = cast_to<Control>(get_inspector_dock());
    if (!control) {
        return;
    }

    TabContainer* parent = cast_to<TabContainer>(control->get_parent());
    if (!parent) {
        return;
    }

    const int32_t index = parent->get_tab_idx_from_control(control);
    if (index < 0) {
        return;
    }

    parent->set_current_tab(index);
}

Variant OrchestratorEditor::get_extra_layout_value(const String& p_key, const Variant& p_default) const {
    return _extra_layout_values.has(p_key) ? _extra_layout_values[p_key] : p_default;
}

void OrchestratorEditor::set_extra_layout_value(const String& p_key, const Variant& p_value) {
    _extra_layout_values[p_key] = p_value;
}

void OrchestratorEditor::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_EXIT_TREE: {
            break;
        }
        case NOTIFICATION_ENTER_TREE: {
            _theme_cache.instantiate();

            _apply_editor_settings();
            [[fallthrough]];
        }
        case NOTIFICATION_TRANSLATION_CHANGED:
        case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
        case NOTIFICATION_THEME_CHANGED: {
            _tab_container->add_theme_stylebox_override(SceneStringName(panel), get_theme_stylebox("ScriptEditor", "EditorStyles"));

            _site_search->set_button_icon(SceneUtils::get_editor_icon("ExternalLink"));
            _filter_scripts->set_right_icon(SceneUtils::get_editor_icon("Search"));

            _recent_history->reset_size();

            if (is_inside_tree()) {
                _update_script_names();
            }
            break;
        }
        case NOTIFICATION_READY: {
            add_theme_stylebox_override(SceneStringName(panel), get_theme_stylebox("ScriptEditorPanel", "EditorStyles"));

            EditorNode->connect("script_add_function_request", callable_mp_this(_add_callback));
            EditorNode->connect("resource_saved", callable_mp_this(_resource_saved_callback));

            EI->get_file_system_dock()->connect("files_moved", callable_mp_this(_files_moved));
            EI->get_file_system_dock()->connect("file_removed", callable_mp_this(_file_removed));
            EI->get_editor_settings()->connect("settings_changed", callable_mp_this(_editor_settings_changed));
            EI->get_resource_filesystem()->connect("filesystem_changed", callable_mp_this(_filesystem_changed));

            get_tree()->connect("tree_changed", callable_mp_this(_tree_changed));

            _script_list->connect(SceneStringName(item_selected), callable_mp_this(_script_selected));
            _script_split->connect("dragged", callable_mp_this(_split_dragged));
            break;
        }
        case NOTIFICATION_APPLICATION_FOCUS_IN: {
            _test_script_times_on_disk();
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_current_editor"), &OrchestratorEditor::_get_current_editor);
    ClassDB::bind_method(D_METHOD("get_open_script_editors"), &OrchestratorEditor::_get_open_script_editors);
    ClassDB::bind_method(D_METHOD("get_breakpoints"), &OrchestratorEditor::_get_breakpoints);

    ClassDB::bind_method(D_METHOD("goto_node", "node_id"), &OrchestratorEditor::_goto_script_node);
    ClassDB::bind_method(D_METHOD("get_current_script"), &OrchestratorEditor::_get_current_script);
    ClassDB::bind_method(D_METHOD("get_open_scripts"), &OrchestratorEditor::_get_open_scripts);
    ClassDB::bind_method(D_METHOD("open_script_create_dialog", "p_name", "p_base_path"), &OrchestratorEditor::open_script_create_dialog);

    ADD_SIGNAL(MethodInfo("editor_script_changed", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, "Script")));
    ADD_SIGNAL(MethodInfo("script_close", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, "Script")));

    ADD_SIGNAL(MethodInfo("scene_changed", PropertyInfo(Variant::OBJECT, "node")));

    // Used by the OrchestratorGraphEditorNodeInputActionPin
    ADD_SIGNAL(MethodInfo("input_action_cache_updated"));
}

OrchestratorEditor::OrchestratorEditor(OrchestratorWindowWrapper* p_window_wrapper) {
    ED_SHORTCUT("orchestrator_editor/reopen_closed_orchestration", "Reopen Closed Orchestration", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_SHIFT, KEY_T));
    ED_SHORTCUT("orchestrator_editor/clear_recent", "Clear Recent History");

    // Component Panel
    ED_SHORTCUT("orchestrator_component_panel/open_graph", "Open Graph", KEY_ENTER);
    ED_SHORTCUT("orchestrator_component_panel/rename_graph", "Rename Graph", KEY_F2);
    ED_SHORTCUT("orchestrator_component_panel/remove_graph", "Remove Graph", KEY_DELETE);
    ED_SHORTCUT("orchestrator_component_panel/goto_event", "Goto Event", KEY_ENTER);
    ED_SHORTCUT("orchestrator_component_panel/remove_event", "Remove Event", KEY_DELETE);
    ED_SHORTCUT("orchestrator_component_panel/disconnect_signal", "Disconnect Signal");
    ED_SHORTCUT("orchestrator_component_panel/open_function_graph", "Open Function Graph", KEY_ENTER);
    ED_SHORTCUT("orchestrator_component_panel/duplicate_function", "Duplicate Function");
    ED_SHORTCUT("orchestrator_component_panel/duplicate_function_no_code", "Duplicate Function (no code)");
    ED_SHORTCUT("orchestrator_component_panel/rename_function", "Rename Function", KEY_F2);
    ED_SHORTCUT("orchestrator_component_panel/remove_function", "Remove Function", KEY_DELETE);
    ED_SHORTCUT("orchestrator_component_panel/duplicate_variable", "Duplicate Variable");
    ED_SHORTCUT("orchestrator_component_panel/rename_variable", "Rename Variable", KEY_F2);
    ED_SHORTCUT("orchestrator_component_panel/remove_variable", "Remove Variable", KEY_DELETE);
    ED_SHORTCUT("orchestrator_component_panel/rename_signal", "Rename Signal", KEY_F2);
    ED_SHORTCUT("orchestrator_component_panel/remove_signal", "Remove Signal", KEY_DELETE);

    _window_wrapper = p_window_wrapper;
    _editor = this;

    add_child(memnew(OrchestratorEditorActionRegistry));
    add_child(memnew(OrchestratorEditorConnectionsDock));

    _editor_cache.instantiate();
    _editor_cache->load(EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator_editor_cache.cfg"));

    _restoring_layout = false;
    _pending_auto_reload = false;
    _auto_reload_running_scripts = true;
    _sort_list_on_update = true;
    _waiting_update_names = false;
    _grab_focus_block = false;

    VBoxContainer* main_container = memnew(VBoxContainer);
    add_child(main_container);

    _menu_hb = memnew(HBoxContainer);
    main_container->add_child(_menu_hb);

    _script_split = memnew(HSplitContainer);
    _script_split->set_v_size_flags(SIZE_EXPAND_FILL);
    main_container->add_child(_script_split);

    _scripts_vbox = memnew(VBoxContainer);
    _scripts_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
    _scripts_vbox->set_visible(_is_editor_setting_script_list_visible());
    _script_split->add_child(_scripts_vbox);

    _filter_scripts = memnew(LineEdit);
    _filter_scripts->set_placeholder("Filter Orchestrations");
    _filter_scripts->set_clear_button_enabled(true);
    _filter_scripts->connect(SceneStringName(text_changed), callable_mp_this(_filter_scripts_text_changed));
    _scripts_vbox->add_child(_filter_scripts);

    _script_list = memnew(ItemList);
    #if GODOT_VERSION >= 0x040300
    _script_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    #endif
    _script_list->set_custom_minimum_size(Size2(100, 60) * EDSCALE);
    _script_list->set_v_size_flags(SIZE_EXPAND_FILL);
    _script_list->set_theme_type_variation("ItemListSecondary");
    _script_list->set_allow_rmb_select(true);
    _script_list->connect("item_clicked", callable_mp_this(_script_list_clicked), CONNECT_DEFERRED);
    _scripts_vbox->add_child(_script_list);
    SET_DRAG_FORWARDING_GCD(_script_list, OrchestratorEditor)

    _context_menu = memnew(PopupMenu);
    _context_menu->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    add_child(_context_menu);

    VBoxContainer* editor_container = memnew(VBoxContainer);
    _script_split->add_child(editor_container);

    _tab_container = memnew(TabContainer);
    _tab_container->set_tabs_visible(false);
    _tab_container->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
    _tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    _tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    _tab_container->connect("tab_changed", callable_mp_this(_tab_changed));
    editor_container->add_child(_tab_container);

    _getting_started = memnew(OrchestratorGettingStarted);
    _getting_started->connect("create_requested", callable_mp_this(_menu_option).bind(FILE_NEW));
    _getting_started->connect("open_requested", callable_mp_this(_menu_option).bind(FILE_OPEN));
    _getting_started->connect("documentation_requested", callable_mp_this(_menu_option).bind(HELP_ONLINE_DOCUMENTATION));
    editor_container->add_child(_getting_started);
    _show_getting_started();

    set_process_input(true);
    set_process_shortcut_input(true);

    _file_menu = memnew(MenuButton);
    _file_menu->set_text("File");
    _file_menu->set_switch_on_hover(true);
    _file_menu->set_shortcut_context(this);
    _menu_hb->add_child(_file_menu);

    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/new", "New Orchestration...", OACCEL_KEY(KEY_MASK_CTRL, KEY_N)));
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/open", "Open..."), FILE_OPEN);
    _file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("orchestrator_editor/reopen_closed_orchestration"), FILE_REOPEN_CLOSED);

    _recent_history = memnew(PopupMenu);
    _recent_history->connect(SceneStringName(id_pressed), callable_mp_this(_open_recent_script));
    #if GODOT_VERSION >= 0x040300
    _file_menu->get_popup()->add_submenu_node_item("Open Recent", _recent_history, FILE_OPEN_RECENT);
    #else
    _file_menu->add_child(_recent_history);
    _file_menu->get_popup()->add_submenu_item("Open Recent", _recent_history->get_name(), FILE_OPEN_RECENT);
    #endif

    _file_menu->get_popup()->add_separator();
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/save", "Save", OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_S)), FILE_SAVE);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/save_as", "Save As..."), FILE_SAVE_AS);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/save_all", "Save All", OACCEL_KEY(KEY_MASK_SHIFT | KEY_MASK_ALT, KEY_S)), FILE_SAVE_ALL);
    ED_SHORTCUT_OVERRIDE("orchestrator_editor/save_all", "macos", OACCEL_KEY(KEY_MASK_META | KEY_MASK_CTRL, KEY_S));

    _file_menu->get_popup()->add_separator();
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/reload_orchestration_soft", "Soft Reload Tool Script", OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_R)), FILE_SOFT_RELOAD_TOOL_SCRIPT);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/copy_path", "Copy Orchestration Path"), FILE_COPY_PATH);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/copy_uid", "Copy Orchestration UID"), FILE_COPY_UID);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/show_in_file_system", "Show in Filesystem"), FILE_SHOW_IN_FILESYSTEM);
    _file_menu->get_popup()->add_separator();

    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/close_orchestration", "Close", OACCEL_KEY(KEY_MASK_CTRL, KEY_W)), FILE_CLOSE);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/close_all", "Close All"), FILE_CLOSE_ALL);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/close_others", "Close Others"), FILE_CLOSE_OTHERS);

    _file_menu->get_popup()->add_separator();
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/toggle_orchestration_panel", "Toggle Orchestration List", OACCEL_KEY(KEY_MASK_CTRL, KEY_BACKSLASH)), FILE_TOGGLE_LEFT_PANEL);
    _file_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/toggle_component_panel", "Toggle Component Panel", OACCEL_KEY(KEY_MASK_CTRL, KEY_SLASH)), FILE_TOGGLE_RIGHT_PANEL);
    _file_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _file_menu->get_popup()->connect("about_to_popup", callable_mp_this(_prepare_file_menu));
    _file_menu->get_popup()->connect("popup_hide", callable_mp_this(_file_menu_closed));

    MenuButton* debug_menu_btn = memnew(MenuButton);
    _menu_hb->add_child(debug_menu_btn);
    debug_menu_btn->hide();

    #if GODOT_VERSION >= 0x040300
    OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton();
    debugger->connect("goto_script_line", callable_mp_this(_goto_script_line));
    debugger->connect("breaked", callable_mp_this(_breaked));
    debugger->connect("breakpoints_cleared_in_tree", callable_mp_this(_clear_breakpoints));
    debugger->connect("breakpoint_set_in_tree", callable_mp_this(_set_breakpoint));
    #endif

    _help_menu = memnew(MenuButton);
    _help_menu->set_text("Help");
    _help_menu->set_switch_on_hover(true);
    _help_menu->set_shortcut_context(this);
    _help_menu->get_popup()->clear();
    _help_menu->get_popup()->add_icon_shortcut(SceneUtils::get_editor_icon("ExternalLink"), ED_SHORTCUT("orchestrator_editor/online_documentation", "Online Documentation"), HELP_ONLINE_DOCUMENTATION);
    _help_menu->get_popup()->add_icon_shortcut(SceneUtils::get_editor_icon("ExternalLink"), ED_SHORTCUT("orchestrator_editor/community", "Community"), HELP_COMMUNITY);

    _help_menu->get_popup()->add_separator();
    _help_menu->get_popup()->add_icon_shortcut(SceneUtils::get_editor_icon("ExternalLink"), ED_SHORTCUT("orchestrator_editor/report_a_bug", "Report a Bug"), HELP_GITHUB_ISSUES);
    _help_menu->get_popup()->add_icon_shortcut(SceneUtils::get_editor_icon("ExternalLink"), ED_SHORTCUT("orchestrator_editor/suggest_a_feature", "Suggest a Feature"), HELP_GITHUB_FEATURE);

    _help_menu->get_popup()->add_separator();
    _help_menu->get_popup()->add_shortcut(ED_SHORTCUT("orchestrator_editor/about_orchestrator", "About " VERSION_NAME), HELP_ABOUT);
    _help_menu->get_popup()->add_icon_shortcut(SceneUtils::get_editor_icon("Heart"), ED_SHORTCUT("orchestrator_editor/support_orchestrator", "Support " VERSION_NAME), HELP_SUPPORT);
    _help_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _menu_hb->add_child(_help_menu);

    _menu_hb->add_spacer(false);

    _script_icon = memnew(TextureRect);
    _menu_hb->add_child(_script_icon);

    _script_name_label = memnew(Label);
    _menu_hb->add_child(_script_name_label);

    _script_icon->hide();
    _script_name_label->hide();

    _menu_hb->add_spacer(false);

    _site_search = memnew(Button);
    _site_search->set_flat(true);
    _site_search->set_focus_mode(FOCUS_NONE);
    _site_search->set_text("Online Docs");
    _site_search->connect(SceneStringName(pressed), callable_mp_this(_menu_option).bind(HELP_ONLINE_DOCUMENTATION));
    _menu_hb->add_child(_site_search);

    Button* help_search = memnew(Button);
    help_search->set_flat(true);
    help_search->set_focus_mode(FOCUS_NONE);
    help_search->set_text("Search Help");
    help_search->set_button_icon(SceneUtils::get_editor_icon("HelpSearch"));
    help_search->connect(SceneStringName(pressed), callable_mp_this(_help_search).bind(""));
    _menu_hb->add_child(help_search);

    _menu_hb->add_child(memnew(VSeparator));

    Label* version = memnew(Label);
    version->set_text(VERSION_NAME " v" VERSION_NUMBER);
    version->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    _menu_hb->add_child(version);

    _updater = memnew(OrchestratorUpdaterButton);
    _menu_hb->add_child(_updater);

    if (_window_wrapper->is_window_available()) {
        _make_floating_separator = memnew(VSeparator);
        _menu_hb->add_child(_make_floating_separator);

        _make_floating = memnew(OrchestratorScreenSelect);
        _make_floating->set_flat(true);
        _make_floating->connect("request_open_in_screen", callable_mp(_window_wrapper, &OrchestratorWindowWrapper::enable_window_on_screen).bind(true));

        // Override default ScreenSelect tooltip if multi-window support is available.
        if (!_make_floating->is_disabled()) {
            _make_floating->set_tooltip_text("Make the Orchestration editor floating.\nRight-click to open the screen selector.");
        }

        _menu_hb->add_child(_make_floating);
        _window_wrapper->connect("window_visibility_changed", callable_mp_this(_window_changed));
    }

    _erase_tab_confirm = memnew(ConfirmationDialog);
    _erase_tab_confirm->set_ok_button_text("Save");
    _erase_tab_confirm->add_button("Discard", DisplayServer::get_singleton()->get_swap_cancel_ok(), "discard");
    _erase_tab_confirm->connect(SceneStringName(confirmed), callable_mp_this(_close_current_tab).bind(true, true));
    _erase_tab_confirm->connect("custom_action", callable_mp_this(_close_discard_current_tab));
    add_child(_erase_tab_confirm);

    _script_create_dialog = memnew(ScriptCreateDialog);
    _script_create_dialog->set_title("Create Orchestration");
    _script_create_dialog->connect("script_created", callable_mp_this(_script_created));
    add_child(_script_create_dialog);

    _file_dialog_option = -1;
    _file_dialog = memnew(OrchestratorFileDialog);
    _file_dialog->connect("file_selected", callable_mp_this(_file_dialog_action));
    add_child(_file_dialog);

    _error_dialog = memnew(AcceptDialog);
    add_child(_error_dialog);

    _disk_changed = memnew(ConfirmationDialog);
    _disk_changed->set_title("Files have been modified outside Orchestrator");
    add_child(_disk_changed);

    VBoxContainer* vbc = memnew(VBoxContainer);
    _disk_changed->add_child(vbc);

    Label* files_are_newer_label = memnew(Label);
    files_are_newer_label->set_text("The following files are newer on disk:");
    vbc->add_child(files_are_newer_label);

    _disk_changed_list = memnew(Tree);
    _disk_changed_list->set_hide_root(true);
    #if GODOT_VERSION >= 0x040300
    _disk_changed_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    #endif
    _disk_changed_list->set_v_size_flags(SIZE_EXPAND_FILL);
    vbc->add_child(_disk_changed_list);

    Label* what_action_label = memnew(Label);
    what_action_label->set_text("What action should be taken?");
    vbc->add_child(what_action_label);

    _disk_changed->set_ok_button_text("Reload from disk");
    _disk_changed->add_button("Ignore external changes", !DisplayServer::get_singleton()->get_swap_cancel_ok(), "resave");
    _disk_changed->connect(SceneStringName(confirmed), callable_mp_this(reload_scripts).bind(false));
    _disk_changed->connect("custom_action", callable_mp_this(_resave_scripts));

    _autosave_timer = memnew(Timer);
    _autosave_timer->set_one_shot(false);
    _autosave_timer->connect(SceneStringName(tree_entered), callable_mp_this(_update_autosave_timer));
    _autosave_timer->connect("timeout", callable_mp_this(_autosave_scripts));
    add_child(_autosave_timer);

    _about_dialog = memnew(OrchestratorAboutDialog);
    add_child(_about_dialog);

    _update_recent_scripts();

    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
    OrchestratorPlugin::get_singleton()->connect("scene_changed", callable_mp_this(notify_scene_changed));

    _log_router = memnew(OrchestratorEditorLogEventRouter);
    add_child(_log_router);
}
