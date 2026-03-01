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
#include "orchestration/serialization/text/text_parser.h"

#include "common/string_utils.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "orchestration/serialization/format.h"
#include "orchestration/serialization/text/text_format.h"
#include "orchestration/serialization/text/variant_parser.h"
#include "script/script.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

#define _printerr() ERR_PRINT(String(_path + ":" + itos(_lines) + " - Parse Error: " + _error_text).utf8().get_data());

String OrchestrationTextParser::_remap_class(const String& p_class) {
    if (p_class == OScript::get_class_static()) {
        return "Orchestration";
    }
    return p_class;
}

Error OrchestrationTextParser::_parse_sub_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    return static_cast<OrchestrationTextParser*>(p_self)->_parse_sub_resource(p_stream, r_res, r_line, r_err_string);
}

Error OrchestrationTextParser::_parse_sub_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    return _parse_sub_resource_dummy(static_cast<DummyReadData*>(p_self), p_stream, r_res, r_line, r_err_string);
}

Error OrchestrationTextParser::_parse_ext_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    OScriptVariantParser::Token token;

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING) {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    if (p_data->no_placeholders) {
        r_res.unref();
    } else {
        String unique_id = token.value;
        if (!p_data->resource_map.has(unique_id)) {
            r_err_string = "Found unique_id reference before mapping, sub-resources stored out of order in resource file";
            return ERR_PARSE_ERROR;
        }
        r_res = p_data->resource_map[unique_id];
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE) {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OrchestrationTextParser::_parse_sub_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    OScriptVariantParser::Token token;

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING) {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    String id = token.value;
    ERR_FAIL_COND_V(!_internal_resources.has(id), ERR_INVALID_PARAMETER);
    r_res = _internal_resources[id];

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE) {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OrchestrationTextParser::_parse_ext_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    return static_cast<OrchestrationTextParser*>(p_self)->_parse_ext_resource(p_stream, r_res, r_line, r_err_string);
}

Error OrchestrationTextParser::_parse_ext_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    return _parse_ext_resource_dummy(static_cast<DummyReadData*>(p_self), p_stream, r_res, r_line, r_err_string);
}

Error OrchestrationTextParser::_parse_sub_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    OScriptVariantParser::Token token;

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING) {
        r_err_string = "Expected number (old style sub-resource index) or String (ext-resource ID)";
        return ERR_PARSE_ERROR;
    }

    if (p_data->no_placeholders) {
        r_res.unref();
    } else {
        String id = token.value;
        ERR_FAIL_COND_V(!p_data->rev_external_resources.has(id), ERR_PARSE_ERROR);
        r_res = p_data->rev_external_resources[id];
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE) {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OrchestrationTextParser::_parse_ext_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string) {
    OScriptVariantParser::Token token;

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING) {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    String id = token.value;
    Error err = OK;

    if (!_ignore_resource_parsing) {
        if (!_external_resources.has(id)) {
            r_err_string = "Can't load cached ext-resource id: " + id;
            return ERR_PARSE_ERROR;
        }

        String path = _external_resources[id].path;
        String type = _external_resources[id].type;

        // todo: support load tokens
        Ref<Resource> res = _external_resources[id].resource;
        if (res.is_valid()) {
            if (res.is_null()) {
                WARN_PRINT("External resource failed to load at: " + path);
            } else {
                #ifdef TOOLS_ENABLED
                    res->set_id_for_path(_path, id);
                #endif
                r_res = res;
            }
        } else {
            r_res = Ref<Resource>();
        }

        #ifdef TOOLS_ENABLED
        if (r_res.is_null()) {
            // Hack to allow checking original path
            r_res.instantiate();
            r_res->set_meta("__load_path__", _external_resources[id].path);
        }
        #endif
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE) {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return err;
}

Error OrchestrationTextParser::_open(const Ref<FileAccess>& p_file, bool p_skip_first_tag, bool p_buffered) {
    // Initialize state
    _lines = 1;
    _stream.data = p_file;
    _stream.set_readahead(p_buffered);
    _is_scene = false;
    _ignore_resource_parsing = false;
    _resources_current = 0;

    OScriptVariantParser::Tag tag;
    if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, tag, _error_text)) {
        _printerr();
        return err;
    }

    if (tag.fields.has("format")) {
        uint32_t format = tag.fields["format"];
        if (format > OrchestrationFormat::FORMAT_VERSION) {
            _error_text = "Saved with a newer version of the format";
            _printerr();
            return ERR_FILE_UNRECOGNIZED;
        }
        _version = format;
    }

    if (tag.name == "orchestration") {
        if (!tag.fields.has("type")) {
            _error_text = "Missing 'type' field in 'orchestration' tag";
            _printerr();
            return ERR_PARSE_ERROR;
        }

        if (tag.fields.has("script_class")) {
            _script_class = tag.fields["script_class"];
        }

        if (tag.fields.has("icon")) {
            _icon_path = tag.fields["icon"];
        }

        _type = tag.fields["type"];
    } else {
        _error_text = "Unrecognized file type: " + tag.name;
        _printerr();
        return ERR_PARSE_ERROR;
    }

    if (tag.fields.has("uid")) {
        _uid = ResourceUID::get_singleton()->text_to_id(tag.fields["uid"]);
    } else {
        _uid = ResourceUID::INVALID_ID;
    }

    if (tag.fields.has("load_steps")) {
        _resources_total = tag.fields["load_steps"];
    } else {
        _resources_total = 0;
    }

    if (!p_skip_first_tag) {
        if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text)) {
            _error_text = "Unexpected end of file";
            _printerr();
            return ERR_FILE_CORRUPT;
        }
    }

    _rp.external_func = _parse_ext_resources;
    _rp.subres_func = _parse_sub_resources;
    _rp.userdata = this;

    return OK;
}

