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
#include "script/serialization/text_loader_instance.h"

#include "editor/plugins/orchestrator_editor_plugin.h"
#include "script/script.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_state.hpp>
#include <godot_cpp/core/version.hpp>

#define _printerr() ERR_PRINT(String(_res_path + ":" + itos(_lines) + " - Parse Error: " + _error_text).utf8().get_data());

bool OScriptTextResourceLoaderInstance::_is_creating_missing_resources_if_class_unavailable_enabled() const
{
    // EditorNode sets this to true, existance of our plugin should be sufficient?
    // todo: not exposed on ResourceLoader
    return OrchestratorPlugin::get_singleton() != nullptr;
}

Error OScriptTextResourceLoaderInstance::_parse_sub_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    return reinterpret_cast<OScriptTextResourceLoaderInstance*>(p_self)->_parse_sub_resource(p_stream, r_res, r_line, r_err_string);
}

Error OScriptTextResourceLoaderInstance::_parse_sub_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    return _parse_sub_resource_dummy(reinterpret_cast<DummyReadData*>(p_self), p_stream, r_res, r_line, r_err_string);
}

Error OScriptTextResourceLoaderInstance::_parse_sub_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    OScriptVariantParser::Token token;
    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING)
    {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    if (p_data->no_placeholderss)
    {
        r_res.unref();
    }
    else
    {
        String unique_id = token.value;
        if (!p_data->resource_map.has(unique_id))
        {
            r_err_string = "Found unique_id reference before mapping, sub-resources stored out of order in resource file";
            return ERR_PARSE_ERROR;
        }
        r_res = p_data->resource_map[unique_id];
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE)
    {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OScriptTextResourceLoaderInstance::_parse_sub_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    OScriptVariantParser::Token token;
    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING)
    {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    String id = token.value;
    ERR_FAIL_COND_V(!_internal_resources.has(id), ERR_INVALID_PARAMETER);
    r_res = _internal_resources[id];

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE)
    {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OScriptTextResourceLoaderInstance::_parse_ext_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    return reinterpret_cast<OScriptTextResourceLoaderInstance*>(p_self)->_parse_ext_resource(p_stream, r_res, r_line, r_err_string);
}

Error OScriptTextResourceLoaderInstance::_parse_ext_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    return _parse_ext_resource_dummy(reinterpret_cast<DummyReadData*>(p_self), p_stream, r_res, r_line, r_err_string);
}

Error OScriptTextResourceLoaderInstance::_parse_ext_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    OScriptVariantParser::Token token;
    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING)
    {
        r_err_string = "Expected number (old style sub-resource index) or String (ext-resource ID)";
        return ERR_PARSE_ERROR;
    }

    if (p_data->no_placeholderss)
    {
        r_res.unref();
    }
    else
    {
        String id = token.value;
        ERR_FAIL_COND_V(!p_data->rev_external_resources.has(id), ERR_PARSE_ERROR);
        r_res = p_data->rev_external_resources[id];
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE)
    {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return OK;
}

Error OScriptTextResourceLoaderInstance::_parse_ext_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string)
{
    OScriptVariantParser::Token token;
    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_NUMBER && token.type != OScriptVariantParser::TK_STRING)
    {
        r_err_string = "Expected number (old style) or string (sub-resource index)";
        return ERR_PARSE_ERROR;
    }

    String id = token.value;
    Error err = OK;

    if (!_ignore_resource_parsing)
    {
        if (!_external_resources.has(id))
        {
            r_err_string = "Can't load cached ext-resource id: " + id;
            return ERR_PARSE_ERROR;
        }

        String path = _external_resources[id].path;
        String type = _external_resources[id].type;

        // todo: support load tokens
        Ref<Resource> res = _external_resources[id].resource;
        if (res.is_valid())
        {
            if (res.is_null())
            {
                WARN_PRINT("External resource failed to load at: " + path);
            }
            else
            {
                #ifdef TOOLS_ENABLED
                    #if GODOT_VERSION >= 0x040400
                    res->set_id_for_path(_local_path, id);
                    #else
                    ResourceCache::get_singleton()->set_id_for_path(_local_path, res->get_path(), id);
                    #endif
                #endif
                r_res = res;
            }
        }
        else
            r_res = Ref<Resource>();

        #ifdef TOOLS_ENABLED
        if (r_res.is_null())
        {
            // Hack to allow checking original path
            r_res.instantiate();
            r_res->set_meta("__load_path__", _external_resources[id].path);
        }
        #endif
    }

    OScriptVariantParser::get_token(p_stream, r_line, token, r_err_string);
    if (token.type != OScriptVariantParser::TK_PARENTHESIS_CLOSE)
    {
        r_err_string = "Expected ')'";
        return ERR_PARSE_ERROR;
    }

    return err;
}

