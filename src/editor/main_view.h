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
#ifndef ORCHESTRATOR_MAIN_VIEW_H
#define ORCHESTRATOR_MAIN_VIEW_H

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorPlugin;
class OScript;
class OrchestratorScreenSelect;
class OrchestratorScriptView;
class OrchestratorWindowWrapper;

namespace godot
{
    class ConfirmationDialog;
    class Container;
    class FileDialog;
    class HBoxContainer;
    class ItemList;
    class LineEdit;
    class MenuButton;
    class Script;
    class ScriptCreateDialog;
}

/// The plug-in main window
class OrchestratorMainView : public Control
{
    GDCLASS(OrchestratorMainView, Control);

private:
    enum AccelMenuIds
    {
        NEW,
        OPEN,
        SAVE,
        SAVE_AS,
        SAVE_ALL,
        SHOW_IN_FILESYSTEM,
        CLOSE,
        CLOSE_ALL,
        RUN,
        TOGGLE_LEFT_PANEL,
        GOTO_NODE,
        ABOUT,
        ONLINE_DOCUMENTATION,
        COMMUNITY,
        GITHUB_ISSUES,
        GITHUB_FEATURE,
        SUPPORT,
        TOGGLE_RIGHT_PANEL
    };

    //! Simple struct for managing details about a script file
    struct ScriptFile
    {
        String file_name;                 //! The filesystem filename
        OrchestratorScriptView* editor;  //! The editor reference
    };

    int _current_index{ -1 };                        //! Current index into open script files
    Vector<ScriptFile> _script_files;                //! Collection of open script files
    MenuButton* _file_menu{ nullptr };               //! File menu
    MenuButton* _goto_menu{ nullptr };               //! Goto menu
    MenuButton* _help_menu{ nullptr };               //! Help menu
    Window* _about_window{ nullptr };                //! About window
    FileDialog* _open_dialog{ nullptr };             //! Open file dialog window
    FileDialog* _save_dialog{ nullptr };             //! Save as file dialog window
    Container* _script_editor_container{ nullptr };  //! Script editor container
    Control* _left_panel{ nullptr };                 //! Left file list panel
    bool _right_panel_visible{ true };               //! Right component panel visibility
    String _file_name_filter;                        //! Current file name filter text
    ItemList* _file_list{ nullptr };                 //! File list
    Control* _updater{ nullptr };                    //! Updater widget
    ConfirmationDialog* _close_confirm{ nullptr };   //! Confirmation dialog
    ConfirmationDialog* _goto_dialog{ nullptr };     //! Goto node dialog
    List<ScriptFile> _script_close_queue;            //! Queue of scripts to be removed
    OrchestratorPlugin* _plugin{ nullptr };          //! Orchestrator plugin
    bool _floating{ false };                         //! Whether this window is floating
    Control* _select_separator{ nullptr };           //! Screen selection separator
    OrchestratorScreenSelect* _select{ nullptr };    //! Screen selection
    OrchestratorWindowWrapper* _wrapper{ nullptr };  //! Window wrapper

    static void _bind_methods();
    OrchestratorMainView() = default;

public:
    /// Creates the main view
    /// @param p_plugin the plugin instance
    /// @param p_window_wrapper the window wrapper
    OrchestratorMainView(OrchestratorPlugin* p_plugin, OrchestratorWindowWrapper* p_window_wrapper);

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Edit the specified object
    /// @param object orchestrator script object
    void edit(const Ref<OScript>& p_script);

    /// Saves all open files
    void apply_changes();

    /// Get the window's layout to be serialized to disk when the editor is restored
    /// @param p_configuration the configuration
    void get_window_layout(const Ref<ConfigFile>& p_configuration);

    /// Set the window's layout, restoring the previous editor state
    /// @param p_configuration the configuration
    void set_window_layout(const Ref<ConfigFile>& p_configuration);

    /// Performs the build step
    /// @return true if the build was successful; false otherwise
    bool build();

private:
    /// Check whether the current index points to an open script
    /// @return true if there is an open script, false otherwise
    bool _has_open_script() const;

