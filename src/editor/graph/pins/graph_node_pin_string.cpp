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
    _pin->set_default_value(p_value);
}

void OrchestratorGraphNodePinString::_on_text_changed(TextEdit* p_text_edit)
{
    if (p_text_edit)
        _set_default_value(p_text_edit->get_text());
}

void OrchestratorGraphNodePinString::_on_text_submitted(const String& p_value, LineEdit* p_line_edit)
{
    if (p_line_edit)
    {
        _set_default_value(p_line_edit->get_text());
        p_line_edit->release_focus();
    }
}

void OrchestratorGraphNodePinString::_on_focus_lost(LineEdit* p_line_edit)
{
    if (p_line_edit)
        _set_default_value(p_line_edit->get_text());
}

Control* OrchestratorGraphNodePinString::_get_default_value_widget()
{
    if (_pin->is_multiline_text())
    {
        TextEdit* text_edit = memnew(TextEdit);
        text_edit->set_placeholder("No value...");
        text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        text_edit->set_v_size_flags(Control::SIZE_EXPAND);
        text_edit->set_h_grow_direction(Control::GROW_DIRECTION_END);
        text_edit->set_custom_minimum_size(Vector2(350, 0));
        text_edit->set_text(_pin->get_effective_default_value());
        text_edit->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        text_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
        text_edit->set_fit_content_height_enabled(true);
        text_edit->connect("text_changed",
                           callable_mp(this, &OrchestratorGraphNodePinString::_on_text_changed).bind(text_edit));
        return text_edit;
    }

    LineEdit* line_edit = memnew(LineEdit);
    line_edit->set_custom_minimum_size(Vector2(30, 0));
    line_edit->set_expand_to_text_length_enabled(true);
    line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    line_edit->set_text(_pin->get_effective_default_value());
    line_edit->set_select_all_on_focus(true);
    line_edit->connect("text_submitted",
                       callable_mp(this, &OrchestratorGraphNodePinString::_on_text_submitted).bind(line_edit));
    line_edit->connect("focus_exited",
                       callable_mp(this, &OrchestratorGraphNodePinString::_on_focus_lost).bind(line_edit));
    return line_edit;
}

bool OrchestratorGraphNodePinString::_render_default_value_below_label() const
{
    return _pin->is_multiline_text();
}