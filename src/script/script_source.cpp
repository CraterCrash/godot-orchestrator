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
#include "script/script_source.h"

#include "script/serialization/format_defs.h"

#include <godot_cpp/templates/hashfuncs.hpp>

bool OScriptSource::_is_path_text(const String& p_path) {
    return p_path.get_extension() == ORCHESTRATOR_SCRIPT_TEXT_EXTENSION;
}

int64_t OScriptSource::hash() const {
    switch (_type) {
        case TEXT: {
            return _source.hash();
        }
        case BINARY: {
            return hash_djb2_buffer(_binary_source.ptr(), _binary_source.size());
        }
        default: {
            return 0;
        }
    }
}

OScriptSource OScriptSource::load(const String& p_path) {
    const Ref<FileAccess> file = open(p_path, FileAccess::READ);
    if (!file.is_valid()) {
        return OScriptSource(p_path, FileAccess::get_open_error());
    }

    if (_is_path_text(p_path)) {
        const String source = file->get_as_text();
        return OScriptSource(TEXT, source, p_path);
    }

    const PackedByteArray _binary_source = file->get_buffer(file->get_length());
    return OScriptSource(BINARY, _binary_source, p_path);
}

Error OScriptSource::save(const OScriptSource& p_source) {
    const String& path = p_source.get_path();

    const Ref<FileAccess> file = open(path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_FILE_CANT_WRITE, "Cannot write to file '" + path + "'.");

    switch (p_source.get_type()) {
        case TEXT: {
            file->store_string(p_source.get_source());
            break;
        }
        case BINARY: {
            file->store_buffer(p_source.get_binary_source());
            break;
        }
        default: {
            ERR_FAIL_V_MSG(ERR_FILE_CANT_WRITE, "An unexpected source _type cannot be saved.");
        }
    }

    return file->get_error();
}

bool OScriptSource::operator==(const OScriptSource& p_other) const {
    return _type == p_other._type &&
        _path == p_other._path &&
        _source == p_other._source &&
        _binary_source == p_other._binary_source;
}

Ref<FileAccess> OScriptSource::open(const String& p_path, FileAccess::ModeFlags p_flags) {
    if (!_is_path_text(p_path)) {
        // Binary format is compressed
        return FileAccess::open_compressed(p_path, p_flags);
    }

    return FileAccess::open(p_path, p_flags);
}

OScriptSource::OScriptSource(const String& p_path, Error p_load_error) {
    _type = UNKNOWN;
    _path = p_path;
    _load_error = p_load_error;
}

OScriptSource::OScriptSource(Type p_type, const String& p_source, const String& p_path) {
    _type = p_type;
    _source = p_source;
    _path = p_path;
}

OScriptSource::OScriptSource(Type p_type, const PackedByteArray& p__binary_source, const String& p_path) {
    _type = p_type;
    _binary_source = p__binary_source;
    _path = p_path;
}
