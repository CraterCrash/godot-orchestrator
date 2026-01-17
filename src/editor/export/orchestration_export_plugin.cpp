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
#include "editor/export/orchestration_export_plugin.h"

#include "common/macros.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/binary/binary_serializer.h"
#include "orchestration/serialization/text/text_parser.h"
#include "script/script_source.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

String OrchestratorEditorExportPlugin::_get_name() const {
    return "OScript";
}

bool OrchestratorEditorExportPlugin::_supports_platform(const Ref<EditorExportPlatform>& p_platform) const {
    return true;
}

void OrchestratorEditorExportPlugin::_export_begin(const PackedStringArray& p_features, bool p_is_debug, const String& p_path, uint32_t p_flags) {
    // todo: re-enable
    // _convert_to_binary = ProjectSettings::get_singleton()->get_setting("editor/export/convert_text_resources_to_binary", true);
}

void OrchestratorEditorExportPlugin::_export_file(const String& p_path, const String& p_type, const PackedStringArray& p_features) {
    if (!_convert_to_binary) {
        return;
    }

    if (p_path.get_extension().to_lower() != ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) {
        return;
    }

    OScriptSource source = OScriptSource::load(p_path);
    if (source.get_source().is_empty()) {
        return;
    }

    OrchestrationTextParser parser;
    Ref<Orchestration> orchestration = parser.load(p_path);
    if (!orchestration.is_valid()) {
        return;
    }

    const String uid_text = ResourceUID::path_to_uid(p_path);
    const int64_t uid = ResourceUID::get_singleton()->text_to_id(uid_text);

    const String export_base_path = EI->get_editor_paths()->get_project_settings_dir().path_join("../exported/orchestrator");
    const String serialized_path = vformat("%s/export-%s-%s.os", export_base_path, p_path.md5_text(), p_path.get_file().get_basename());

    OrchestrationBinarySerializer serializer;
    serializer.save(orchestration, serialized_path, 0);
    serializer.set_uid(serialized_path, uid);

    DirAccess::open("res://")->make_dir_recursive(export_base_path);

    const PackedByteArray bytes = FileAccess::get_file_as_bytes(serialized_path);
    add_file(p_path.get_basename() + ".os", bytes, true);
}

void OrchestratorEditorExportPlugin::_bind_methods() {
}