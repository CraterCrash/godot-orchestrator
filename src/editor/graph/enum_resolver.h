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
#ifndef ORCHESTRATOR_EDITOR_ENUM_RESOLVER_H
#define ORCHESTRATOR_EDITOR_ENUM_RESOLVER_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class OrchestratorEditorEnumResolver
{
public:
    struct EnumItem
    {
        String friendly_name;
        String real_name;
        int64_t value;
    };

    static List<EnumItem> resolve_enum_items(const String& p_target_class);
};

#endif // ORCHESTRATOR_EDITOR_ENUM_RESOLVER_H
