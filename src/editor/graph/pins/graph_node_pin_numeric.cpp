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

#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

OrchestratorGraphNodePinNumeric::OrchestratorGraphNodePinNumeric(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinNumeric::_bind_methods()
{
}

bool OrchestratorGraphNodePinNumeric::_set_default_value(const String& p_value)
{
    if (String(_pin->get_effective_default_value()) == p_value)
        return false;

    switch (_pin->get_type())
    {
        case Variant::INT:
        {
            EditorUndoRedoManager* undo = OrchestratorPlugin::get_singleton()->get_undo_redo();
            undo->create_action("Orchestration: Change integer pin");
            undo->add_do_method(_pin.ptr(), "set_default_value", p_value.to_int());
            undo->add_do_method(_line_edit, "set_text", p_value);
            undo->add_undo_method(_pin.ptr(), "set_default_value", _pin->get_effective_default_value());
            undo->add_undo_method(_line_edit, "set_text", String(_pin->get_effective_default_value()));
            undo->commit_action();
            return true;
        }
        case Variant::FLOAT:
        {
            EditorUndoRedoManager* undo = OrchestratorPlugin::get_singleton()->get_undo_redo();
            undo->create_action("Orchestration: Change float pin");
            undo->add_do_method(_pin.ptr(), "set_default_value", p_value.to_float());
            undo->add_do_method(_line_edit, "set_text", p_value);
            undo->add_undo_method(_pin.ptr(), "set_default_value", _pin->get_effective_default_value());
            undo->add_undo_method(_line_edit, "set_text", String(_pin->get_effective_default_value()));
            undo->commit_action();
            return true;
        }
        default:
            ERR_PRINT("Cannot set default value for an unknown numeric pin type");
            return false;
    }
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
    _line_edit->set_h_size_flags(Control::SIZE_EXPAND);
    _line_edit->set_text(_pin->get_effective_default_value());
    _line_edit->add_theme_constant_override("minimum_character_width", 0);
    _line_edit->set_select_all_on_focus(true);
    _line_edit->connect("text_submitted", callable_mp(this, &OrchestratorGraphNodePinNumeric::_on_text_submitted));
    _line_edit->connect("focus_exited", callable_mp(this, &OrchestratorGraphNodePinNumeric::_on_focus_lost));
    return _line_edit;
}
