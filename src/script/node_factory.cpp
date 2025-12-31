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
#include "script/node_factory.h"

#include "orchestration/orchestration.h"
#include "script/node.h"

HashMap<StringName, OScriptNodeFactory::ScriptNodeInfo> OScriptNodeFactory::_nodes;

void OScriptNodeFactory::_add_node_class(const StringName& p_class, const StringName& p_inherits) {
    ERR_FAIL_COND_MSG(_nodes.has(p_class), vformat("Class '%s' already exists.", p_class));

    _nodes[p_class] = ScriptNodeInfo();

    ScriptNodeInfo& info = _nodes[p_class];
    info.name = p_class;
    info.inherits = p_inherits;

    if (!info.inherits.is_empty()) {
        ERR_FAIL_COND_MSG(!_nodes.has(info.inherits), vformat("Class '%s' is not defined as a node", p_inherits));
        info.inherits_ptr = &_nodes[info.inherits];
    }
}

bool OScriptNodeFactory::_is_base_node_type(const StringName& p_class) {
    return OScriptNode::get_class_static().match(p_class);
}

Ref<OScriptNode> OScriptNodeFactory::create_node_from_name(const String& p_class_name, Orchestration* p_owner) {
    ERR_FAIL_COND_V_MSG(!_nodes.has(p_class_name), nullptr, "No node found with name: " + p_class_name);

    // No unique ID is assigned by default
    Ref<OScriptNode> node(_nodes[p_class_name].creation_func());
    node->_orchestration = p_owner;

    return node;
}

