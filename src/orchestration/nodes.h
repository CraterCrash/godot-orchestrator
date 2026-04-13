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
#pragma once

#include "orchestration/nodes/editable_pin_node.h"

// Constants
#include "orchestration/nodes/constants.h"

// Data
#include "orchestration/nodes/arrays.h"
#include "orchestration/nodes/coercion_node.h"
#include "orchestration/nodes/compose.h"
#include "orchestration/nodes/dictionary.h"
#include "orchestration/nodes/decompose.h"
#include "orchestration/nodes/type_cast.h"

// Dialogue
#include "orchestration/nodes/dialogue.h"

// Flow Control
#include "orchestration/nodes/branch.h"
#include "orchestration/nodes/chance.h"
#include "orchestration/nodes/delay.h"
#include "orchestration/nodes/for_loop.h"
#include "orchestration/nodes/random.h"
#include "orchestration/nodes/select.h"
#include "orchestration/nodes/sequence.h"
#include "orchestration/nodes/switch.h"
#include "orchestration/nodes/while.h"

// Functions
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/function_entry.h"
#include "orchestration/nodes/function_result.h"
#include "orchestration/nodes/event.h"

// Input
#include "orchestration/nodes/input_action.h"

// Math
#include "orchestration/nodes/operator_node.h"

// Memory
#include "orchestration/nodes/memory.h"

// Properties
#include "orchestration/nodes/properties.h"

// Resource
#include "orchestration/nodes/preload.h"
#include "orchestration/nodes/resource_path.h"

// Scene
#include "orchestration/nodes/instantiate_scene.h"
#include "orchestration/nodes/scene_node.h"
#include "orchestration/nodes/scene_tree.h"

// Signals
#include "orchestration/nodes/emit_member_signal.h"
#include "orchestration/nodes/emit_signal.h"

// Utility
#include "orchestration/nodes/autoload.h"
#include "orchestration/nodes/await.h"
#include "orchestration/nodes/comment.h"
#include "orchestration/nodes/engine_singleton.h"
#include "orchestration/nodes/self.h"
#include "orchestration/nodes/print_string.h"

// Variables
#include "orchestration/nodes/local_variable.h"
#include "orchestration/nodes/variables.h"
