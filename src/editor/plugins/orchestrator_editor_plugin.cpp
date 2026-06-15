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
#include "editor/plugins/orchestrator_editor_plugin.h"

#include "common/macros.h"
#include "common/resource_utils.h"
#include "common/version.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/export/orchestration_export_plugin.h"
#include "editor/gui/window_wrapper.h"
#include "editor/inspector/function_inspector_plugin.h"
#include "editor/inspector/orchestration_inspector_plugin.h"
#include "editor/inspector/signal_inspector_plugin.h"
#include "editor/inspector/type_cast_inspector_plugin.h"
#include "editor/inspector/variable_inspector_plugin.h"
#include "editor/script_editor_view.h"
#include "editor/settings/settings_dialog.h"
#include "script/script.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#if GODOT_VERSION >= 0x040500
#include <godot_cpp/classes/dpi_texture.hpp>
#endif
#include "common/scene_utils.h"
#include "editor/settings/editor_settings.h"

#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/viewport.hpp>

OrchestratorPlugin* OrchestratorPlugin::_plugin = nullptr;

String OrchestratorPlugin::_get_orchestrator_metedata_path() {
    return EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator_metadata.cfg");
}

void OrchestratorPlugin::_focus_another_editor() {
    GUARD_NULL(_window_wrapper);
    if (_window_wrapper->get_window_enabled()) {
        ERR_FAIL_COND(_last_editor.is_empty());

        EI->get_base_control()->get_viewport()->gui_release_focus();
        EI->set_main_screen_editor(_last_editor);
    }
}

bool OrchestratorPlugin::_is_exiting() const {
    // todo: would be nice to have a better way to know the editor is exiting
    //  see https://github.com/godotengine/godot/pull/107861
    return EI->get_base_control()->get_modulate() != Color(1, 1, 1, 1);
}

void OrchestratorPlugin::_register_plugins() {
    // Inspector plugins
    _register_plugin<OrchestratorEditorInspectorPluginFunction>();
    _register_plugin<OrchestratorEditorInspectorPluginSignal>();
    _register_plugin<OrchestratorEditorInspectorPluginVariable>();
    _register_plugin<OrchestratorEditorInspectorPluginTypeCast>();
    _register_plugin<OrchestratorEditorInspectorPluginOrchestration>();

    // Export Plugins
    _register_plugin<OrchestratorEditorExportPlugin>();

    // Debugger Plugins
    _register_plugin<OrchestratorEditorDebuggerPlugin>();
}

void OrchestratorPlugin::_unregister_plugins() {
    _unregister_plugin<EditorDebuggerPlugin>();
    _unregister_plugin<EditorExportPlugin>();
    _unregister_plugin<EditorInspectorPlugin>();
}

