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
#include "common/file_utils.h"

#include "common/macros.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace FileUtils {
    namespace {
        /// Maps a ConfigFile backed cache written by earlier Orchestrator versions directly into the
        /// Godot project settings directory onto its name within the dedicated cache directory.
        struct LegacyCacheFile {
            const char* legacy_name;
            const char* name;
        };

        constexpr LegacyCacheFile LEGACY_CACHE_FILES[] = {
            { "orchestrator_editor_cache.cfg", "editor_cache.cfg" },
            { "orchestrator_metadata.cfg", "metadata.cfg" }
        };

        /// Maps a line based cache file name prefix written by earlier Orchestrator versions onto the
        /// metadata section that now holds it. The text trailing the prefix was the caller supplied
        /// suffix and becomes the key within that section. Action menu caches store a record per
        /// entry rather than a single value, so they decode differently.
        struct LegacyCacheList {
            const char* legacy_prefix;
            const char* section;
            bool menu_records;
        };

        constexpr LegacyCacheList LEGACY_CACHE_LISTS[] = {
            { "orchestrator_menu_favorites.", "action_favorites", true },
            { "orchestrator_menu_recents.", "action_recents", true },
            { "orchestrator_recent_history.", "search_recent_history", false },
            { "orchestrator_favorites.", "search_favorites", false }
        };

        /// Returns the directory that holds all Orchestrator editor cache files, creating it when
        /// it does not yet exist.
        String _get_editor_cache_dir() {
            const String path = EI->get_editor_paths()->get_project_settings_dir().path_join("orchestrator");
            if (!DirAccess::dir_exists_absolute(path)) {
                DirAccess::make_dir_recursive_absolute(path);
            }
            return path;
        }

        /// Returns whether the text trailing a legacy cache file prefix is a suffix that Orchestrator
        /// wrote. Suffixes are always plain identifiers, which separates our files from Godot's own
        /// <code>&lt;file&gt;-folding-&lt;md5&gt;.cfg</code> caches that can share the same prefix.
        bool _is_cache_file_suffix(const String& p_suffix) {
            return !p_suffix.contains(".") && !p_suffix.contains("-");
        }

        /// Reads a legacy line based cache, returning each unique non-empty line in file order.
        PackedStringArray _read_legacy_values(const Ref<FileAccess>& p_file) {
            PackedStringArray values;
            for_each_line(p_file, [&](const String& line) {
                if (const String trimmed = line.strip_edges(); !trimmed.is_empty()) {
                    if (!values.has(trimmed)) {
                        values.push_back(trimmed);
                    }
                }
            });
            return values;
        }

        /// Reads a legacy action menu cache. The format is a positive integer format version, a blank
        /// line, then a four line record per entry holding the action, its description, its icon and a
        /// blank separator.
        Array _read_legacy_menu_records(const Ref<FileAccess>& p_file) {
            PackedStringArray lines;
            for_each_line(p_file, [&](const String& line) {
                lines.push_back(line.strip_edges());
            });

            Array records;
            if (lines.is_empty() || !lines[0].is_valid_int() || lines[0].to_int() <= 0) {
                return records;
            }

            for (int index = 2; (index + 2) < lines.size(); index += 4) {
                Dictionary record;
                record["action"] = lines[index];
                record["description"] = lines[index + 1];
                record["icon"] = lines[index + 2];
                records.push_back(record);
            }
            return records;
        }

        /// Moves the ConfigFile backed caches into the dedicated cache directory. These must be in
        /// place before the line based caches are folded into the metadata file.
        void _migrate_cache_files(const String& p_legacy_path, const String& p_cache_path) {
            for (const LegacyCacheFile& legacy : LEGACY_CACHE_FILES) {
                const String source = p_legacy_path.path_join(legacy.legacy_name);
                const String target = p_cache_path.path_join(legacy.name);

                if (!FileAccess::file_exists(source) || FileAccess::file_exists(target)) {
                    continue;
                }

                DirAccess::rename_absolute(source, target);
            }
        }

        /// Folds the line based favorite and history caches into sections of the metadata file,
        /// discarding each legacy file once it has been read.
        void _migrate_cache_lists(const String& p_legacy_path, const String& p_metadata_path) {
            Ref<ConfigFile> metadata(memnew(ConfigFile));
            metadata->load(p_metadata_path);

            bool changed = false;
            for (const String& file_name : DirAccess::get_files_at(p_legacy_path)) {
                for (const LegacyCacheList& legacy : LEGACY_CACHE_LISTS) {
                    if (!file_name.begins_with(legacy.legacy_prefix)) {
                        continue;
                    }

                    const String suffix = file_name.substr(String(legacy.legacy_prefix).length());
                    if (!_is_cache_file_suffix(suffix)) {
                        break;
                    }

                    const String source = p_legacy_path.path_join(file_name);

                    // A cache written without a suffix is unreachable because every dialog and menu
                    // supplies one, so it is discarded rather than stored under an empty key. An
                    // already migrated section wins, which keeps a repeated run from undoing edits.
                    if (!suffix.is_empty() && !metadata->has_section_key(legacy.section, suffix)) {
                        const Ref<FileAccess> file = FileAccess::open(source, FileAccess::READ);
                        if (legacy.menu_records) {
                            if (const Array records = _read_legacy_menu_records(file); !records.is_empty()) {
                                metadata->set_value(legacy.section, suffix, records);
                                changed = true;
                            }
                        } else {
                            if (const PackedStringArray values = _read_legacy_values(file); !values.is_empty()) {
                                metadata->set_value(legacy.section, suffix, values);
                                changed = true;
                            }
                        }
                    }

                    DirAccess::remove_absolute(source);
                    break;
                }
            }

            if (changed) {
                metadata->save(p_metadata_path);
            }
        }
    }

    String get_editor_cache_file(const String& p_file_name) {
        return _get_editor_cache_dir().path_join(p_file_name);
    }

    void migrate_editor_cache_files() {
        const String legacy_path = EI->get_editor_paths()->get_project_settings_dir();
        const String cache_path = _get_editor_cache_dir();

        _migrate_cache_files(legacy_path, cache_path);
        _migrate_cache_lists(legacy_path, cache_path.path_join("metadata.cfg"));
    }

    void for_each_line(const Ref<FileAccess>& p_file, const std::function<void(const String&)>& p_callback) {
        if (p_file.is_valid() && p_file->is_open()) {
            while (!p_file->eof_reached()) {
                p_callback(p_file->get_line());
            }
        }
    }
}
