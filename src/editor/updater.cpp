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
#include "editor/updater.h"

#include "common/callable_lambda.h"
#include "common/godot_version.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/link_button.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

OrchestratorVersion::Build OrchestratorVersion::Build::parse(const String& p_build)
{
    int pos = 0;
    while (pos < p_build.length() && !String::chr(p_build[pos]).is_valid_int())
        pos++;

    Build build;
    build.name = p_build.substr(0, pos);
    build.version = p_build.substr(pos).to_int();
    return build;
}

String OrchestratorVersion::Build::to_string() const
{
    if (version == 0)
        return name;

    return vformat("%s%d", name, version);
}

OrchestratorVersion OrchestratorVersion::parse(const String& p_tag_version)
{
    OrchestratorVersion version;

    const PackedStringArray parts = (p_tag_version.begins_with("v")
        ? p_tag_version.substr(1).split(".")
        : p_tag_version.split("."));

    if (parts.size() >= 1 && parts[0].is_valid_int())
        version.major = parts[0].to_int();

    if (parts.size() >= 2 && parts[1].is_valid_int())
        version.minor = parts[1].to_int();

    if (parts.size() >= 3 && parts[2].is_valid_int())
        version.patch = parts[2].to_int();
    else if (parts.size() >= 3)
        version.build = Build::parse(parts[2]);

    if (parts.size() >= 4)
        version.build = Build::parse(parts[3]);

    return version;
}

bool OrchestratorVersion::is_after(const OrchestratorVersion& p_other) const
{
    // List of builds that are in "release" order, i.e. stable comes first, development last, etc.
    static PackedStringArray builds = Array::make("stable", "rc", "dev");

    // This major version is after the other
    if (major > p_other.major)
        return true;

    // This minor version is after the other
    if (major == p_other.major && minor > p_other.minor)
        return true;

    // This patch version is after the other
    if (major == p_other.major && minor == p_other.minor && patch > p_other.patch)
        return true;

    if (major == p_other.major && minor == p_other.minor && patch == p_other.patch)
    {
        const int64_t build_name_index = builds.find(build.name);
        const int64_t other_build_name_index = builds.find(p_other.build.name);
        if (build_name_index < other_build_name_index)
            return true;

        if (build_name_index == other_build_name_index && build.version > p_other.build.version)
            return true;
    }

    return false;
}

bool OrchestratorVersion::is_equal(const OrchestratorVersion& p_other) const
{
    return major == p_other.major
        && minor == p_other.minor
        && patch == p_other.patch
        && build.name == p_other.build.name
        && build.version == p_other.build.version;
}

bool OrchestratorVersion::is_compatible(const OrchestratorVersion& p_other) const
{
    // Currently used by passing Compat and checking against Godot Version argument
    if (p_other.major >= major && p_other.minor >= minor && p_other.patch >= patch)
        return true;

    return false;
}

String OrchestratorVersion::to_string() const
{
    return vformat("%d.%d.%d.%s", major, minor, patch, build.to_string());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorUpdaterReleaseNotesDialog::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        connect("canceled", callable_mp_lambda(this, [this] { queue_free(); }));
        connect("confirmed", callable_mp_lambda(this, [this] { queue_free(); }));
    }
}

OrchestratorUpdaterReleaseNotesDialog::OrchestratorUpdaterReleaseNotesDialog()
{
    set_title("Release Notes");

    _text = memnew(RichTextLabel);
    _text->set_use_bbcode(true);
    _text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(_text);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorUpdaterVersionPicker::_set_button_enable_state(bool p_enabled)
{
    get_ok_button()->set_disabled(!p_enabled);
    _show_release_notes->set_disabled(!p_enabled);
}

void OrchestratorUpdaterVersionPicker::_check_godot_compatibility()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        if (!selected->get_meta("compatible", false))
        {
            AcceptDialog* notify = memnew(AcceptDialog);
            notify->set_title("Godot version incompatible");
            notify->set_text("Your current version of Godot is incompatible. Please update your editor first.");
            add_child(notify);
            notify->connect("canceled", callable_mp_lambda(this, [notify] { notify->queue_free(); }));
            notify->connect("confirmed", callable_mp_lambda(this, [notify] { notify->queue_free(); }));
            notify->popup_centered();
            return;
        }

        _request_download();
    }
}

