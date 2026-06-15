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

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

class OrchestratorEditorActionDefinition;

/// A small documentation panel mirroring the engine's <code>EditorHelpBit</code>: a styled title band
/// reading <code>Setting &lt;full_path&gt;: &lt;Type&gt; = Default</code> above a bbcode body that
/// supports meta links.
class OrchestratorEditorHelpBit : public VBoxContainer {
    GDCLASS(OrchestratorEditorHelpBit, VBoxContainer);

    struct ThemeCache {
        Ref<Font> doc_bold_font;
        Ref<Font> doc_italic_font;
        Ref<Font> doc_code_font;
        Ref<Font> doc_kbd_font;
        int doc_code_font_size = 0;
        int doc_kbd_font_size = 0;
        Color code_text_color;
        Color code_bg_color;
        Color link_color;
    } _theme_cache;

    RichTextLabel* _title = nullptr;
    RichTextLabel* _content = nullptr;

    float _content_min_height = 0.0;
    float _content_max_height = 0.0;

    void _meta_clicked(const Variant& p_meta);

    /// Refreshes _theme_cache from the editor theme. Called on NOTIFICATION_THEME_CHANGED.
    void _update_theme_cache();

    /// Parses Godot documentation bbcode and renders it into p_rtl using _theme_cache.
    /// Ported and adapted from the engine's EditorHelp::_add_text_to_rt.
    void _add_text_to_rt(const String& p_bbcode, RichTextLabel* p_rtl);

    /// Renders the title band.
    /// @param p_label the leading symbol kind (e.g. "Setting")
    /// @param p_full_path the full property path, rendered as a help link
    /// @param p_type the property type
    /// @param p_default the default value
    void _set_title(const String& p_label, const String& p_full_path, const String& p_type, const String& p_default);

    /// Renders the body of the help from documentation bbcode.
    /// @param p_bbcode the help content.
    void _set_content(const String& p_bbcode);

    void _update_content_height();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    /// Resolves a documentation symbol and renders it. The symbol format mirrors the engine's
    /// <code>EditorHelpBit::parse_symbol</code>: <code>item_type|class_name|item_name</code>. Lookups are
    /// resolved against our own data sources (e.g. <code>OrchestratorSettings</code>), since the engine's
    /// <code>DocTools</code> is not readable from GDExtension.
    /// @param p_symbol the documentation symbol to resolve and display.
    void parse_symbol(const String& p_symbol);

    /// Renders help for an action: a category breadcrumb title above the action's tooltip body.
    /// @param p_action the action to display, or an invalid reference to clear.
    void parse_action(const Ref<OrchestratorEditorActionDefinition>& p_action);

    void set_content_help_limits(float p_min, float p_max);

    /// When <code>p_in_tooltip</code> is true the inner labels use the tooltip theme variations
    /// (<code>EditorHelpBitTooltipTitle</code>/<code>EditorHelpBitTooltipContent</code>), which carry the
    /// correct title-band and content backgrounds; otherwise the in-panel variations are used.
    explicit OrchestratorEditorHelpBit(bool p_in_tooltip = false);
};

/// An interactive tooltip popup for the inspector, mirroring the engine's <code>EditorHelpBitTooltip</code>.
///
/// Godot's built-in tooltip auto-dismisses on mouse motion, which is incompatible with clickable meta
/// links. This popup is self-managed: a grace timer governs dismissal, and entering the popup window
/// (WM mouse enter) cancels it, so the cursor can travel from the property into the popup to click a
/// link. It hides only once the mouse is over neither the originating property nor this popup.
class OrchestratorEditorHelpBitTooltip : public PopupPanel {
    GDCLASS(OrchestratorEditorHelpBitTooltip, PopupPanel);

    OrchestratorEditorHelpBit* _help_bit = nullptr;
    Timer* _dismiss_timer = nullptr;

    void _dismiss_timeout();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    /// Resolves a documentation symbol and renders it in the tooltip. See
    /// <code>OrchestratorEditorHelpBit::parse_symbol</code> for the symbol format.
    /// @param p_symbol the documentation symbol to resolve and display.
    void set_content(const String& p_symbol);

    void popup_at_mouse();

    void request_dismiss();
    void cancel_dismiss();

    OrchestratorEditorHelpBitTooltip();
};