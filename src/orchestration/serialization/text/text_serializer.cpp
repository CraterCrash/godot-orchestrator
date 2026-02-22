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
#include "orchestration/serialization/text/text_serializer.h"

#include "common/dictionary_utils.h"
#include "common/string_utils.h"
#include "core/godot/object/class_db.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/format.h"
#include "orchestration/serialization/text/text_format.h"
#include "orchestration/serialization/text/text_parser.h"
#include "orchestration/serialization/text/variant_parser.h"
#include "script/script.h"
#include "script/script_server.h"
#include "script/serialization/format_defs.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

String OrchestrationTextSerializer::_write_resources(void* p_userdata, const Ref<Resource>& p_resource) {
    return static_cast<OrchestrationTextSerializer*>(p_userdata)->_write_resource(p_resource);
}

String OrchestrationTextSerializer::_write_resource(const Ref<Resource>& p_resource) {
    if (p_resource->get_meta("_skip_save_", false)) {
        return "null";
    }

    if (_external_resources.has(p_resource)) {
        return _write_external_resource_ref(p_resource);
    }

    if (_internal_resources.has(p_resource)) {
        return _write_internal_resource_ref(p_resource);
    }

    if (!_is_resource_built_in(p_resource)) {
        if (p_resource->get_path() == _path) {
            // Circular reference
            return "null";
        }
        return _write_resource_ref(p_resource);
    }

    // Internal resource
    ERR_FAIL_V_MSG("null", "Resource was not pre-cached for the resource section, bug?");
}

String OrchestrationTextSerializer::_write_resource_ref(const Ref<Resource>& p_resource) {
    const String path = p_resource->get_path();
    return vformat(R"(Resource("%s"))", _relative_paths ? StringUtils::path_to_file(_path, path) : path);
}

String OrchestrationTextSerializer::_write_external_resource_ref(const Ref<Resource>& p_resource) {
    return vformat(R"(ExtResource("%s"))", _external_resources[p_resource]);
}

String OrchestrationTextSerializer::_write_internal_resource_ref(const Ref<Resource>& p_resource) {
    return vformat(R"(SubResource("%s"))", _internal_resources[p_resource]);
}

String OrchestrationTextSerializer::_create_start_tag(const String& p_class, const String& p_script_class, const String& p_icon_path, uint32_t p_steps, uint32_t p_version, int64_t p_uid) { // NOLINT
    return OrchestrationTextFormat::create_start_tag(p_class, p_script_class, p_icon_path, p_steps, p_version, p_uid);
}

String OrchestrationTextSerializer::_create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline) {
    return OrchestrationTextFormat::create_ext_resource_tag(p_type, p_path, p_id, p_newline);
}

String OrchestrationTextSerializer::_create_obj_tag(const Ref<Resource>& p_resource, const String& p_uid) {
    return vformat(R"([obj type="%s" id="%s"])", _resource_get_class(p_resource), p_uid);
}

String OrchestrationTextSerializer::_create_resource_tag() { // NOLINT
    return "[resource]";
}

void OrchestrationTextSerializer::_find_resources_object(const Variant& p_variant, bool p_main) {
    const Ref<Resource> resource = p_variant;
    if (resource.is_null() || resource->get_meta("_skip_save_", false) || _external_resources.has(resource)) {
        return;
    }
    _find_resources_resource(resource, p_main);
}

void OrchestrationTextSerializer::_find_resources_resource(const Ref<Resource>& p_resource, bool p_main) { // NOLINT
    if (!p_main && !_bundle_resources && !_is_resource_built_in(p_resource)) {
        if (p_resource->get_path() == _path) {
            ERR_PRINT(vformat("(Circular reference to resource being saved found: %s will be null next time its loaded.", _path));
            return;
        }

        // Use a numeric ID as base to sort in a natural order before saving.
        // This increase the chances of thread loading to fetch them first.
        _external_resources[p_resource] = itos(_external_resources.size() + 1) + "_" + _generate_scene_unique_id();
        return;
    }

    if (_resource_set.has(p_resource)) {
        return;
    }

    _find_resources_resource_properties(p_resource, p_main);
}

PackedStringArray OrchestrationTextSerializer::get_recognized_extensions(const Ref<Resource>& p_resource) {
    return recognize(p_resource) ? Array::make(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) : Array();
}

bool OrchestrationTextSerializer::recognize(const Ref<Resource>& p_resource) {
    return p_resource.is_valid() && p_resource->get_class() == Orchestration::get_class_static();
}

Error OrchestrationTextSerializer::set_uid(const String& p_path, int64_t p_uid) {
    if (p_path.get_extension().naturalnocasecmp_to(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) != 0) {
        return ERR_FILE_UNRECOGNIZED;
    }

    OrchestrationTextParser parser;

    String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    Error err = parser.set_uid(local_path, p_uid);
    if (err == OK) {
        Ref<DirAccess> dir = DirAccess::open("res://");
        dir->remove(local_path);
        dir->rename(local_path + ".uidren", local_path);
    }

    return err;
}

bool OrchestrationTextSerializer::recognize_path(const Ref<Resource>& p_resource, const String& p_path) {
    return p_path.get_extension().naturalnocasecmp_to(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) == 0;
}

