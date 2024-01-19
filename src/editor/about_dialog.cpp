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
#include "authors.gen.h"
#include "donors.gen.h"
#include "license.gen.h"
#include "about_dialog.h"
#include "common/version.h"
#include "plugin/plugin.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

OrchestratorAboutDialog::OrchestratorAboutDialog()
{
    set_title("About Godot Orchestrator");
    set_hide_on_ok(true);

    VBoxContainer* vbc = memnew(VBoxContainer);
    add_child(vbc);

    HBoxContainer* hbc = memnew(HBoxContainer);
    hbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    hbc->set_alignment(BoxContainer::ALIGNMENT_CENTER);
    hbc->add_theme_constant_override("separation", 30);
    vbc->add_child(hbc);

    _logo = memnew(TextureRect);
    _logo->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    _logo->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    _logo->set_custom_minimum_size(Size2(75, 0));
    hbc->add_child(_logo);

    VBoxContainer* version_info_vbc = memnew(VBoxContainer);

    // Add a dummy control node for spacing.
    Control* v_spacer = memnew(Control);
    version_info_vbc->add_child(v_spacer);

    _version_btn = memnew(LinkButton);
    String hash = String(VERSION_HASH);
    if (hash.length() != 0)
        hash = " " + vformat("[%s]", hash.left(9));
    _version_btn->set_text(VERSION_FULL_NAME + hash);
    // Set the text to copy in metadata as it slightly differs from the button's text.
    _version_btn->set_meta("text_to_copy", "v" VERSION_FULL_BUILD + hash);
    _version_btn->set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);
    _version_btn->set_tooltip_text("Click to copy.");
    version_info_vbc->add_child(_version_btn);

    Label* about_text = memnew(Label);
    about_text->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
    about_text->set_text(String::utf8("\xc2\xa9 2023-present ") + ("Vahera Studios, LLC and it's contributors."));
    version_info_vbc->add_child(about_text);

    hbc->add_child(version_info_vbc);

    TabContainer* tc = memnew(TabContainer);
    tc->set_tab_alignment(TabBar::ALIGNMENT_CENTER);
    tc->set_custom_minimum_size(Size2(400, 200));
    tc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    tc->set_theme_type_variation("TabContainerOdd");
    vbc->add_child(tc);

    // Authors

    List<String> dev_sections;
    dev_sections.push_back("Project Founders");
    dev_sections.push_back("Lead Developer");
    dev_sections.push_back("Developers");
    const char *const *dev_src[] = { AUTHORS_FOUNDERS, AUTHORS_LEAD_DEVELOPERS, AUTHORS_DEVELOPERS };
    tc->add_child(_populate_list("Authors", dev_sections, dev_src, 1, false));

    // Donors
    List<String> donor_sections;
    donor_sections.push_back("Gold donors");
    donor_sections.push_back("Silver donors");
    donor_sections.push_back("Bronze donors");
    const char *const *donor_src[] = { DONORS_GOLD, DONORS_SILVER, DONORS_BRONZE };
    tc->add_child(_populate_list("Donors", donor_sections, donor_src, 3, true));

    // License
    _license_text = memnew(RichTextLabel);
    _license_text->set_threaded(true);
    _license_text->set_name("License");
    _license_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _license_text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _license_text->set_text(String::utf8(ORCHESTRATOR_LICENSE_TEXT));
    tc->add_child(_license_text);
}

void OrchestratorAboutDialog::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_on_version_pressed"), &OrchestratorAboutDialog::_on_version_pressed);
}

void OrchestratorAboutDialog::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        _version_btn->connect("pressed", callable_mp(this, &OrchestratorAboutDialog::_on_version_pressed));
        _patreon_btn->connect("pressed", callable_mp(this, &OrchestratorAboutDialog::_on_patreon_button));
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        _theme_changing = true;
        callable_mp(this, &OrchestratorAboutDialog::_on_theme_changed).call_deferred();
    }
}

ScrollContainer* OrchestratorAboutDialog::_populate_list(const String &p_name, const List<String> &p_sections,
                                             const char *const *const p_src[], int p_flag_single_column, bool p_donor)
{
    ScrollContainer* sc = memnew(ScrollContainer);
    sc->set_name(p_name);
    sc->set_v_size_flags(Control::SIZE_EXPAND);

    VBoxContainer *vbc = memnew(VBoxContainer);
    vbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    sc->add_child(vbc);

    for (int i = 0; i < p_sections.size(); i++)
    {
        bool single_column = p_flag_single_column & 1 << i;
        const char *const *names_ptr = p_src[i];
        if (*names_ptr)
        {
            Label* label = memnew(Label);
            label->set_theme_type_variation("HeaderSmall");
            label->set_text(p_sections[i]);
            vbc->add_child(label);

            ItemList *list = memnew(ItemList);
            list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            list->set_same_column_width(true);
            list->set_auto_height(true);
            list->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
            list->add_theme_constant_override("h_separation", 16);
            while (*names_ptr)
                list->add_item(String::utf8(*names_ptr++), nullptr, false);
            list->set_max_columns(list->get_item_count() < 4 || single_column ? 1 : 16);
            vbc->add_child(list);

            HSeparator* hs = memnew(HSeparator);
            hs->set_modulate(Color(0, 0, 0, 0));
            vbc->add_child(hs);
        }
    }

    if (p_donor)
    {
        _patreon_btn = memnew(LinkButton);
        _patreon_btn->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
        _patreon_btn->set_text("Become a patreon member at https://patreon.com/vahera");
        _patreon_btn->set_focus_mode(Control::FOCUS_NONE);
        _patreon_btn->set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);
        vbc->add_child(_patreon_btn);
    }

    return sc;
}

void OrchestratorAboutDialog::_on_version_pressed()
{
    DisplayServer::get_singleton()->clipboard_set(_version_btn->get_meta("text_to_copy"));
}

void OrchestratorAboutDialog::_on_theme_changed()
{
    if (!_theme_changing)
        return;

    const Ref<Font> font = get_theme_font(StringName("source"), "EditorFonts");
    const int font_size  = get_theme_font_size(StringName("source_size"), "EditorFonts");

    _license_text->begin_bulk_theme_override();
    _license_text->add_theme_font_override("normal_font", font);
    _license_text->add_theme_font_size_override("normal_font_size", font_size);
    _license_text->add_theme_constant_override("line_separation", 4);
    _license_text->end_bulk_theme_override();

    _logo->set_texture(ResourceLoader::get_singleton()->load("res://addons/orchestrator/icons/Orchestrator_Logo.svg"));

    Ref<Theme> theme = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_theme();
    if (theme.is_valid())
    {
        Ref<StyleBox> sb = theme->get_stylebox("panel", "EditorAbout");
        if (sb.is_valid())
        {
            Ref<StyleBox> sbd = sb->duplicate();
            add_theme_stylebox_override("panel", sbd);
        }
    }

    _theme_changing = false;
}

void OrchestratorAboutDialog::_on_patreon_button()
{
    OS::get_singleton()->shell_open(OrchestratorPlugin::get_singleton()->get_patreon_url());
}