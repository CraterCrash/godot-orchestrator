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
#include "editor/editor.h"
#include "editor/plugins/inspector_plugins.h"
#include "editor/plugins/orchestration_editor_export_plugin.h"
#include "editor/script_editor_view.h"
#include "editor/window_wrapper.h"
#include "script/script.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/theme.hpp>

OrchestratorPlugin* OrchestratorPlugin::_plugin = nullptr;

void OrchestratorPlugin::_focus_another_editor()
{
    GUARD_NULL(_window_wrapper);

    if (_window_wrapper->get_window_enabled())
    {
        ERR_FAIL_COND(_last_editor.is_empty());

        EI->get_base_control()->get_viewport()->gui_release_focus();
        EI->set_main_screen_editor(_last_editor);
    }
}

bool OrchestratorPlugin::_is_exiting() const
{
    // todo: would be nice to have a better way to know the editor is exiting
    //  see https://github.com/godotengine/godot/pull/107861
    return EI->get_base_control()->get_modulate() != Color(1, 1, 1, 1);
}

void OrchestratorPlugin::_register_inspector_plugins()
{
    _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginFunction));
    _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginSignal));
    _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginVariable));
    _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginTypeCast));

    for (const Ref<EditorInspectorPlugin>& plugin : _inspector_plugins)
        add_inspector_plugin(plugin);
}

void OrchestratorPlugin::_register_export_plugins()
{
    _export_plugins.push_back(memnew(OrchestratorEditorExportPlugin));

    for (const Ref<EditorExportPlugin>& plugin : _export_plugins)
        add_export_plugin(plugin);
}

void OrchestratorPlugin::_register_debugger_plugins()
{
    #if GODOT_VERSION >= 0x040300
    _debugger_plugin.instantiate();

    add_debugger_plugin(_debugger_plugin);
    #endif
}

void OrchestratorPlugin::_add_plugin_icon_to_editor_theme()
{
    // Register the plugin's icon for CreateScript Dialog
    Ref<Theme> theme = get_editor_interface()->get_editor_theme();
    if (theme.is_valid() && !theme->has_icon(_get_plugin_name(), "EditorIcons"))
    {
        theme->set_icon(_get_plugin_name(), "EditorIcons", _get_plugin_icon());
        theme->set_icon(OScriptLanguage::get_singleton()->_get_type(), "EditorIcons", _get_plugin_icon());
    }
}

void OrchestratorPlugin::_window_visibility_changed(bool p_visible)
{
    if (p_visible)
        _focus_another_editor();
    else
        EI->set_main_screen_editor(_get_plugin_name());
}

void OrchestratorPlugin::_main_screen_changed(const String& p_name)
{
    if (p_name != _get_plugin_name())
        _last_editor = p_name;
}

String OrchestratorPlugin::get_plugin_version() const
{
    return VERSION_NUMBER;
}

void OrchestratorPlugin::_edit(Object* p_object)
{
    GUARD_NULL(_editor_panel);
    GUARD_NULL(_window_wrapper);

    if (!p_object || !_handles(p_object))
        return;

    Ref<Resource> resource = cast_to<Resource>(p_object);
    if (!resource.is_valid())
        return;

    if (resource->get_path().is_empty())
        return;

    _editor_panel->edit(resource);
    _window_wrapper->move_to_foreground();
}

bool OrchestratorPlugin::_handles(Object* p_object) const
{
    return p_object != nullptr && p_object->get_class() == "OScript";
}

bool OrchestratorPlugin::_has_main_screen() const
{
    return true;
}

void OrchestratorPlugin::_make_visible(bool p_visible)
{
    GUARD_NULL(_window_wrapper);

    if (p_visible)
    {
        if (_window_wrapper->get_window_enabled())
        {
            // EditorPlugin::selected_notify is not exposed to GDExtension, but this method
            // is called just before "selected_notify" as a way to address this until the
            // method can be exposed.
            _focus_another_editor();
        }
        _window_wrapper->show();
    }
    else
        _window_wrapper->hide();
}

String OrchestratorPlugin::_get_plugin_name() const
{
    return VERSION_NAME;
}

Ref<Texture2D> OrchestratorPlugin::_get_plugin_icon() const
{
    return ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_16x16.png");
}

void OrchestratorPlugin::_save_external_data()
{
    GUARD_NULL(_editor_panel);

    if (!_is_exiting())
        _editor_panel->save_all_scripts();

    // When the editor saves the scene, it will propagate a call to save all external
    // resources used by the scene. If one of those resources is a script that is
    // open in the editor, we need to update the script times. If we don't, this will
    // trigger a notification that the file was modified outside the editor.
    _editor_panel->update_script_times();
}

String OrchestratorPlugin::_get_unsaved_status(const String& p_for_scene) const
{
    if (!_editor_panel)
        return {};

    const PackedStringArray unsaved_scripts = _editor_panel->get_unsaved_scripts();
    if (unsaved_scripts.is_empty())
        return {};

    PackedStringArray message;
    if (!p_for_scene.is_empty())
    {
        PackedStringArray unsaved_built_in_scripts;
        const String scene_file = p_for_scene.get_file();
        for (const String &E : unsaved_scripts)
        {
            if (!ResourceUtils::is_file(E) && E.contains(scene_file))
                unsaved_built_in_scripts.append(E);
        }

        if (unsaved_built_in_scripts.is_empty())
            return {};

        message.resize(unsaved_built_in_scripts.size() + 1);
        message.append("There are unsaved changes in the following built-in script(s):");

        for (const String &E : unsaved_built_in_scripts)
            message.append(E.trim_suffix("(*)"));

        return String("\n").join(message);
    }

    message.push_back("Save changes to the following Orchestrator file(s) before quitting?");
    for (const String& E : unsaved_scripts)
        message.push_back(E.trim_suffix("(*)"));

    return String("\n").join(message);
}

