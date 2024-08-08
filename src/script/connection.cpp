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
#include "script/connection.h"

OScriptConnection OScriptConnection::from_dict(const Dictionary& p_dict)
{
    OScriptConnection conn;
    conn.from_node = StringName(p_dict["from_node"]).to_int();
    conn.from_port = p_dict["from_port"];
    conn.to_node = StringName(p_dict["to_node"]).to_int();
    conn.to_port = p_dict["to_port"];
    return conn;
}

Dictionary OScriptConnection::to_dict() const
{
    Dictionary d;
    d["from_node"] = StringName(vformat("%d", from_node));
    d["from_port"] = from_port;
    d["to_node"] = StringName(vformat("%d", to_node));
    d["to_port"] = to_port;
    return d;
}