void OrchestratorPlugin::_register_shortcuts() {
    //~ Begin Top-Level Editor
    ED_SHORTCUT("editor/new_file", "New Orchestration...", OACCEL_KEY(KEY_MASK_CTRL, KEY_N));
    ED_SHORTCUT("editor/open_file", "Open...");
    ED_SHORTCUT("editor/reopen_closed_file", "Reopen Closed Orchestration", OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_SHIFT, KEY_T));
    ED_SHORTCUT("editor/close_file", "Close", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_W));
    ED_SHORTCUT("editor/close_all", "Close All");
    ED_SHORTCUT("editor/close_other_tabs", "Close Other Tabs");
    ED_SHORTCUT("editor/clear_recent_files", "Clear Recent History");
    ED_SHORTCUT("editor/save", "Save", OACCEL_KEY(KEY_MASK_ALT | KEY_MASK_CMD_OR_CTRL, KEY_S));
    ED_SHORTCUT("editor/save_as", "Save As...");
    ED_SHORTCUT("editor/save_all", "Save All", OACCEL_KEY(KEY_MASK_SHIFT | KEY_MASK_ALT, KEY_S));
    ED_SHORTCUT("editor/copy_path", "Copy Orchestration Path");
    ED_SHORTCUT("editor/copy_uid", "Copy Orchestration UID");
    ED_SHORTCUT("editor/show_in_file_system", "Show in FileSystem");
    ED_SHORTCUT("editor/toggle_file_panel", "Toggle Orchestration Panel", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_BACKSLASH));
    ED_SHORTCUT("editor/toggle_component_panel", "Toggle Component Panel", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_SLASH));
    ED_SHORTCUT("editor/soft_reload_tool_script", "Soft Reload Tool Script", OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_ALT, KEY_R));
    ED_SHORTCUT("editor/run", "Run Tool Script");
    ED_SHORTCUT("editor/settings", "Settings", OACCEL_KEY(KEY_MASK_CTRL | KEY_MASK_SHIFT, KEY_S));

    //~ Begin Graph Editor
    ED_SHORTCUT("graph_editor/bookmark/toggle", "Toggle Bookmark", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_ALT, KEY_B));
    ED_SHORTCUT("graph_editor/bookmark/remove_all", "Remove All Bookmarks", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_ALT | KEY_MASK_SHIFT, KEY_B));
    ED_SHORTCUT("graph_editor/bookmark/goto_next", "Goto Next Bookmark", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_B));
    ED_SHORTCUT("graph_editor/bookmark/goto_previous", "Goto Previous Bookmark", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_SHIFT, KEY_B));
    ED_SHORTCUT("graph_editor/breakpoint/toggle", "Toggle Breakpoint", KEY_F9);
    ED_SHORTCUT_OVERRIDE("graph_editor/breakpoint/toggle", "macos", OACCEL_KEY(KEY_MASK_META | KEY_MASK_SHIFT, KEY_B));
    ED_SHORTCUT("graph_editor/breakpoint/remove_all", "Remove All Breakpoints", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_SHIFT, KEY_F9));
    ED_SHORTCUT_OVERRIDE("graph_editor/breakpoint/remove_all", "macos", OACCEL_KEY(KEY_MASK_META | KEY_MASK_SHIFT, KEY_B));
    ED_SHORTCUT("graph_editor/breakpoint/goto_next", "Goto Next Breakpoint", OACCEL_KEY(KEY_MASK_CTRL, KEY_PERIOD));
    ED_SHORTCUT("graph_editor/breakpoint/goto_previous", "Goto Previous Breakpoint", OACCEL_KEY(KEY_MASK_CTRL, KEY_COMMA));
    ED_SHORTCUT("graph_editor/breakpoint/disable", "Disable breakpoint");
    ED_SHORTCUT("graph_editor/breakpoint/enable", "Enable breakpoint");
    ED_SHORTCUT("graph_editor/goto_node", "Goto Node", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_L));

    ED_SHORTCUT("graph_editor/break_node_links", "Break Node Link(s)");

    ED_SHORTCUT("graph_editor/toggle_resizer", "Toggle Resizer");
    ED_SHORTCUT("graph_editor/resize_to_content", "Resize to Content");
    ED_SHORTCUT("graph_editor/refresh_nodes", "Refresh Nodes");
    ED_SHORTCUT("graph_editor/add_call_parent_function", "Add Call to Parent Function");
    ED_SHORTCUT("graph_editor/add_option_pin", "Add Option Pin");
    ED_SHORTCUT("graph_editor/await_function", "Await Function");
    ED_SHORTCUT("graph_editor/detach_from_frame", "Detach from Frame");
    ED_SHORTCUT("graph_editor/expand_node", "Expand Node");
    ED_SHORTCUT("graph_editor/collapse_to_function", "Collapse to Function");
    ED_SHORTCUT("graph_editor/alignment/align_top", "Align Top");
    ED_SHORTCUT("graph_editor/alignment/align_middle", "Align Middle");
    ED_SHORTCUT("graph_editor/alignment/align_bottom", "Align Bottom");
    ED_SHORTCUT("graph_editor/alignment/align_left", "Align Left");
    ED_SHORTCUT("graph_editor/alignment/align_center", "Align Center");
    ED_SHORTCUT("graph_editor/alignment/align_right", "Align Right");
    ED_SHORTCUT("graph_editor/view_documentation", "View Documentation");
    ED_SHORTCUT("graph_editor/zoom_in", "Zoom In", KEY_KP_ADD);
    ED_SHORTCUT("graph_editor/zoom_out", "Zoom Out", KEY_KP_SUBTRACT);

    ED_SHORTCUT("graph_editor/frame/set_frame_title", "Set Frame Title");
    ED_SHORTCUT("graph_editor/frame/set_comment_text", "Set Comment Text");
    ED_SHORTCUT("graph_editor/frame/enable_auto_shrink", "Enable Auto Shrink");
    ED_SHORTCUT("graph_editor/frame/enable_tint_color", "Enable Tint Color");
    ED_SHORTCUT("graph_editor/frame/set_tint_color", "Set Tint Color");

    OrchestratorEditorSettings* oes = OrchestratorEditorSettings::get_singleton();

    {
        Ref<InputEventMouseButton> mb;
        mb.instantiate();
        mb->set_button_index(MOUSE_BUTTON_LEFT);
        mb->set_command_or_control_autoremap(true);
        oes->register_shortcut("graph_editor/reroutes/create_reroute", "Create Reroute Node", mb);
    }
    ED_SHORTCUT("graph_editor/reroutes/delete_reroute", "Remove Reroute Node", OACCEL_KEY(KEY_MASK_SHIFT, KEY_DELETE));

    ED_SHORTCUT("graph_editor/create_nodes/branch", "Create Branch", KEY_B);
    ED_SHORTCUT("graph_editor/create_nodes/comment", "Create Comment", KEY_C);
    ED_SHORTCUT("graph_editor/create_nodes/delay", "Create Delay", KEY_D);
    ED_SHORTCUT("graph_editor/create_nodes/sequence", "Create Sequence", KEY_S);

    //~ Begin Components Panel
    ED_SHORTCUT("graph_components_panel/open", "Open", KEY_ENTER);
    ED_SHORTCUT("graph_components_panel/focus", "Focus", KEY_ENTER);
    ED_SHORTCUT("graph_components_panel/rename", "Rename", KEY_F2);
    ED_SHORTCUT("graph_components_panel/remove", "Remove", KEY_DELETE);
    ED_SHORTCUT("graph_components_panel/disconnect_signal", "Disconnect", KEY_NONE);
    ED_SHORTCUT("graph_components_panel/duplicate", "Duplicate", KEY_NONE);
    ED_SHORTCUT("graph_components_panel/duplicate_without_code", "Duplicate (No Code)", KEY_NONE);

    // Transients
    // EditorSettings in pre-4.6 does not expose shortcuts for non-customized values
    // For now, until we drop pre-4.6 support, we explicitly remap as transient bindings
    oes->register_shortcut("editor/open_search", "Focus Search/Filter Bar", OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_F), true);

    oes->register_shortcut("debugger/step_into", "Step Into", KEY_F11, true);
    oes->register_shortcut("debugger/step_over", "Step Over", KEY_F10, true);
    oes->register_shortcut("debugger/break", "Break", KEY_NONE, true);
    oes->register_shortcut("debugger/continue", "Continue", KEY_F12, true);
}