Error OrchestrationTextParser::_load() {
    while (true) {
        if (_next_tag.name != "ext_resource") {
            break;
        }

        if (!_next_tag.fields.has("path")) {
            _error_text = "Missing 'path' in external resource tag";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        if (!_next_tag.fields.has("type")) {
            _error_text = "Missing 'type' in external resource tag";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        if (!_next_tag.fields.has("id")) {
            _error_text = "Missing 'id' in external resource tag";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        String path = _next_tag.fields["path"];
        String type = _next_tag.fields["type"];
        String id = _next_tag.fields["id"];

        if (_next_tag.fields.has("uid")) {
            const String uid_text = _next_tag.fields["uid"];
            int64_t uid = ResourceUID::get_singleton()->text_to_id(uid_text);
            if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid)) {
                // If a UID is found and the path is valid, it will be used; otherwise, fall back to path
                path = ResourceUID::get_singleton()->get_id_path(uid);
            } else {
                #ifdef TOOLS_ENABLED
                // Silence a warning that can happen during the initial filesystem scan
                if (ResourceLoader::get_singleton()->get_resource_uid(path) != uid) {
                    WARN_PRINT(String(_path + ":" + itos(_lines) + " - ext_resource, invalid UID: " + uid_text + " - using text path instead: " + path).utf8().get_data());
                }
                #else
                WARN_PRINT(String(_path + ":" + itos(_lines) + " - ext_resource, invalid UID: " + uid_text + " - using text path instead: " + path).utf8().get_data());
                #endif
            }
        }

        if (!path.contains("://") && path.is_relative_path()) {
            // path is relative to file being loaded, so convert to a resource path
            path = ProjectSettings::get_singleton()->localize_path(_path.get_base_dir().path_join(path));
        }

        if (_remaps.has(path)) {
            path = _remaps[path];
        }

        _external_resources[id].path = path;
        _external_resources[id].type = type;
        _external_resources[id].resource = ResourceLoader::get_singleton()->load(path, type, (ResourceLoader::CacheMode)_cache_mode);

        if (!_external_resources[id].resource.is_valid()) {
            _error_text = "[ext_resource] referenced non-existent resource at: " + path;
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text)) {
            _printerr();
            return err;
        }

        _resources_current++;
    }

    // These are the ones that count
    _resources_total -= _resources_current;
    _resources_current = 0;

    while (true) {
        if (_next_tag.name != "obj") {
            break;
        }

        if (!_next_tag.fields.has("type")) {
            _error_text = "Missing 'type' in subresource tag";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        if (!_next_tag.fields.has("id")) {
            _error_text = "Missing 'id' in subresource tag";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        String type = _next_tag.fields["type"];
        String id = _next_tag.fields["id"];
        String path = _path + "::" + id;

        Ref<Resource> res;
        bool do_assign{ false };
        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && ResourceCache::has(path)) {
            // Reuse existing
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (cache.is_valid() && cache->get_class() == type) {
                res = cache;
                res->reset_state();
                do_assign = true;
            }
        }

        MissingResource* missing_resource = nullptr;

        if (res.is_null()) {
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (_cache_mode == ResourceFormatLoader::CACHE_MODE_IGNORE && cache.is_valid()) {
                // cached, do not assign
                res = cache;
            } else {
                // Create
                Variant obj = ClassDB::instantiate(_remap_class(type));
                if (!obj) {
                    if (_is_creating_missing_resources_if_class_unavailable_enabled()) {
                        missing_resource = memnew(MissingResource);
                        missing_resource->set_original_class(type);
                        missing_resource->set_recording_properties(true);
                        obj = missing_resource;
                    } else {
                        _error_text = "Cannot create sub resource of type: " + type;
                        _printerr();
                        return ERR_FILE_CORRUPT;
                    }
                }

                Resource* r = Object::cast_to<Resource>(obj);
                if (!r) {
                    _error_text = "Cannot create sub resource of type, because not a resource: " + type;
                    _printerr();
                    return ERR_FILE_CORRUPT;
                }

                res = Ref<Resource>(r);
                do_assign = true;
            }
        }

        _resources_current++;

        if (_progress && _resources_total > 0) {
            *_progress = _resources_current / static_cast<float>(_resources_total);
        }

        _internal_resources[id] = res;
        if (do_assign) {
            if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE) {
                if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE) {
                    res->take_over_path(path);
                } else {
                    // res->set_path(path);
                }
            } else {
                res->set_path_cache(path);
            }

            res->set_scene_unique_id(id);
        }

        Dictionary missing_properties;
        while (true) {
            String assign;
            Variant value;

            if (const Error err = OScriptVariantParser::parse_tag_assign_eof(&_stream, _lines, _error_text, _next_tag, assign, value, &_rp)) {
                _printerr();
                return err;
            }

            if (!assign.is_empty()) {
                if (do_assign) {
                    _set_resource_property(res, missing_resource, assign, value, missing_properties);
                }
            } else if (!_next_tag.name.is_empty()) {
                break;
            } else {
                _error_text = "Premature EOF while parsing [obj]";
                _printerr();
                return ERR_FILE_CORRUPT;
            }
        }

        if (missing_resource) {
            missing_resource->set_recording_properties(false);
        }

        if (!missing_properties.is_empty()) {
            res->set_meta("metadata/_missing_resources", missing_properties);
        }
    }

    while (true) {
        if (_next_tag.name != "resource") {
            break;
        }

        if (_is_scene) {
            _error_text += "found the 'resource' tag on a scene file!";
            _printerr();
            return ERR_FILE_CORRUPT;
        }

        Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(_path);
		if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && cache.is_valid() && cache->get_class() == _type) {
		    cache->reset_state();
			_resource = cache;
		}

		MissingResource *missing_resource = nullptr;

		if (!_resource.is_valid()) {
			Variant obj = ClassDB::instantiate(_remap_class(_type));
			if (!obj) {
				if (_is_creating_missing_resources_if_class_unavailable_enabled()) {
					missing_resource = memnew(MissingResource);
					missing_resource->set_original_class(_type);
					missing_resource->set_recording_properties(true);
					obj = missing_resource;
				} else {
					_error_text += "Can't create sub resource of type: " + _type;
					_printerr();
					return ERR_FILE_CORRUPT;
				}
			}

			Resource *r = Object::cast_to<Resource>(obj);
			if (!r) {
				_error_text += "Can't create sub resource of type, because not a resource: " + _type;
				_printerr();
				return ERR_FILE_CORRUPT;
			}

			_resource = Ref<Resource>(r);
		}

		Dictionary missing_resource_properties;

		while (true) {
			String assign;
			Variant value;

		    if (Error err = OScriptVariantParser::parse_tag_assign_eof(&_stream, _lines, _error_text, _next_tag, assign, value, &_rp)) {
				if (err != ERR_FILE_EOF) {
					_printerr();
				} else {
				    err = OK;
					if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE) {
						if (!ResourceCache::has(_path)) {
						    // _resource->set_path(_path);
						}
					    // todo: requires Godot change to support this
						// _resource->set_as_translation_remapped(_translation_remapped);
					} else {
			            _resource->set_path_cache(_path);
					}
				}
				return err;
			}

			if (!assign.is_empty()) {
			    _set_resource_property(_resource, missing_resource, assign, value, missing_resource_properties);
			} else if (!_next_tag.name.is_empty()) {
				_error_text = "Extra tag found when parsing main resource file";
				_printerr();
				return ERR_FILE_CORRUPT;
			} else {
			    break;
			}
		}

		_resources_current++;

		if (_progress && _resources_total > 0) {
		    *_progress = _resources_current / static_cast<float>(_resources_total);
		}

		if (missing_resource) {
		    missing_resource->set_recording_properties(false);
		}

		if (!missing_resource_properties.is_empty()) {
		    _resource->set_meta("metadata/_missing_resources", missing_resource_properties);
		}

		return OK;
	}

	// altered behavior as we don't support scene tags
	if (_next_tag.name == "node" && !_is_scene) {
		_error_text += "found the 'node' tag on a resource file!";
		_printerr();
		return ERR_FILE_CORRUPT;
	} else {
		_error_text += "Unknown tag in file: " + _next_tag.name;
		_printerr();
		return ERR_FILE_CORRUPT;
	}
}

