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
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/weak_ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

ResourceCache* ResourceCache::_singleton = nullptr;

bool ResourceCache::CacheEntry::is_resource(const Ref<Resource>& p_resource) const
{
    WeakRef* ref = Object::cast_to<WeakRef>(reference);
    if (ref)
    {
        Ref<Resource> res = ref->get_ref();
        if (res.is_valid() && p_resource.ptr() == Object::cast_to<Resource>(ref->get_ref()))
            return true;
    }
    return false;
}

void ResourceCache::_clear()
{
    if (!_resources.is_empty())
    {
        if (OS::get_singleton()->is_stdout_verbose())
        {
            ERR_PRINT(vformat("%d resources still in use at exit (Orchestrator).", _resources.size()));
            for (const KeyValue<String, Resource*>& E : _resources)
                UtilityFunctions::print("Resource still in use: ", E.key, " (", E.value->get_class(), ")");
        }
        else
        {
            ERR_PRINT(vformat(
                "%d resources still in use at exit (Orchestrator) (run with --verbose for details)",
                _resources.size()));
        }
    }
    _resources.clear();
}

bool ResourceCache::has_path(const String& p_path)
{
    MutexLock lock(*_path_cache_lock.ptr());

    Resource** res = _resources.getptr(p_path);
    if (res && (*res)->get_reference_count() == 0)
    {
        // Resource is in the process of being deleted, ignore
        // (*res)->path_cache = String(); // todo: Not exposed to GDExtension
        _resources.erase(p_path);
        res = nullptr;
    }

    return !res ? false : true;
}

Ref<Resource> ResourceCache::get_ref(const String& p_path)
{
    Ref<Resource> ref;
    MutexLock lock(*_mutex.ptr());

    Resource** res = _resources.getptr(p_path);
    if (res)
        ref = Ref<Resource>(*res);

    if (res && !ref.is_valid())
    {
        // Resource is in the process of being deleted, ignore
        // (*res)->path_cache = String(); // todo: Not exposed to GDExtension
        _resources.erase(p_path);
        res = nullptr;
    }
    return ref;
}

void ResourceCache::remove_ref(const String& p_path)
{
    MutexLock lock(*_mutex.ptr());
    _resource_path_cache.erase(p_path);
    _resources.erase(p_path);
}

void ResourceCache::remove_path_cache(const String& p_path, const String& p_res_path, const String& p_id)
{
    MutexLock lock(*_path_cache_lock.ptr());
    _resource_path_cache[p_path].erase(p_res_path);
}

void ResourceCache::add_path_cache(const String& p_path, const String& p_res_path, const String& p_id)
{
    MutexLock lock(*_path_cache_lock.ptr());
    _resource_path_cache[p_path][p_res_path] = p_id;
}

String ResourceCache::get_id_for_path(const String& p_path, const String& p_res_path)
{
    MutexLock lock(*_path_cache_lock.ptr());
    if (_resource_path_cache[p_path].has(p_res_path))
    {
        return _resource_path_cache[p_path][p_res_path];
    }
    return "";
}

void ResourceCache::set_id_for_path(const String& p_path, const String& p_res_path, const String& p_id)
{
    if (p_id.is_empty())
        remove_path_cache(p_path, p_res_path, p_id);
    else
        add_path_cache(p_path, p_res_path, p_id);
}

#if GODOT_VERSION < 0x040400
String ResourceCache::get_scene_unique_id(const String& p_path, const Ref<Resource>& p_resource)
{
    ERR_FAIL_COND_V_MSG(!p_resource.is_valid(), String(), "No resource path was supplied to get_scene_unique_id");

    if (_resource_scene_unique_ids.has(p_path))
    {
        for (const CacheEntry& entry : _resource_scene_unique_ids[p_path])
        {
            if (entry.is_resource(p_resource))
                return entry.id;
        }
    }
    return {};
}

void ResourceCache::set_scene_unique_id(const String& p_path, const Ref<Resource>& p_resource, const String& p_id)
{
    ERR_FAIL_COND_MSG(!p_resource.is_valid(), "Cannot set scene unique id on invalid resource.");

    Variant weak_ref = UtilityFunctions::weakref(p_resource);
    ERR_FAIL_COND_MSG(!weak_ref, "Cannot set scene unique id on an invalid weak reference.");

    if (p_id.is_empty())
    {
        const List<CacheEntry>& list = _resource_scene_unique_ids[p_path];
        for (const CacheEntry& entry : list)
        {
            if (entry.is_resource(p_resource))
            {
                _resource_scene_unique_ids[p_path].erase(entry);
                break;
            }
        }
    }
    else
    {
        CacheEntry entry;
        entry.reference = weak_ref;
        entry.id = p_id;

        _resource_scene_unique_ids[p_path].push_back(entry);
    }
}
#endif

ResourceCache::ResourceCache()
{
    _singleton = this;
    _mutex.instantiate();
    _path_cache_lock.instantiate();
}

ResourceCache::~ResourceCache()
{
    _singleton = nullptr;
}