void OScriptTextResourceLoaderInstance::open(const Ref<FileAccess>& p_file, bool p_skip_first_tag)
{
    // Initialize state
    _error = OK;
    _lines = 1;
    _file = p_file;
    _stream.data = _file;
    _is_scene = false;
    _ignore_resource_parsing = false;
    _resource_current = 0;

    OScriptVariantParser::Tag tag;
    if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, tag, _error_text))
    {
        _error = err;
        _printerr();
        return;
    }

    if (tag.fields.has("format"))
    {
        uint32_t format = tag.fields["format"];
        if (format > FORMAT_VERSION)
        {
            _error_text = "Saved with a newer version of the format";
            _printerr();
            _error = ERR_PARSE_ERROR;
            return;
        }
    }

    if (tag.name == "orchestration")
    {
        if (!tag.fields.has("type"))
        {
            _error_text = "Missing 'type' field in 'orchestration' tag";
            _printerr();
            _error = ERR_PARSE_ERROR;
            return;
        }

        if (tag.fields.has("script_class"))
            _script_class = tag.fields["script_class"];

        _res_type = tag.fields["type"];
    }
    else
    {
        _error_text = "Unrecognized file type: " + tag.name;
        _printerr();
        _error = ERR_PARSE_ERROR;
        return;
    }

    if (tag.fields.has("uid"))
        _res_uid = ResourceUID::get_singleton()->text_to_id(tag.fields["uid"]);
    else
        _res_uid = ResourceUID::INVALID_ID;

    if (tag.fields.has("load_steps"))
        _resources_total = tag.fields["load_step"];
    else
        _resources_total = 0;

    if (!p_skip_first_tag)
    {
        if (const Error err = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text))
        {
            _error_text = "Unexpected end of file";
            _printerr();
            _error = ERR_FILE_CORRUPT;
        }
    }

    _rp.external_func = _parse_ext_resources;
    _rp.subres_func = _parse_sub_resources;
    _rp.userdata = this;
}

