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
#include "editor/editor_panel.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/version.h"
#include "editor/about_dialog.h"
#include "editor/editor_viewport.h"
#include "editor/getting_started.h"
#include "editor/goto_node_dialog.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/script_editor_viewport.h"
#include "editor/updater.h"
#include "editor/window_wrapper.h"
#include "file_dialog.h"
#include "script/language.h"
#include "script/script.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/templates/hash_set.hpp>

PackedStringArray OrchestratorEditorPanel::FileListContext::get_open_file_names() const
{
    PackedStringArray file_names;
    for (const OrchestrationFile& file : open_files)
        file_names.push_back(file.file_name);

    return file_names;
}

String OrchestratorEditorPanel::FileListContext::get_selected_file_name() const
{
    return current_index == -1 ? String() : open_files[current_index].file_name;
}

const OrchestratorEditorPanel::OrchestrationFile* OrchestratorEditorPanel::FileListContext::get_selected() const
{
    if (current_index == -1)
        return nullptr;

    return &open_files[current_index];
}

int OrchestratorEditorPanel::FileListContext::get_file_index(const String& p_file_name) const
{
    for (int i = 0; i < open_files.size(); ++i)
    {
        if (open_files[i].file_name == p_file_name)
            return i;
    }
    return -1;
}

void OrchestratorEditorPanel::FileListContext::hide_all()
{
    for (const OrchestrationFile& file : open_files)
        file.viewport->hide();
}

void OrchestratorEditorPanel::FileListContext::show(const String& p_file_name)
{
    const int index = get_file_index(p_file_name);
    if (index == -1)
        return;

    current_index = index;

    hide_all();

    open_files[current_index].viewport->show();
}

bool OrchestratorEditorPanel::FileListContext::is_current_unsaved() const
{
    return current_index != -1 && open_files[current_index].viewport->is_modified();
}

void OrchestratorEditorPanel::FileListContext::remove_at(int p_index)
{
    if (is_index_valid(p_index))
    {
        open_files[p_index].viewport->queue_free();
        open_files.remove_at(p_index);

        if (open_files.size() > 0)
        {
            if (current_index == p_index && current_index > 0)
                current_index--;
        }
        else
        {
            current_index = -1;
        }
    }
}

bool OrchestratorEditorPanel::FileListContext::rename(const String& p_old_file_name, const String& p_new_file_name)
{
    const int index = get_file_index(p_old_file_name);
    if (index != -1)
    {
        open_files.write[index].file_name = p_new_file_name;
        open_files[index].viewport->rename(p_new_file_name);
        return true;
    }
    return false;
}

bool OrchestratorEditorPanel::FileListContext::is_index_valid(int p_index) const
{
    return p_index >= 0 && p_index < open_files.size();
}

void OrchestratorEditorPanel::_right_panel_offset_changed(int p_offset)
{
    _right_panel_split_offset = p_offset;

    for (const OrchestrationFile& file : _files_context.open_files)
        file.viewport->set_split_offset(_right_panel_split_offset);
}

void OrchestratorEditorPanel::_update_scene_tab_signals(bool p_connect)
{
    Node* editor_node = get_tree()->get_root()->get_child(0);
    if (!editor_node)
        return;

    Node* scene_tabs = editor_node->find_child("*EditorSceneTabs*", true, false);
    if (!scene_tabs)
        return;

    if (p_connect)
    {
        OCONNECT(scene_tabs, "tab_changed", callable_mp(this, &OrchestratorEditorPanel::_scene_tab_changed));

        #if GODOT_VERSION >= 0x040300
        OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton();
        if (debugger)
        {
            OCONNECT(debugger, "goto_script_line", callable_mp(this, &OrchestratorEditorPanel::_goto_script_line));
            OCONNECT(debugger, "breakpoints_cleared_in_tree", callable_mp(this, &OrchestratorEditorPanel::_clear_all_breakpoints));
            OCONNECT(debugger, "breakpoint_set_in_tree", callable_mp(this, &OrchestratorEditorPanel::_set_breakpoint));
        }
        #endif

        return;
    }

    ODISCONNECT(scene_tabs, "tab_changed", callable_mp(this, &OrchestratorEditorPanel::_scene_tab_changed));

    #if GODOT_VERSION >= 0x040300
    OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton();
    if (debugger)
    {
        ODISCONNECT(debugger, "goto_script_line", callable_mp(this, &OrchestratorEditorPanel::_goto_script_line));
        ODISCONNECT(debugger, "breakpoints_cleared_in_tree", callable_mp(this, &OrchestratorEditorPanel::_clear_all_breakpoints));
        ODISCONNECT(debugger, "breakpoint_set_in_tree", callable_mp(this, &OrchestratorEditorPanel::_set_breakpoint));
    }
    #endif
}

void OrchestratorEditorPanel::_update_file_system_dock_signals(bool p_connect)
{
    FileSystemDock* file_system_dock = EditorInterface::get_singleton()->get_file_system_dock();
    if (!file_system_dock)
        return;

    if (p_connect)
    {
        OCONNECT(file_system_dock, "file_removed", callable_mp(this, &OrchestratorEditorPanel::_file_removed));
        OCONNECT(file_system_dock, "files_moved", callable_mp(this, &OrchestratorEditorPanel::_file_moved));
        OCONNECT(file_system_dock, "folder_removed", callable_mp(this, &OrchestratorEditorPanel::_folder_removed));
        return;
    }

    ODISCONNECT(file_system_dock, "file_removed", callable_mp(this, &OrchestratorEditorPanel::_file_removed));
    ODISCONNECT(file_system_dock, "files_moved", callable_mp(this, &OrchestratorEditorPanel::_file_moved));
    ODISCONNECT(file_system_dock, "folder_removed", callable_mp(this, &OrchestratorEditorPanel::_folder_removed));
}

