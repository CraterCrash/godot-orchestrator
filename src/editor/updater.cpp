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
#include "editor/updater.h"

#include "common/scene_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/link_button.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorUpdater::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        set_visible(false);

        _http_update_request = memnew(HTTPRequest);
        add_child(_http_update_request);

        _http_download_request = memnew(HTTPRequest);
        add_child(_http_download_request);

        Timer* timer = memnew(Timer);
        timer->set_wait_time(60 * 60); // every hour
        timer->set_autostart(true);
        add_child(timer);

        MarginContainer* margin = memnew(MarginContainer);
        margin->add_theme_constant_override("margin_left", 4);
         margin->add_theme_constant_override("margin_right", 4);
        add_child(margin);

        _button = memnew(Button);
        _button->set_text("...");
        _button->set_tooltip_text("An update is available for Godot Orchestrator");
        _button->add_theme_color_override("font_color", Color(0, 1, 0));
        _button->add_theme_color_override("font_hover_color", Color(0, 1, 0));
        _button->set_vertical_icon_alignment(VERTICAL_ALIGNMENT_CENTER);
        _button->set_focus_mode(FOCUS_NONE);
        _button->set_v_size_flags(SIZE_SHRINK_CENTER);
        margin->add_child(_button);

        _update_available_dialog = memnew(AcceptDialog);
        add_child(_update_available_dialog);

        _update_available_dialog->set_title("Download Update!");
        _update_available_dialog->set_size(Vector2i(300, 250));
        _update_available_dialog->set_ok_button_text("Close");

        VBoxContainer* vbox = memnew(VBoxContainer);
        vbox->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
        vbox->set_h_grow_direction(GROW_DIRECTION_BOTH);
        vbox->set_v_grow_direction(GROW_DIRECTION_BOTH);
        vbox->add_theme_constant_override("separation", 10);
        _update_available_dialog->add_child(vbox);

        TextureRect* texture = memnew(TextureRect);
        texture->set_texture(OrchestratorPlugin::get_singleton()->get_plugin_icon_hires());
        texture->set_clip_contents(true);
        texture->set_custom_minimum_size(Vector2(300, 80));
        texture->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        texture->set_stretch_mode(TextureRect::StretchMode::STRETCH_KEEP_ASPECT_CENTERED);
        vbox->add_child(texture);

        _update_available_text = memnew(Label);
        _update_available_text->set_text("Nothing available to download");
        _update_available_text->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        vbox->add_child(_update_available_text);

        CenterContainer* center1 = memnew(CenterContainer);
        vbox->add_child(center1);

        _download_button = memnew(Button);
        _download_button->set_text("Download update");
        _download_button->set_focus_mode(FOCUS_NONE);
        center1->add_child(_download_button);

        CenterContainer* center2 = memnew(CenterContainer);
        vbox->add_child(center2);

        LinkButton* show_release_notes = memnew(LinkButton);
        show_release_notes->set_text("Read release notes...");
        show_release_notes->set_focus_mode(FOCUS_NONE);
        center2->add_child(show_release_notes);

        _update_available_dialog->connect("confirmed", callable_mp(this, &OrchestratorUpdater::_on_dialog_confirmed));
        _download_button->connect("pressed", callable_mp(this, &OrchestratorUpdater::_on_start_download));
        show_release_notes->connect("pressed", callable_mp(this, &OrchestratorUpdater::_on_show_release_notes));

        _http_update_request->connect("request_completed", callable_mp(this, &OrchestratorUpdater::_on_update_check_completed));
        _http_download_request->connect("request_completed", callable_mp(this, &OrchestratorUpdater::_on_download_completed));

        _check_for_updates();

        timer->connect("timeout", callable_mp(this, &OrchestratorUpdater::_check_for_updates));
        _button->connect("pressed", callable_mp(this, &OrchestratorUpdater::_show_update_dialog));
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        set_visible(false);

        while (get_child_count() > 0)
        {
            if (Node* child = get_child(0))
            {
                remove_child(child);
                memdelete(child);
            }
        }
    }
}