bool OrchestratorPlugin::_is_plugin_just_installed() const {
    if (FileAccess::file_exists(_get_orchestrator_metedata_path())) {
        return false;
    }

    Ref<FileAccess> file = FileAccess::open(_get_orchestrator_metedata_path(), FileAccess::WRITE);
    if (file.is_valid()) {
        file->close();
        return true;
    }

    return false;
}

void OrchestratorPlugin::_add_plugin_icon_to_editor_theme() {
    // Register the plugin's icon for CreateScript Dialog
    Ref<Theme> theme = EI->get_editor_theme();
    if (theme.is_valid() && !theme->has_icon(_get_plugin_name(), "EditorIcons")) {
        theme->set_icon(_get_plugin_name(), "EditorIcons", _get_plugin_icon());
        theme->set_icon(OScriptLanguage::get_singleton()->_get_type(), "EditorIcons", _get_plugin_icon());
    }
}

void OrchestratorPlugin::_window_visibility_changed(bool p_visible) {
    if (p_visible) {
        _focus_another_editor();
    } else {
        EI->set_main_screen_editor(_get_plugin_name());
    }
}

void OrchestratorPlugin::_main_screen_changed(const String& p_name) {
    if (p_name != _get_plugin_name()) {
        _last_editor = p_name;
    }
}

String OrchestratorPlugin::get_plugin_version() const {
    return VERSION_NUMBER;
}

void OrchestratorPlugin::_edit(Object* p_object) {
    GUARD_NULL(_editor_panel);
    GUARD_NULL(_window_wrapper);

    if (!p_object || !_handles(p_object)) {
        return;
    }

    Ref<Resource> resource = cast_to<Resource>(p_object);
    if (!resource.is_valid()) {
        return;
    }

    if (resource->get_path().is_empty()) {
        return;
    }

    make_active();

    _editor_panel->edit(resource);
    _window_wrapper->move_to_foreground();
}

bool OrchestratorPlugin::_handles(Object* p_object) const {
    return p_object != nullptr && p_object->get_class() == "OScript";
}

bool OrchestratorPlugin::_has_main_screen() const {
    return true;
}

void OrchestratorPlugin::_make_visible(bool p_visible) {
    GUARD_NULL(_window_wrapper);
    if (p_visible) {
        if (_window_wrapper->get_window_enabled()) {
            // EditorPlugin::selected_notify is not exposed to GDExtension, but this method
            // is called just before "selected_notify" as a way to address this until the
            // method can be exposed.
            _focus_another_editor();
        }
        _window_wrapper->show();
    }
    else {
        _window_wrapper->hide();
    }
}

