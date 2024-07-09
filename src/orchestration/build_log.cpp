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
#include "orchestration/build_log.h"

#include "script/node.h"

void BuildLog::_add_failure(FailureType p_type, const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message)
{
    Failure failure;
    failure.type = p_type;
    failure.node = p_node;
    failure.pin = p_pin;
    failure.message = p_message;
    _failures.push_back(failure);
}

void BuildLog::error(const OScriptNode* p_node, const String& p_message)
{
    error(p_node, nullptr, p_message);
}

void BuildLog::error(const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message)
{
    _add_failure(FT_Error, p_node, p_pin, p_message);
}

void BuildLog::warn(const OScriptNode* p_node, const String& p_message)
{
    error(p_node, nullptr, p_message);
}

void BuildLog::warn(const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message)
{
    _add_failure(FT_Warning, p_node, p_pin, p_message);
}