Error OScriptTextResourceLoaderInstance::load()
{
    if (_error != OK)
        return _error;

    while (true)
    {
        if (_next_tag.name != "ext_resource")
            break;

        if (!_next_tag.fields.has("path"))
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "Missing 'path' in external resource tag";
            _printerr();
            return _error;
        }

        if (!_next_tag.fields.has("type"))
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "Missing 'type' in external resource tag";
            _printerr();
            return _error;
        }

        if (!_next_tag.fields.has("id"))
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "Missing 'id' in external resource tag";
            _printerr();
            return _error;
        }

        String path = _next_tag.fields["path"];
        String type = _next_tag.fields["type"];
        String id = _next_tag.fields["id"];

        if (_next_tag.fields.has("uid"))
        {
            String uidt = _next_tag.fields["uid"];
            int64_t uid = ResourceUID::get_singleton()->text_to_id(uidt);
            if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid))
            {
                // If a UID is found and the path is valid, it will be used; otherwise, fall back to path
                path = ResourceUID::get_singleton()->get_id_path(uid);
            }
            else
            {
                #ifdef TOOLS_ENABLED
                // Silence a warning that can happen during the initial filesystem scan
                if (ResourceLoader::get_singleton()->get_resource_uid(path) != uid)
                {
                    WARN_PRINT(String(_res_path + ":" + itos(_lines) + " - ext_resource, invalid UID: " + uidt + " - using text path instead: " + path).utf8().get_data());
                }
                #else
                WARN_PRINT(String(_res_path + ":" + itos(_lines) + " - ext_resource, invalid UID: " + uidt + " - using text path instead: " + path).utf8().get_data());
                #endif
            }
        }

        if (!path.contains("://") && path.is_relative_path())
        {
            // path is relative to file being loaded, so convert to a resource path
            path = ProjectSettings::get_singleton()->localize_path(_local_path.get_base_dir().path_join(path));
        }

        if (_remaps.has(path))
            path = _remaps[path];

        _external_resources[id].path = path;
        _external_resources[id].type = type;
        _external_resources[id].resource = ResourceLoader::get_singleton()->load(path, type, (ResourceLoader::CacheMode)_cache_mode);
        if (!_external_resources[id].resource.is_valid())
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "[ext_resource] referenced non-existent resource at: " + path;
            _printerr();
            return _error;
        }

        _error = OScriptVariantParser::parse_tag(&_stream, _lines, _next_tag, _error_text);
        if (_error)
        {
            _printerr();
            return _error;
        }

        _resource_current++;
    }

    // These are the oens that count
    _resources_total -= _resource_current;
    _resource_current = 0;

    while (true)
    {
        if (_next_tag.name != "obj")
            break;

        if (!_next_tag.fields.has("type"))
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "Missing 'type' in subresource tag";
            _printerr();
            return _error;
        }
        if (!_next_tag.fields.has("id"))
        {
            _error = ERR_FILE_CORRUPT;
            _error_text = "Missing 'id' in subresource tag";
            _printerr();
            return _error;
        }

        String type = _next_tag.fields["type"];
        String id = _next_tag.fields["id"];

        String path = _local_path + "::" + id;

        Ref<Resource> res;
        bool do_assign{ false };
        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && ResourceCache::has(path))
        {
            // Reuse existing
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (cache.is_valid() && cache->get_class() == type)
            {
                res = cache;
                #if GODOT_VERSION >= 0x040400
                res->reset_state();
                #endif
                do_assign = true;
            }
        }

        MissingResource* missing_resource = nullptr;

        if (res.is_null())
        {
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (_cache_mode == ResourceFormatLoader::CACHE_MODE_IGNORE && cache.is_valid())
            {
                // cached, do not assign
                res = cache;
            }
            else
            {
                // Create
                Variant obj = ClassDB::instantiate(type);
                if (!obj)
                {
                    if (_is_creating_missing_resources_if_class_unavailable_enabled())
                    {
                        missing_resource = memnew(MissingResource);
                        missing_resource->set_original_class(type);
                        missing_resource->set_recording_properties(true);
                        obj = missing_resource;
                    }
                    else
                    {
                        _error_text = "Cannot create sub resource of type: " + type;
                        _printerr();
                        _error = ERR_FILE_CORRUPT;
                        return _error;
                    }
                }

                Resource* r = Object::cast_to<Resource>(obj);
                if (!r)
                {
                    _error_text = "Cannot create sub resource of type, because not a resource: " + type;
                    _printerr();
                    _error = ERR_FILE_CORRUPT;
                    return _error;
                }

                res = Ref<Resource>(r);
                do_assign = true;
            }
        }

        _resource_current++;

        if (_progress && _resources_total > 0)
            *_progress = _resource_current / float(_resources_total);

        _internal_resources[id] = res;
        if (do_assign)
        {
            if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
            {
                if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE)
                    res->take_over_path(path);
                else
                    res->set_path(path);
            }
            else
            {
                #if GODOT_VERSION >= 0x040400
                res->set_path_cache(path);
                #endif
            }

            #if GODOT_VERSION >= 0x040300
            res->set_scene_unique_id(id);
            #else
            ResourceCache::get_singleton()->set_scene_unique_id(_local_path, res, id);
            #endif
        }

        Dictionary missing_properties;
        while (true)
        {
            String assign;
            Variant value;

            _error = OScriptVariantParser::parse_tag_assign_eof(&_stream, _lines, _error_text, _next_tag, assign, value, &_rp);
            if (_error)
            {
                _printerr();
                return _error;
            }

            if (!assign.is_empty())
            {
                if (do_assign)
                {
                    bool set_valid{ true };
                    if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
                    {
                        // If the property being set is a missing resource and the parent isn't, setting it
                        // most likely will not work, so save it as metadata
                        Ref<MissingResource> mr = value;
                        if (mr.is_valid())
                        {
                            missing_properties[assign] = mr;
                            set_valid = false;
                        }
                    }
                    if (value.get_type() == Variant::ARRAY)
                    {
                        Array set_array = value;
                        // todo: how to deal with is valid?
                        Variant get_value = res->get(assign);
                        if (get_value.get_type() == Variant::ARRAY)
                        {
                            Array get_array = get_value;
                            if (!set_array.is_same_typed(get_array))
                                value = Array(set_array, get_array.get_typed_builtin(), get_array.get_typed_class_name(), get_array.get_typed_script());
                        }
                    }

                    if (set_valid)
                        res->set(assign, value);
                }
            }
            else if (!_next_tag.name.is_empty())
            {
                _error = OK;
                break;
            }
            else
            {
                _error = ERR_FILE_CORRUPT;
                _error_text = "Premature EOF while parsing [obj]";
                _printerr();
                return _error;
            }
        }

        if (missing_resource)
            missing_resource->set_recording_properties(false);

        if (!missing_properties.is_empty())
            res->set_meta("metadata/_missing_resources", missing_properties);
    }

    while (true)
    {
        if (_next_tag.name != "resource")
            break;

        if (_is_scene)
        {
            _error_text += "found the 'resource' tag on a scene file!";
            _printerr();
            _error = ERR_FILE_CORRUPT;
            return _error;
        }

        Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(_local_path);
		if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && cache.is_valid() && cache->get_class() == _res_type)
		{
		    #if GODOT_VERSION >= 0x040400
		    cache->reset_state();
		    #endif
			_resource = cache;
		}

		MissingResource *missing_resource = nullptr;

		if (!_resource.is_valid())
		{
			Variant obj = ClassDB::instantiate(_res_type);
			if (!obj)
			{
				if (_is_creating_missing_resources_if_class_unavailable_enabled())
				{
					missing_resource = memnew(MissingResource);
					missing_resource->set_original_class(_res_type);
					missing_resource->set_recording_properties(true);
					obj = missing_resource;
				}
			    else
			    {
					_error_text += "Can't create sub resource of type: " + _res_type;
					_printerr();
					_error = ERR_FILE_CORRUPT;
					return _error;
				}
			}

			Resource *r = Object::cast_to<Resource>(obj);
			if (!r)
			{
				_error_text += "Can't create sub resource of type, because not a resource: " + _res_type;
				_printerr();
				_error = ERR_FILE_CORRUPT;
				return _error;
			}

			_resource = Ref<Resource>(r);
		}

		Dictionary missing_resource_properties;

		while (true)
		{
			String assign;
			Variant value;

			_error = OScriptVariantParser::parse_tag_assign_eof(&_stream, _lines, _error_text, _next_tag, assign, value, &_rp);
			if (_error)
			{
				if (_error != ERR_FILE_EOF)
				{
					_printerr();
				}
			    else
			    {
					_error = OK;
					if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
					{
						if (!ResourceCache::has(_res_path))
							_resource->set_path(_res_path);

					    // todo: does this create any issues?
					    #if GODOT_VERSION >= 0x040400
						_resource->set_as_translation_remapped(_translation_remapped);
					    #endif
					}
			        else
			        {
			            #if GODOT_VERSION >= 0x040400
			            _resource->set_path_cache(_res_path);
			            #endif
					}
				}
				return _error;
			}

			if (!assign.is_empty())
			{
				bool set_valid = true;

				if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
				{
					// If the property being set is a missing resource (and the parent is not),
					// then setting it will most likely not work.
					// Instead, save it as metadata.

					Ref<MissingResource> mr = value;
					if (mr.is_valid()) {
						missing_resource_properties[assign] = mr;
						set_valid = false;
					}
				}

				if (value.get_type() == Variant::ARRAY)
				{
					Array set_array = value;
				    // todo: Godot does not expose the Object::get(StringName, bool&) method, assume valid?
				    // bool is_get_valid = false;
				    Variant get_value = _resource->get(assign);
				    if (get_value.get_type() == Variant::ARRAY)
					{
						Array get_array = get_value;
						if (!set_array.is_same_typed(get_array)) {
							value = Array(set_array, get_array.get_typed_builtin(), get_array.get_typed_class_name(), get_array.get_typed_script());
						}
					}
				}

				if (set_valid)
				    _resource->set(assign, value);
			}
		    else if (!_next_tag.name.is_empty())
		    {
				_error = ERR_FILE_CORRUPT;
				_error_text = "Extra tag found when parsing main resource file";
				_printerr();
				return _error;
			}
		    else
				break;
		}

		_resource_current++;

		if (_progress && _resources_total > 0)
			*_progress = _resource_current / float(_resources_total);

		if (missing_resource)
			missing_resource->set_recording_properties(false);

		if (!missing_resource_properties.is_empty())
			_resource->set_meta("metadata/_missing_resources", missing_resource_properties);

		_error = OK;

		return _error;
	}

	// altered behavior as we don't support scene tags
	if (_next_tag.name == "node" && !_is_scene)
	{
		_error_text += "found the 'node' tag on a resource file!";
		_printerr();
		_error = ERR_FILE_CORRUPT;
		return _error;
	}
    else
    {
		_error_text += "Unknown tag in file: " + _next_tag.name;
		_printerr();
		_error = ERR_FILE_CORRUPT;
		return _error;
	}
}