String OrchestratorPlugin::_get_plugin_name() const {
    return VERSION_NAME;
}

Ref<Texture2D> OrchestratorPlugin::_get_plugin_icon() const {
    #if GODOT_VERSION >= 0x040500
    Ref<FileAccess> file = FileAccess::open("res://addons/orchestrator/icons/Orchestrator_Logo_16x16.svg", FileAccess::READ);
    if (file.is_valid()) {
        return DPITexture::create_from_string(file->get_as_text(), EDSCALE);
    }
    #endif
    return ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_Logo_16x16.svg");
}

void OrchestratorPlugin::_save_external_data() {
    GUARD_NULL(_editor_panel);

    if (!_is_exiting()) {
        _editor_panel->save_all_scripts();
    }

    // When the editor saves the scene, it will propagate a call to save all external
    // resources used by the scene. If one of those resources is a script that is
    // open in the editor, we need to update the script times. If we don't, this will
    // trigger a notification that the file was modified outside the editor.
    _editor_panel->update_script_times();
}

String OrchestratorPlugin::_get_unsaved_status(const String& p_for_scene) const {
    if (!_editor_panel) {
        return {};
    }

    const PackedStringArray unsaved_scripts = _editor_panel->get_unsaved_scripts();
    if (unsaved_scripts.is_empty()) {
        return {};
    }

    PackedStringArray message;
    if (!p_for_scene.is_empty()) {
        PackedStringArray unsaved_built_in_scripts;
        const String scene_file = p_for_scene.get_file();
        for (const String &E : unsaved_scripts) {
            if (!ResourceUtils::is_file(E) && E.contains(scene_file)) {
                unsaved_built_in_scripts.append(E);
            }
        }

        if (unsaved_built_in_scripts.is_empty()) {
            return {};
        }

        message.resize(unsaved_built_in_scripts.size() + 1);
        message.append("There are unsaved changes in the following built-in script(s):");

        for (const String &E : unsaved_built_in_scripts) {
            message.append(E.trim_suffix("(*)"));
        }

        return String("\n").join(message);
    }

    message.push_back("Save changes to the following Orchestrator file(s) before quitting?");
    for (const String& E : unsaved_scripts) {
        message.push_back(E.trim_suffix("(*)"));
    }

    return String("\n").join(message);
}

void OrchestratorPlugin::_apply_changes() {
    if (_editor_panel) {
        _editor_panel->apply_scripts();
    }
}

void OrchestratorPlugin::_set_window_layout(const Ref<ConfigFile>& p_configuration) {
    if (_editor_panel) {
        _editor_panel->set_window_layout(p_configuration);
    }

    if (restore_windows_on_load()) {
        if (_window_wrapper->is_window_available() && p_configuration->has_section_key("Orchestrator", "window_rect")) {
            _window_wrapper->restore_window_from_saved_position(
                p_configuration->get_value("Orchestrator", "window_rect", Rect2i()),
                p_configuration->get_value("Orchestrator", "window_screen", -1),
                p_configuration->get_value("Orchestrator", "window_screen_rect", Rect2i()));
        } else {
            _window_wrapper->set_window_enabled(false);
        }
    }
}

void OrchestratorPlugin::_get_window_layout(const Ref<ConfigFile>& p_configuration) {
    if (_editor_panel) {
        _editor_panel->get_window_layout(p_configuration);
    }

    if (_window_wrapper->get_window_enabled()) {
        const int screen = _window_wrapper->get_window_screen();
        p_configuration->set_value("Orchestrator", "window_rect", _window_wrapper->get_window_rect());
        p_configuration->set_value("Orchestrator", "window_screen", screen);
        p_configuration->set_value("Orchestrator", "window_screen_rect", DisplayServer::get_singleton()->screen_get_usable_rect(screen));
    } else {
        if (p_configuration->has_section_key("Orchestrator", "window_rect")) {
            p_configuration->erase_section_key("Orchestrator", "window_rect");
        }
        if (p_configuration->has_section_key("Orchestrator", "window_screen")) {
            p_configuration->erase_section_key("Orchestrator", "window_screen");
        }
        if (p_configuration->has_section_key("Orchestrator", "window_screen_rect")) {
            p_configuration->erase_section_key("Orchestrator", "window_screen_rect");
        }
    }
}

bool OrchestratorPlugin::_build() {
    return true;
}

void OrchestratorPlugin::_enable_plugin() {
}

void OrchestratorPlugin::_disable_plugin() {
}