void OrchestratorUpdater::_show_update_dialog()
{
    _download_button->set_text("Download");
    _download_button->set_disabled(false);

    _update_available_text->set_text(vformat("%s is available for download.", _version.tag));
    _update_available_dialog->popup_centered();
}

void OrchestratorUpdater::_check_for_updates()
{
    if (_http_update_request->request(OrchestratorPlugin::get_singleton()->get_github_release_url()) != OK)
            ERR_PRINT("Failed to send request to check for updates");
}

void OrchestratorUpdater::_on_update_check_completed(int p_result, int p_code, const PackedStringArray& p_headers,
                                                     const PackedByteArray& p_data)
{
    if (p_result != HTTPRequest::RESULT_SUCCESS && p_code != 200)
        return;

    const Variant json = JSON::parse_string(p_data.get_string_from_utf8());
    if (json.get_type() != Variant::ARRAY)
        return;

    const PackedStringArray tags = _get_tags(json);
    if (tags.is_empty())
        return;

    const std::vector<Version> versions = _parse_tag_versions(tags);
    if (versions.empty())
        return;

    const std::vector<Version> later_versions = _get_versions_after_current_build(versions);
    if (later_versions.empty())
        return;

    _version.tag = later_versions[0].tag;
    _version.build = later_versions[0].build;
    _version.major = later_versions[0].major;
    _version.minor = later_versions[0].minor;
    _version.patch = later_versions[0].patch;

    _button->set_text(_version.tag + " is available!");

    set_visible(true);
}

PackedStringArray OrchestratorUpdater::_get_tags(const Array& p_data)
{
    PackedStringArray tags;
    for (int i = 0; i < p_data.size(); i++)
    {
        const Dictionary& dict = p_data[i];
        const Array keys = dict.keys();
        for (int j = 0; j < keys.size(); ++j)
        {
            const String tag_name = keys[j];
            if (tag_name.match("tag_name"))
            {
                String tag = dict[tag_name];
                tags.push_back(tag);
            }
        }
    }
    return tags;
}

std::vector<OrchestratorUpdater::Version> OrchestratorUpdater::_parse_tag_versions(const PackedStringArray& p_tags)
{
    std::vector<Version> versions;
    for (const String& tag : p_tags)
    {
        Version parsed_version;
        parsed_version.tag = tag;

        const PackedStringArray parts = (tag.begins_with("v") ? tag.substr(1).split(".") : tag.split("."));

        if (parts.size() >= 1)
        {
            // Major
            if (parts[0].is_valid_int())
                parsed_version.major = parts[0].to_int();
        }

        if (parts.size() >= 2)
        {
            // Minor
            if (parts[1].is_valid_int())
                parsed_version.minor = parts[1].to_int();
        }

        if (parts.size() >= 3)
        {
            // Maintenance
            if (parts[2].is_valid_int())
                parsed_version.patch  = parts[2].to_int();
            else
                parsed_version.build = parts[2];
        }

        versions.push_back(parsed_version);
    }
    return versions;
}