String OrchestrationTextParser::get_resource_script_class(const String& p_path) {
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    _lines = 1;
    _stream.data = file;
    _ignore_resource_parsing = true;
    _path = p_path;

    OScriptVariantParser::Tag tag;
    if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, tag, _error_text)) {
        _printerr();
        return {};
    }

    if (tag.fields.has("format")) {
        uint32_t format = tag.fields["format"];
        if (format > OrchestrationFormat::FORMAT_VERSION) {
            _error_text = "Saved with a newer format";
            _printerr();
            return {};
        }
    }

    if (tag.name != "orchestration") {
        return {};
    }

    return tag.fields.has("script_class") ? tag.fields["script_class"] : "";
}

int64_t OrchestrationTextParser::get_resource_uid(const String& p_path) {
    // When creating a new script, this is called, and the file path won't yet be valid.
    if (!FileAccess::file_exists(p_path)) {
        return ResourceUID::INVALID_ID;
    }

    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), ResourceUID::INVALID_ID);

    _lines = 1;
    _stream.data = file;
    _ignore_resource_parsing = true;
    _path = p_path;

    OScriptVariantParser::Tag tag;
    if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, tag, _error_text)) {
        _printerr();
        return ResourceUID::INVALID_ID;
    }

    if (tag.fields.has("uid")) {
        String uid_text = tag.fields["uid"];
        return ResourceUID::get_singleton()->text_to_id(uid_text);
    }

    return ResourceUID::INVALID_ID;
}

