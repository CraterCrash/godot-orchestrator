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
#ifndef ORCHESTRATOR_EDITOR_PANEL_H
#define ORCHESTRATOR_EDITOR_PANEL_H

#include "common/version.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/file_system_dock.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorViewport;
class OrchestratorFileDialog;
class OrchestratorGettingStarted;
class OrchestratorGotoNodeDialog;
class OrchestratorScreenSelect;
class OrchestratorUpdater;
class OrchestratorWindowWrapper;
class OScript;

/// The main editor panel for when the Orchestrator plugin main view is active.
class OrchestratorEditorPanel : public PanelContainer
{
    GDCLASS(OrchestratorEditorPanel, PanelContainer);
    static void _bind_methods();

    enum MenuOptions
    {
        FILE_NEW,
        FILE_OPEN,
        FILE_OPEN_RECENT,
        FILE_SAVE,
        FILE_SAVE_AS,
        FILE_SAVE_ALL,
        FILE_COPY_PATH,
        FILE_SHOW_IN_FILESYSTEM,
        FILE_CLOSE,
        FILE_CLOSE_ALL,
        FILE_CLOSE_OTHERS,
        FILE_TOGGLE_LEFT_PANEL,
        FILE_TOGGLE_RIGHT_PANEL,
        GOTO_NODE,
        HELP_ABOUT,
        HELP_ONLINE_DOCUMENTATION,
        HELP_COMMUNITY,
        HELP_GITHUB_ISSUES,
        HELP_GITHUB_FEATURE,
        HELP_SUPPORT
    };

    // Represents a file that is currently open in the plugin.
    struct OrchestrationFile
    {
        String file_name;
        OrchestratorEditorViewport* viewport{ nullptr };
    };

    // Represents file list context details
    struct FileListContext
    {
        Vector<OrchestrationFile> open_files;  //! List of open files
        List<OrchestrationFile> close_queue;   //! Queue of files to close
        int current_index{ -1 };               //! Current opened file

        PackedStringArray get_open_file_names() const;

        String get_selected_file_name() const;
        const OrchestrationFile* get_selected() const;

        int get_file_index(const String& p_file_name) const;

        void hide_all();
        void show(const String& p_file_name);

        bool is_current_unsaved() const;

        void remove_at(int p_index);
        bool rename(const String& p_old_file_name, const String& p_new_file_name);

        bool is_index_valid(int p_index) const;
    };

    const String RECENT_HISTORY_POPUP_NAME{ "OrchestratorRecentHistory" };
    const String RECENT_HISTORY_SECTION{ "recent_files" };
    const String RECENT_HISTORY_KEY{ "orchestrations" };
    const String LAYOUT_SECTION{ "Orchestrator" };
    const String LAYOUT_LEFT_PANEL{ "file_list_visibility" };
    const String LAYOUT_RIGHT_PANEL{ "component_panel_visibility" };
    const String LAYOUT_OPEN_FILES{ "open_files" };
    const String LAYOUT_OPEN_FILES_SELECTED{ "open_files_selected" };
    const Vector2i SEPARATOR_SIZE{ 0, 24 };
    const Size2 ABOUT_DIALOG_SIZE{ 780, 500 };
    const int MAX_RECENT_FILES{ 10 };

protected:
    FileListContext _files_context;                           //! File list context
    bool _left_panel_visible{ true };                         //! Whether the left panel is visible
    bool _right_panel_visible{ true };                        //! Whether the right panel is visible
    bool _floating{ false };                                  //! Whether the panel is floating
    MenuButton* _file_menu{ nullptr };                        //! File menu
    MenuButton* _goto_menu{ nullptr };                        //! Goto menu
    MenuButton* _help_menu{ nullptr };                        //! Help menu
    PopupMenu* _recent_history{ nullptr };                    //! Recent file history popup
    PopupMenu* _file_list_context_menu{ nullptr };            //! File list context menu
    ItemList* _file_list{ nullptr };                          //! File list
    Window* _about_dialog{ nullptr };                         //! About dialog
    OrchestratorFileDialog* _file_open_dialog{ nullptr };     //! File open dialog
    OrchestratorFileDialog* _file_save_dialog{ nullptr };     //! File save dialog
    ConfirmationDialog* _close_confirm{ nullptr };            //! Close confirmation dialog
    OrchestratorGotoNodeDialog* _goto_dialog{ nullptr };      //! Goto node dialog
    OrchestratorScreenSelect* _screen_select{ nullptr };      //! Window screen selector
    OrchestratorWindowWrapper* _window_wrapper{ nullptr };    //! Window wrapper
    OrchestratorGettingStarted* _getting_started{ nullptr };  //! Getting started landing
    OrchestratorUpdater* _updater{ nullptr };                 //! Updater
    Control* _select_separator{ nullptr };                    //! Separator that is hidden based on float state
    Control* _left_panel{ nullptr };                          //! Togglable left panel
    Container* _viewport_container{ nullptr };                //! Main viewport container
    String _file_name_filter;                                 //! The filter text
    PackedStringArray _recent_files;                          //! The recent files list

