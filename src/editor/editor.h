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
#ifndef ORCHESTRATOR_EDITOR_H
#define ORCHESTRATOR_EDITOR_H

#include "editor/graph/graph_node_theme_cache.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/script_create_dialog.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorLogEventRouter;
class OrchestratorGettingStarted;
class OrchestratorEditorCache;
class OrchestratorEditorView;
class OrchestratorFileDialog;
class OrchestratorUpdaterButton;
class OrchestratorWindowWrapper;

typedef OrchestratorEditorView* (*OrchestratorEditorViewFunc)(const Ref<Resource>& p_resource);

/// Main editor screen that handles all the editor coordination for Orchestrator
class OrchestratorEditor : public PanelContainer {
    GDCLASS(OrchestratorEditor, PanelContainer);

public:
    struct InputAction {
        String name;
        bool custom = false;

        bool operator==(const InputAction& other) const {
            return name == other.name && custom == other.custom;
        }
    };

private:
    enum {
        FILE_NEW,
        FILE_OPEN,
        FILE_OPEN_RECENT,
        FILE_REOPEN_CLOSED,
        FILE_SAVE,
        FILE_SAVE_AS,
        FILE_SAVE_ALL,
        FILE_SOFT_RELOAD_TOOL_SCRIPT,
        FILE_COPY_PATH,
        FILE_COPY_UID,
        FILE_SHOW_IN_FILESYSTEM,
        FILE_CLOSE,
        FILE_CLOSE_ALL,
        FILE_CLOSE_OTHERS,
        FILE_TOGGLE_LEFT_PANEL,
        FILE_TOGGLE_RIGHT_PANEL,
        HELP_ABOUT,
        HELP_ONLINE_DOCUMENTATION,
        HELP_COMMUNITY,
        HELP_GITHUB_ISSUES,
        HELP_GITHUB_FEATURE,
        HELP_SUPPORT
    };

    enum ScriptSortBy {
        SORT_BY_NAME,
        SORT_BY_PATH,
        SORT_BY_NONE
    };

    enum ScriptListName {
        DISPLAY_NAME,
        DISPLAY_DIR_AND_NAME,
        DISPLAY_FULL_PATH
    };

    const Size2 ABOUT_DIALOG_SIZE{ 780, 500 };

    enum {
        ORCHESTRATOR_VIEW_FUNC_MAX = 32
    };

    static int _editor_view_func_count;
    static OrchestratorEditorViewFunc _editor_view_funcs[ORCHESTRATOR_VIEW_FUNC_MAX];

    static OrchestratorEditor* _editor;

    OrchestratorEditorLogEventRouter* _log_router = nullptr;
    OrchestratorWindowWrapper* _window_wrapper = nullptr;
    OrchestratorGettingStarted* _getting_started = nullptr;
    OrchestratorFileDialog* _file_dialog = nullptr;
    OrchestratorUpdaterButton* _updater = nullptr;
    Ref<ConfigFile> _editor_cache;
    Ref<OrchestratorEditorGraphNodeThemeCache> _theme_cache;

