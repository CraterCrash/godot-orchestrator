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

void BuildLog::error(const String& p_message)
{
    _messages.push_back(vformat("[b][color=#a95853]ERROR[/color][/b]: Node %s ([url={\"goto_node\":\"%d\"}]#%d[/url]) - %s",
        _current_node->get_class(), _current_node->get_id(), _current_node->get_id(), p_message));
    _errors++;
}

void BuildLog::warn(const String& p_message)
{
    _messages.push_back(vformat("[b][color=yellow]WARNING[/color][/b]: Node %s ([url={\"goto_node\":\"%d\"}]#%d[/url] - %s",
        _current_node->get_class(), _current_node->get_id(), _current_node->get_id(), p_message));
    _warnings++;
}