void OrchestratorPlugin::_apply_changes()
{
    if (_editor_panel)
        _editor_panel->apply_scripts();
}

void OrchestratorPlugin::_set_window_layout(const Ref<ConfigFile>& p_configuration)
{
    if (_editor_panel)
        _editor_panel->set_window_layout(p_configuration);

    if (restore_windows_on_load())
    {
        if (_window_wrapper->is_window_available() && p_configuration->has_section_key("Orchestrator", "window_rect"))
        {
            _window_wrapper->restore_window_from_saved_position(
                p_configuration->get_value("Orchestrator", "window_rect", Rect2i()),
                p_configuration->get_value("Orchestrator", "window_screen", -1),
                p_configuration->get_value("Orchestrator", "window_screen_rect", Rect2i()));
        }
        else
            _window_wrapper->set_window_enabled(false);
    }
}

void OrchestratorPlugin::_get_window_layout(const Ref<ConfigFile>& p_configuration)
{
    if (_editor_panel)
        _editor_panel->get_window_layout(p_configuration);

    if (_window_wrapper->get_window_enabled())
    {
        p_configuration->set_value("Orchestrator", "window_rect", _window_wrapper->get_window_rect());
        int screen = _window_wrapper->get_window_screen();
        p_configuration->set_value("Orchestrator", "window_screen", screen);
        p_configuration->set_value("Orchestrator", "window_screen_rect",
                                   DisplayServer::get_singleton()->screen_get_usable_rect(screen));
    }
    else
    {
        if (p_configuration->has_section_key("Orchestrator", "window_rect"))
            p_configuration->erase_section_key("Orchestrator", "window_rect");
        if (p_configuration->has_section_key("Orchestrator", "window_screen"))
            p_configuration->erase_section_key("Orchestrator", "window_screen");
        if (p_configuration->has_section_key("Orchestrator", "window_screen_rect"))
            p_configuration->erase_section_key("Orchestrator", "window_screen_rect");
    }
}

bool OrchestratorPlugin::_build()
{
    return true;
}

void OrchestratorPlugin::_enable_plugin()
{
}

void OrchestratorPlugin::_disable_plugin()
{
}

PackedStringArray OrchestratorPlugin::_get_breakpoints() const
{
    // When the game is started with the debugger, it uses this method to gather all breakpoints,
    // and these are passed to the CLI of the game process. this should obtain all breakpoints
    // currently set and return them using the format of "<script_file>:<node_id>".
    return _editor_panel ? _editor_panel->get_breakpoints() : PackedStringArray();
}

String OrchestratorPlugin::get_github_issues_url()
{
    return "https://github.com/CraterCrash/godot-orchestrator/issues/new/choose";
}

String OrchestratorPlugin::get_patreon_url()
{
    return "https://donate.cratercrash.space/";
}

String OrchestratorPlugin::get_community_url()
{
    return "https://discord.cratercrash.space/";
}

String OrchestratorPlugin::get_plugin_online_documentation_url()
{
    return VERSION_DOCS_URL;
}

bool OrchestratorPlugin::restore_windows_on_load()
{
    return EDITOR_GET("interface/multi_window/restore_windows_on_load");
}

void OrchestratorPlugin::request_editor_restart()
{
    AcceptDialog* request = memnew(AcceptDialog);
    request->set_title("Restart editor");
    _editor_panel->add_child(request);

    VBoxContainer* container = memnew(VBoxContainer);
    Label* label = memnew(Label);
    label->set_text("The editor requires a restart.");
    label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    container->add_child(label);

    request->add_child(container);

    request->connect("confirmed", callable_mp(EI, &EditorInterface::restart_editor).bind(true));
    request->popup_centered();
}

Ref<Texture2D> OrchestratorPlugin::get_plugin_icon_hires() const
{
    return ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_Logo.svg");
}

Ref<ConfigFile> OrchestratorPlugin::get_metadata()
{
    const String file = EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator_metadata.cfg");

    Ref<ConfigFile> metadata(memnew(ConfigFile));
    metadata->load(file);

    return metadata;
}

void OrchestratorPlugin::save_metadata(const Ref<ConfigFile>& p_metadata)
{
    const String file = EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator_metadata.cfg");

    p_metadata->save(file);
}

void OrchestratorPlugin::make_active()
{
    if (_has_main_screen())
        EI->set_main_screen_editor(_get_plugin_name());
}

void OrchestratorPlugin::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        // Plugins only enter the tree once and this happens before the main view.
        // It's safe then to cache the plugin reference here.
        _plugin = this;

        _register_inspector_plugins();
        _register_export_plugins();
        _register_debugger_plugins();

        _add_plugin_icon_to_editor_theme();

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
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        disconnect("main_screen_changed", callable_mp_this(_main_screen_changed));

        SAFE_MEMDELETE(_editor_panel);
        _plugin = nullptr;
    }
}

void OrchestratorPlugin::_bind_methods()
{
}

OrchestratorPlugin::OrchestratorPlugin()
{
    #if TOOLS_ENABLED
    OrchestratorScriptGraphEditorView::register_editor();
    #endif
}