void OrchestratorUpdaterVersionPicker::_request_download()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        get_ok_button()->release_focus();
        _set_button_enable_state(false);
        _tree->deselect_all();

        const String download_url = selected->get_meta("download_url");
        if (_download->request(download_url) == OK)
        {
            #if GODOT_VERSION >= 0x040300
            _progress->set_indeterminate(true);
            #endif
            set_process(true);
        }
    }
}

void OrchestratorUpdaterVersionPicker::_handle_custom_action(const StringName& p_action)
{
    if (p_action.match("show_release_notes"))
    {
        OS::get_singleton()->shell_open(_tree->get_selected()->get_meta("release_url"));
        // OrchestratorUpdaterReleaseNotesDialog* dialog = memnew(OrchestratorUpdaterReleaseNotesDialog);
        // dialog->set_text(_tree->get_selected()->get_meta("release_notes"));
        // dialog->set_exclusive(true);
        // dialog->set_transient(true);
        // dialog->popup_exclusive_centered_ratio(this, 0.3);
    }
}

void OrchestratorUpdaterVersionPicker::_download_completed(int p_status, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_data)
{
    _progress->set_visible(false);
    #if GODOT_VERSION >= 0x040300
    _progress->set_indeterminate(false);
    #endif

    set_process(false);

    if (p_code != 200)
    {
        _status->set_text(vformat("Failed: %d", p_code));
        return;
    }

    _install();
}

void OrchestratorUpdaterVersionPicker::_restart_editor()
{
    EditorInterface::get_singleton()->restart_editor(true);
}

void OrchestratorUpdaterVersionPicker::_install()
{
    _status->set_text("Installing, please wait...");
    const String file_name = _download->get_download_file();

    // Open downloaded zip file
    Ref<ZIPReader> reader = memnew(ZIPReader);
    if (reader->open(file_name) != OK)
    {
        _status->set_visible(false);
        get_ok_button()->set_disabled(false);

        OS::get_singleton()->alert("Unable to read the downloaded plug-in file.", "Update failed");
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

    _status->set_visible(false);

    AcceptDialog* dialog = memnew(AcceptDialog);
    dialog->set_title("Update Installed");
    dialog->set_text("Update installed, editor requires a restart.");
    dialog->set_ok_button_text("Restart");
    add_child(dialog);

    dialog->connect("confirmed", callable_mp_lambda(this, [this, dialog] {
        dialog->queue_free();

        Timer* timer = memnew(Timer);
        timer->set_one_shot(true);
        timer->set_wait_time(0.5f);
        timer->set_autostart(true);
        timer->connect("timeout", callable_mp(this, &OrchestratorUpdaterVersionPicker::_restart_editor));
        add_child(timer);
    }));

    dialog->popup_centered();
}

void OrchestratorUpdaterVersionPicker::_cancel_and_close()
{
    if (_download)
    {
        const int status = _download->get_http_client_status();
        if (status == HTTPClient::STATUS_BODY)
        {
            set_process(false);

            _download->cancel_request();

            #if GODOT_VERSION >= 0x040300
            _progress->set_indeterminate(false);
            #endif

            _progress->set_visible(false);
            _status->set_visible(false);
        }
    }

    hide();
}

void OrchestratorUpdaterVersionPicker::_filter_changed(int p_index)
{
    _update_tree(p_index == 1);
}

void OrchestratorUpdaterVersionPicker::_update_tree(bool p_stable_only)
{
    _tree->clear();
    _tree->create_item();
    for (const ReleaseItem& release_item : _releases)
    {
        if (p_stable_only)
        {
            OrchestratorVersion tag_version = OrchestratorVersion::parse(release_item.release.tag);
            if (!tag_version.build.name.match("stable"))
                continue;
        }

        int64_t unix_time = Time::get_singleton()->get_unix_time_from_datetime_string(release_item.release.published);

        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, release_item.release.tag);
        item->set_text(1, release_item.godot_compatibility);
        item->set_text(2, vformat("%s", release_item.release.prerelease ? "Yes" : "No"));
        item->set_text(3, Time::get_singleton()->get_datetime_string_from_unix_time(unix_time, true));
        item->set_text(4, String::humanize_size(release_item.release.asset_size));

        item->set_meta("download_url", release_item.release.plugin_asset_url);

        const String release_url = StringUtils::default_if_empty(release_item.blog_url, release_item.release.release_url);
        item->set_meta("release_url", release_url);

        const OrchestratorVersion compat_version = OrchestratorVersion::parse(release_item.godot_compatibility);
        if (!compat_version.is_compatible(_godot_version))
        {
            item->add_button(0, SceneUtils::get_editor_icon("KeyXScale"), -1, true, "Your Godot version is not compatible");
            item->set_meta("compatible", false);
        }
        else
        {
            item->add_button(0, SceneUtils::get_editor_icon("KeyCall"));
            item->set_meta("compatible", true);
        }
    }
}

