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
#include "editor/plugins/orchestration_editor_export_plugin.h"

#include "orchestrator_editor_plugin.h"
#include "script/serialization/binary_saver_instance.h"
#include "script/serialization/text_loader_instance.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

String OrchestratorEditorExportPlugin::_get_name() const
{
    return "OScript";
}

bool OrchestratorEditorExportPlugin::_supports_platform(const Ref<EditorExportPlatform>& p_platform) const
{
    return true;
}

void OrchestratorEditorExportPlugin::_export_begin(const PackedStringArray& p_features, bool p_is_debug, const String& p_path, uint32_t p_flags)
{
}

void OrchestratorEditorExportPlugin::_export_file(const String& p_path, const String& p_type, const PackedStringArray& p_features)
{
    // if (p_path.get_extension() != "torch")
    //     return;
    //
    // PackedByteArray bytes = FileAccess::get_file_as_bytes(p_path);
    // if (bytes.is_empty())
    //     return;
    //
    // Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    // if (!file.is_valid())
    // {
    //     ERR_PRINT("Failed to open the file: " + p_path);
    //     return;
    // }
    //
    // // Load the text-based orchestration from disk
    // OScriptTextResourceLoaderInstance loader;
    // loader._local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    // loader._cache_mode = ResourceFormatLoader::CacheMode::CACHE_MODE_IGNORE;
    // loader._res_path = loader._local_path;
    //
    // loader.open(file, false);
    // if (loader.load() != OK)
    // {
    //     ERR_PRINT("Failed to load the text-based script: " + p_path);
    //     return;
    // }
    //
    // Ref<DirAccess> user = DirAccess::open("user://");
    // if (!user.is_valid())
    // {
    //     ERR_PRINT("Failed to open user:// directory.");
    //     return;
    // }
    // user->make_dir_recursive("user://orchestrator");
    //
    //
    // OScriptBinaryResourceSaverInstance saver;
    // const String temp_path = "user://orchestrator/exported-orchestration.tmp";
    // if (saver.save(temp_path, loader._resource, FileAccess::WRITE) != OK)
    // {
    //     ERR_PRINT("Failed to cache binary instance of " + p_path);
    //     return;
    // }
    //
    // bytes = FileAccess::get_file_as_bytes(temp_path);
    //
    // const String save_path = vformat("res://.godot/exported/orchestrator/export-%s-%s.os", p_path.md5_text(), p_path.get_file().get_basename());
    // add_file(save_path, bytes, true);
}

void OrchestratorEditorExportPlugin::_bind_methods()
{
}