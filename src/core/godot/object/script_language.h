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
#ifndef ORCHESTRATOR_CORE_GODOT_OBJECT_SCRIPT_LANGUAGE_H
#define ORCHESTRATOR_CORE_GODOT_OBJECT_SCRIPT_LANGUAGE_H

#include <godot_cpp/classes/script.hpp>

using namespace godot;

namespace GDE {
    struct Script {
        Script() = delete;

        static MethodInfo get_method_info(const Ref<godot::Script>& p_script, const StringName& p_function);
        static bool inherits_script(const Ref<godot::Script>& p_script, const Ref<godot::Script>& p_parent_script);
        static bool is_valid(const Ref<godot::Script>& p_script);
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_OBJECT_SCRIPT_LANGUAGE_H