void OrchestratorEditorPanel::_update_file_list()
{
    _file_list->clear();

    // todo: merge this into files_context

    HashSet<String> stems;
    HashSet<String> duplicate_stems;
    for (const OrchestrationFile& file : _files_context.open_files)
    {
        const String file_name = file.file_name.get_file();
        if (!stems.has(file_name))
            stems.insert(file_name);
        else
            duplicate_stems.insert(file_name);
    }

    for (int i = 0; i < _files_context.open_files.size(); i++)
    {
        const OrchestrationFile& file = _files_context.open_files[i];

        if (_file_name_filter.is_empty() || file.file_name.contains(_file_name_filter))
        {
            const String stem = file.file_name.get_file();
            const String base = file.file_name.get_base_dir().replace("res://", "");
            const String full = base.is_empty() ? stem : vformat("%s/%s", base, stem);

            String text = duplicate_stems.has(stem) ? full : stem;
            int32_t idx = _file_list->add_item(text, SceneUtils::get_editor_icon("GDScript"));

            if (i == _files_context.current_index)
                _file_list->select(idx);
        }
    }
}

void OrchestratorEditorPanel::_update_getting_started()
{
    if (!_has_open_files())
    {
        _viewport_container->hide();
        _getting_started->show();
    }
    else
    {
        _getting_started->hide();
        _viewport_container->show();
    }
}

void OrchestratorEditorPanel::_recent_history_selected(int p_index)
{
    bool is_clear = p_index == _recent_history->get_item_count() - 1;
    if (!is_clear)
    {
        const int file_index = _files_context.get_file_index(_recent_files[p_index]);
        if (file_index == -1)
            _open_script_file(_recent_files[p_index]);
        else
            _show_editor_viewport(_recent_files[p_index]);

        _update_file_list();
        return;
    }

    _recent_files.clear();
    _save_recent_history();
    _update_recent_history();
}

void OrchestratorEditorPanel::_update_recent_history()
{
    _recent_history->clear();

    for (const String& recent_file_name : _recent_files)
        _recent_history->add_item(recent_file_name.replace("res://", ""));

    _recent_history->add_separator();
    _recent_history->add_item("Clear Recent Files");

    if (_recent_files.is_empty())
        _recent_history->set_item_disabled(_recent_history->get_item_count() - 1, true);
}

void OrchestratorEditorPanel::_save_recent_history()
{
    _recent_files = _recent_files.slice(0, MAX_RECENT_FILES);

    OrchestratorPlugin* plugin = OrchestratorPlugin::get_singleton();
    Ref<ConfigFile> metadata = plugin->get_metadata();
    metadata->set_value(RECENT_HISTORY_SECTION, RECENT_HISTORY_KEY, _recent_files);
    plugin->save_metadata(metadata);
}


void OrchestratorEditorPanel::_prepare_file_menu()
{
    PopupMenu* popup = _file_menu->get_popup();

    bool no_open_file = !_has_open_files();
    popup->set_item_disabled(popup->get_item_index(FILE_SAVE), no_open_file);
    popup->set_item_disabled(popup->get_item_index(FILE_SAVE_AS), no_open_file);
    popup->set_item_disabled(popup->get_item_index(FILE_SAVE_ALL), no_open_file);
    popup->set_item_disabled(popup->get_item_index(FILE_SHOW_IN_FILESYSTEM), no_open_file);
    popup->set_item_disabled(popup->get_item_index(FILE_CLOSE), no_open_file);
    popup->set_item_disabled(popup->get_item_index(FILE_CLOSE_ALL), no_open_file);
}

void OrchestratorEditorPanel::_prepare_goto_menu()
{
    PopupMenu* popup = _goto_menu->get_popup();
    popup->set_item_disabled(popup->get_item_index(GOTO_NODE), !_has_open_files());
}

void OrchestratorEditorPanel::_handle_menu_option(int p_option)
{
    OrchestratorPlugin* plugin = OrchestratorPlugin::get_singleton();

    switch (p_option)
    {
        case FILE_NEW:
            _show_create_new_script_dialog();
            break;
        case FILE_OPEN:
            _file_open_dialog->popup_file_dialog();
            break;
        case FILE_SAVE_AS:
            _file_save_dialog->popup_file_dialog();
            break;
        case FILE_SAVE:
            _save_script();
            break;
        case FILE_SAVE_ALL:
            _save_all_scripts();
            break;
        case FILE_CLOSE:
            if (_files_context.is_current_unsaved())
                _ask_close_current_unsaved_editor();
            else
                _close_script(false);
            break;
        case FILE_CLOSE_ALL:
            _close_all_scripts();
            break;
        case FILE_CLOSE_OTHERS:
            _close_other_scripts();
            break;
        case FILE_COPY_PATH:
            DisplayServer::get_singleton()->clipboard_set(_files_context.get_selected_file_name());
            break;
        case HELP_ONLINE_DOCUMENTATION:
            OS::get_singleton()->shell_open(plugin->get_plugin_online_documentation_url());
            break;
        case HELP_COMMUNITY:
            OS::get_singleton()->shell_open(plugin->get_community_url());
            break;
        case HELP_GITHUB_ISSUES:
        case HELP_GITHUB_FEATURE:
            OS::get_singleton()->shell_open(plugin->get_github_issues_url());
            break;
        case HELP_SUPPORT:
            OS::get_singleton()->shell_open(plugin->get_patreon_url());
            break;
        case HELP_ABOUT:
            _about_dialog->popup_centered(ABOUT_DIALOG_SIZE * EditorInterface::get_singleton()->get_editor_scale());
            break;
        case FILE_SHOW_IN_FILESYSTEM:
            _navigate_to_file_in_filesystem();
            break;
        case FILE_TOGGLE_LEFT_PANEL:
            _left_panel_visible = !_left_panel->is_visible();
            _left_panel->set_visible(_left_panel_visible);
            break;
        case FILE_TOGGLE_RIGHT_PANEL:
            _right_panel_visible = !_right_panel_visible;
            for (const OrchestrationFile& file : _files_context.open_files)
                file.viewport->notify_component_panel_visibility_changed(_right_panel_visible);
            break;
        case GOTO_NODE:
            _goto_dialog->popup_centered();
            break;
        default:
            break;
    }
}

