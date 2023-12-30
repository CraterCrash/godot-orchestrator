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
#ifndef ORCHESTRATOR_SCRIPT_ACTION_H
#define ORCHESTRATOR_SCRIPT_ACTION_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

/// An action that can be added to a popup-menu with a delegate callable to invoke
/// when the menu option is selected.
class OScriptAction : public RefCounted
{
    GDCLASS(OScriptAction, RefCounted);
    static void _bind_methods() { }

    String _text;
    String _icon;
    Callable _callback;
    bool _disabled{ false };

    OScriptAction() = default;

public:
    OScriptAction(const String& p_text, const String& p_icon, Callable p_callable, bool p_disabled = false)
        : _text(p_text)
        , _icon(p_icon)
        , _callback(p_callable)
        , _disabled(p_disabled)
    {
    }

    /// Get the text for the item
    /// @return the item text
    String get_text() const { return _text; }

    /// Get the icon for the item
    /// @return the icon
    String get_icon() const { return _icon; }

    /// Get the callback handler or Callable
    /// @return the callback callable object
    Callable get_handler() const { return _callback; }
};

#endif  // ORCHESTRATOR_SCRIPT_ACTION_H