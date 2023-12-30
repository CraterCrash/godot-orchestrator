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
#include "plugin.h"

#include "common/version.h"
#include "editor/main_view.h"
#include "script/script.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

OrchestratorPlugin* OrchestratorPlugin::_plugin = nullptr;

OrchestratorPlugin::OrchestratorPlugin()
    : _editor(*get_editor_interface())
{
}

void OrchestratorPlugin::_bind_methods()
{
}

void OrchestratorPlugin::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        // Plugins only enter the tree once and this happens before the main view.
        // It's safe then to cache the plugin reference here.
        _plugin = this;

        // Register the plugin's icon for CreateScript Dialog
        Ref<Theme> theme = ThemeDB::get_singleton()->get_default_theme();
        if (theme.is_valid() && !theme->has_icon(_get_plugin_name(), "EditorIcons"))
            theme->set_icon(_get_plugin_name(), "EditorIcons", _get_plugin_icon());

        _main_view = memnew(OrchestratorMainView(this));
        _editor.get_editor_main_screen()->add_child(_main_view);
        _make_visible(false);
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        memdelete(_main_view);
        _main_view = nullptr;

        _plugin = nullptr;
    }
}

void OrchestratorPlugin::_edit(Object* p_object)
{
    if (p_object && _handles(p_object))
    {
        Ref<OScript> script(Object::cast_to<OScript>(p_object));
        if (script.is_valid())
            _main_view->edit(script);
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
    if (_main_view)
        _main_view->set_visible(p_visible);
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

String OrchestratorPlugin::get_patreon_url() const
{
    return "https://patreon.com/vahera";
}

String OrchestratorPlugin::get_plugin_version() const
{
    return VERSION_NUMBER;
}

void OrchestratorPlugin::_apply_changes()
{
    if (_main_view)
        _main_view->apply_changes();
}

bool OrchestratorPlugin::_build()
{
    if (_main_view)
        return _main_view->build();

    return true;
}

void OrchestratorPlugin::_enable_plugin()
{
}

void OrchestratorPlugin::_disable_plugin()
{
}

void register_plugin_classes()
{
    ORCHESTRATOR_REGISTER_CLASS(OrchestratorPlugin)
}