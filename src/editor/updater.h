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
#ifndef ORCHESTRATOR_UPDATER_H
#define ORCHESTRATOR_UPDATER_H

#include <godot_cpp/classes/h_box_container.hpp>
#include <vector>

using namespace godot;

/// Forward declarations
namespace godot
{
    class AcceptDialog;
    class Button;
    class Label;
    class HTTPRequest;
}

/// Displays an update button in the main view toolbar
class OrchestratorUpdater : public HBoxContainer
{
    GDCLASS(OrchestratorUpdater, HBoxContainer);

    static void _bind_methods() { }

    struct Version
    {
        const int64_t NOT_AVAILABLE{ -1 };

        String tag;
        String build;
        int64_t major{ NOT_AVAILABLE };
        int64_t minor{ NOT_AVAILABLE };
        int64_t patch{ NOT_AVAILABLE };
    };

    Version _version;                                   //! The latest version build
    Button* _button{ nullptr };                         //! The update button widget
    HTTPRequest* _http_update_request{ nullptr };       //! Responsible for checking for updates periodically.
    HTTPRequest* _http_download_request{ nullptr };     //! Responsible for downloading the update
    AcceptDialog* _update_available_dialog{ nullptr };  //! Update available dialog widget
    Label* _update_available_text{ nullptr };           //! Update available text widget
    Button* _download_button{ nullptr };                //! Download update button

protected:
    /// Show the update available dialog.
    void _show_update_dialog();

    /// Initiates the update check
    void _check_for_updates();

    /// Handles the HTTP request response when checking for updates
    /// @param p_result the HTTP result code
    /// @param p_code the HTTP status code
    /// @param p_headers the HTTP headers
    /// @param p_data the HTTP response data
    void _on_update_check_completed(int p_result, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_data);

    /// Dispatched when the user closes the update available dialog.
    void _on_dialog_confirmed();

    /// Dispatched when the user requests the update to be downloaded.
    void _on_start_download();

    /// Dispatched when the user selects to view release notes.
    /// Opens the release notes in the browser.
    void _on_show_release_notes();

    /// Dispatched when the HTTP download has finished
    /// @param p_result the HTTP result code
    /// @param p_code the HTTP status code
    /// @param p_headers the HTTP headers
    /// @param p_data the HTTP response data
    void _on_download_completed(int p_result, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_data);

    /// Gets all tags from the HTTP request response
    /// @param p_data the data to be parsed
    /// @return the repository's tags
    PackedStringArray _get_tags(const Array& p_data);

    /// All versions related to each tag
    /// @param p_tags array of all repository tags
    /// @return all parsed versions
    std::vector<Version> _parse_tag_versions(const PackedStringArray& p_tags);

    /// Gets the most recent build from the parsed versions
    /// @param p_versions the parsed versions
    /// @return list of all versions that come after the current library version
    std::vector<Version> _get_versions_after_current_build(const std::vector<Version>& p_versions);

public:

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);
};

#endif // ORCHESTRATOR_UPDATER_H