bool OrchestratorEditorPanel::_has_open_files() const
{
    return _files_context.current_index >= 0 && _files_context.current_index < _files_context.open_files.size();
}

void OrchestratorEditorPanel::_show_editor_viewport(const String& p_file_name)
{
    EditorInterface::get_singleton()->inspect_object(nullptr);

    _files_context.show(p_file_name);

    if (_recent_files.has(p_file_name))
        _recent_files.remove_at(_recent_files.find(p_file_name));

    _recent_files.insert(0, p_file_name);

    _save_recent_history();
    _update_recent_history();
}

void OrchestratorEditorPanel::_open_script_file(const String& p_file_name)
{
    const Ref<Resource> resource = ResourceLoader::get_singleton()->load(p_file_name);
    if (resource.is_valid())
        edit_resource(resource);
    else
        OS::get_singleton()->alert("Failed to load the orchestration file.", "Orchestration invalid");
}

void OrchestratorEditorPanel::_save_script_file(const String& p_file_name)
{
    const OrchestrationFile* file = _files_context.get_selected();
    if (file)
    {
        if (file->viewport->save_as(p_file_name))
        {
            // Update filename in the open files list
            _files_context.open_files.write[_files_context.current_index].file_name = p_file_name;
            _update_file_list();
        }
    }

    EditorInterface::get_singleton()->get_resource_filesystem()->update_file(p_file_name);
}

void OrchestratorEditorPanel::_save_script()
{
    if (_has_open_files())
    {
        if (const OrchestrationFile* file = _files_context.get_selected())
            file->viewport->apply_changes();
    }
}

void OrchestratorEditorPanel::_save_all_scripts()
{
    if (_has_open_files())
    {
        for (const OrchestrationFile& file : _files_context.open_files)
            file.viewport->apply_changes();
    }
}

void OrchestratorEditorPanel::_ask_close_current_unsaved_editor()
{
    if (_has_open_files())
    {
        _close_confirm->set_text("CLose and save changes to " + _files_context.get_selected_file_name());
        _close_confirm->popup_centered();
    }
}

void OrchestratorEditorPanel::_close_script(bool p_save)
{
    _close_script(_files_context.current_index, p_save);
}

void OrchestratorEditorPanel::_close_script(int p_index, bool p_save)
{
    if (!_has_open_files())
        return;

    if (_files_context.is_index_valid(p_index))
    {
        if (p_save)
        {
            const OrchestrationFile& file = _files_context.open_files[p_index];
            file.viewport->apply_changes();

            // Drop resource from cache
            ResourceCache::get_singleton()->remove_ref(file.file_name);
        }
        _files_context.remove_at(p_index);
    }

    if (_has_open_files())
        _show_editor_viewport(_files_context.get_selected_file_name());

    _update_getting_started();
    _update_file_list();
}

void OrchestratorEditorPanel::_close_all_scripts()
{
    for (const OrchestrationFile& file : _files_context.open_files)
        _files_context.close_queue.push_back(file);

    _queue_close_scripts();
}

void OrchestratorEditorPanel::_close_other_scripts()
{
    const String current_file_name = _files_context.get_selected_file_name();
    for (const OrchestrationFile& file : _files_context.open_files)
    {
        if (!file.file_name.match(current_file_name))
            _files_context.close_queue.push_back(file);
    }

    _queue_close_scripts();
}

void OrchestratorEditorPanel::_queue_close_scripts()
{
    while (!_files_context.close_queue.is_empty())
    {
        const OrchestrationFile file = _files_context.close_queue.front()->get();
        _files_context.close_queue.pop_front();

        _show_editor_viewport(file.file_name);

        if (file.viewport->is_modified())
        {
            file.viewport->connect("tree_exited", callable_mp(this, &OrchestratorEditorPanel::_queue_close_scripts), CONNECT_ONE_SHOT);
            _ask_close_current_unsaved_editor();
            break;
        }
        _close_script(false);
    }
    _update_file_list();
}

