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
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/weak_ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

ResourceCache* ResourceCache::_singleton = nullptr;

bool ResourceCache::CacheEntry::is_resource(const Ref<Resource>& p_resource) const {
    WeakRef* ref = Object::cast_to<WeakRef>(reference);
    if (ref) {
        Ref<Resource> res = ref->get_ref();
        if (res.is_valid() && p_resource.ptr() == Object::cast_to<Resource>(ref->get_ref())) {
            return true;
        }
    }
    return false;
}

void ResourceCache::_clear() {
    if (!_resources.is_empty()) {
        if (OS::get_singleton()->is_stdout_verbose()) {
            ERR_PRINT(vformat("%d resources still in use at exit (Orchestrator).", _resources.size()));
        } else {
            ERR_PRINT(vformat(
                "%d resources still in use at exit (Orchestrator) (run with --verbose for details)",
                _resources.size()));
        }
    }
    _resources.clear();
}

bool ResourceCache::has_path(const String& p_path) {
    MutexLock lock(*_path_cache_lock.ptr());

    Resource** res = _resources.getptr(p_path);
    if (res && (*res)->get_reference_count() == 0) {
        // Resource is in the process of being deleted, ignore
        (*res)->set_path_cache(String());
        _resources.erase(p_path);
        res = nullptr;
    }

    return !res ? false : true;
}

Ref<Resource> ResourceCache::get_ref(const String& p_path) {
    Ref<Resource> ref;
    MutexLock lock(*_mutex.ptr());

    Resource** res = _resources.getptr(p_path);
    if (res) {
        ref = Ref<Resource>(*res);
    }

    if (res && !ref.is_valid()) {
        // Resource is in the process of being deleted, ignore
        (*res)->set_path_cache(String());
        _resources.erase(p_path);
        res = nullptr;
    }
    return ref;
}

void ResourceCache::remove_ref(const String& p_path) {
    MutexLock lock(*_mutex.ptr());
    _resource_path_cache.erase(p_path);
    _resources.erase(p_path);
}

void ResourceCache::remove_path_cache(const String& p_path, const String& p_res_path, const String& p_id) {
    MutexLock lock(*_path_cache_lock.ptr());
    _resource_path_cache[p_path].erase(p_res_path);
}

void ResourceCache::add_path_cache(const String& p_path, const String& p_res_path, const String& p_id) {
    MutexLock lock(*_path_cache_lock.ptr());
    _resource_path_cache[p_path][p_res_path] = p_id;
}

String ResourceCache::get_id_for_path(const String& p_path, const String& p_res_path) {
    MutexLock lock(*_path_cache_lock.ptr());
    if (_resource_path_cache[p_path].has(p_res_path)) {
        return _resource_path_cache[p_path][p_res_path];
    }
    return "";
}

void ResourceCache::set_id_for_path(const String& p_path, const String& p_res_path, const String& p_id) {
    if (p_id.is_empty())
        remove_path_cache(p_path, p_res_path, p_id);
    else
        add_path_cache(p_path, p_res_path, p_id);
}

ResourceCache::ResourceCache() {
    _singleton = this;
    _mutex.instantiate();
    _path_cache_lock.instantiate();
}

ResourceCache::~ResourceCache() {
    _singleton = nullptr;
}