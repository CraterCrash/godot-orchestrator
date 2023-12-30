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
#include "format_saver.h"

#include "internal/format_saver_instance.h"
#include "plugin/settings.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

PackedStringArray OScriptResourceSaver::_get_recognized_extensions(const Ref<Resource>& p_resource) const
{
    PackedStringArray extensions;
    if (p_resource->get_name().ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION))
        extensions.append(ORCHESTRATOR_SCRIPT_EXTENSION);

    return extensions;
}

bool OScriptResourceSaver::_recognize(const Ref<Resource>& p_resource) const
{
    // Allow saving any objects using OrchestratorScript format
    return true;
}

Error OScriptResourceSaver::_set_uid(const String& p_path, int64_t p_uid)
{
    return OK;
}

bool OScriptResourceSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION);
}

Error OScriptResourceSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (os->get_setting("save_copy_as_text_resource"))
    {
        String new_name = local_path.get_basename() + ".tres";
        ResourceSaver::get_singleton()->save(p_resource, new_name, p_flags);
    }

    OScriptResourceSaverInstance saver;
    return saver.save(local_path, p_resource, p_flags);
}
