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
#include "script/instances/node_instance.h"

#include "script/node.h"

int OScriptNodeInstance::get_id()
{
    return id;
}

Ref<OScriptNode> OScriptNodeInstance::get_base_node()
{
    return { _base };
}

OScriptNodeInstance::~OScriptNodeInstance()
{
    if (input_pin_count > 0)
    {
        if (input_pins != nullptr)
            memdelete_arr(input_pins);
        if (input_default_stack_pos != nullptr)
            memdelete_arr(input_default_stack_pos);
    }

    if (output_pin_count > 0)
    {
        if (output_pins != nullptr)
            memdelete_arr(output_pins);
    }

    if (execution_output_pin_count > 0)
    {
        if (execution_output_pins != nullptr)
            memdelete_arr(execution_output_pins);
        if (execution_outputs != nullptr)
            memdelete_arr(execution_outputs);
    }
}