int64_t OScriptTextResourceLoaderInstance::get_uid(const Ref<FileAccess>& p_file)
{
    _error = OK;

    _lines = 1;
    _file = p_file;

    _stream.data = _file;

    _ignore_resource_parsing = true;

    OScriptVariantParser::Tag tag;
    Error err = OScriptVariantParser::parse_tag(&_stream, _lines, tag, _error_text);

    if (err) {
        _printerr();
        return ResourceUID::INVALID_ID;
    }

    if (tag.fields.has("uid")) { //field is optional
        String uidt = tag.fields["uid"];
        return ResourceUID::get_singleton()->text_to_id(uidt);
    }

    return ResourceUID::INVALID_ID;
}

PackedStringArray OScriptTextResourceLoaderInstance::get_classes_used(const Ref<FileAccess>& p_file)
{
    PackedStringArray classes;
    // open(p_file, false, true);
    // if (_error == OK)
    // {
    //     for(int i = 0; i < _internal_resources.size(); i++)
    //     {
    //         p_file->seek(_internal_resources[i].offset);
    //         String type = _read_unicode_string();
    //         ERR_FAIL_COND_V(p_file->get_error() != OK, {});
    //         if (type != String())
    //             classes.push_back(type);
    //     }
    // }
    return classes;
}

OScriptTextResourceLoaderInstance::OScriptTextResourceLoaderInstance()
{
    _error = OK;
    // _version = 0;
    // _godot_version = 0;
    _res_uid = ResourceUID::INVALID_ID;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
}