void OrchestratorUpdaterVersionPicker::_update_notify_settings()
{
    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    settings->set_notify_prerelease_builds(_notify_any_release->is_pressed());
}

void OrchestratorUpdaterVersionPicker::update_tree()
{
    _update_tree(_release_filter->get_selected() == 1);
}

void OrchestratorUpdaterVersionPicker::clear_releases()
{
    _set_button_enable_state(false);
    _releases.clear();
}

void OrchestratorUpdaterVersionPicker::add_release(const OrchestratorRelease& p_release, const String& p_godot_compatibility, const String& p_blog_url)
{
    ReleaseItem item;
    item.release = p_release;
    item.godot_compatibility = p_godot_compatibility;
    item.blog_url = p_blog_url;
    _releases.push_back(item);
}

void OrchestratorUpdaterVersionPicker::_notification(int p_what)
{
    if (p_what == NOTIFICATION_VISIBILITY_CHANGED)
    {
        if (is_visible())
        {
            OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
            _notify_any_release->set_pressed_no_signal(settings->is_notify_about_prereleases());

            _update_tree();

            _tree->deselect_all();

            get_ok_button()->release_focus();
            _set_button_enable_state(false);

            _progress->set_value_no_signal(0);
        }
    }
    else if (p_what == NOTIFICATION_READY)
    {
        _download->connect("request_completed", callable_mp(this, &OrchestratorUpdaterVersionPicker::_download_completed));
        _release_filter->connect("item_selected", callable_mp(this, &OrchestratorUpdaterVersionPicker::_filter_changed));
        _notify_any_release->connect("pressed", callable_mp(this, &OrchestratorUpdaterVersionPicker::_update_notify_settings));
        _tree->connect("item_activated", callable_mp(this, &OrchestratorUpdaterVersionPicker::_check_godot_compatibility));
        _tree->connect("item_selected", callable_mp(this, &OrchestratorUpdaterVersionPicker::_set_button_enable_state).bind(true));

        connect("custom_action", callable_mp(this, &OrchestratorUpdaterVersionPicker::_handle_custom_action));
        connect("confirmed", callable_mp(this, &OrchestratorUpdaterVersionPicker::_check_godot_compatibility));
        connect("canceled", callable_mp(this, &OrchestratorUpdaterVersionPicker::_cancel_and_close));
    }
    else if (p_what == NOTIFICATION_PROCESS)
    {
        // Make the progress bar visible again when retrying the download.
        _progress->set_visible(true);
        _status->set_visible(true);

        if (_download->get_downloaded_bytes() > 0)
        {
            _progress->set_max(_download->get_body_size());
            _progress->set_value(_download->get_downloaded_bytes());
        }

        int client_status = _download->get_http_client_status();
        if (client_status == HTTPClient::STATUS_BODY)
        {
            if (_download->get_body_size() > 0)
            {
                #if GODOT_VERSION >= 0x040300
                _progress->set_indeterminate(false);
                #endif
                _status->set_text(vformat("Downloading (%s / %s)...",
                    String::humanize_size(_download->get_downloaded_bytes()),
                    String::humanize_size(_download->get_body_size())));
            }
            else
            {
                #if GODOT_VERSION >= 0x040300
                _progress->set_indeterminate(true);
                #endif
                _status->set_text(vformat("Downloading... (%s)",
                    String::humanize_size(_download->get_downloaded_bytes())));
            }
        }
    }
}

void OrchestratorUpdaterVersionPicker::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("install_completed"));
}