PackedStringArray OrchestrationTextParser::get_dependencies(const String& p_path, bool p_add_types) {
    PackedStringArray results;

    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), results);

    _path = p_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_REPLACE;

    Error err = _open(file);
    if (err != OK) {
        return results;
    }

    _ignore_resource_parsing = true;

    while (_next_tag.name == "ext_resource") {
        if (!_next_tag.fields.has("type")) {
            _error_text = "Missing 'type' in external resource tag";
            _printerr();
            return {};
        }

        if (!_next_tag.fields.has("id")) {
            _error_text = "Missing 'id' in external resource tag";
            _printerr();
            return {};
        }

        String path = _next_tag.fields["path"];
        String type = _next_tag.fields["type"];
        String fallback_path;

        bool using_uids = false;
        if (_next_tag.fields.has("uid")) {
            String uid_text = _next_tag.fields["uid"];
            int64_t uid = ResourceUID::get_singleton()->text_to_id(uid_text);
            if (uid != ResourceUID::INVALID_ID) {
                fallback_path = path;
                path = ResourceUID::get_singleton()->id_to_text(uid);
                using_uids = true;
            }
        }

        if (!using_uids && !path.contains("://") && path.is_relative_path()) {
            path = ProjectSettings::get_singleton()->localize_path(_path.get_base_dir().path_join(path));
        }

        if (p_add_types) {
            path += "::" + type;
        }

        if (!fallback_path.is_empty()) {
            if (!p_add_types) {
                path += "::"; // Ensures that path comes third, even when no type
            }
            path += "::" + fallback_path;
        }

        results.push_back(path);

        if (OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text, &_rp) != OK) {
            print_line(_error_text);
            _error_text = "Unexpected end of file";
            _printerr();
            return results;
        }
    }

    return results;
}

