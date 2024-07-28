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
#include "graph_node_pin_numeric.h"

bool OrchestratorGraphNodePinNumeric::_set_default_value(const String& p_value)
{
    if (_pin->get_type() == Variant::INT)
    {
        // We allow float to coerce to int
        if (p_value.is_valid_int() || p_value.is_valid_float())
        {
            _pin->set_default_value(p_value.to_int());
            _line_edit->set_text(_pin->get_effective_default_value());
            return true;
        }
    }

    if (_pin->get_type() == Variant::FLOAT)
    {
        if (p_value.is_valid_float())
        {
            _pin->set_default_value(p_value.to_float());
            _line_edit->set_text(_pin->get_effective_default_value());
            return true;
        }
    }

    _line_edit->set_text(_pin->get_effective_default_value());
    _line_edit->call_deferred("grab_focus");
    _line_edit->call_deferred("select_all");
    return false;
}

void OrchestratorGraphNodePinNumeric::_on_text_submitted(const String& p_value)
{
    if (_set_default_value(p_value))
        _line_edit->release_focus();
}

void OrchestratorGraphNodePinNumeric::_on_focus_lost()
{
    _set_default_value(_line_edit->get_text());
}

Control* OrchestratorGraphNodePinNumeric::_get_default_value_widget()
{
    _line_edit = memnew(LineEdit);
    _line_edit->set_expand_to_text_length_enabled(true);
    _line_edit->set_h_size_flags(SIZE_EXPAND);
    _line_edit->set_text(_pin->get_effective_default_value());
    _line_edit->add_theme_constant_override("minimum_character_width", 0);
    _line_edit->set_select_all_on_focus(true);
    _line_edit->connect("text_submitted", callable_mp(this, &OrchestratorGraphNodePinNumeric::_on_text_submitted));
    _line_edit->connect("focus_exited", callable_mp(this, &OrchestratorGraphNodePinNumeric::_on_focus_lost));
    return _line_edit;
}

void OrchestratorGraphNodePinNumeric::_bind_methods()
{
}

OrchestratorGraphNodePinNumeric::OrchestratorGraphNodePinNumeric(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

