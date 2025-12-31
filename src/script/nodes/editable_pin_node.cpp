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
#include "script/nodes/editable_pin_node.h"

void OScriptEditablePinNode::_adjust_connections(int p_start_offset, int p_adjustment, EPinDirection p_direction) {
    get_orchestration()->adjust_connections(this, p_start_offset, p_adjustment, p_direction);
}

String OScriptEditablePinNode::_get_pin_name_given_index(int p_index) const {
    const String prefix = get_pin_prefix();
    return vformat("%s_%d", prefix, p_index);
}

