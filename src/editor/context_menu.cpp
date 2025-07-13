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
#include "editor/context_menu.h"

#include "common/scene_utils.h"

void OrchestratorEditorContextMenu::_id_pressed(int p_id)
{
    if (_callables.has(p_id))
    {
        const Callable& callable = _callables[p_id];
        if (callable.is_valid())
            callable.call();
    }
}

void OrchestratorEditorContextMenu::_cleanup_menu()
{
    clear(true);

    // Cleans up any OrchestratorEditorContextMenu submenu nodes created
    for (int i = _submenus.size() - 1; i >= 0; i--)
        _submenus[i]->queue_free();

    // Cleanup root
    queue_free();
}

int OrchestratorEditorContextMenu::add_separator(const String& p_label)
{
    _menu->add_separator(p_label);
    return _menu->get_item_id(_menu->get_item_count() - 1);
}

int OrchestratorEditorContextMenu::add_item(const String& p_label, const Callable& p_callable, bool p_disabled, Key p_key)
{
    _menu->add_item(p_label, -1, p_key);

    int id = _menu->get_item_id(_menu->get_item_count() - 1);
    _callables[id] = p_callable;

    if (p_disabled)
        set_item_disabled(id, p_disabled);

    return id;
}

int OrchestratorEditorContextMenu::add_icon_item(const String& p_icon_name, const String& p_label, const Callable& p_callable, bool p_disabled, Key p_key)
{
    _menu->add_icon_item(SceneUtils::get_editor_icon(p_icon_name), p_label, -1, p_key);

    int id = _menu->get_item_id(_menu->get_item_count() - 1);
    _callables[id] = p_callable;

    if (p_disabled)
        set_item_disabled(id, p_disabled);

    return id;
}

void OrchestratorEditorContextMenu::set_item_disabled(int p_id, bool p_disabled)
{
    _menu->set_item_disabled(p_id, p_disabled);
}

void OrchestratorEditorContextMenu::set_item_tooltip(int p_id, const String& p_tooltip_text)
{
    _menu->set_item_tooltip(p_id, p_tooltip_text);
}

OrchestratorEditorContextMenu* OrchestratorEditorContextMenu::add_submenu(const String& p_label)
{
    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu(false));
    _submenus.push_back(menu);
    _menu->add_child(menu->_menu);

    _menu->add_submenu_item(p_label, menu->_menu->get_name());

    return menu;
}

void OrchestratorEditorContextMenu::clear(bool p_include_submenus)
{
    _menu->clear(p_include_submenus);
}

void OrchestratorEditorContextMenu::set_position(const Vector2& p_position)
{
    _menu->set_position(p_position);
}

void OrchestratorEditorContextMenu::popup()
{
    _menu->reset_size();
    _menu->popup();
}

void OrchestratorEditorContextMenu::set_auto_destroy(bool p_auto_destroy)
{
    OrchestratorEditorContextMenu* parent = cast_to<OrchestratorEditorContextMenu>(get_parent());
    ERR_FAIL_COND_MSG(parent, "Can only set auto destroy on parent context menu");

    if (p_auto_destroy && !_auto_destroy)
    {
        _auto_destroy = true;

        // When the user does not select a choice
        _menu->connect("close_requested", callable_mp(this, &OrchestratorEditorContextMenu::_cleanup_menu));
        // When the user makes a choice
        _menu->connect("popup_hide", callable_mp(this, &OrchestratorEditorContextMenu::_cleanup_menu));
    }
    else if (_auto_destroy)
    {
        // When the user does not select a choice
        _menu->disconnect("close_requested", callable_mp(this, &OrchestratorEditorContextMenu::_cleanup_menu));
        // When the user makes a choice
        _menu->disconnect("popup_hide", callable_mp(this, &OrchestratorEditorContextMenu::_cleanup_menu));
    }
}

void OrchestratorEditorContextMenu::_bind_methods()
{
}

OrchestratorEditorContextMenu::OrchestratorEditorContextMenu() : OrchestratorEditorContextMenu(true)
{
}

OrchestratorEditorContextMenu::OrchestratorEditorContextMenu(bool p_parent)
{
    _menu = memnew(PopupMenu);
    _menu->connect("id_pressed", callable_mp(this, &OrchestratorEditorContextMenu::_id_pressed));

    if (p_parent)
        add_child(_menu);
}
