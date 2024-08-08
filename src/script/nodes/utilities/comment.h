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
#ifndef ORCHESTRATOR_SCRIPT_NODE_COMMENT_H
#define ORCHESTRATOR_SCRIPT_NODE_COMMENT_H

#include "script/script.h"

/// Provides the ability to add a comment/text section with a frame around existing nodes.
class OScriptNodeComment : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeComment, OScriptNode);
    static void _bind_methods() { }

protected:
    String _comments;
    String _title{ "Comment" };
    bool _align_center{ false };
    Color _background_color{ 0.6, 0.6, 0.6, 0.05 };
    Color _text_color{ 1.0, 1.0, 1.0, 1.0 };
    int _font_size{ 0 };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo> *r_list) const;
    bool _get(const StringName &p_name, Variant &r_value) const;
    bool _set(const StringName &p_name, const Variant &p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "comment"; }
    String get_icon() const override;
    //~ End OScriptNodeInterface

    /// Return whether the title of the comment should be center aligned
    /// @return true if the title should be aligned center, false to align to the left
    bool is_title_center_aligned() const { return _align_center; }

    /// Get the comment node's background color
    /// @return the background color
    Color get_background_color() const { return _background_color; }

    /// Get the comment text's color
    /// @return the text color
    Color get_text_color() const { return _text_color; }

    /// Get the comment text's font size
    /// @return the font size, 0 means use the default size
    int get_font_size() const { return _font_size; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_COMMENT_H
