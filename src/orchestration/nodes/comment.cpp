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
#include "orchestration/nodes/comment.h"

#include "common/string_utils.h"
#include "core/godot/io/resource_uid.h"

void OScriptNodeComment::_get_property_list(List<PropertyInfo>* r_list) const {
    const String movement_modes = "Group Movement,Comment";
}

void OScriptNodeComment::_validate_property(PropertyInfo& p_property) const {
    if (p_property.name.match("background_color")) {
        if (_tint_color_enabled) {
            p_property.usage &= ~PROPERTY_USAGE_READ_ONLY;
        } else {
            p_property.usage |= PROPERTY_USAGE_READ_ONLY;
        }
    }
}

String OScriptNodeComment::get_tooltip_text() const {
    if (!_comments.is_empty()) {
        return _comments;
    } else {
        return "Adds comment functionality to the node graph.";
    }
}

String OScriptNodeComment::get_node_title() const {
    return _title;
}

String OScriptNodeComment::get_icon() const {
    if (!_icon_path.is_empty() && _icon_path.begins_with("uid://")) {
        return GDE::ResourceUID::uid_to_path(_icon_path);
    }
    return StringUtils::default_if_empty(_icon_path, "VisualShaderNodeComment");
}

void OScriptNodeComment::set_title_text(const String& p_title) {
    if (p_title != _title) {
        _title = p_title;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_icon_path(const String& p_icon_path) {
    if (p_icon_path != _icon_path) {
        _icon_path = p_icon_path;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_title_text_center_aligned(bool p_enabled) {
    if (p_enabled != _title_center_aligned) {
        _title_center_aligned = p_enabled;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_tint_color_enabled(bool p_enabled) {
    if (p_enabled != _tint_color_enabled) {
        _tint_color_enabled = p_enabled;
        _notify_pins_changed();
        emit_changed();

        // Triggers validation for background_color being enabled/disabled in Inspector
        notify_property_list_changed();
    }
}

void OScriptNodeComment::set_tint_color(const Color& p_tint_color) {
    if (!_tint_color.is_equal_approx(p_tint_color)) {
        _tint_color = p_tint_color;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_comments_text(const String& p_comments) {
    if (p_comments != _comments) {
        _comments = p_comments;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_comments_text_color(const Color& p_comments_color) {
    if (!_comments_color.is_equal_approx(p_comments_color)) {
        _comments_color = p_comments_color;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_comments_text_font_size(int p_comments_font_size) {
    if (p_comments_font_size != _comments_font_size) {
        _comments_font_size = p_comments_font_size;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_autoshrink_enabled(bool p_enabled) {
    if (p_enabled != _autoshink_enabled) {
        _autoshink_enabled = p_enabled;
        _notify_pins_changed();
        emit_changed();
    }
}

void OScriptNodeComment::set_attached_nodes(const PackedInt64Array& p_nodes) {
    // The GraphFrame tracks what nodes are attached to it.
    // Attached nodes move with the frame as the frame moves, so this binds them together.
    _attached_nodes = p_nodes;
    emit_changed();
}

void OScriptNodeComment::attach_node(const Ref<OScriptNode>& p_node) {
    if (p_node.is_valid() && !_attached_nodes.has(p_node->get_id())) {
        _attached_nodes.push_back(p_node->get_id());
    }
}

void OScriptNodeComment::detach_node(const Ref<OScriptNode>& p_node) {
    if (p_node.is_valid() && _attached_nodes.has(p_node->get_id())) {
        _attached_nodes.erase(p_node->get_id());
    }
}

void OScriptNodeComment::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_icon_path"), &OScriptNodeComment::get_icon_path);
    ClassDB::bind_method(D_METHOD("set_icon_path", "icon_path"), &OScriptNodeComment::set_icon_path);

    ClassDB::bind_method(D_METHOD("get_title_text"), &OScriptNodeComment::get_title_text);
    ClassDB::bind_method(D_METHOD("set_title_text", "title"), &OScriptNodeComment::set_title_text);
    ClassDB::bind_method(D_METHOD("is_title_text_center_aligned"), &OScriptNodeComment::is_title_text_center_aligned);
    ClassDB::bind_method(D_METHOD("set_title_text_center_aligned", "enabled"), &OScriptNodeComment::set_title_text_center_aligned);

    ClassDB::bind_method(D_METHOD("is_tint_color_enabled"), &OScriptNodeComment::is_tint_color_enabled);
    ClassDB::bind_method(D_METHOD("set_tint_color_enabled", "enabled"), &OScriptNodeComment::set_tint_color_enabled);
    ClassDB::bind_method(D_METHOD("get_tint_color"), &OScriptNodeComment::get_tint_color);
    ClassDB::bind_method(D_METHOD("set_tint_color", "tint_color"), &OScriptNodeComment::set_tint_color);

    ClassDB::bind_method(D_METHOD("get_comments_text_font_size"), &OScriptNodeComment::get_comments_text_font_size);
    ClassDB::bind_method(D_METHOD("set_comments_text_font_size", "font_size"), &OScriptNodeComment::set_comments_text_font_size);
    ClassDB::bind_method(D_METHOD("get_comments_text_color"), &OScriptNodeComment::get_comments_text_color);
    ClassDB::bind_method(D_METHOD("set_comments_text_color", "comments_text_color"), &OScriptNodeComment::set_comments_text_color);
    ClassDB::bind_method(D_METHOD("get_comments_text"), &OScriptNodeComment::get_comments_text);
    ClassDB::bind_method(D_METHOD("set_comments_text", "comments"), &OScriptNodeComment::set_comments_text);

    ClassDB::bind_method(D_METHOD("is_autoshrink_enabled"), &OScriptNodeComment::is_autoshrink_enabled);
    ClassDB::bind_method(D_METHOD("set_autoshrink_enabled", "enabled"), &OScriptNodeComment::set_autoshrink_enabled);

    ClassDB::bind_method(D_METHOD("get_attached_nodes"), &OScriptNodeComment::get_attached_nodes);
    ClassDB::bind_method(D_METHOD("set_attached_nodes", "attached_nodes"), &OScriptNodeComment::set_attached_nodes);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "icon", PROPERTY_HINT_FILE), "set_icon_path", "get_icon_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title_text", "get_title_text");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_center"), "set_title_text_center_aligned", "is_title_text_center_aligned");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "background_color_enabled"), "set_tint_color_enabled", "is_tint_color_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_tint_color", "get_tint_color");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "font_size", PROPERTY_HINT_RANGE, "0,64"), "set_comments_text_font_size", "get_comments_text_font_size");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "text_color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_comments_text_color", "get_comments_text_color");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "comments", PROPERTY_HINT_MULTILINE_TEXT), "set_comments_text", "get_comments_text");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoshrink_enabled"), "set_autoshrink_enabled", "is_autoshrink_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "attached_nodes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_attached_nodes", "get_attached_nodes");
}