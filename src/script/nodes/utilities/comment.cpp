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
#include "comment.h"

#include "common/string_utils.h"

void OScriptNodeComment::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String movement_modes = "Group Movement,Comment";

    r_list->push_back(PropertyInfo(Variant::STRING, "title"));
    r_list->push_back(PropertyInfo(Variant::STRING, "icon", PROPERTY_HINT_FILE));
    r_list->push_back(PropertyInfo(Variant::BOOL, "align_center"));
    r_list->push_back(PropertyInfo(Variant::COLOR, "background_color"));
    r_list->push_back(PropertyInfo(Variant::INT, "font_size", PROPERTY_HINT_RANGE, "0,64"));
    r_list->push_back(PropertyInfo(Variant::COLOR, "text_color", PROPERTY_HINT_COLOR_NO_ALPHA));
    r_list->push_back(PropertyInfo(Variant::STRING, "comments", PROPERTY_HINT_MULTILINE_TEXT));
}

bool OScriptNodeComment::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("comments"))
    {
        r_value = _comments;
        return true;
    }
    else if (p_name.match("align_center"))
    {
        r_value = _align_center;
        return true;
    }
    else if (p_name.match("background_color"))
    {
        r_value = _background_color;
        return true;
    }
    else if (p_name.match("font_size"))
    {
        r_value = _font_size;
        return true;
    }
    else if (p_name.match("text_color"))
    {
        r_value = _text_color;
        return true;
    }
    else if (p_name.match("title"))
    {
        r_value = _title;
        return true;
    }
    else if (p_name.match("icon"))
    {
        r_value = _icon;
        return true;
    }
    return false;
}

bool OScriptNodeComment::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("comments"))
    {
        _comments = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("align_center"))
    {
        _align_center = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("background_color"))
    {
        _background_color = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("font_size"))
    {
        _font_size = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("text_color"))
    {
        _text_color = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("title"))
    {
        _title = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("icon"))
    {
        _icon = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

String OScriptNodeComment::get_tooltip_text() const
{
    if (!_comments.is_empty())
        return _comments;
    else
        return "Adds comment functionality to the node graph.";
}

String OScriptNodeComment::get_node_title() const
{
    return _title;
}

String OScriptNodeComment::get_icon() const
{
    return StringUtils::default_if_empty(_icon, "VisualShaderNodeComment");
}