std::vector<OrchestratorUpdater::Version> OrchestratorUpdater::_get_versions_after_current_build(const std::vector<Version>& p_versions)
{
    std::vector<Version> versions;
    for (const Version& version : p_versions)
    {
        if (version.major > VERSION_MAJOR)
        {
            versions.push_back(version);
        }
        else if (version.major == VERSION_MAJOR && version.minor > VERSION_MINOR)
        {
            versions.push_back(version);
        }
        else if (version.major == VERSION_MAJOR && version.minor == VERSION_MINOR && (VERSION_MAINTENANCE == 0 && version.patch > 0))
        {
            versions.push_back(version);
        }
        else if (version.major == VERSION_MAJOR && version.minor == VERSION_MINOR && version.patch == version.NOT_AVAILABLE)
        {
            const String status = VERSION_STATUS;
            if (VERSION_MAINTENANCE == 0 && version.patch == -1 && !status.begins_with("stable"))
            {
                // Requires checking the build types
                if (version.build.begins_with("rc") && status.begins_with("rc"))
                {
                    // Both are Release Candidates
                    int64_t version_build = version.build.substr(2).to_int();
                    int64_t local_build = status.substr(2).to_int();
                    if (version_build > local_build)
                        versions.push_back(version);
                }
                else if (version.build.begins_with("dev") && status.begins_with("dev"))
                {
                    //  Both are Dev builds
                    int64_t version_build = version.build.substr(3).to_int();
                    int64_t local_build = status.substr(3).to_int();
                    if (version_build > local_build)
                        versions.push_back(version);
                }
                else if (version.build.begins_with("stable"))
                {
                    // Tag is a stable release, local build is not stable
                    versions.push_back(version);
                }
                else if (version.build.begins_with("rc"))
                {
                    // Tag is a release candidate, local build is a dev build
                    versions.push_back(version);
                }
            }
            else if (version.patch >= 0 && VERSION_MAINTENANCE < version.patch)
            {
                // Patch outdated
                versions.push_back(version);
            }
        }
    }
    return versions;
}

void OrchestratorUpdater::_on_dialog_confirmed()
{
}

void OrchestratorUpdater::_on_start_download()
{
    _download_button->set_text("Downloading...");
    _download_button->set_disabled(true);
    _http_download_request->request(OrchestratorPlugin::get_singleton()->get_github_release_tag_url(_version.tag));
}

void OrchestratorUpdater::_on_show_release_notes()
{
    OS::get_singleton()->shell_open(OrchestratorPlugin::get_singleton()->get_github_release_notes_url(_version.tag));
}

void OrchestratorUpdater::_on_download_completed(int p_result, int p_code, const PackedStringArray& p_headers,
                                                 const PackedByteArray& p_data)
{
    // Handle if the request failed
    if (p_result != HTTPRequest::RESULT_SUCCESS || p_code != 200)
    {
        _update_available_dialog->hide();
        OS::get_singleton()->alert("Download request failed.", "Update failed.");
        return;
    }

    // Open target file to write contents to
    // Error if we cannot open the target file
    Ref<FileAccess> update = FileAccess::open("user://update.zip", FileAccess::WRITE);
    if (!update.is_valid())
    {
        _update_available_dialog->hide();
        OS::get_singleton()->alert("Unable to open temporary file.", "Update failed");
        return;
    }
    update->store_buffer(p_data);
    update->close();

    // Check if download was successful
    if (!FileAccess::file_exists("user://update.zip"))
    {
        _update_available_dialog->hide();
        OS::get_singleton()->alert("Update file cannot be found", "Update failed");
        return;
    }

    // Open downloaded zip file
    Ref<ZIPReader> reader = memnew(ZIPReader);
    if (reader->open("user://update.zip") != OK)
    {
        _update_available_dialog->hide();
        OS::get_singleton()->alert("Unable to read the downloaded plug-in ZIP file.", "Update failed");
        return;
    }

    // The addon does not remove any existing files, it only overrides files in "addons\orchestrator".
    // If users want a fresh installation, they should re-install the addon.
    const PackedStringArray files = reader->get_files();
    for (const String& file : files)
    {
        // Make sure the directory exists
        const String base_dir = file.get_base_dir();
        DirAccess::make_dir_recursive_absolute("res://" + base_dir);

        Ref<FileAccess> file_access = FileAccess::open("res://" + file, FileAccess::WRITE);
        if (file_access.is_valid() && file_access->is_open())
            file_access->store_buffer(reader->read_file(file));
    }
    reader->close();

    // Remove downloaded zip file
    DirAccess::remove_absolute("user://update.zip");

    // Hide dialog and request restart
    _update_available_dialog->hide();
    OrchestratorPlugin::get_singleton()->request_editor_restart();
}

