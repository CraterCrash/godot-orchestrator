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
#include "editor/doc/editor_help.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "core/godot/core_string_names.h"
#include "editor/actions/definition.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace {

/// Translates a request meta ("kind:target") into the editor's class-help topic format. Member
/// references address "Class:member" (the engine uses ':' between class and member).
String _help_topic(const String& p_kind, const String& p_target) {
    if (p_kind == "class") {
        return vformat("class_name:%s", p_target);
    }
    if (p_kind == "member") {
        return vformat("class_property:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "method" || p_kind == "constructor" || p_kind == "operator") {
        return vformat("class_method:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "signal") {
        return vformat("class_signal:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "constant") {
        return vformat("class_constant:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "enum") {
        return vformat("class_enum:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "annotation") {
        return vformat("class_annotation:%s", p_target.replace(".", ":"));
    }
    if (p_kind == "theme_item") {
        return vformat("class_theme_item:%s", p_target.replace(".", ":"));
    }
    return {};
}

bool _is_doc_reference_keyword(const String& p_keyword) {
    static const char* keywords[] = { "method", "member", "signal", "constant", "constructor",
        "operator", "enum", "annotation", "theme_item", nullptr };
    for (int i = 0; keywords[i]; i++) {
        if (p_keyword == keywords[i]) {
            return true;
        }
    }
    return false;
}

/// Formats a setting's default value for the tooltip title. Integer enums/flags are resolved to their
/// hint_string labels (e.g. "Name", or "A|B" for flags); strings are quoted; a Nil value yields "".
String _format_default_value(const Variant& p_value, int64_t p_hint, const String& p_hint_string) {
    const Variant::Type type = p_value.get_type();

    if (type == Variant::INT && !p_hint_string.is_empty()
            && (p_hint == PROPERTY_HINT_ENUM || p_hint == PROPERTY_HINT_FLAGS)) {
        const int64_t value = p_value;
        const PackedStringArray entries = p_hint_string.split(",");

        if (p_hint == PROPERTY_HINT_ENUM) {
            for (int i = 0; i < entries.size(); i++) {
                String label = entries[i];
                int64_t entry_value = i;
                const int colon = label.rfind(":");
                if (colon != -1) {
                    entry_value = label.substr(colon + 1).to_int();
                    label = label.substr(0, colon);
                }
                if (entry_value == value) {
                    return label;
                }
            }
        } else {
            PackedStringArray names;
            for (int i = 0; i < entries.size(); i++) {
                String label = entries[i];
                int64_t bit = int64_t(1) << i;
                const int colon = label.rfind(":");
                if (colon != -1) {
                    bit = label.substr(colon + 1).to_int();
                    label = label.substr(0, colon);
                }
                if (value & bit) {
                    names.push_back(label);
                }
            }
            if (!names.is_empty()) {
                return String("|").join(names);
            }
        }
    }

    switch (type) {
        case Variant::NIL:
            return String();
        case Variant::STRING:
        case Variant::STRING_NAME:
            return vformat("\"%s\"", p_value);
        default:
            return vformat("%s", p_value);
    }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OrchestratorEditorHelpBit
///

void OrchestratorEditorHelpBit::_meta_clicked(const Variant& p_meta) {
    const String meta = p_meta;

    if (meta.begins_with("http://") || meta.begins_with("https://")) {
        OS::get_singleton()->shell_open(meta);
    } else if (meta.begins_with("www.")) {
        OS::get_singleton()->shell_open("https://" + meta);
    } else {
        // Documentation reference ("kind:target"); open the editor's built-in class reference.
        const int sep = meta.find(":");
        if (sep != -1) {
            const String topic = _help_topic(meta.substr(0, sep), meta.substr(sep + 1));
            if (!topic.is_empty()) {
                EI->set_main_screen_editor("Script");
                EI->get_script_editor()->goto_help(topic);
            }
        }
    }

    emit_signal("request_hide");
}

void OrchestratorEditorHelpBit::_update_theme_cache() {
    _theme_cache.doc_bold_font = SceneUtils::get_editor_font("doc_bold");
    _theme_cache.doc_italic_font = SceneUtils::get_editor_font("doc_italic");
    _theme_cache.doc_code_font = SceneUtils::get_editor_font("doc_source");
    _theme_cache.doc_kbd_font = SceneUtils::get_editor_font("doc_keyboard");
    _theme_cache.doc_code_font_size = get_theme_font_size("doc_source_size", "EditorHelp");
    _theme_cache.doc_kbd_font_size = get_theme_font_size("doc_keyboard_size", "EditorHelp");

    const Color code_color = SceneUtils::get_editor_color("code_color", "EditorHelp");
    const Color error_color = SceneUtils::get_editor_color("error_color", "Editor");
    _theme_cache.code_text_color = code_color.lerp(error_color, 0.6f);
    _theme_cache.code_bg_color = Color(0, 0, 0, 0.25f);
    _theme_cache.link_color = SceneUtils::get_editor_color("accent_color", "Editor");
}

// Adapted from the engine's EditorHelp::_add_text_to_rt: parses Godot documentation bbcode and drives
// the RichTextLabel directly, using the cached editor theme. Symbol references route through ClassDB
// and our "kind:target" meta scheme (handled by _meta_clicked) instead of DocTools.
void OrchestratorEditorHelpBit::_add_text_to_rt(const String& p_bbcode, RichTextLabel* p_rtl) {
    Vector<String> tag_stack;
    int pos = 0;
    const int len = p_bbcode.length();
    while (pos < len) {
        int brk_pos = p_bbcode.find("[", pos);
        if (brk_pos == -1) {
            brk_pos = len;
        }
        if (brk_pos > pos) {
            p_rtl->add_text(p_bbcode.substr(pos, brk_pos - pos));
        }
        if (brk_pos == len) {
            break;
        }

        const int brk_end = p_bbcode.find("]", brk_pos + 1);
        if (brk_end == -1) {
            p_rtl->add_text(p_bbcode.substr(brk_pos));
            break;
        }

        const String tag = p_bbcode.substr(brk_pos + 1, brk_end - brk_pos - 1);

        // Closing tag: pop the matching formatting span.
        if (tag.begins_with("/")) {
            if (!tag_stack.is_empty() && tag_stack[tag_stack.size() - 1] == tag.substr(1)) {
                p_rtl->pop();
                tag_stack.remove_at(tag_stack.size() - 1);
            }
            pos = brk_end + 1;
            continue;
        }

        const int sp = tag.find(" ");
        const int eq = tag.find("=");
        int cut = sp;
        if (eq != -1 && (cut == -1 || eq < cut)) {
            cut = eq;
        }
        const String name = (cut != -1) ? tag.substr(0, cut) : tag;

        // Verbatim spans: monospace + darkened background + reddish text.
        if (name == "code" || name == "codeblock" || name == "kbd") {
            const bool keyboard = (name == "kbd");
            const String close = vformat("[/%s]", name);
            int end_pos = p_bbcode.find(close, brk_end + 1);
            if (end_pos == -1) {
                end_pos = len;
            }
            const String text = p_bbcode.substr(brk_end + 1, end_pos - (brk_end + 1));

            p_rtl->push_font(keyboard ? _theme_cache.doc_kbd_font : _theme_cache.doc_code_font, keyboard ? _theme_cache.doc_kbd_font_size : _theme_cache.doc_code_font_size);
            p_rtl->push_bgcolor(_theme_cache.code_bg_color);
            p_rtl->push_color(_theme_cache.code_text_color);
            p_rtl->add_text(text);
            p_rtl->pop();
            p_rtl->pop();
            p_rtl->pop();

            pos = (end_pos == len) ? len : end_pos + close.length();
            continue;
        }

        // Formatting spans with a matching close tag.
        if (tag == "b") {
            p_rtl->push_font(_theme_cache.doc_bold_font);
            tag_stack.push_back("b");
        } else if (tag == "i") {
            p_rtl->push_font(_theme_cache.doc_italic_font);
            tag_stack.push_back("i");
        } else if (tag == "u") {
            p_rtl->push_underline();
            tag_stack.push_back("u");
        } else if (tag == "s") {
            p_rtl->push_strikethrough();
            tag_stack.push_back("s");
        } else if (tag == "center") {
            p_rtl->push_paragraph(HORIZONTAL_ALIGNMENT_CENTER);
            tag_stack.push_back("center");
        } else if (name == "color") {
            p_rtl->push_color(Color::from_string(tag.substr(eq + 1), Color()));
            tag_stack.push_back("color");
        } else if (name == "bgcolor") {
            p_rtl->push_bgcolor(Color::from_string(tag.substr(eq + 1), Color()));
            tag_stack.push_back("bgcolor");
        } else if (name == "url") {
            p_rtl->push_meta(tag.substr(eq + 1), RichTextLabel::META_UNDERLINE_ON_HOVER);
            tag_stack.push_back("url");
        } else if (tag == "br") {
            p_rtl->newline();
        } else if (tag == "lb") {
            p_rtl->add_text("[");
        } else if (tag == "rb") {
            p_rtl->add_text("]");
        } else if (name == "param") {
            // A named parameter: monospace, no link.
            p_rtl->push_font(_theme_cache.doc_code_font, _theme_cache.doc_code_font_size);
            p_rtl->push_color(_theme_cache.code_text_color);
            p_rtl->add_text(tag.substr(sp + 1).strip_edges());
            p_rtl->pop();
            p_rtl->pop();
        } else if (sp == -1 && ClassDB::class_exists(tag)) {
            // [ClassName] -> class help link.
            p_rtl->push_color(_theme_cache.link_color);
            p_rtl->push_meta(vformat("class:%s", tag), RichTextLabel::META_UNDERLINE_ON_HOVER);
            p_rtl->add_text(tag);
            p_rtl->pop();
            p_rtl->pop();
        } else if (sp != -1 && name == "class") {
            const String target = tag.substr(sp + 1).strip_edges();
            p_rtl->push_color(_theme_cache.link_color);
            p_rtl->push_meta(vformat("class:%s", target), RichTextLabel::META_UNDERLINE_ON_HOVER);
            p_rtl->add_text(target);
            p_rtl->pop();
            p_rtl->pop();
        } else if (sp != -1 && _is_doc_reference_keyword(name)) {
            const String target = tag.substr(sp + 1).strip_edges();
            if (target.find(".") != -1) {
                // [method Class.member] etc. -> member help link.
                p_rtl->push_color(_theme_cache.link_color);
                p_rtl->push_meta(vformat("%s:%s", name, target), RichTextLabel::META_UNDERLINE_ON_HOVER);
                p_rtl->add_text(target);
                p_rtl->pop();
                p_rtl->pop();
            } else {
                // No class context to resolve against: render as code.
                p_rtl->push_font(_theme_cache.doc_code_font, _theme_cache.doc_code_font_size);
                p_rtl->push_color(_theme_cache.code_text_color);
                p_rtl->add_text(target);
                p_rtl->pop();
                p_rtl->pop();
            }
        } else {
            // Unknown tag -> emit literally.
            p_rtl->add_text(vformat("[%s]", tag));
        }

        pos = brk_end + 1;
    }

    // Close any spans left open by malformed input.
    while (!tag_stack.is_empty()) {
        p_rtl->pop();
        tag_stack.remove_at(tag_stack.size() - 1);
    }
}

void OrchestratorEditorHelpBit::_set_title(const String& p_label, const String& p_full_path, const String& p_type, const String& p_default) {
    _title->clear();
    _title->push_font(SceneUtils::get_editor_font("doc_bold"));

    if (!p_label.is_empty()) {
        _title->push_color(SceneUtils::get_editor_color("title_color", "EditorHelp"));
        _title->append_text(vformat("%s ", p_label));
        _title->pop();
    }

    _title->push_meta(p_full_path, RichTextLabel::META_UNDERLINE_ON_HOVER);
    _title->push_color(SceneUtils::get_editor_color("text_color", "EditorHelp"));
    _title->append_text(p_full_path);
    _title->pop();
    _title->pop();
    _title->pop();

    if (!p_type.is_empty()) {
        const Color symbol_color = SceneUtils::get_editor_color("symbol_color", "EditorHelp");
        _title->push_font(SceneUtils::get_editor_font("doc"));
        _title->push_color(symbol_color);
        _title->append_text(": ");
        _title->pop();
        _title->push_color(SceneUtils::get_editor_color("type_color", "EditorHelp"));
        _title->append_text(p_type);
        _title->pop();
        if (!p_default.is_empty()) {
            _title->push_color(symbol_color);
            _title->append_text(vformat(" = %s", p_default));
            _title->pop();
        }
        _title->pop();
    }

    if (is_inside_tree()) {
        _update_content_height();
    }
}

void OrchestratorEditorHelpBit::_set_content(const String& p_bbcode) {
    _content->clear();

    if (p_bbcode.strip_edges().is_empty()) {
        _content->push_font(_theme_cache.doc_italic_font);
        _content->add_text("No description available.");
        _content->pop();
    } else {
        _add_text_to_rt(p_bbcode, _content);
    }

    if (is_inside_tree()) {
        _update_content_height();
    }
}

void OrchestratorEditorHelpBit::_update_content_height() {
    int32_t content_height = _content->get_content_height();
    const Ref<StyleBox> style_box = _content->get_theme_stylebox(CoreStringName(normal));
    if (style_box.is_valid()) {
        content_height += style_box->get_content_margin(SIDE_TOP) + style_box->get_content_margin(SIDE_BOTTOM);
    }

    const Vector2 content_min_size = _content->get_custom_minimum_size();
    _content->set_custom_minimum_size(Size2(content_min_size.x, CLAMP(content_height, _content_min_height, _content_max_height)));
}

void OrchestratorEditorHelpBit::parse_symbol(const String& p_symbol) {
    const PackedStringArray slices = p_symbol.split("|", true, 2);
    ERR_FAIL_COND_MSG(slices.size() < 3, R"(Invalid symbol: expected "item_type|class_name|item_name".)");

    const String& item_type = slices[0];
    const String& class_name = slices[1];
    const String& item_name = slices[2];

    String label;
    String name;
    String type;
    String default_value;
    String body;

    if (item_type == "property" && class_name == "OrchestratorSettings") {
        OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
        label = "Setting";
        name = item_name;

        for (const PropertyInfo& property : DictionaryUtils::to_properties(settings->get_property_list())) {
            if (property.name == item_name) {
                type = Variant::get_type_name(property.type);
                default_value = _format_default_value(settings->property_get_revert(item_name), property.hint, property.hint_string);
                break;
            }
        }

        body = settings->get_property_description(item_name);
    } else if (item_type == "class") {
        // Named by class_name; readable class documentation isn't available to GDExtension yet, so the
        // body falls back to "No description available." until a doc source is wired.
        label = "Class";
        name = class_name;
    } else if (item_type == "type") {
        label = "Type";
        name = class_name;
    }

    _set_title(label, name, type, default_value);
    _set_content(body);
}

void OrchestratorEditorHelpBit::parse_action(const Ref<OrchestratorEditorActionDefinition>& p_action) {
    _title->clear();

    if (p_action.is_null()) {
        _content->clear();
        if (is_inside_tree()) {
            _update_content_height();
        }
        return;
    }

    _title->push_font(SceneUtils::get_editor_font("doc_bold"));

    const String categories = StringUtils::join(" > ", p_action->category.split("/"));
    if (!categories.is_empty()) {
        _title->push_color(SceneUtils::get_editor_color("title_color", "EditorHelp"));
        _title->add_text(vformat("%s: ", categories));
        _title->pop();
    }

    _title->add_text(p_action->name);
    _title->pop();

    _set_content(p_action->tooltip);
}

void OrchestratorEditorHelpBit::set_content_help_limits(float p_min, float p_max) {
    ERR_FAIL_COND(p_min > p_max);
    _content_min_height = p_min;
    _content_max_height = p_max;

    if (is_inside_tree()) {
        _update_content_height();
    }
}

void OrchestratorEditorHelpBit::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            _update_content_height();
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_cache();
            _content->add_theme_color_override("selection_color", get_theme_color("selection_color", "EditorHelp"));
            break;
        }
    }
}

void OrchestratorEditorHelpBit::_bind_methods() {
    ADD_SIGNAL(MethodInfo("request_hide"));
}

OrchestratorEditorHelpBit::OrchestratorEditorHelpBit(bool p_in_tooltip) {
    add_theme_constant_override("separation", 0);

    _content_min_height = 48 * EDSCALE;
    _content_max_height = 360 * EDSCALE;

    _title = memnew(RichTextLabel);
    _title->set_theme_type_variation(p_in_tooltip ? "EditorHelpBitTooltipTitle" : "EditorHelpBitTitle");
    _title->set_custom_minimum_size(Size2(640 * EDSCALE, 0));
    _title->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
    _title->set_fit_content(true);
    _title->connect("meta_clicked", callable_mp_this(_meta_clicked));
    add_child(_title);

    _content = memnew(RichTextLabel);
    _content->set_theme_type_variation(p_in_tooltip ? "EditorHelpBitTooltipContent" : "EditorHelpBitContent");
    _content->set_custom_minimum_size(Size2(640 * EDSCALE, _content_min_height));
    _content->set_v_size_flags(SIZE_EXPAND_FILL);
    _content->set_fit_content(true);
    _content->set_use_bbcode(true);
    _content->connect("meta_clicked", callable_mp_this(_meta_clicked));
    add_child(_content);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OrchestratorEditorHelpBitTooltip
///

void OrchestratorEditorHelpBitTooltip::_dismiss_timeout() {
    hide();
}

void OrchestratorEditorHelpBitTooltip::set_content(const String& p_symbol) {
    _help_bit->parse_symbol(p_symbol);
}

void OrchestratorEditorHelpBitTooltip::popup_at_mouse() {
    cancel_dismiss();
    reset_size();
    const Vector2 offset = ProjectSettings::get_singleton()->get_setting("display/mouse_cursor/tooltip_position_offset", Point2());
    const Vector2i position = DisplayServer::get_singleton()->mouse_get_position() + Vector2i(offset);
    popup(Rect2i(position, Size2i()));
}

void OrchestratorEditorHelpBitTooltip::request_dismiss() {
    _dismiss_timer->start();
}

void OrchestratorEditorHelpBitTooltip::cancel_dismiss() {
    _dismiss_timer->stop();
}

void OrchestratorEditorHelpBitTooltip::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_WM_MOUSE_ENTER: {
            cancel_dismiss();
            break;
        }
        case NOTIFICATION_WM_MOUSE_EXIT: {
            request_dismiss();
            break;
        }
    }
}

void OrchestratorEditorHelpBitTooltip::_bind_methods() {
}

OrchestratorEditorHelpBitTooltip::OrchestratorEditorHelpBitTooltip() {
    set_flag(FLAG_NO_FOCUS, true);
    set_theme_type_variation("TooltipPanel");

    _help_bit = memnew(OrchestratorEditorHelpBit(true));
    _help_bit->set_content_help_limits(48 * EDSCALE, 360 * EDSCALE);
    _help_bit->connect("request_hide", callable_mp_this(_dismiss_timeout));
    add_child(_help_bit);

    _dismiss_timer = memnew(Timer);
    _dismiss_timer->set_one_shot(true);
    _dismiss_timer->set_wait_time(0.2);
    _dismiss_timer->connect("timeout", callable_mp_this(_dismiss_timeout));
    add_child(_dismiss_timer);
}