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
#pragma once

#include "orchestration/node.h"

/// Provides the ability to add a comment/text section with a frame around existing nodes.
class OScriptNodeComment : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeComment, OScriptNode);

    String _title = "Comment";
    bool _title_center_aligned = false;

    String _icon_path;

    Color _tint_color = Color(0.6, 0.6, 0.6, 0.05);
    bool _tint_color_enabled = true;

    String _comments;
    Color _comments_color = Color(1.0, 1.0, 1.0, 1.0);
    int _comments_font_size = 0;

    bool _autoshink_enabled = true;
    PackedInt64Array _attached_nodes;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo> *r_list) const;
    void _validate_property(PropertyInfo& p_property) const;
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "comment"; }
    String get_icon() const override;
    //~ End OScriptNodeInterface

    String get_title_text() const { return _title; }
    void set_title_text(const String& p_title);

    String get_icon_path() const { return _icon_path; }
    void set_icon_path(const String& p_icon_path);

    bool is_title_text_center_aligned() const { return _title_center_aligned; }
    void set_title_text_center_aligned(bool p_enabled);

    bool is_tint_color_enabled() const { return _tint_color_enabled; }
    void set_tint_color_enabled(bool p_enabled);

    Color get_tint_color() const { return _tint_color; }
    void set_tint_color(const Color& p_tint_color);

    String get_comments_text() const { return _comments; }
    void set_comments_text(const String& p_comments);

    Color get_comments_text_color() const { return _comments_color; }
    void set_comments_text_color(const Color& p_comments_color);

    int get_comments_text_font_size() const { return _comments_font_size; }
    void set_comments_text_font_size(int p_comments_font_size);

    bool is_autoshrink_enabled() const { return _autoshink_enabled; }
    void set_autoshrink_enabled(bool p_enabled);

    PackedInt64Array get_attached_nodes() const { return _attached_nodes; }
    void set_attached_nodes(const PackedInt64Array& p_nodes);

    void attach_node(const Ref<OScriptNode>& p_node);
    void detach_node(const Ref<OScriptNode>& p_node);
};