void OrchestratorEditorPanel::_show_create_new_script_dialog()
{
    const String inherits = OrchestratorSettings::get_singleton()->get_setting("settings/default_type");

    // Get dialog and cache current position
    _script_create_dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);

    // Find LanguageMenu option and force Orchestrator as the selected choice
    // Must be called before "config" to guarantee that the dialog logic for templates and language works properly.
    const String language_name = OScriptLanguage::get_singleton()->_get_name();
    TypedArray<Node> nodes = _script_create_dialog->find_children("*", OptionButton::get_class_static(), true, false);
    if (!nodes.is_empty())
    {
        OptionButton* menu = Object::cast_to<OptionButton>(nodes[0]);
        for (int i = 0; i < menu->get_item_count(); i++)
        {
            if (menu->get_item_text(i).match(language_name))
            {
                menu->select(i);
                break;
            }
        }
    }

    _script_create_dialog->set_title("Create Orchestration");
    _script_create_dialog->config(inherits, "new_script.os", false, false);

    Ref<EditorSettings> editor_settings = EditorInterface::get_singleton()->get_editor_settings();
    editor_settings->set_project_metadata("script_setup", "last_selected_language", language_name);

    _script_create_dialog->popup_centered();
}

void OrchestratorEditorPanel::_script_file_created(const Ref<Script>& p_script)
{
    ERR_FAIL_COND_MSG(!p_script.is_valid() || !p_script->get_class().match(OScript::get_class_static()), "The script is not an orchestration.");
    edit_script(p_script);
}

void OrchestratorEditorPanel::_file_filter_changed(const String& p_text)
{
    if (!_file_name_filter.match(p_text))
    {
        _file_name_filter = p_text;
        _update_file_list();
    }
}

void OrchestratorEditorPanel::_file_list_selected(int p_index)
{
    _show_editor_viewport(_files_context.open_files[p_index].file_name);
}

void OrchestratorEditorPanel::_show_file_list_context_menu(int p_index, const Vector2& p_position, int p_button)
{
    if (p_button == MOUSE_BUTTON_RIGHT)
    {
        _file_list_context_menu->reset_size();
        _file_list_context_menu->set_position(_file_list->get_screen_position() + p_position);
        _file_list_context_menu->popup();
    }
}

void OrchestratorEditorPanel::_close_tab(bool p_save)
{
    _close_script(p_save);
}

void OrchestratorEditorPanel::_close_tab_discard_changes(const String& p_data)
{
    if (_has_open_files())
    {
        const OrchestrationFile* file = _files_context.get_selected();
        file->viewport->reload_from_disk();

        _close_script(false);
    }
    _close_confirm->hide();
}

void OrchestratorEditorPanel::_scene_tab_changed(int p_index)
{
    if (is_visible() && _has_open_files())
    {
        _files_context.get_selected()->viewport->notify_scene_tab_changed();

        if (SceneTree* scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop()))
        {
            if (Node* current_scene = scene_tree->get_edited_scene_root())
            {
                const Ref<OScript>& script = current_scene->get_script();
                if (script.is_valid())
                {
                    for (const OrchestrationFile& file : _files_context.open_files)
                    {
                        if (file.viewport->is_same_script(script))
                        {
                            _show_editor_viewport(file.file_name);
                            file.viewport->notify_scene_tab_changed();
                            _update_file_list();
                            break;
                        }
                    }
                }
            }
        }
    }
}

void OrchestratorEditorPanel::_file_removed(const String& p_file_name)
{
    const int index = _files_context.get_file_index(p_file_name);
    if (index != -1)
        _close_script(index, false);
}

void OrchestratorEditorPanel::_file_moved(const String& p_old_file_name, const String& p_new_file_name)
{
    if (_files_context.rename(p_old_file_name, p_new_file_name))
        _update_file_list();
}

void OrchestratorEditorPanel::_folder_removed(const String& p_folder_name)
{
    for (int i = 0; i < _files_context.open_files.size();)
    {
        const OrchestrationFile& file = _files_context.open_files[i];
        if (file.file_name.begins_with(p_folder_name))
        {
            _close_script(i, false);
            continue;
        }
        i++;
    }
}

void OrchestratorEditorPanel::_add_script_function(Object* p_object, const String& p_function_name, const PackedStringArray& p_args)
{
    Ref<Script> script = p_object->get_script();
    if (!script.is_valid())
        return;

    Ref<OScript> orchestration = script;
    if (!orchestration.is_valid())
        return;

    for (int i = 0; i < _files_context.open_files.size(); i++)
    {
        const OrchestrationFile& file = _files_context.open_files[i];
        if (file.viewport->is_same_script(orchestration))
        {
            OrchestratorPlugin::get_singleton()->make_active();
            file.viewport->show();

            file.viewport->add_script_function(p_object, p_function_name, p_args);
            return;
        }
    }

    edit_script(orchestration);
    _files_context.get_selected()->viewport->add_script_function(p_object, p_function_name, p_args);
}

void OrchestratorEditorPanel::_focus_viewport(OrchestratorEditorViewport* p_viewport)
{
    for (int i = 0; i < _files_context.open_files.size(); i++)
    {
        if (_files_context.open_files[i].viewport == p_viewport)
        {
            _files_context.show(_files_context.open_files[i].file_name);
            _update_file_list();
            break;
        }
    }
}

void OrchestratorEditorPanel::_build_log_meta_clicked(const Variant& p_meta)
{
    const Dictionary value = JSON::parse_string(String(p_meta));
    if (value.has("script"))
    {
        for (const OrchestrationFile& file : _files_context.open_files)
        {
            if (file.file_name == value["script"])
            {
                if (value.has("goto_node"))
                    file.viewport->goto_node(value["goto_node"]);
                return;
            }
        }

        if (value.has("goto_node"))
        {
            _open_script_file(value["script"]);
            _files_context.get_selected()->viewport->goto_node(value["goto_node"]);
        }
    }
}

