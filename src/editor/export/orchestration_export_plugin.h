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
#ifndef ORCHESTRATOR_EDITOR_EXPORT_PLUGIN_H
#define ORCHESTRATOR_EDITOR_EXPORT_PLUGIN_H

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

/// An export plugin that is responsible for converting text-based orchestrations to binary
/// so that exported games use the fasting load times possible.
class OrchestratorEditorExportPlugin : public EditorExportPlugin {
    GDCLASS(OrchestratorEditorExportPlugin, EditorExportPlugin);

    bool _convert_to_binary = false;

protected:
    static void _bind_methods();

public:
    //~ Begin EditorExportPlugin Interface
    String _get_name() const override;
    bool _supports_platform(const Ref<EditorExportPlatform>& p_platform) const override;
    void _export_begin(const PackedStringArray& p_features, bool p_is_debug, const String& p_path, uint32_t p_flags) override;
    void _export_file(const String& p_path, const String& p_type, const PackedStringArray& p_features) override;
    //~ End EditorExportPlugin Interface
};

#endif // ORCHESTRATOR_EDITOR_EXPORT_PLUGIN_H