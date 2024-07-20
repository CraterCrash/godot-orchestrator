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
#include "graph_node_pin_string.h"

#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/text_edit.hpp>

OrchestratorGraphNodePinString::OrchestratorGraphNodePinString(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinString::_bind_methods()
{
}

void OrchestratorGraphNodePinString::_set_default_value(const String& p_value)
{
    if (_pin->get_effective_default_value() != p_value)
    {
        if (_pin->is_multiline_text())
        {
            // TextEdit reacts weirdly with UndoRedo - for now skipped.
            _pin->set_default_value(p_value);
        }
        else
        {
            EditorUndoRedoManager* undo = OrchestratorPlugin::get_singleton()->get_undo_redo();
            undo->create_action("Orchestration: Change string pin");
            undo->add_do_method(_pin.ptr(), "set_default_value", p_value);
            undo->add_do_method(_line_edit, "set_text", p_value);
            undo->add_undo_method(_pin.ptr(), "set_default_value", _pin->get_effective_default_value());
            undo->add_undo_method(_line_edit, "set_text", _pin->get_effective_default_value());
            undo->commit_action();
        }
    }
}

void OrchestratorGraphNodePinString::_on_text_changed()
{
    if (_text_edit)
        _set_default_value(_text_edit->get_text());
}

void OrchestratorGraphNodePinString::_on_text_submitted(const String& p_value)
{
    if (_line_edit)
    {
        _set_default_value(p_value);
        _line_edit->release_focus();
    }
}

void OrchestratorGraphNodePinString::_on_focus_lost()
{
    if (_line_edit)
        _set_default_value(_line_edit->get_text());
}

Control* OrchestratorGraphNodePinString::_get_default_value_widget()
{
    if (_pin->is_multiline_text())
    {
        _text_edit = memnew(TextEdit);
        _text_edit->set_placeholder("No value...");
        _text_edit->set_h_size_flags(Control::SIZE_EXPAND);
        _text_edit->set_v_size_flags(Control::SIZE_EXPAND);
        _text_edit->set_h_grow_direction(Control::GROW_DIRECTION_END);
        _text_edit->set_custom_minimum_size(Vector2(350, 0));
        _text_edit->set_text(_pin->get_effective_default_value());
        _text_edit->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        _text_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
        _text_edit->set_fit_content_height_enabled(true);
        _text_edit->connect("text_changed", callable_mp(this, &OrchestratorGraphNodePinString::_on_text_changed));
        return _text_edit;
    }

    _line_edit = memnew(LineEdit);
    _line_edit->set_custom_minimum_size(Vector2(30, 0));
    _line_edit->set_expand_to_text_length_enabled(true);
    _line_edit->set_h_size_flags(Control::SIZE_EXPAND);
    _line_edit->set_text(_pin->get_effective_default_value());
    _line_edit->set_select_all_on_focus(true);
    _line_edit->connect("text_submitted", callable_mp(this, &OrchestratorGraphNodePinString::_on_text_submitted));
    _line_edit->connect("focus_exited", callable_mp(this, &OrchestratorGraphNodePinString::_on_focus_lost));
    return _line_edit;
}

bool OrchestratorGraphNodePinString::_render_default_value_below_label() const
{
    return _pin->is_multiline_text();
}