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
#include "graph_node_pin_bool.h"

#include <godot_cpp/classes/check_box.hpp>

OrchestratorGraphNodePinBool::OrchestratorGraphNodePinBool(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinBool::_bind_methods()
{
}

void OrchestratorGraphNodePinBool::_on_default_value_changed(bool p_new_value)
{
    _pin->set_default_value(p_new_value);
}

Control* OrchestratorGraphNodePinBool::_get_default_value_widget()
{
    CheckBox* check_box = memnew(CheckBox);
    check_box->set_focus_mode(FOCUS_NONE);
    check_box->set_h_size_flags(SIZE_EXPAND_FILL);
    check_box->set_pressed(_pin->get_default_value());
    check_box->connect("toggled", callable_mp(this, &OrchestratorGraphNodePinBool::_on_default_value_changed));
    return check_box;
}