Error OrchestrationTextSerializer::save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) {
    const Ref<Orchestration> orchestration = p_resource;
    ERR_FAIL_COND_V_MSG(!orchestration.is_valid(), ERR_INVALID_PARAMETER, "Resource is not an orchestration");

    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(file.is_null(), ERR_FILE_CANT_OPEN, "Cannot write file '" + p_path + "'.");

    _decode_and_set_flags(p_path, p_flags);

    // Walk the resource and gather all resource details
    _find_resources(p_resource, true);

    const String global_name = orchestration->get_global_name();
    const String icon_path = orchestration->get_icon_path();

    file->store_line(_create_start_tag(
        OScript::get_class_static(),
        global_name,
        icon_path,
        _saved_resources.size() + _external_resources.size(),
        OrchestrationFormat::FORMAT_VERSION,
        _get_resource_id_for_path(_path, true)));

    #ifdef TOOLS_ENABLED
    // Keep order from cached ids
    HashSet<String> cached_ids_found;
    for (KeyValue<Ref<Resource>, String>& E : _external_resources) {
        const String cached_id = ResourceCache::get_singleton()->get_id_for_path(_path, E.key->get_path());
        if (cached_id.is_empty() || cached_ids_found.has(cached_id)) {
            const int separator = E.value.find("_");
            if (separator != -1) {
                E.value = E.value.substr(0, separator + 1);
            } else {
                E.value = "";
            }
        } else {
            E.value = cached_id;
            cached_ids_found.insert(cached_id);
        }
    }

    // Create ids for non-cached resources
    for (KeyValue<Ref<Resource>, String>& E : _external_resources) {
        if (cached_ids_found.has(E.value)) {
            continue;
        }

        String attempt;
        while (attempt.is_empty() || cached_ids_found.has(attempt)) {
            attempt = E.value + _generate_scene_unique_id();
        }

        cached_ids_found.insert(attempt);
        E.value = attempt;

        // Update resource cache details
        Ref<Resource> res = E.key;
        ResourceCache::get_singleton()->set_id_for_path(_path, res->get_path(), attempt);
    }
    #else
    int counter = 1;
    for (KeyValue<Ref<Resource>, String>& E : _external_resources) {
        E.value = itos(counter++);
    }
    #endif

    Vector<ResourceSort> sorted_external_resources;
    for (const KeyValue<Ref<Resource>, String>& E : _external_resources) {
        ResourceSort rs;
        rs.resource = E.key;
        rs.id = E.value;
        sorted_external_resources.push_back(rs);
    }
    sorted_external_resources.sort();

    // Store external resource tags
    for (const ResourceSort& rs : sorted_external_resources) {
        file->store_string(_create_ext_resource_tag(rs.resource->get_class(), rs.resource->get_path(), rs.id));
    }

    if (_external_resources.size()) {
        // Add a separation between external resources and next block
        file->store_line(String());
    }

    HashSet<String> used_unique_ids;
    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E ; E = E->next()) {
        const Ref<Resource> res = E->get();
        if (E->next() && _is_resource_built_in(res)) {
            if (!res->get_scene_unique_id().is_empty()) {
                if (used_unique_ids.has(res->get_scene_unique_id())) {
                    res->set_scene_unique_id(""); // Repeated
                } else {
                    used_unique_ids.insert(res->get_scene_unique_id());
                }
            }
        }
    }

    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E ; E = E->next()) {
        const Ref<Resource> res = E->get();
        ERR_CONTINUE(!_resource_set.has(res));

        const bool main = E->next() == nullptr;
        if (main) {
            file->store_line(_create_resource_tag());
        } else {
            bool generated = false;
            const String uid = _create_resource_uid(res, used_unique_ids, generated);

            if (generated) {
                res->set_scene_unique_id(uid);
                used_unique_ids.insert(uid);
            }

            if (_take_over_paths) {
                res->set_path(vformat("%s::%s", p_path, uid));
            }

            _internal_resources[res] = uid;
            _set_resource_edited(res, false);
            file->store_line(_create_obj_tag(res, uid));
        }

        Dictionary missing_properties = p_resource->get_meta("_missing_resources_", Dictionary());
        const TypedArray<Dictionary> property_list = res->get_property_list();
        for (uint32_t i = 0; i < property_list.size(); i++) {
            const PropertyInfo pi = DictionaryUtils::to_property(property_list[i]);
            if (_skip_editor && pi.name.begins_with("__editor")) {
                continue;
            }

            if (pi.name.match("metadata/_missing_resources")) {
                continue;
            }

            if (!(pi.usage & PROPERTY_USAGE_STORAGE)) {
                continue;
            }

            Variant value;
            if (pi.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT) {
                NonPersistentKey key;
                key.base = res;
                key.property = pi.name;
                if (_non_persistent_map.has(key)) {
                    value = _non_persistent_map[key];
                }
            } else {
                value = res->get(pi.name);
            }

            if (pi.type == Variant::OBJECT && missing_properties.has(pi.name)) {
                // Was the missing resource overridden
                Ref<Resource> ores = value;
                if (ores.is_null()) {
                    value = missing_properties[pi.name];
                }
            }

            Variant default_value = GDE::ClassDB::get_property_default_value(res->get_class(), pi.name);
            if (default_value.get_type() != Variant::NIL) {
                bool valid = false;
                Variant result = false;
                Variant::evaluate(Variant::OP_EQUAL, value, default_value, result, valid);
                if (valid && result) {
                    continue;
                }
            }

            if (pi.type == Variant::OBJECT) {
                Object* object = Object::cast_to<Object>(value);
                if (!object && !(pi.usage & PROPERTY_USAGE_STORE_IF_NULL)) {
                    continue;
                }
            }

            String vars;
            OScriptVariantWriter::write_to_string(value, vars, _write_resources, this);
            file->store_line(StringUtils::property_name_encode(pi.name) + " = " + vars);
        }

        if (E->next()) {
            // Separator
            file->store_line(String());
        }
    }

    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
        return ERR_CANT_CREATE;
    }

    #ifdef TOOLS_ENABLED
    if (orchestration.is_valid()) {
        orchestration->set_edited(false);
    }
    #endif

    return OK;
}