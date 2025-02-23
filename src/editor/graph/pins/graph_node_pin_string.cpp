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
#include "graph_node_pin_string.h"

#include "common/callable_lambda.h"
#include "editor/graph/graph_node.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

Control* OrchestratorGraphNodePinText::_get_default_value_widget()
{
    _editor = memnew(TextEdit);
    _editor->set_placeholder("No value...");
    _editor->set_h_size_flags(SIZE_EXPAND);
    _editor->set_v_size_flags(SIZE_EXPAND);
    _editor->set_h_grow_direction(GROW_DIRECTION_END);
    _editor->set_custom_minimum_size(Vector2(350, 0));
    _editor->set_text(_pin->get_effective_default_value());
    _editor->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    _editor->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    _editor->set_fit_content_height_enabled(true);
    _editor->connect("text_changed", callable_mp(this, &OrchestratorGraphNodePinText::_text_changed));

    return _editor;
}

void OrchestratorGraphNodePinText::_text_changed()
{
    if (_editor)
        _pin->set_default_value(_editor->get_text());
}

OrchestratorGraphNodePinText::OrchestratorGraphNodePinText(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodePinString::_text_submitted(const String& p_value)
{
    if (!_editor)
        return;

    _pin->set_default_value(_editor->get_text());
    _editor->release_focus();

    if (_popup && _popup->is_inside_tree() && _popup->is_visible())
        _popup->hide();
}

void OrchestratorGraphNodePinString::_focus_entered()
{
    if (!_editor)
        return;

    _suggestions = get_graph_node()->get_script_node()->get_suggestions(_pin);
    if (!_suggestions.is_empty())
    {
        _popup = memnew(PopupMenu);
        _popup->set_flag(PopupMenu::FLAG_NO_FOCUS, true);
        _popup->set_allow_search(false);
        _popup->set_position(_editor->get_screen_position() + Vector2(0, _editor->get_size().height));
        _popup->connect("window_input", callable_mp(this, &OrchestratorGraphNodePinString::_window_input));
        _popup->connect("index_pressed", callable_mp(this, &OrchestratorGraphNodePinString::_suggestion_picked));
        _popup->connect("popup_hide", callable_mp(this, &OrchestratorGraphNodePinString::_popup_hide));
        _popup->connect("tree_exiting", callable_mp_lambda(this, [=]{ _popup = nullptr; }));

        _popup->clear();
        for (const String suggestion : _suggestions)
            _popup->add_item(suggestion);

        _editor->add_child(_popup);
        _popup->popup();
    }

    _editor->grab_focus();
    _editor->select_all();
}

void OrchestratorGraphNodePinString::_focus_exited()
{
    if (!_editor)
        return;

    _pin->set_default_value(_editor->get_text());
    _editor->deselect();
}

void OrchestratorGraphNodePinString::_popup_hide()
{
    if (_editor)
        _editor->release_focus();

    if (_popup)
        _popup->queue_free();
}

void OrchestratorGraphNodePinString::_window_input(const Ref<InputEvent>& p_event)
{
    if (!_editor)
        return;

    const Ref<InputEventKey> k = p_event;
    if (k.is_valid() && k->is_pressed() && k->get_keycode() != KEY_ENTER)
        _editor->get_viewport()->push_input(p_event, false);
}

void OrchestratorGraphNodePinString::_suggestion_picked(int p_index)
{
    if (!_popup || !_editor)
        return;

    _editor->set_text(_popup->get_item_text(p_index));
    _editor->emit_signal("text_submitted", _editor->get_text());
}

Control* OrchestratorGraphNodePinString::_get_default_value_widget()
{
    _editor = memnew(LineEdit);
    _editor->set_custom_minimum_size(Vector2(30, 0));
    _editor->set_expand_to_text_length_enabled(true);
    _editor->set_h_size_flags(SIZE_EXPAND);
    _editor->set_text(_pin->get_effective_default_value());
    _editor->set_select_all_on_focus(true);
    _editor->connect("text_submitted", callable_mp(this, &OrchestratorGraphNodePinString::_text_submitted));
    _editor->connect("focus_entered", callable_mp(this, &OrchestratorGraphNodePinString::_focus_entered));
    _editor->connect("focus_exited", callable_mp(this, &OrchestratorGraphNodePinString::_focus_exited));

    return _editor;
}

OrchestratorGraphNodePinString::OrchestratorGraphNodePinString(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}