    TextureRect* _script_icon = nullptr;
    Label* _script_name_label = nullptr;
    Window* _about_dialog = nullptr;
    HBoxContainer* _menu_hb = nullptr;
    HSplitContainer* _script_split = nullptr;
    MenuButton* _file_menu = nullptr;
    MenuButton* _goto_menu = nullptr;
    MenuButton* _help_menu = nullptr;
    PopupMenu* _context_menu = nullptr;
    Button* _site_search = nullptr;
    Timer* _autosave_timer = nullptr;
    uint64_t idle = 0;
    PopupMenu* _recent_history = nullptr;
    VSeparator* _make_floating_separator = nullptr;
    Button* _make_floating = nullptr;
    bool _floating = false;
    ItemList* _script_list = nullptr;
    LineEdit* _filter_scripts = nullptr;
    VBoxContainer* _scripts_vbox = nullptr;
    TabContainer* _tab_container = nullptr;
    List<int> _script_close_queue;
    Tree* _disk_changed_list = nullptr;
    AcceptDialog* _error_dialog = nullptr;
    ConfirmationDialog* _disk_changed = nullptr;
    ConfirmationDialog* _erase_tab_confirm = nullptr;
    ScriptCreateDialog* _script_create_dialog = nullptr;
    bool _restoring_layout = false;
    bool _pending_auto_reload = false;
    bool _auto_reload_running_scripts = false;
    bool _reload_all_scripts = false;
    bool _sort_list_on_update = false;
    bool _grab_focus_block = false;
    bool _waiting_update_names = false;
    int _file_dialog_option = -1;
    List<String> _previous_scripts;
    Vector<String> _script_paths_to_reload;
    Object* _previous_item = nullptr;
    Vector<InputAction> _input_action_cache;
    HashMap<String, Variant> _extra_layout_values;
    Vector<OrchestratorEditorView*> _restore_queue;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    static void _open_in_browser(const String& p_url);

    void _set_script_create_dialog_language(const String& p_language_name);

    void _prepare_file_menu();
    void _file_menu_closed();

    void _file_dialog_action(const String& p_file);

    void _menu_option(int p_option);
    void _tab_changed(int p_tab);

    void _show_getting_started();
    void _show_tabs();

    void _close_current_tab(bool p_save = true, bool p_history_back = true);
    void _close_tab(int p_idx, bool p_save = true, bool p_history_back = true);
    void _close_discard_current_tab(const String& p_value);
    void _close_other_tabs();
    void _close_all_tabs();
    void _queue_close_tabs();
    void _ask_close_current_unsaved_tab(OrchestratorEditorView* p_current);

    void _go_to_tab(int p_idx);

    void _copy_script_path();
    void _copy_script_uid();

    void _live_auto_reload_running_scripts();

    void _filter_scripts_text_changed(const String& p_text);
    void _update_script_colors();
    void _update_script_names();
    void _update_selected_editor_menu();
    void _update_online_doc();

    void _script_list_clicked(int p_item, Vector2 p_local_mouse_pos, MouseButton p_button_index);
    void _make_script_list_context_menu();
    void _script_selected(int p_index);

    void _script_changed();
    void _script_created(const Ref<Script>& p_script);
    void _add_callback(Object* p_object, const String& p_function, const PackedStringArray& p_args);
    void _resave_scripts(const String& p_value);
    void _reload_scripts(bool p_refresh_only = false);
    void _resource_saved_callback(const Ref<Resource>& p_resource);
    void _mark_built_in_scripts_as_saved(const String& p_full_path);

    PackedStringArray _get_recent_scripts() const;
    void _set_recent_scripts(const PackedStringArray& p_scripts);
    void _add_recent_script(const String& p_path);
    void _update_recent_scripts();
    void _open_recent_script(int p_index);

    void _autosave_scripts();
    void _update_autosave_timer();

    bool _test_script_times_on_disk(const Ref<Resource>& p_for_script = Ref<Resource>());

    Ref<Script> _get_current_script() const;
    TypedArray<Script> _get_open_scripts() const;

    void _goto_script_node(int p_node);
    void _goto_script_line(const Ref<RefCounted>& p_script, int p_line);

    void _breaked(bool p_breaked, bool p_can_debug);
    PackedStringArray _get_breakpoints() const;
    void _set_breakpoint(const Ref<RefCounted>& p_script, int p_node, bool p_enabled);
    void _clear_breakpoints();
    Array _get_cached_breakpoints_for_script(const String& p_path) const;

    void _window_changed(bool p_visible);
    void _tree_changed();
    void _split_dragged(float p_value);
    void _apply_editor_settings();
    void _editor_settings_changed();
    void _filesystem_changed();
    void _file_removed(const String& p_file);
    void _files_moved(const String& p_old_file, const String& p_new_file);

