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

#include "common/callable_lambda.h"
#include "common/version.h"
#include "editor/editor_panel.h"
#include "editor/graph/graph_edit.h"
#include "editor/plugins/inspector_plugins.h"
#include "editor/plugins/orchestration_editor_export_plugin.h"
#include "editor/window_wrapper.h"
#include "script/script.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

OrchestratorPlugin* OrchestratorPlugin::_plugin = nullptr;

OrchestratorPlugin::OrchestratorPlugin()
{
}

void OrchestratorPlugin::_bind_methods()
{
}

void OrchestratorPlugin::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        OrchestratorGraphEdit::initialize_clipboard();

        // Plugins only enter the tree once and this happens before the main view.
        // It's safe then to cache the plugin reference here.
        _plugin = this;

        _build_panel = memnew(OrchestratorBuildOutputPanel);
        Button* button = add_control_to_bottom_panel(_build_panel, "Orchestration Build");
        _build_panel->set_tool_button(button);

        _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginFunction));
        _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginSignal));
        _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginVariable));
        _inspector_plugins.push_back(memnew(OrchestratorEditorInspectorPluginTypeCast));
        for (const Ref<EditorInspectorPlugin>& plugin : _inspector_plugins)
            add_inspector_plugin(plugin);

        _export_plugins.push_back(memnew(OrchestratorEditorExportPlugin));
        for (const Ref<EditorExportPlugin>& plugin : _export_plugins)
            add_export_plugin(plugin);

        #if GODOT_VERSION >= 0x040300
        _debugger_plugin.instantiate();
        add_debugger_plugin(_debugger_plugin);
        #endif

        // Register the plugin's icon for CreateScript Dialog
        Ref<Theme> theme = get_editor_interface()->get_editor_theme();
        if (theme.is_valid() && !theme->has_icon(_get_plugin_name(), "EditorIcons"))
            theme->set_icon(_get_plugin_name(), "EditorIcons", _get_plugin_icon());

        _editor_cache.instantiate();
        _editor_cache->load();

        _window_wrapper = memnew(OrchestratorWindowWrapper);
        _window_wrapper->set_window_title(vformat("Orchestrator - Godot Engine"));
        _window_wrapper->set_margins_enabled(true);

        _editor_panel = memnew(OrchestratorEditorPanel(_window_wrapper));

        get_editor_interface()->get_editor_main_screen()->add_child(_window_wrapper);
        _window_wrapper->set_wrapped_control(_editor_panel);
        _window_wrapper->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        _window_wrapper->hide();
        _window_wrapper->connect("window_visibility_changed", callable_mp(this, &OrchestratorPlugin::_on_window_visibility_changed));

        _theme_cache.instantiate();

        _make_visible(false);

        connect("main_screen_changed", callable_mp(this, &OrchestratorPlugin::_on_main_screen_changed));
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        disconnect("main_screen_changed", callable_mp(this, &OrchestratorPlugin::_on_main_screen_changed));

        OrchestratorGraphEdit::free_clipboard();

        remove_control_from_bottom_panel(_build_panel);
        memdelete(_build_panel);
        _build_panel = nullptr;

        memdelete(_editor_panel);
        _editor_panel = nullptr;

        _plugin = nullptr;
    }
}

void OrchestratorPlugin::_edit(Object* p_object)
{
    if (p_object && _handles(p_object))
    {
        Ref<Resource> resource = Object::cast_to<Resource>(p_object);
        if (resource.is_valid())
        {
            _editor_panel->edit_resource(resource);
            _window_wrapper->move_to_foreground();
        }
    }
}

bool OrchestratorPlugin::_handles(Object* p_object) const
{
    return p_object->get_class() == "OScript";
}

bool OrchestratorPlugin::_has_main_screen() const
{
    return true;
}