OrchestratorUpdaterVersionPicker::OrchestratorUpdaterVersionPicker()
{
    GodotVersionInfo gd_version;

    // Generate the editor's current version
    // Used in compatibility checks
    _godot_version = OrchestratorVersion::parse(vformat(
        "v%d.%d.%d", gd_version.major(), gd_version.minor(), gd_version.patch()));

    set_title("Select Version");

    set_ok_button_text("Download & Install");
    set_cancel_button_text("Close");
    set_hide_on_ok(false);

    _show_release_notes = add_button("Show Release Notes", false, "show_release_notes");

    VBoxContainer* vbox = memnew(VBoxContainer);
    add_child(vbox);

    HBoxContainer* hbox = memnew(HBoxContainer);
    vbox->add_child(hbox);

    _release_filter = memnew(OptionButton);
    _release_filter->add_item("All releases");
    _release_filter->add_item("Stable only");
    SceneUtils::add_margin_child(hbox, "Filter:", _release_filter);

    Label* spacer = memnew(Label);
    spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    hbox->add_child(spacer);

    _notify_any_release = memnew(CheckBox);
    _notify_any_release->set_text("Notify about pre-release versions");
    _notify_any_release->set_focus_mode(Control::FOCUS_NONE);
    hbox->add_child(_notify_any_release);

    _tree = memnew(Tree);
    _tree->set_hide_root(true);
    _tree->set_select_mode(Tree::SELECT_ROW);
    _tree->set_columns(5);
    _tree->set_column_titles_visible(true);
    _tree->set_column_title(0, "Version");
    _tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
    _tree->set_column_title(1, "Godot Compatibility");
    _tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_LEFT);
    _tree->set_column_title(2, "Pre-release");
    _tree->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_LEFT);
    _tree->set_column_title(3, "Published");
    _tree->set_column_title_alignment(3, HORIZONTAL_ALIGNMENT_LEFT);
    _tree->set_column_title(4, "Size");
    _tree->set_column_title_alignment(4, HORIZONTAL_ALIGNMENT_LEFT);
    _tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    vbox->add_child(_tree);

    _progress = memnew(ProgressBar);
    _progress->set_visible(false);
    vbox->add_child(_progress);

    _status = memnew(Label);
    _status->set_visible(false);
    vbox->add_child(_status);

    _download = memnew(HTTPRequest);
    const String cache_dir = EditorInterface::get_singleton()->get_editor_paths()->get_cache_dir();
    _download->set_download_file(cache_dir.path_join("tmp_orchestrator_update.zip"));
    add_child(_download);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Error OrchestratorUpdaterButton::_send_http_request(const String& p_url, const String& p_filename, const Callable& p_callback)
{
    // Windows: /users/<user>/AppData/Local/Godot/<filename>

    // Creates HTTP request and adds to the scene
    HTTPRequest* request = memnew(HTTPRequest);
    request->set_download_file(p_filename);
    add_child(request);

    // Setup callback for when request receives response
    request->connect(
        "request_completed",
        callable_mp_lambda(this, [=] (int p_result, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_data) {
            if (p_result == HTTPRequest::RESULT_SUCCESS && p_code == 200)
                p_callback.call();

            // Queue HTTPRequest to remove from scene
            request->queue_free();
        }),
        CONNECT_ONE_SHOT);

    const Error error = request->request(p_url);
    if (error != OK)
        request->queue_free();

    return error;
}

void OrchestratorUpdaterButton::_process_release_manifests()
{
    _manifests.clear();

    const String cache_dir = EditorInterface::get_singleton()->get_editor_paths()->get_cache_dir();
    const PackedByteArray bytes = FileAccess::get_file_as_bytes(cache_dir.path_join("tmp_orchestrator_release_manifests.json"));
    const Array data = JSON::parse_string(bytes.get_string_from_utf8());

    if (data.size() == 0)
        return;

    for (int index = 0; index < data.size(); ++index)
    {
        const Dictionary& release = data[index];
        if (release.has("version") && release.has("godot_compatibility"))
        {
            OrchestratorReleaseManifest manifest;
            manifest.name = release["version"];
            manifest.godot_compatibility = release["godot_compatibility"];

            if (release.has("blog_url"))
                manifest.blog_url = release["blog_url"];

            if (manifest.name.is_empty() || manifest.godot_compatibility.is_empty())
                continue;

            _manifests[manifest.name] = manifest;
        }
    }

    if (!_manifests.is_empty() && !_releases.is_empty())
        _update_picker();
}

