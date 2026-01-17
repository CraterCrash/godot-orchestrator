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
#ifndef ORCHESTRATOR_CONTEXT_MENU_H
#define ORCHESTRATOR_CONTEXT_MENU_H

#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// A custom editor control that provides context menu behavior.
///
/// Normally in Godot, to provide context menu behavior, one would use a <code>PopupMenu</code> in an object,
/// add various items to it, and then route the <code>id_pressed</code> or <code>index_pressed</code> signals
/// to a function that would delegate based on a large switch statement.
///
/// While the Godot-way is functional, we've often created specialized function handlers for each switch use
/// case to avoid the main handler function being bloated. This class reduces the need for the switch,
/// and instead you provide the callback function directly on the selection. This means a chosen menu item
/// is directly dispatched.
///
/// In addition, because each menu item maintains a callable, custom state can be bound at menu creation to
/// be passed directly to the calling function, rather than relying on using <code>set_item_metadata</code>
/// or other gimmicks to provide contextual data.
///
/// Lastly, management of a <code>PopupMenu</code> requires that things like <code>popup_hide</code> or the
/// <code>close_requested</code> signals be handled for each use case. Instead, this implementation makes
/// that aspect simple. You either add the <code>OrchestratorEditorContextMenu</code> to the scene directly
/// as a static control that is destroyed when the parent is destroyed, or can be set to automatically
/// cleanup and destroy itself using <code>set_auto_destroy</code> after each closure of the menu, whether
/// the user picks something or not. This keeps the scene node tree clean and allows an instance of this
/// class to be allocated in-flight when the context menu is to be shown.
///
class OrchestratorEditorContextMenu : public Control {
    GDCLASS(OrchestratorEditorContextMenu, Control);

    PopupMenu* _menu;
    HashMap<int, Callable> _callables;
    bool _auto_destroy = false;
    Vector<OrchestratorEditorContextMenu*> _submenus;

    void _id_pressed(int p_id);
    void _cleanup_menu();

protected:
    static void _bind_methods();

    // Used internally only
    explicit OrchestratorEditorContextMenu(bool p_parent);

public:
    int add_separator(const String& p_label = String());
    int add_item(const String& p_label, const Callable& p_callable, bool p_disabled = false, Key p_key = KEY_NONE);
    int add_icon_item(const String& p_icon_name, const String& p_label, const Callable& p_callable, bool p_disabled = false, Key p_key = KEY_NONE);

    void set_item_disabled(int p_id, bool p_disabled);
    void set_item_tooltip(int p_id, const String& p_tooltip_text);

    OrchestratorEditorContextMenu* add_submenu(const String& p_label);

    void clear(bool p_include_submenus = false);
    void set_position(const Vector2& p_position);
    void popup();

    void set_auto_destroy(bool p_auto_destroy);

    OrchestratorEditorContextMenu();
};

#endif // ORCHESTRATOR_CONTEXT_MENU_H