Error OrchestrationTextParser::rename_dependencies(const String& p_path, const Dictionary& p_renames) {
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);

    Error err = _open(file, true, false);
    if (err) {
        return err;
    }

    _ignore_resource_parsing = true;

    Ref<FileAccess> fw; // File that will be written to
    String base_path = _path.get_base_dir();
    uint64_t tag_end = file->get_position();

    while (true) {
        err = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text, &_rp);
        ERR_FAIL_COND_V(err != OK, ERR_FILE_CORRUPT);

        if (_next_tag.name != "ext_resource") {
            // There are no external resources in the file, therefore nothing to rename.
            if (fw.is_null()) {
                // Write file was never opened, exit immediately.
                return OK;
            }
            break;
        } else {
            if (fw.is_null()) {
                fw = FileAccess::open(vformat("%s.depren", p_path), FileAccess::WRITE);

                if (_uid == ResourceUID::INVALID_ID) {
                    _uid = ResourceSaver::get_singleton()->get_resource_id_for_path(p_path);
                }

                String tag = OrchestrationTextFormat::create_start_tag(
                    _resource_type,
                    _script_class,
                    _icon_path,
                    _resources_total,
                    OrchestrationFormat::FORMAT_VERSION,
                    _uid);

                fw->store_line(tag);
            }

            if (!_next_tag.fields.has("path") || !_next_tag.fields.has("id") || !_next_tag.fields.has("type")) {
                ERR_FAIL_V(ERR_FILE_CORRUPT);
            }

            String path = _next_tag.fields["path"];
            String type = _next_tag.fields["type"];
            String id = _next_tag.fields["id"];

            if (_next_tag.fields.has("uid")) {
                String uid_text = _next_tag.fields["uid"];
                int64_t uid = ResourceUID::get_singleton()->text_to_id(uid_text);
                if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid)) {
                    path = ResourceUID::get_singleton()->get_id_path(uid);
                }
            }

            bool relative = false;
            if (!path.begins_with("res://")) {
                path = base_path.path_join(path).simplify_path();
                relative = true;
            }

            if (p_renames.has(path)) {
                path = p_renames[path];
            }

            if (relative) {
                path = StringUtils::path_to_file(base_path, path);
            }

            const String ext_tag = OrchestrationTextFormat::create_ext_resource_tag(type, path, id, false);
            fw->store_line(ext_tag);

            tag_end = file->get_position();
        }
    }

    file->seek(tag_end);

    constexpr uint32_t buffer_size = 2048;
    uint8_t* buffer = static_cast<uint8_t*>(alloca(buffer_size));

    uint32_t num_read = file->get_buffer(buffer, buffer_size);
    ERR_FAIL_COND_V_MSG(num_read == UINT32_MAX, ERR_CANT_CREATE, "Failed to allocate memory for buffer");
    ERR_FAIL_COND_V(num_read == 0, ERR_FILE_CORRUPT);

    if (*buffer == '\n') {
        // Skip first newline character since we added one.
        if (num_read > 1) {
            fw->store_buffer(buffer + 1, num_read - 1);
        }
    } else {
        fw->store_buffer(buffer, num_read);
    }

    while (!file->eof_reached()) {
        num_read = file->get_buffer(buffer, buffer_size);
        fw->store_buffer(buffer, num_read);
    }

    bool all_ok = fw->get_error() == OK;
    if (!all_ok) {
        return ERR_CANT_CREATE;
    }

    return OK;
}

