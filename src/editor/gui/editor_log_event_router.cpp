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
#include "editor/gui/editor_log_event_router.h"

#include "common/macros.h"
#include "editor/editor.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

RichTextLabel* OrchestratorEditorLogEventRouter::_locate_editor_output_log() {
    const TypedArray<Node> editor_log_nodes = EditorNode->find_children("*", "EditorLog", true, false);
    if (editor_log_nodes.is_empty()) {
        return nullptr;
    }

    for (uint32_t i = 0; i < editor_log_nodes.size(); i++) {
        Node* editor_log_node = cast_to<Node>(editor_log_nodes[i]);
        if (!editor_log_node) {
            continue;
        }

        const TypedArray<Node> log_nodes = editor_log_node->find_children("*", "RichTextLabel", true, false);
        if (log_nodes.is_empty()) {
            return nullptr;
        }

        for (uint32_t j = 0; j < log_nodes.size(); j++) {
            RichTextLabel* label = cast_to<RichTextLabel>(log_nodes[j]);
            if (!label) {
                continue;
            }
            return label;
        }
    }
    return nullptr;
}

void OrchestratorEditorLogEventRouter::_meta_clicked(const String& p_meta) {
    if (!p_meta.contains(":")) {
        return;
    }

    const PackedStringArray parts = p_meta.rsplit(":", true, 1);
    const String path = parts[0];
    const int node_id = parts[1].to_int();

    if (path.begins_with("res://")) {
        if (FileAccess::file_exists(path) && path.get_extension().to_lower().match("torch")) {
            OrchestratorPlugin::get_singleton()->make_active();

            const Ref<Resource> resource = ResourceLoader::get_singleton()->load(path);
            OrchestratorEditor::get_singleton()->edit(resource, node_id);
        }
    }
}

void OrchestratorEditorLogEventRouter::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            RichTextLabel* label = _locate_editor_output_log();
            if (label && !label->is_connected("meta_clicked", callable_mp_this(_meta_clicked))) {
                label->connect("meta_clicked", callable_mp_this(_meta_clicked));
            }
            break;
        }
        case NOTIFICATION_EXIT_TREE: {
            RichTextLabel* label = _locate_editor_output_log();
            if (label && label->is_connected("meta_clicked", callable_mp_this(_meta_clicked))) {
                label->disconnect("meta_clicked", callable_mp_this(_meta_clicked));
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditorLogEventRouter::_bind_methods() {
}