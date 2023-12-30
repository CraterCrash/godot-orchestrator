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
#ifndef ORCHESTRATOR_SCRIPT_SAVER_H
#define ORCHESTRATOR_SCRIPT_SAVER_H

#include "format.h"

#include <godot_cpp/classes/resource_format_saver.hpp>

using namespace godot;

/// Defines a resource format implementation for saving Orchestrator scripts.
class OScriptResourceSaver : public ResourceFormatSaver
{
    GDCLASS(OScriptResourceSaver, ResourceFormatSaver);
    static void _bind_methods() { }

public:

    //~ Begin ResourceFormatSaver Interface
    PackedStringArray _get_recognized_extensions(const Ref<Resource>& p_resource) const override;
    bool _recognize(const Ref<Resource>& p_resource) const override;
    Error _set_uid(const String& p_path, int64_t p_uid) override;
    bool _recognize_path(const Ref<Resource>& p_resource, const String& p_path) const override;
    Error _save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
    //~ End ResourceFormatSaver Interface

};

#endif  // ORCHESTRATOR_SCRIPT_SAVER_H