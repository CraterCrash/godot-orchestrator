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
#ifndef ORCHESTRATOR_SCRIPT_NODES_H
#define ORCHESTRATOR_SCRIPT_NODES_H

#include "editable_pin_node.h"

// Constants
#include "constants/constants.h"

// Data
#include "data/arrays.h"
#include "data/coercion_node.h"
#include "data/compose.h"
#include "data/dictionary.h"
#include "data/decompose.h"
#include "data/type_cast.h"

// Dialogue
#include "dialogue/dialogue_choice.h"
#include "dialogue/dialogue_message.h"

// Flow Control
#include "flow_control/branch.h"
#include "flow_control/chance.h"
#include "flow_control/delay.h"
#include "flow_control/for.h"
#include "flow_control/for_each.h"
#include "flow_control/random.h"
#include "flow_control/select.h"
#include "flow_control/sequence.h"
#include "flow_control/switch.h"
#include "flow_control/while.h"

// Functions
#include "functions/call_builtin_function.h"
#include "functions/call_member_function.h"
#include "functions/call_script_function.h"
#include "functions/function_entry.h"
#include "functions/function_result.h"
#include "functions/event.h"

// Input
#include "input/input_action.h"

// Math
#include "math/operator_node.h"

// Properties
#include "properties/property_get.h"
#include "properties/property_set.h"

// Resource
#include "resources/preload.h"
#include "resources/resource_path.h"

// Scene
#include "scene/instantiate_scene.h"
#include "scene/scene_node.h"
#include "scene/scene_tree.h"

// Signals
#include "signals/await_signal.h"
#include "signals/emit_member_signal.h"
#include "signals/emit_signal.h"

// Utility
#include "utilities/autoload.h"
#include "utilities/comment.h"
#include "utilities/engine_singleton.h"
#include "utilities/self.h"
#include "utilities/print_string.h"

// Variables
#include "variables/local_variable.h"
#include "variables/variable_get.h"
#include "variables/variable_set.h"

void register_script_node_classes();

#endif  // ORCHESTRATOR_SCRIPT_NODES_H
