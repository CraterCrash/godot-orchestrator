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
#ifndef ORCHESTRATOR_ORCHESTRATION_FORMAT_TEXT_H
#define ORCHESTRATOR_ORCHESTRATION_FORMAT_TEXT_H

#include <godot_cpp/classes/resource_loader.hpp>

using namespace godot;

struct OrchestrationTextFormat {
    // Helper methods used by Parser and Serializer
    static int64_t get_resource_id_for_path(const String& p_path, bool p_generate);
    static String create_start_tag(const String& p_class, const String& p_script_class, const String& p_icon_path, uint32_t p_steps, uint32_t p_version, int64_t p_uid);
    static String create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline = true);
};

#endif // ORCHESTRATOR_ORCHESTRATION_FORMAT_TEXT_H
