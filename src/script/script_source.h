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
#ifndef ORCHESTRATOR_SCRIPT_SOURCE_H
#define ORCHESTRATOR_SCRIPT_SOURCE_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// An immutable class that maintains a reference to the type of source code, its original file path, and
/// it's contents based on its type. Note that type is resolved based on file extension.
class OScriptSource {
public:
    enum Type {
        UNKNOWN,
        TEXT,
        BINARY
    };

private:
    Type _type = UNKNOWN;
    String _path;
    String _source;
    PackedByteArray _binary_source;
    Error _load_error = OK;

    static bool _is_path_text(const String& p_path);

public:

    bool is_valid() const { return _type != UNKNOWN; }

    Type get_type() const { return _type; }
    const String& get_path() const { return _path; }
    const String& get_source() const { return _source; }
    const PackedByteArray& get_binary_source() const { return _binary_source; }
    Error get_load_error() const { return _load_error; }

    int64_t hash() const;

    static OScriptSource load(const String& p_path);
    static Error save(const OScriptSource& p_source);
    static Ref<FileAccess> open(const String& p_path, FileAccess::ModeFlags p_flags);

    bool operator==(const OScriptSource& p_other) const;

    explicit OScriptSource() = default;
    explicit OScriptSource(const String& p_path, Error p_load_error);
    explicit OScriptSource(Type p_type, const String& p_source, const String& p_path);
    explicit OScriptSource(Type p_type, const PackedByteArray& p_binary_source, const String& p_path);
};

#endif // ORCHESTRATOR_SCRIPT_SOURCE_H