PackedStringArray OrchestratorPlugin::_get_breakpoints() const {
    // When the game is started with the debugger, it uses this method to gather all breakpoints,
    // and these are passed to the CLI of the game process. this should obtain all breakpoints
    // currently set and return them using the format of "<script_file>:<node_id>".
    return _editor_panel ? _editor_panel->get_breakpoints() : PackedStringArray();
}

String OrchestratorPlugin::get_github_issues_url() {
    return "https://github.com/CraterCrash/godot-orchestrator/issues/new/choose";
}

String OrchestratorPlugin::get_patreon_url() {
    return "https://donate.cratercrash.space/";
}

String OrchestratorPlugin::get_community_url() {
    return "https://discord.cratercrash.space/";
}

String OrchestratorPlugin::get_plugin_online_documentation_url() {
    return VERSION_DOCS_URL;
}

bool OrchestratorPlugin::restore_windows_on_load() {
    return EDITOR_GET("interface/multi_window/restore_windows_on_load");
}

void OrchestratorPlugin::request_editor_restart() {
    AcceptDialog* request = memnew(AcceptDialog);
    request->set_title("Restart editor");
    request->reset_size();

    VBoxContainer* container = memnew(VBoxContainer);
    Label* label = memnew(Label);
    label->set_text("The editor requires a restart.");
    label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    container->add_child(label);
    request->add_child(container);

    request->connect(SceneStringName(confirmed), callable_mp(EI, &EditorInterface::restart_editor).bind(true));
    request->connect(SceneStringName(canceled), callable_mp(EI, &EditorInterface::restart_editor).bind(true));

    get_tree()->create_timer(1)->connect("timeout", callable_mp(EI, &EditorInterface::popup_dialog_centered).bind(request, Vector2()));
}

Ref<Texture2D> OrchestratorPlugin::get_plugin_icon_hires() const {
    return ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_Logo.svg");
}

Ref<ConfigFile> OrchestratorPlugin::get_metadata() {
    Ref<ConfigFile> metadata(memnew(ConfigFile));
    metadata->load(_get_orchestrator_metedata_path());
    return metadata;
}

void OrchestratorPlugin::save_metadata(const Ref<ConfigFile>& p_metadata) {
    p_metadata->save(_get_orchestrator_metedata_path());
}

void OrchestratorPlugin::make_active() {
    if (_has_main_screen()) {
        EI->set_main_screen_editor(_get_plugin_name());
    }
}

void OrchestratorPlugin::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            // Plugins only enter the tree once and this happens before the main view.
            // It's safe then to cache the plugin reference here.
            _plugin = this;

            _register_plugins();
            _register_shortcuts();

            _window_wrapper = memnew(OrchestratorWindowWrapper);
            _window_wrapper->set_window_title(vformat("Orchestrator - Godot Engine"));
            _window_wrapper->set_margins_enabled(true);
            _window_wrapper->set_v_size_flags(Control::SIZE_EXPAND_FILL);
            _window_wrapper->hide();
            _window_wrapper->connect("window_visibility_changed", callable_mp_this(_window_visibility_changed));

            _editor_panel = memnew(OrchestratorEditor(_window_wrapper));
            _window_wrapper->set_wrapped_control(_editor_panel);

            EI->get_editor_main_screen()->add_child(_window_wrapper);

            _make_visible(false);

            connect("main_screen_changed", callable_mp_this(_main_screen_changed));
            break;
        }
        case NOTIFICATION_EXIT_TREE: {
            disconnect("main_screen_changed", callable_mp_this(_main_screen_changed));

            _unregister_plugins();

            SAFE_MEMDELETE(_editor_panel);
            _plugin = nullptr;
            break;
        }
        case NOTIFICATION_READY: {
            if (_is_plugin_just_installed()) {
                // When a GDExtension is first installed or loaded, there is a known bug that causes
                // an issue with initialization of the ScriptLanguage, see
                // https://github.com/godotengine/godot/pull/114131
                //
                // We currently force and init in the extension_interface.cpp in the EDITOR level,
                // however, this only initializes the language but does not update the create script
                // dialog due to the order of operations.
                callable_mp_this(request_editor_restart).call_deferred();
            } else {
                _add_plugin_icon_to_editor_theme();
            }
            break;
        }
        case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
            // Editor settings changed (possibly a native shortcut rebind). Re-sync our transient
            // shadow shortcuts so menus bound to them reflect the live native bindings.
            OrchestratorEditorSettings::get_singleton()->refresh_transient_shortcuts();
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorPlugin::_bind_methods() {
}

OrchestratorPlugin::OrchestratorPlugin() {
    #if TOOLS_ENABLED
    OrchestratorScriptGraphEditorView::register_editor();
    #endif
}