    //~ Begin Godot Interface
    void _notification(int p_what);
    //~ End Godot interface

    void _update_scene_tab_signals(bool p_connect = true);
    void _update_file_system_dock_signals(bool p_connect = true);
    void _update_file_list();
    void _update_getting_started();
    void _recent_history_selected(int p_index);
    void _update_recent_history();
    void _save_recent_history();
    void _prepare_file_menu();
    void _prepare_goto_menu();
    void _goto_node(int p_node_id);
    void _navigate_to_file_in_filesystem();
    void _handle_menu_option(int p_option);
    bool _has_open_files() const;
    void _show_editor_viewport(const String& p_file_name);
    void _open_script_file(const String& p_file_name);
    void _save_script_file(const String& p_file_name);
    void _save_script();
    void _save_all_scripts();
    void _ask_close_current_unsaved_editor();
    void _close_script(bool p_save = true);
    void _close_script(int p_index, bool p_save = true);
    void _close_all_scripts();
    void _close_other_scripts();
    void _queue_close_scripts();
    void _show_create_new_script_dialog();
    void _script_file_created(const Ref<Script>& p_script);
    void _file_filter_changed(const String& p_text);
    void _file_list_selected(int p_index);
    void _show_file_list_context_menu(int p_index, const Vector2& p_position, int p_button);
    void _close_tab(bool p_save = true);
    void _close_tab_discard_changes(const String& p_data);
    void _window_changed(bool p_visible);
    void _scene_tab_changed(int p_index);
    void _file_removed(const String& p_file_name);
    void _file_moved(const String& p_old_file_name, const String& p_new_file_name);
    void _folder_removed(const String& p_folder_name);
    void _add_script_function(Object* p_object, const String& p_function_name, const PackedStringArray& p_args);

    #if GODOT_VERSION >= 0x040300
    void _goto_script_line(const Ref<Script>& p_script, int p_line);
    void _clear_all_breakpoints();
    void _set_breakpoint(const Ref<Script>& p_script, int p_line, bool p_enabled);
    #endif

    /// Constructor, intentially protected
    OrchestratorEditorPanel() = default;

public:
    /// Edit a specific resource
    /// @param p_resource the resource to edit
    void edit_resource(const Ref<Resource>& p_resource);

    /// Edit a specific orchestration script
    /// @param p_script the script to edit
    void edit_script(const Ref<OScript>& p_script);

    /// Apply any changes
    void apply_changes();

    /// Performs the build step
    /// @return true if the build was successful, false otherwise
    bool build();

    /// Get the window's current layout
    /// @param p_config the configuration to populate, always valid
    void get_window_layout(const Ref<ConfigFile>& p_config);

    /// Apply the window layout
    /// @param p_config the configuration to read from
    void set_window_layout(const Ref<ConfigFile>& p_config);

    #if GODOT_VERSION >= 0x040300
    /// Get all active, defined breakpoints
    /// @return list of breakpoints
    PackedStringArray get_breakpoints() const;
    #endif

    /// Constructs the editor panel
    /// @param p_window_wrapper
    explicit OrchestratorEditorPanel(OrchestratorWindowWrapper* p_window_wrapper);
};

#endif  // ORCHESTRATOR_EDITOR_PANEL_H