#if GODOT_VERSION >= 0x040300
void OrchestratorEditorPanel::_goto_script_line(const Ref<Script>& p_script, int p_line)
{
    const Ref<OScript> script = p_script;
    if (!script.is_valid())
        return;

    for (const OrchestrationFile& file : _files_context.open_files)
    {
        if (file.file_name == script->get_path())
        {
            // Make plugin active
            OrchestratorPlugin::get_singleton()->make_active();
            // Show viewport
            _show_editor_viewport(file.file_name);
            // Goto node
            file.viewport->goto_node(p_line + 1);
            // Update files list
            _update_file_list();
            return;
        }
    }

    edit_script(script);

    // Allow specifying p_line as -1 to open script but not to jump to a specific node
    if (p_line != -1)
    {
        // Goto the node in the script
        _files_context.get_selected()->viewport->goto_node(p_line + 1);
    }
}

void OrchestratorEditorPanel::_clear_all_breakpoints()
{
    // Clear all breakpoints in the script editors
    for (const OrchestrationFile& file : _files_context.open_files)
        file.viewport->clear_breakpoints();

    // Clear cache breakpoints
    OrchestratorPlugin::get_singleton()->get_editor_cache()->clear_all_breakpoints();
}

void OrchestratorEditorPanel::_set_breakpoint(const Ref<Script>& p_script, int p_line, bool p_enabled)
{
    Ref<OScript> script = p_script;
    if (!script.is_valid())
        return;

    const int node_id = p_line + 1;

    Ref<OrchestratorEditorCache> cache = OrchestratorPlugin::get_singleton()->get_editor_cache();
    cache->set_breakpoint(script->get_path(), node_id, p_enabled);
    cache->set_disabled_breakpoint(script->get_path(), node_id, true);

    for (const OrchestrationFile& file : _files_context.open_files)
    {
        if (file.viewport->is_same_script(script))
            file.viewport->set_breakpoint(node_id, p_enabled);
    }
}
#endif

void OrchestratorEditorPanel::_window_changed(bool p_visible)
{
    _select_separator->set_visible(!p_visible);
    _screen_select->set_visible(!p_visible);
    _floating = p_visible;
}

void OrchestratorEditorPanel::_goto_node(int p_node_id)
{
    if (!_has_open_files())
        return;

    if (const OrchestrationFile* file = _files_context.get_selected())
        file->viewport->goto_node(p_node_id);
}

void OrchestratorEditorPanel::_navigate_to_file_in_filesystem()
{
    if (!_has_open_files())
        return;

    const String file_name = _files_context.get_selected_file_name();
    if (!file_name.is_empty())
        EditorInterface::get_singleton()->get_file_system_dock()->navigate_to_path(file_name);
}

void OrchestratorEditorPanel::edit_resource(const Ref<Resource>& p_resource)
{
    const Ref<OScript> script = p_resource;
    if (script.is_valid())
        edit_script(script);
}

void OrchestratorEditorPanel::edit_script(const Ref<OScript>& p_script)
{
    ERR_FAIL_COND_MSG(p_script->get_path().is_empty(), "Script has no path, cannot be opened.");

    // When editing an Orchestration; make panel active
    OrchestratorPlugin::get_singleton()->make_active();

    // Before opening a new file, all existing file viewports should be hidden.
    // Unlike the Script tab, we do not use tabs but rather control which editor is visible.
    _files_context.hide_all();

    // Check whether the file is already open in the editor
    const int file_index = _files_context.get_file_index(p_script->get_path());
    if (file_index != -1)
    {
        _files_context.show(p_script->get_path());
        _update_file_list();
        _prepare_file_menu();
        return;
    }

    OrchestratorScriptEditorViewport* viewport = memnew(OrchestratorScriptEditorViewport(p_script));
    viewport->set_split_offset(_right_panel_split_offset);
    viewport->connect("focus_requested", callable_mp(this, &OrchestratorEditorPanel::_focus_viewport).bind(viewport));
    viewport->connect("dragged", callable_mp(this, &OrchestratorEditorPanel::_right_panel_offset_changed));
    _viewport_container->add_child(viewport);

    OrchestrationFile file;
    file.file_name = p_script->get_path();
    file.viewport = viewport;

    _files_context.current_index = _files_context.open_files.size();
    _files_context.open_files.push_back(file);

    _update_getting_started();

    _update_file_list();
    _prepare_file_menu();

    _show_editor_viewport(file.file_name);

    viewport->notify_component_panel_visibility_changed(_right_panel_visible);
}

void OrchestratorEditorPanel::apply_changes()
{
    for (const OrchestrationFile& file : _files_context.open_files)
        file.viewport->apply_changes();
}

bool OrchestratorEditorPanel::build()
{
    for (const OrchestrationFile& file : _files_context.open_files)
    {
        if (!file.viewport->build())
            return false;
    }
    return true;
}