    OrchestratorEditorView* _get_current_editor() const;
    TypedArray<OrchestratorEditorView> _get_open_script_editors() const;

    void _view_layout_restored(OrchestratorEditorView* p_view);
    void _save_layout();
    void _save_editor_state(OrchestratorEditorView* p_editor);
    void _save_previous_state(Dictionary p_state);
    void _save_history();

    void _help_search(const String& p_text);

    bool _is_editor_setting_script_list_visible() const;

    void _project_settings_changed();
    void _update_input_actions_cache();

    // Needed for Godot
    OrchestratorEditor() = default;

public:
    static OrchestratorEditor* get_singleton() { return _editor; }

    Ref<OrchestratorEditorGraphNodeThemeCache> get_theme_cache() const;

    bool toggle_scripts_panel();
    bool is_scripts_panel_toggled();
    void toggle_components_panel();

    void apply_scripts();
    void reload_scripts(bool p_refresh_only = false);

    PackedStringArray get_unsaved_scripts() const;
    void save_current_script();
    void save_all_scripts();

    void update_script_times();
    void update_docs_from_script(const Ref<Script>& p_script);
    void clear_docs_from_script(const Ref<Script>& p_script);

    Vector<Ref<Script>> get_open_scripts() const;

    bool script_goto_node(const Ref<Script>& p_script, int p_node);
    bool script_goto_method(const Ref<Script>& p_script, const String& p_method);

    void open_script_create_dialog(const String& p_base_name, const String& p_base_path);
    Ref<Resource> open_file(const String& p_file);

    void ensure_select_current();

    bool is_editor_floating();

    _FORCE_INLINE_ bool edit(const Ref<Resource>& p_resource, bool p_grab_focus = true) { return edit(p_resource, -1, p_grab_focus); }
    bool edit(const Ref<Resource>& p_resource, int p_node, bool p_grab_focus = true);

    void notify_script_close(const Ref<Script>& p_script);
    void notify_script_changed(const Ref<Script>& p_script);
    void notify_scene_changed(Node* p_node);

    void trigger_live_script_reload(const String& p_script_path);
    void trigger_live_script_reload_all();
    void set_live_auto_reload_running_scripts(bool p_enabled);

    PackedStringArray get_breakpoints();

    void set_window_layout(const Ref<ConfigFile>& p_layout);
    void get_window_layout(const Ref<ConfigFile>& r_layout);

    bool can_take_away_focus() const;

    Variant get_drag_data_fw(const Point2& p_point, Control* p_from);
    bool can_drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) const;
    void drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from);

    PackedStringArray get_recognized_extensions() const;

    static void register_create_view_function(OrchestratorEditorViewFunc p_function);

    //~ Begin EditorNode helpers
    void find_scene_scripts(Node* p_base, Node* p_current, HashSet<Ref<Script>>& r_used);
    void push_item(Object* p_object, const String& p_property = "", bool p_inspector_only = false);
    void cache_and_push_item(Object* p_object, const String& p_property = "", bool p_inspector_only = false);
    void edit_previous_item();
    void save_resource(const Ref<Resource>& p_resource);
    void save_resource_in_path(const Ref<Resource>& p_resource, const String& p_path);
    void save_resource_as(const Ref<Resource>& p_resource, const String& p_path = String());
    void save_editor_layout_delayed();
    void disambiguate_filenames(const Vector<String>& p_paths, Vector<String>& r_filenames);
    Node* get_connections_dock() const;
    Node* get_inspector_dock() const;
    void make_inspector_visible();
    //~ End EditorNode helpers

    const Vector<InputAction>& get_input_actions_cache() const { return _input_action_cache; }

    Variant get_extra_layout_value(const String& p_key, const Variant& p_default = Variant()) const;
    void set_extra_layout_value(const String& p_key, const Variant& p_value);

    explicit OrchestratorEditor(OrchestratorWindowWrapper* p_window_wrapper);
    ~OrchestratorEditor() override = default;
};

#endif // ORCHESTRATOR_EDITOR_H