void OrchestratorUpdaterButton::_process_releases()
{
    _releases.clear();

    const String cache_dir = EditorInterface::get_singleton()->get_editor_paths()->get_cache_dir();
    const PackedByteArray bytes = FileAccess::get_file_as_bytes(cache_dir.path_join("tmp_orchestrator_releases.json"));
    const Array data = JSON::parse_string(bytes.get_string_from_utf8());

    if (data.size() == 0)
        return;

    for (int index = 0; index < data.size(); ++index)
    {
        const Dictionary& published_release = data[index];

        OrchestratorRelease release;
        release.tag = published_release["tag_name"];
        release.release_url = published_release["html_url"];
        release.body = published_release["body"];
        release.draft = published_release["draft"];
        release.prerelease = published_release["prerelease"];
        release.published = published_release["published_at"];

        const Array assets = published_release["assets"];
        if (!assets.is_empty())
        {
            for (int asset_index = 0; asset_index < assets.size(); ++asset_index)
            {
                const Dictionary& asset_release = assets[asset_index];
                if (asset_release.has("browser_download_url")
                    && String(asset_release["browser_download_url"]).ends_with("-plugin.zip"))
                {
                    release.plugin_asset_url = asset_release["browser_download_url"];
                    release.asset_size = asset_release["size"];
                    break;
                }
            }
        }

        // If it has no download artifact, skip it
        if (release.plugin_asset_url.is_empty())
            continue;

        if (!OrchestratorVersion::parse(release.tag).is_after(_plugin_version))
            continue;

        _releases.push_back(release);
    }

    if (!_manifests.is_empty() && !_releases.is_empty())
        _update_picker();
}

void OrchestratorUpdaterButton::_update_picker()
{
    if (_releases.is_empty() || _manifests.is_empty())
        return;

    OrchestratorSettings* settings=  OrchestratorSettings::get_singleton();
    const bool notify_pre_releases = settings->is_notify_about_prereleases();

    _picker->clear_releases();

    bool releases_added = false;
    for (const OrchestratorRelease& release : _releases)
    {
        // If the release is marked as Draft or Prerelease+NoNotify from GitHub, skip.
        if (release.draft || (release.prerelease && !notify_pre_releases))
            continue;

        // In case a dev/rc build is not marked pre-release but the user wants only stable releases,
        // check the build name and filter as a last resort.
        OrchestratorVersion version = OrchestratorVersion::parse(release.tag);
        if(!version.build.name.match("stable") && !notify_pre_releases)
            continue;

        if (!_manifests.has(release.tag))
            continue;

        OrchestratorReleaseManifest manifest = _manifests.get(release.tag);
        _picker->add_release(release, manifest.godot_compatibility, manifest.blog_url);

        releases_added = true;
    }

    set_visible(releases_added);

    if (releases_added)
        _button->set_text("An update is available!");

    if (_picker->is_visible())
        _picker->update_tree();
}

void OrchestratorUpdaterButton::_show_update_dialog()
{
    _picker->popup_centered_ratio(0.4);
}

void OrchestratorUpdaterButton::_check_for_updates()
{
    const String cache_dir = EditorInterface::get_singleton()->get_editor_paths()->get_cache_dir();

    _send_http_request(
        VERSION_RELEASES_URL,
        cache_dir.path_join("tmp_orchestrator_releases.json"),
        callable_mp(this, &OrchestratorUpdaterButton::_process_releases));

    _send_http_request(
        VERSION_MANIFESTS_URL,
        cache_dir.path_join("tmp_orchestrator_release_manifests.json"),
        callable_mp(this, &OrchestratorUpdaterButton::_process_release_manifests));
}

void OrchestratorUpdaterButton::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        set_visible(false);

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

        _picker = memnew(OrchestratorUpdaterVersionPicker);
        add_child(_picker);

        _check_for_updates();

        timer->connect("timeout", callable_mp(this, &OrchestratorUpdaterButton::_check_for_updates));
        _button->connect("pressed", callable_mp(this, &OrchestratorUpdaterButton::_show_update_dialog));

        ProjectSettings* settings = ProjectSettings::get_singleton();
        settings->connect("settings_changed", callable_mp(this, &OrchestratorUpdaterButton::_update_picker));
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        ProjectSettings* settings = ProjectSettings::get_singleton();
        settings->disconnect("settings_changed", callable_mp(this, &OrchestratorUpdaterButton::_update_picker));

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

OrchestratorUpdaterButton::OrchestratorUpdaterButton()
{
    // Generate current plugin version
    // Used in resolving what patches exist
    _plugin_version = OrchestratorVersion::parse(vformat(
        "v%d.%d.%d.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MAINTENANCE, VERSION_STATUS));
}