PackedStringArray OrchestrationTextParser::get_classes_used(const String& p_path) {
    PackedStringArray results;
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), results);

    _path = p_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE;

    Error err = _open(file);
    if (err != OK) {
        return results;
    }

    _ignore_resource_parsing = true;

    DummyReadData dummy_read;
    dummy_read.no_placeholders = true;
    _rp.external_func = _parse_ext_resource_dummys;
    _rp.subres_func = _parse_sub_resource_dummys;
    _rp.userdata = &dummy_read; // NOLINT

    while (_next_tag.name == "ext_resource") {
        err = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text, &_rp);
        if (err) {
            _printerr();
            return results;
        }
    }

    while (_next_tag.name == "obj" || _next_tag.name == "resource") {
        if (_next_tag.name == "obj") {
            if (!_next_tag.fields.has("type")) {
                _error_text = "Missing 'type' in obj resource tag";
                _printerr();
                return results;
            }
            results.push_back(_next_tag.fields["type"]);
        } else {
            results.push_back(_next_tag.fields["res_type"]); //??
        }

        while (true) {
            String name;
            Variant value;

            err = OScriptVariantParser::parse_tag_assign_eof(&_stream, _lines, _error_text, _next_tag, name, value, &_rp);
            if (err) {
                if (err != ERR_FILE_EOF) {
                    _printerr();
                }
                return results;
            }

            if (!name.is_empty()) {
                continue;
            }

            if (!_next_tag.name.is_empty()) {
                break;
            } else {
                _error_text = "Premature end of file, file is likely corrupt.";
                _printerr();
                return results;
            }
        }
    }

    return results;
}

Variant OrchestrationTextParser::load(const String& p_path) {
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    _path = p_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_REPLACE;

    Error err = _open(file);
    if (err != OK) {
        return {};
    }

    err = _load();
    if (err != OK) {
        return {};
    }

    Ref<Orchestration> orchestration = _resource;
    if (orchestration.is_valid()) {
        orchestration->_script_path = p_path;
        orchestration->post_initialize();
    }

    return _resource;
}

Error OrchestrationTextParser::set_uid(const String& p_path, int64_t p_uid) {
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);

    Error err = _open(file, true, false);
    ERR_FAIL_COND_V(err != OK, err);

    _ignore_resource_parsing = true;

    const Ref<FileAccess> fw = FileAccess::open(vformat("%s.uidren", p_path), FileAccess::WRITE);
    ERR_FAIL_COND_V(fw.is_null(), ERR_FILE_CANT_WRITE);

    fw->store_string(OrchestrationTextFormat::create_start_tag(
        OScript::get_class_static(),
        _script_class,
        _icon_path,
        _resources_total,
        OrchestrationFormat::FORMAT_VERSION,
        p_uid).strip_edges());

    uint8_t c = file->get_8();
    while (!file->eof_reached()) {
        fw->store_8(c);
        c = file->get_8();
    }

    bool all_ok = fw->get_error() == OK;
    if (!all_ok) {
        return ERR_CANT_CREATE;
    }

    return OK;
}