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
#ifndef ORCHESTRATOR_SCRIPT_SERIALIZATION_H
#define ORCHESTRATOR_SCRIPT_SERIALIZATION_H

#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

using namespace godot;

/// A ResourceFormatLoader that loads Orchestrator resource files using binary format
class OScriptBinaryResourceLoader : public ResourceFormatLoader
{
    GDCLASS(OScriptBinaryResourceLoader, ResourceFormatLoader);
    static void _bind_methods();

public:
    //~ Begin ResourceFormatLoader Interface
    PackedStringArray _get_recognized_extensions() const override;
    bool _recognize_path(const String& p_path, const StringName& type) const override;
    bool _handles_type(const StringName& p_type) const override;
    String _get_resource_type(const String& p_path) const override;
    String _get_resource_script_class(const String& p_path) const override;
    int64_t _get_resource_uid(const String& p_path) const override;
    PackedStringArray _get_dependencies(const String& p_path, bool p_add_types) const override;
    Error _rename_dependencies(const String& p_path, const Dictionary& p_renames) const override;
    bool _exists(const String& p_path) const override;
    PackedStringArray _get_classes_used(const String& p_path) const override;
    Variant _load(const String& p_path, const String& p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
    //~ End ResourceFormatLoader Interface
};

/// A ResourceFormatLoader that loads Orchestrator resource files using text format
class OScriptTextResourceLoader : public ResourceFormatLoader
{
    GDCLASS(OScriptTextResourceLoader, ResourceFormatLoader);
    static void _bind_methods();

public:
    //~ Begin ResourceFormatLoader Interface
    PackedStringArray _get_recognized_extensions() const override;
    bool _recognize_path(const String& p_path, const StringName& type) const override;
    bool _handles_type(const StringName& p_type) const override;
    String _get_resource_type(const String& p_path) const override;
    String _get_resource_script_class(const String& p_path) const override;
    int64_t _get_resource_uid(const String& p_path) const override;
    PackedStringArray _get_dependencies(const String& p_path, bool p_add_types) const override;
    Error _rename_dependencies(const String& p_path, const Dictionary& p_renames) const override;
    bool _exists(const String& p_path) const override;
    PackedStringArray _get_classes_used(const String& p_path) const override;
    Variant _load(const String& p_path, const String& p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
    //~ End ResourceFormatLoader Interface
};

/// A ResourceFormatSaver that saves a Orchestrator resource files using binary format
class OScriptBinaryResourceSaver : public ResourceFormatSaver
{
    GDCLASS(OScriptBinaryResourceSaver, ResourceFormatSaver);
    static void _bind_methods();

    String _get_local_path(const String& p_path) const;

public:
    //~ Begin ResourceFormatSaver Interface
    PackedStringArray _get_recognized_extensions(const Ref<Resource>& p_resource) const override;
    bool _recognize(const Ref<Resource>& p_resource) const override;
    Error _set_uid(const String& p_path, int64_t p_uid) override;
    bool _recognize_path(const Ref<Resource>& p_resource, const String& p_path) const override;
    Error _save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
    //~ End ResourceFormatSaver Interface
};

/// A ResourceFormatSaver that saves a Orchestrator resource files using text format
class OScriptTextResourceSaver : public ResourceFormatSaver
{
    GDCLASS(OScriptTextResourceSaver, ResourceFormatSaver);
    static void _bind_methods();

    String _get_local_path(const String& p_path) const;

public:
    //~ Begin ResourceFormatSaver Interface
    PackedStringArray _get_recognized_extensions(const Ref<Resource>& p_resource) const override;
    bool _recognize(const Ref<Resource>& p_resource) const override;
    Error _set_uid(const String& p_path, int64_t p_uid) override;
    bool _recognize_path(const Ref<Resource>& p_resource, const String& p_path) const override;
    Error _save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
    //~ End ResourceFormatSaver Interface

    Error save_to_buffer(const Ref<Resource>& p_resource);
};


#endif // ORCHESTRATOR_SCRIPT_SERIALIZATION_H