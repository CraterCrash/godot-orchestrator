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
#ifndef ORCHESTRATOR_UPDATER_H
#define ORCHESTRATOR_UPDATER_H

#include "godot_cpp/templates/hash_map.hpp"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/progress_bar.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

struct OrchestratorVersion
{
    struct Build
    {
        String name;
        int64_t version{ 0 };

        static Build parse(const String& p_build);

        String to_string() const;
    };

public:
    int64_t major{ 0 };
    int64_t minor{ 0 };
    int64_t patch{ 0 };
    Build build;

    /// Parses a specific version tag name
    /// @param p_tag_version the tag version to parse
    /// @return an instance of a version struct
    static OrchestratorVersion parse(const String& p_tag_version);

    /// Returns whether this version is after the supplied version.
    /// @param p_other the version this is expected to be after
    /// @return true if this is after the specified version, false otherwise
    bool is_after(const OrchestratorVersion& p_other) const;

    /// Returns whether this version is equal to the supplied version.
    /// @param p_other the version this is expected to match
    /// @return true if these are equal
    bool is_equal(const OrchestratorVersion& p_other) const;

    bool is_compatible(const OrchestratorVersion& p_other) const;

    /// Converts version to a string
    /// @return the version formatted as a string
    String to_string() const;
};

/// A manifest record for outlining Godot and Orchestrator compatibility
struct OrchestratorReleaseManifest
{
    String name;                //! Orchestrator release names
    String godot_compatibility; //! Godot's compatibility expectations
    String blog_url;            //! An optional blog url
};

/// Represents a release that is available for download
struct OrchestratorRelease
{
    String tag;                 //! The release tag
    String release_url;         //! Link to the HTML releases page on GitHub
    String plugin_asset_url;    //! The plugin asset download URL
    String body;                //! The release notes
    bool draft;                 //! Whether release is draft
    bool prerelease;            //! Whether release is a pre-release
    int64_t asset_size{ 0 };    //! Size of download asset in bytes
    String published;           //! Date/Time published
};

/// Update release notes dialog
class OrchestratorUpdaterReleaseNotesDialog : public AcceptDialog
{
    GDCLASS(OrchestratorUpdaterReleaseNotesDialog, AcceptDialog);
    static void _bind_methods() { }

protected:
    RichTextLabel* _text{ nullptr };

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    /// Set the release notes text
    /// @param p_text the release notes text
    void set_text(const String& p_text) { _text->clear(); _text->append_text(p_text); }

    OrchestratorUpdaterReleaseNotesDialog();
};

/// Update dialog picker
class OrchestratorUpdaterVersionPicker : public ConfirmationDialog
{
    GDCLASS(OrchestratorUpdaterVersionPicker, ConfirmationDialog);
    static void _bind_methods();

    struct ReleaseItem
    {
        OrchestratorRelease release;
        String godot_compatibility;
        String blog_url;
    };

protected:
    Vector<ReleaseItem> _releases;
    OrchestratorVersion _godot_version;
    Tree* _tree{ nullptr };
    Button* _show_release_notes{ nullptr };
    ProgressBar* _progress{ nullptr };
    Label* _status{ nullptr };
    HTTPRequest* _download{ nullptr };
    OptionButton* _release_filter{ nullptr };
    CheckBox* _notify_any_release{ nullptr };

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _set_button_enable_state(bool p_enabled);
    void _check_godot_compatibility();
    void _request_download();
    void _handle_custom_action(const StringName& p_action);
    void _download_completed(int p_status, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_data);
    void _restart_editor();
    void _install();
    void _cancel_and_close();
    void _filter_changed(int p_index);
    void _update_tree(bool p_stable_only = false);
    void _update_notify_settings();

public:
    /// Updates the tree
    void update_tree();

    /// Clear all releases
    void clear_releases();

    /// Add a release to the picker
    /// @param p_release the release
    /// @param p_godot_compatibility the release's Godot compatibility
    /// @param p_blog_url optional blog url, will default to GitHub release page if empty
    void add_release(const OrchestratorRelease& p_release, const String& p_godot_compatibility, const String& p_blog_url = String());

    OrchestratorUpdaterVersionPicker();
};

/// Displays an update button in the main view toolbar
class OrchestratorUpdaterButton : public HBoxContainer
{
    GDCLASS(OrchestratorUpdaterButton, HBoxContainer);

    static void _bind_methods() { }

    OrchestratorVersion _plugin_version;
    Vector<OrchestratorRelease> _releases;                      //! Collection of releases
    HashMap<String, OrchestratorReleaseManifest> _manifests;    // Map of manifests
    OrchestratorUpdaterVersionPicker* _picker{ nullptr };   //! Version picker
    Variant _releases_data;
    Variant _manifest_data;
    Button* _button{ nullptr };                             //! The update button widget

protected:
    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    /// Sends an HTTP request to the specified URL
    /// @param p_url the URL to call
    /// @param p_file_name the destination file that will receive the content
    /// @param p_callback the callback to fire when the download completes
    /// @return the error code if the request failed
    Error _send_http_request(const String& p_url, const String& p_file_name, const Callable& p_callback);

    /// Handle processing of the release manifest data
    void _process_release_manifests();

    /// Handle processing of the release data
    void _process_releases();

    /// Update the version picker with the details
    void _update_picker();

    /// Show the update available dialog.
    void _show_update_dialog();

    /// Initiates the update check
    void _check_for_updates();

public:
    OrchestratorUpdaterButton();
};

#endif // ORCHESTRATOR_UPDATER_H