void OrchestratorEditorPanel::get_window_layout(const Ref<ConfigFile>& p_configuration)
{
    p_configuration->set_value(LAYOUT_SECTION, LAYOUT_OPEN_FILES, _files_context.get_open_file_names());

    HSplitContainer* parent = Object::cast_to<HSplitContainer>(_left_panel->get_parent());

    p_configuration->set_value(LAYOUT_SECTION, LAYOUT_LEFT_PANEL, _left_panel_visible);
    p_configuration->set_value(LAYOUT_SECTION, LAYOUT_LEFT_PANEL_OFFSET, parent->get_split_offset());

    p_configuration->set_value(LAYOUT_SECTION, LAYOUT_RIGHT_PANEL, _right_panel_visible);
    p_configuration->set_value(LAYOUT_SECTION, LAYOUT_RIGHT_PANEL_OFFSET, _right_panel_split_offset);

    if (_has_open_files())
        p_configuration->set_value(LAYOUT_SECTION, LAYOUT_OPEN_FILES_SELECTED, _files_context.get_selected_file_name());
    else if (p_configuration->has_section_key(LAYOUT_SECTION, LAYOUT_OPEN_FILES_SELECTED))
        p_configuration->erase_section_key(LAYOUT_SECTION, LAYOUT_OPEN_FILES_SELECTED);
}

void OrchestratorEditorPanel::set_window_layout(const Ref<ConfigFile>& p_configuration)
{
    const bool restore_windows = OrchestratorPlugin::get_singleton()->restore_windows_on_load();
    if (!restore_windows && !p_configuration->has_section_key(LAYOUT_SECTION, LAYOUT_OPEN_FILES))
        return;

    _left_panel_visible = p_configuration->get_value(LAYOUT_SECTION, LAYOUT_LEFT_PANEL, true);
    _left_panel->set_visible(_left_panel_visible);

    HSplitContainer* parent = Object::cast_to<HSplitContainer>(_left_panel->get_parent());
    parent->set_split_offset(p_configuration->get_value(LAYOUT_SECTION, LAYOUT_LEFT_PANEL_OFFSET, 0));

    _right_panel_visible = p_configuration->get_value(LAYOUT_SECTION, LAYOUT_RIGHT_PANEL, true);
    _right_panel_split_offset = p_configuration->get_value(LAYOUT_SECTION, LAYOUT_RIGHT_PANEL_OFFSET, 0);

    PackedStringArray open_files = p_configuration->get_value(LAYOUT_SECTION, LAYOUT_OPEN_FILES, PackedStringArray());
    for (const String& file_name : open_files)
        edit_resource(ResourceLoader::get_singleton()->load(file_name));

    String selected_file_name = p_configuration->get_value(LAYOUT_SECTION, LAYOUT_OPEN_FILES_SELECTED, "");
    if (!selected_file_name.is_empty())
    {
        const int index = _files_context.get_file_index(selected_file_name);
        if (index != -1)
        {
            _file_list->select(index);
            _show_editor_viewport(selected_file_name);
        }
    }
}

#if GODOT_VERSION >= 0x040300
PackedStringArray OrchestratorEditorPanel::get_breakpoints() const
{
    PackedStringArray breakpoints;
    for (const OrchestrationFile& file : _files_context.open_files)
        breakpoints.append_array(file.viewport->get_breakpoints());
    return breakpoints;
}
#endif