void OrchestratorPlugin::_make_visible(bool p_visible)
{
    if (p_visible)
    {
        _window_wrapper->show();

        // EditorPlugin::selected_notify is not exposed to GDExtension, but this method
        // is called just before "selected_notify" as a way to address this until the
        // method can be exposed.
        _focus_another_editor();
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
    return ResourceLoader::get_singleton()->load(OScriptLanguage::ICON);
}

String OrchestratorPlugin::get_plugin_online_documentation_url() const
{
    return VERSION_DOCS_URL;
}

String OrchestratorPlugin::get_github_release_url() const
{
    return VERSION_RELEASES_URL;
}

String OrchestratorPlugin::get_github_release_tag_url(const String& p_tag)
{
    const int index = p_tag.rfind(".");
    const String tag = p_tag.left(index) + "-" + p_tag.substr(index + 1);

    return vformat(
        "https://github.com/CraterCrash/godot-orchestrator/releases/download/%s/godot-orchestrator-%s-plugin.zip",
        p_tag,
        tag);
}

String OrchestratorPlugin::get_github_release_notes_url(const String& p_tag)
{
    return vformat("https://github.com/CraterCrash/godot-orchestrator/releases/%s", p_tag);
}

String OrchestratorPlugin::get_github_issues_url() const
{
    return "https://github.com/CraterCrash/godot-orchestrator/issues/new/choose";
}

String OrchestratorPlugin::get_patreon_url() const
{
    return "https://donate.cratercrash.space/";
}

String OrchestratorPlugin::get_community_url() const
{
    return "https://discord.cratercrash.space/";
}

bool OrchestratorPlugin::restore_windows_on_load()
{
    Ref<EditorSettings> es = get_editor_interface()->get_editor_settings();
    if (es.is_valid())
        return es->get_setting("interface/multi_window/restore_windows_on_load");
    return false;
}

Ref<Texture2D> OrchestratorPlugin::get_plugin_icon_hires() const
{
    return ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_Logo.svg");
}

Ref<ConfigFile> OrchestratorPlugin::get_metadata()
{
    const String file = get_editor_interface()->get_editor_paths()->get_project_settings_dir()
        .path_join("orchestrator_metadata.cfg");

    Ref<ConfigFile> metadata(memnew(ConfigFile));
    metadata->load(file);

    return metadata;
}

void OrchestratorPlugin::save_metadata(const Ref<ConfigFile>& p_metadata)
{
    const String file = get_editor_interface()->get_editor_paths()->get_project_settings_dir()
            .path_join("orchestrator_metadata.cfg");

    p_metadata->save(file);
}

void OrchestratorPlugin::make_active()
{
    get_editor_interface()->set_main_screen_editor(_get_plugin_name());
}

void OrchestratorPlugin::make_build_panel_active()
{
    make_bottom_panel_item_visible(_build_panel);
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

    request->connect("confirmed", callable_mp_lambda(this, []{ EditorInterface::get_singleton()->restart_editor(true); }));
    request->popup_centered();
}

String OrchestratorPlugin::get_plugin_version() const
{
    return VERSION_NUMBER;
}

void OrchestratorPlugin::_apply_changes()
{
    if (_editor_panel)
        _editor_panel->apply_changes();

    if (_editor_cache.is_valid())
        _editor_cache->save();
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
    if (_editor_panel)
    {
        _build_panel->reset();
        return _editor_panel->build();
    }
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
    #if GODOT_VERSION >= 0x040300
    // When the game is started with the debugger, it uses this method to gather all breakpoints,
    // and these are passed to the CLI of the game process. this should obtain all breakpoints
    // currently set and return them using the format of "<script_file>:<node_id>".
    return _editor_panel->get_breakpoints();
    #else
    return PackedStringArray();
    #endif
}

void OrchestratorPlugin::_focus_another_editor()
{
    if (_window_wrapper->get_window_enabled())
    {
        ERR_FAIL_COND(_last_editor.is_empty());
        EditorInterface::get_singleton()->set_main_screen_editor(_last_editor);
    }
}

void OrchestratorPlugin::_on_window_visibility_changed(bool p_visible)
{
    _focus_another_editor();
}

void OrchestratorPlugin::_on_main_screen_changed(const String& p_name)
{
    if (p_name != _get_plugin_name())
        _last_editor = p_name;
}