    /// Gets the index into the script files array by file name
    int _get_script_file_index_by_file_name(const String& p_file_name) const;

    bool _is_current_script_unsaved() const;
    void _ask_close_current_unsaved_script();

    /// Open the specified script in the main view.
    /// @param p_script the script to be opened, should be valid
    void _open_script(const Ref<OScript>& p_script);

    /// Saves the currently opened script
    void _save_script();

    /// Saves all opened scripts
    void _save_all_scripts();

    /// Closes the currently opened script
    /// @param p_save whether the contents are saved or discarded
    void _close_script(bool p_save = true);

    /// Closes the script at the specified index
    /// @param p_index the index of the script in the file array
    /// @param p_save whether the contents are saved or discarded
    void _close_script(int p_index, bool p_save = true);

    /// Closes all opened scripts
    void _close_all_scripts();

    void _queue_close_scripts();

    /// Displays the new script dialog
    void _show_create_new_script_dialog();

    /// Shows the specified file's script editor view.
    /// @param p_file_name the file's script editor to focus
    void _show_script_editor_view(const String& p_file_name);

    /// Dispatches an update to the files list
    void _update_files_list();

    /// Navigate to the current file in the FileSystemDock
    void _navigate_to_current_path();

    /// Dispatched immediate before the file menu is shown to update its state
    void _on_prepare_file_menu();

    /// Dispatched when the file menu is closed
    void _on_file_menu_closed();

    /// Dispatched when a file, help, or other menu option is selected
    /// @param p_option the menu option id
    void _on_menu_option(int p_option);

    /// Dispatched when the new script dialog creates a new script file
    /// @param p_script the new script that was created
    void _on_script_file_created(const Ref<Script>& p_script);

    /// Dispatched when opening a script file in the open file dialog
    /// @param p_file_name the script file to open
    void _on_open_script_file(const String& p_file_name);

    /// Dispatched when saving a script file using save-as dialog.
    /// @param p_file_name the script file to save
    void _on_save_script_file(const String& p_file_name);

    /// Dispatched when the file filter is modified by the user
    /// @param p_text the new filter text
    void _on_file_filters_changed(const String& p_text);

    /// Dispatched when the file selection in the file list changes
    /// @param p_index the index in the file list that was selected
    void _on_file_list_selected(int p_index);

    /// Dispatched to close the current tab, optionally saving the contents.
    /// @param p_save whether to save the contents
    void _on_close_current_tab(bool p_save = true);

    /// Dispatched to close the current tab, discarding any pending changes.
    void _on_close_discard_current_tab(const String& p_data);

    /// Dispatched when the goto menu is shown
    void _on_prepare_goto_menu();

    /// Dispatched when the user clicks OK to goto a node
    void _on_goto_node(LineEdit* p_edit);

    /// Dispatched when the user closes or accepts the goto node value
    void _on_goto_node_closed(LineEdit* p_edit);

    /// Dispatched when the goto dialog window's visibility changes.
    /// @param p_edit the line edit control, should never be null
    void _on_goto_node_visibility_changed(LineEdit* p_edit);

    /// Dispatched when the window's floating status changes
    /// @param p_visible the current visibility status
    void _on_window_changed(bool p_visible);

    /// Dispatched when the user changes scene tabs.
    /// @param p_tab_index the new tab index that gained focus
    void _on_scene_tab_changed(int p_tab_index);

    /// Dispatched when a file is removed from the project
    /// @param p_file_name the file name
    void _on_file_removed(const String& p_file_name);

    /// Dispatched when a file is moved.
    /// @param p_old_name the old file name
    /// @param p_new_name the new file name
    void _on_files_moved(const String& p_old_name, const String& p_new_name);

    /// Dispatched when a folder is removed from the project
    /// @param p_folder_name the folder name
    void _on_folder_removed(const String& p_folder_name);
};

#endif  // ORCHESTRATOR_MAIN_VIEW_H