void OrchestratorEditorPanel::_notification(int p_what)
{
    switch(p_what)
    {
        case NOTIFICATION_READY:
        {
            add_theme_stylebox_override("panel", SceneUtils::get_editor_style("ScriptEditorPanel"));

            if (Node* editor_node = get_tree()->get_root()->get_child(0))
                editor_node->connect("script_add_function_request", callable_mp(this, &OrchestratorEditorPanel::_add_script_function));

            VBoxContainer* vbox = memnew(VBoxContainer);
            add_child(vbox);

            HBoxContainer* toolbar = memnew(HBoxContainer);
            vbox->add_child(toolbar);

            HBoxContainer* main_menu = memnew(HBoxContainer);
            main_menu->set_h_size_flags(SIZE_EXPAND_FILL);
            toolbar->add_child(main_menu);

            _recent_history = memnew(PopupMenu);
            _recent_history->set_name(RECENT_HISTORY_POPUP_NAME);
            _recent_history->connect("index_pressed", callable_mp(this, &OrchestratorEditorPanel::_recent_history_selected));

            _file_menu = memnew(MenuButton);
            _file_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
            _file_menu->set_text("File");
            _file_menu->get_popup()->add_item("New Orchestration...", FILE_NEW, OACCEL_KEY(KEY_MASK_CTRL, KEY_N));
            _file_menu->get_popup()->add_item("Open...", FILE_OPEN);
            _file_menu->get_popup()->add_child(_recent_history);
            _file_menu->get_popup()->add_submenu_item("Open Recent", _recent_history->get_name(), FILE_OPEN_RECENT);
            _file_menu->get_popup()->add_separator();
            _file_menu->get_popup()->add_item("Save", FILE_SAVE, OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_S));
            _file_menu->get_popup()->add_item("Save As...", FILE_SAVE_AS);
            _file_menu->get_popup()->add_item("Save All", FILE_SAVE_ALL, OACCEL_KEY(KEY_MASK_SHIFT | KEY_MASK_ALT, KEY_S));
            _file_menu->get_popup()->add_separator();
            _file_menu->get_popup()->add_item("Show in Filesystem", FILE_SHOW_IN_FILESYSTEM);
            _file_menu->get_popup()->add_separator();
            _file_menu->get_popup()->add_item("Close", FILE_CLOSE, OACCEL_KEY(KEY_MASK_CTRL, KEY_W));
            _file_menu->get_popup()->add_item("Close All", FILE_CLOSE_ALL);
            _file_menu->get_popup()->add_separator();
            _file_menu->get_popup()->add_item("Toggle Orchestration List", FILE_TOGGLE_LEFT_PANEL, OACCEL_KEY(KEY_MASK_CTRL, KEY_BACKSLASH));
            _file_menu->get_popup()->add_item("Toggle Component Panel", FILE_TOGGLE_RIGHT_PANEL, OACCEL_KEY(KEY_MASK_CTRL, KEY_SLASH));
            _file_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option));
            _file_menu->get_popup()->connect("about_to_popup", callable_mp(this, &OrchestratorEditorPanel::_prepare_file_menu));
            main_menu->add_child(_file_menu);

            _goto_menu = memnew(MenuButton);
            _goto_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
            _goto_menu->set_text("Goto");
            _goto_menu->get_popup()->add_item("Goto Node", GOTO_NODE, OACCEL_KEY(KEY_MASK_CTRL, KEY_L));
            _goto_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option));
            _goto_menu->get_popup()->connect("about_to_popup", callable_mp(this, &OrchestratorEditorPanel::_prepare_goto_menu));
            main_menu->add_child(_goto_menu);

            _help_menu = memnew(MenuButton);
            _help_menu->set_v_size_flags(SIZE_SHRINK_BEGIN);
            _help_menu->set_text("Help");
            _help_menu->get_popup()->clear();
            _help_menu->get_popup()->add_icon_item(SceneUtils::get_editor_icon("ExternalLink"), "Online Documentation", HELP_ONLINE_DOCUMENTATION);
            _help_menu->get_popup()->add_icon_item(SceneUtils::get_editor_icon("ExternalLink"), "Community", HELP_COMMUNITY);
            _help_menu->get_popup()->add_separator();
            _help_menu->get_popup()->add_icon_item(SceneUtils::get_editor_icon("ExternalLink"), "Report a Bug", HELP_GITHUB_ISSUES);
            _help_menu->get_popup()->add_icon_item(SceneUtils::get_editor_icon("ExternalLink"), "Suggest a Feature", HELP_GITHUB_FEATURE);
            _help_menu->get_popup()->add_separator();
            _help_menu->get_popup()->add_item("About " VERSION_NAME, HELP_ABOUT);
            _help_menu->get_popup()->add_icon_item(SceneUtils::get_editor_icon("Heart"), "Support " VERSION_NAME, HELP_SUPPORT);
            _help_menu->get_popup()->connect("id_pressed", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option));
            main_menu->add_child(_help_menu);

            HBoxContainer* right_menu = memnew(HBoxContainer);
            right_menu->set_alignment(BoxContainer::ALIGNMENT_END);
            right_menu->set_anchors_preset(PRESET_FULL_RECT);
            right_menu->add_theme_constant_override("separation", 0);
            toolbar->add_child(right_menu);

            Button* open_docs = memnew(Button);
            open_docs->set_text("Online Docs");
            open_docs->set_button_icon(SceneUtils::get_editor_icon("ExternalLink"));
            open_docs->set_flat(true);
            open_docs->set_focus_mode(FOCUS_NONE);
            open_docs->call_deferred("connect", "pressed", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option).bind(HELP_ONLINE_DOCUMENTATION));
            right_menu->add_child(open_docs);

            VSeparator* separator = memnew(VSeparator);
            separator->set_v_size_flags(SIZE_SHRINK_CENTER);
            separator->set_custom_minimum_size(SEPARATOR_SIZE);
            right_menu->add_child(separator);

            Label* version = memnew(Label);
            version->set_text(VERSION_NAME " v" VERSION_NUMBER);
            version->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
            right_menu->add_child(version);

            _updater = memnew(OrchestratorUpdaterButton);
            right_menu->add_child(_updater);

            if (_window_wrapper->is_window_available())
            {
                separator = memnew(VSeparator);
                separator->set_v_size_flags(SIZE_SHRINK_CENTER);
                separator->set_custom_minimum_size(SEPARATOR_SIZE);
                right_menu->add_child(separator);
                _select_separator = separator;

                _screen_select = memnew(OrchestratorScreenSelect);
                _screen_select->set_flat(true);
                _screen_select->set_tooltip_text("Make the Orchestration editor floating.");
                _screen_select->connect("request_open_in_screen", callable_mp(_window_wrapper, &OrchestratorWindowWrapper::enable_window_on_screen).bind(true));
                right_menu->add_child(_screen_select);
                _window_wrapper->connect("window_visibility_changed", callable_mp(this, &OrchestratorEditorPanel::_window_changed));
            }

            HSplitContainer* main_container = memnew(HSplitContainer);
            main_container->set_v_size_flags(SIZE_EXPAND_FILL);
            vbox->add_child(main_container);

            VSplitContainer* left_panel = memnew(VSplitContainer);
            main_container->add_child(left_panel);
            _left_panel = left_panel;

            VBoxContainer* files_container = memnew(VBoxContainer);
            files_container->set_anchors_preset(PRESET_FULL_RECT);
            files_container->set_v_size_flags(SIZE_EXPAND_FILL);
            _left_panel->add_child(files_container);

            LineEdit* file_filters = memnew(LineEdit);
            file_filters->set_placeholder("Filter orchestrations");
            file_filters->set_clear_button_enabled(true);
            file_filters->set_right_icon(SceneUtils::get_editor_icon("Search"));
            file_filters->connect("text_changed", callable_mp(this, &OrchestratorEditorPanel::_file_filter_changed));
            files_container->add_child(file_filters);

            _file_list = memnew(ItemList);
            _file_list->set_custom_minimum_size(Vector2i(165, 0));
            _file_list->set_allow_rmb_select(true);
            _file_list->set_focus_mode(FOCUS_NONE);
            _file_list->set_v_size_flags(SIZE_EXPAND_FILL);
            _file_list->connect("item_selected", callable_mp(this, &OrchestratorEditorPanel::_file_list_selected));
            _file_list->connect("item_clicked", callable_mp(this, &OrchestratorEditorPanel::_show_file_list_context_menu));
            files_container->add_child(_file_list);

            _file_list_context_menu = memnew(PopupMenu);
            _file_list_context_menu->clear();
            _file_list_context_menu->add_item("Save", FILE_SAVE, OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_S));
            _file_list_context_menu->add_item("Save As...", FILE_SAVE_AS);
            _file_list_context_menu->add_item("Close", FILE_CLOSE, OACCEL_KEY(KEY_MASK_CTRL, KEY_W));
            _file_list_context_menu->add_item("Close All", FILE_CLOSE_ALL);
            _file_list_context_menu->add_item("Close Other Tabs", FILE_CLOSE_OTHERS);
            _file_list_context_menu->add_separator();
            _file_list_context_menu->add_item("Copy Orchestration Path", FILE_COPY_PATH);
            _file_list_context_menu->add_item("Show in FileSystem", FILE_SHOW_IN_FILESYSTEM);
            _file_list_context_menu->add_separator();
            _file_list_context_menu->add_item("Toggle Orchestration List", FILE_TOGGLE_LEFT_PANEL, OACCEL_KEY(KEY_MASK_CTRL, KEY_BACKSLASH));
            _file_list_context_menu->connect("id_pressed", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option));
            files_container->add_child(_file_list_context_menu);

            _viewport_container = memnew(VBoxContainer);
            _viewport_container->set_v_size_flags(SIZE_EXPAND_FILL);
            _viewport_container->set_visible(false);
            main_container->add_child(_viewport_container);

            _getting_started = memnew(OrchestratorGettingStarted);
            _getting_started->connect("create_requested", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option).bind(FILE_NEW));
            _getting_started->connect("open_requested", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option).bind(FILE_OPEN));
            _getting_started->connect("documentation_requested", callable_mp(this, &OrchestratorEditorPanel::_handle_menu_option).bind(HELP_ONLINE_DOCUMENTATION));
            main_container->add_child(_getting_started);

            _about_dialog = memnew(OrchestratorAboutDialog);
            add_child(_about_dialog);

            const String filter = OScriptLanguage::get_singleton()->get_script_extension_filter();

            _file_open_dialog = memnew(OrchestratorFileDialog);
            _file_open_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
            _file_open_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
            _file_open_dialog->set_title("Open Orchestration Script");
            _file_open_dialog->add_filter(filter, "Orchestrator Scripts");
            _file_open_dialog->connect("file_selected", callable_mp(this, &OrchestratorEditorPanel::_open_script_file));
            add_child(_file_open_dialog);

            _file_save_dialog = memnew(OrchestratorFileDialog);
            _file_save_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
            _file_save_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
            _file_save_dialog->set_title("Save As Orchestration Script");
            _file_save_dialog->add_filter(filter, "Orchestrator Scripts");
            _file_save_dialog->connect("file_selected", callable_mp(this, &OrchestratorEditorPanel::_save_script_file));
            add_child(_file_save_dialog);

            // Create edited close confirmation dialog
            _close_confirm = memnew(ConfirmationDialog);
            _close_confirm->set_ok_button_text("Save");
            _close_confirm->add_button("Discard", DisplayServer::get_singleton()->get_swap_cancel_ok(), "discard");
            _close_confirm->connect("confirmed", callable_mp(this, &OrchestratorEditorPanel::_close_tab).bind(true));
            _close_confirm->connect("custom_action", callable_mp(this, &OrchestratorEditorPanel::_close_tab_discard_changes));
            add_child(_close_confirm);

            _goto_dialog = memnew(OrchestratorGotoNodeDialog);
            _goto_dialog->connect("goto_node", callable_mp(this, &OrchestratorEditorPanel::_goto_node));
            add_child(_goto_dialog);

            const Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
            _recent_files = metadata->get_value(RECENT_HISTORY_SECTION, RECENT_HISTORY_KEY, PackedStringArray());

            _update_recent_history();

            _script_create_dialog = memnew(ScriptCreateDialog);
            _script_create_dialog->connect("script_created", callable_mp(this, &OrchestratorEditorPanel::_script_file_created));
            add_child(_script_create_dialog);

            OrchestratorPlugin::get_singleton()->get_build_panel()->connect(
                "meta_clicked",
                callable_mp(this, &OrchestratorEditorPanel::_build_log_meta_clicked));

            break;
        }
        case NOTIFICATION_ENTER_TREE:
        {
            _update_scene_tab_signals();
            _update_file_system_dock_signals();
            break;
        }
        case NOTIFICATION_EXIT_TREE:
        {
            _update_scene_tab_signals(false);
            _update_file_system_dock_signals(false);
            break;
        }
        default:
            break;
    }
}

void OrchestratorEditorPanel::_bind_methods()
{
}

OrchestratorEditorPanel::OrchestratorEditorPanel(OrchestratorWindowWrapper* p_window_wrapper)
{
    _window_wrapper = p_window_wrapper;

    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}