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
#include "script/script_cache.h"

#include "core/godot/io/resource_loader.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/binary/binary_parser.h"
#include "orchestration/serialization/text/text_parser.h"
#include "script/compiler/analyzer.h"
#include "script/compiler/compiler.h"
#include "script/parser/parser.h"

#include <godot_cpp/classes/native_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/mutex_lock.hpp>

OScriptParserRef::Status OScriptParserRef::get_status() const {
    return _status;
}

String OScriptParserRef::get_path() const {
    return _path;
}

uint32_t OScriptParserRef::get_source_hash() const {
    return _source_hash;
}

OScriptParser* OScriptParserRef::get_parser() {
    if (_parser == nullptr) {
        _parser = memnew(OScriptParser);
    }
    return _parser;
}

OScriptAnalyzer* OScriptParserRef::get_analyzer() {
    if (_analyzer == nullptr) {
        _analyzer = memnew(OScriptAnalyzer(get_parser()));
    }
    return _analyzer;
}

Error OScriptParserRef::raise_status(Status p_new_status) {
    ERR_FAIL_COND_V(_clearing, ERR_BUG);
    ERR_FAIL_COND_V(_parser == nullptr && _status != EMPTY, ERR_BUG);

    while (_result == OK && p_new_status > _status) {
        switch (_status) {
            case EMPTY: {
                // Calling parse will clear the parser, which can destruct another OScriptParserRef which
                // can clear the last reference to the script with this path, calling remove_script,
                // which clears this OScriptParserRef. It's ok if it's the first thing done here.
                get_parser()->clear();
                _status = PARSED;

                String remapped_path = GDE::ResourceLoader::path_remap(_path);
                const OScriptSource source = OScriptCache::get_source_code(remapped_path);
                _source_hash = source.hash();

                _result = get_parser()->parse(source, _path);
                break;
            }
            case PARSED: {
                _status = INHERITANCE_SOLVED;
                _result = get_analyzer()->resolve_inheritance();
                break;
            }
            case INHERITANCE_SOLVED: {
                _status = INTERFACE_SOLVED;
                _result = get_analyzer()->resolve_interface();
                break;
            }
            case INTERFACE_SOLVED: {
                _status = FULLY_SOLVED;
                _result = get_analyzer()->resolve_body();
                break;
            }
            case FULLY_SOLVED: {
                return _result;
            }
        }
    }

    return _result;
}

void OScriptParserRef::clear() {
    if (_clearing) {
        return;
    }

    _clearing = true;
    OScriptParser* parser = _parser;
    OScriptAnalyzer* analyzer = _analyzer;

    _parser = nullptr;
    _analyzer = nullptr;
    _status = EMPTY;
    _result = OK;
    _source_hash = 0;

    _clearing = false;

    if (analyzer != nullptr) {
        memdelete(analyzer);
    }

    if (parser != nullptr) {
        memdelete(parser);
    }
}

OScriptParserRef::~OScriptParserRef() {
    clear();

    if (!_abandoned) {
        MutexLock lock(OScriptCache::get_cache_mutex());
        OScriptCache::_singleton->_parser_map.erase(_path);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptCache

OScriptCache* OScriptCache::_singleton = nullptr;

Mutex& OScriptCache::get_cache_mutex() {
    return *_singleton->_mutex.ptr();
}

void OScriptCache::move_script(const String& p_source, const String& p_target) {
    if (_singleton == nullptr || p_source == p_target || p_source.is_empty()) {
        return;
    }

    MutexLock lock(get_cache_mutex());
    if (_singleton->_cleared) {
        return;
    }

    remove_parser(p_source);

    if (_singleton->_shallow_cache.has(p_source) && !p_source.is_empty()) {
        _singleton->_shallow_cache[p_target] = _singleton->_shallow_cache[p_source];
    }
    _singleton->_shallow_cache.erase(p_source);

    if (_singleton->_full_cache.has(p_source) && !p_source.is_empty()) {
        _singleton->_full_cache[p_target] = _singleton->_full_cache[p_source];
    }
    _singleton->_full_cache.erase(p_source);
}

void OScriptCache::remove_script(const String& p_path) {
    if (_singleton == nullptr) {
        return;
    }

    MutexLock lock(get_cache_mutex());
    if (_singleton->_cleared) {
        return;
    }

    if (HashMap<String, Vector<ObjectID>>::Iterator E = _singleton->_abandoned_parser_map.find(p_path)) {
        for (ObjectID parser_ref_id : E->value) {
            Ref<OScriptParserRef> parser_ref = { ObjectDB::get_instance(parser_ref_id) };
            if (parser_ref.is_valid()) {
                parser_ref->clear();
            }
        }
    }

    _singleton->_abandoned_parser_map.erase(p_path);
    if (_singleton->_parser_map.has(p_path)) {
        _singleton->_parser_map[p_path]->clear();
    }

    remove_parser(p_path);

    _singleton->_dependencies.erase(p_path);
    _singleton->_shallow_cache.erase(p_path);
    _singleton->_full_cache.erase(p_path);
}

bool OScriptCache::has_parser(const String& p_path) {
    MutexLock lock(get_cache_mutex());
    return _singleton->_parser_map.has(p_path);
}

Ref<OScriptParserRef> OScriptCache::get_parser(const String& p_path, OScriptParserRef::Status p_status, Error& r_error, const String& p_owner) {
    MutexLock lock(get_cache_mutex());

    Ref<OScriptParserRef> ref;
    if (!p_owner.is_empty() && p_path != p_owner) {
        _singleton->_dependencies[p_owner].insert(p_path);
        _singleton->_parser_inverse_dependencies[p_path].insert(p_owner);
    }

    if (_singleton->_parser_map.has(p_path)) {
        ref = Ref<OScriptParserRef>(_singleton->_parser_map[p_path]);
        if (ref.is_null()) {
            r_error = ERR_INVALID_DATA;
            return ref;
        }
    } else {
        String remapped_path = GDE::ResourceLoader::path_remap(p_path);
        if (!FileAccess::file_exists(remapped_path)) {
            r_error = ERR_FILE_NOT_FOUND;
            return ref;
        }
        ref.instantiate();
        ref->_path = p_path;
        _singleton->_parser_map[p_path] = ref.ptr();
    }

    r_error = ref->raise_status(p_status);
    return ref;
}

void OScriptCache::remove_parser(const String& p_path) { // NOLINT
    MutexLock lock(get_cache_mutex());

    if (_singleton->_parser_map.has(p_path)) {
        OScriptParserRef* parser_ref = _singleton->_parser_map[p_path];
        parser_ref->_abandoned = true;

        const ObjectID object_id(parser_ref->get_instance_id());
        _singleton->_abandoned_parser_map[p_path].push_back(object_id);
    }

    // Can't clear the parser because some other parser might be currently using it in the chain of calls.
    _singleton->_parser_map.erase(p_path);

    // Have to copy while iterating, because parser_inverse_dependencies is modified.
    HashSet<String> inverse_dependencies = _singleton->_parser_inverse_dependencies[p_path];
    _singleton->_parser_inverse_dependencies.erase(p_path);
    for (const String& dependency_path : inverse_dependencies) {
        remove_parser(dependency_path);
    }
}

OScriptSource OScriptCache::get_source_code(const String& p_path) {
    return OScriptSource::load(p_path);
}

Ref<Orchestration> OScriptCache::get_orchestration(const String& p_path) {
    Ref<Orchestration> orchestration;
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    if (p_path.get_extension() == ORCHESTRATOR_SCRIPT_EXTENSION) {
        OrchestrationBinaryParser parser;
        orchestration = parser.load(local_path);
    } else {
        OrchestrationTextParser parser;
        orchestration = parser.load(local_path);
    }

    if (orchestration.is_valid()) {
        if (!orchestration->has_graph("EventGraph")) {
            WARN_PRINT("Legacy orchestration '" + p_path + "' loaded, creating event graph...");
            orchestration->create_graph("EventGraph", OScriptGraph::GraphFlags::GF_EVENT);
        }
        orchestration->post_initialize();
        return orchestration;
    }

    return {};
}

uint32_t OScriptCache::get_source_code_hash(const String& p_path) {
    if (p_path.get_extension() == ORCHESTRATOR_SCRIPT_EXTENSION) {
        const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
        ERR_FAIL_COND_V(!file.is_valid(), 0);

        const PackedByteArray bytes = file->get_buffer(file->get_length());
        return hash_djb2_buffer(bytes.ptr(), bytes.size());
    }

    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(!file.is_valid(), 0);

    const String text = file->get_as_text();
    return text.hash();
}

Ref<OScript> OScriptCache::get_shallow_script(const String& p_path, Error& r_error, const String& p_owner) {
    MutexLock lock(get_cache_mutex());

    if (!p_owner.is_empty()) {
        _singleton->_dependencies[p_owner].insert(p_path);
    }
    if (_singleton->_full_cache.has(p_path)) {
        return _singleton->_full_cache[p_path];
    }
    if (_singleton->_shallow_cache.has(p_path)) {
        return _singleton->_shallow_cache[p_path];
    }

    const String remapped_path = GDE::ResourceLoader::path_remap(p_path);

    Ref<OScript> script;
    script.instantiate();
    script->take_over_path(p_path);

    r_error = script->load_source_code(remapped_path);
    if (r_error) {
        return {}; // Returns null and does not cache when the script fails to load.
    }

    Ref<OScriptParserRef> parser_ref = get_parser(p_path, OScriptParserRef::PARSED, r_error);
    if (r_error == OK) {
        OScriptCompiler::make_scripts(script.ptr(), parser_ref->get_parser()->get_tree(), true);
    }

    _singleton->_shallow_cache[p_path] = script;
    return script;
}

Ref<OScript> OScriptCache::get_full_script(const String& p_path, Error& r_error, const String& p_owner, bool p_update_from_disk) {
    MutexLock lock(get_cache_mutex());

    if (!p_owner.is_empty()) {
        _singleton->_dependencies[p_owner].insert(p_path);
    }

    Ref<OScript> script;
    r_error = OK;
    if (_singleton->_full_cache.has(p_path)) {
        script = _singleton->_full_cache[p_path];
        if (!p_update_from_disk) {
            return script;
        }
    }

    if (script.is_null()) {
        script = get_shallow_script(p_path, r_error);
        // Only exit early if script failed to load, otherwise let reload report errors.
        if (script.is_null()) {
            return script;
        }
    }

    const String remapped_path = GDE::ResourceLoader::path_remap(p_path);

    if (p_update_from_disk) {
        r_error = script->load_source_code(remapped_path);
        if (r_error) {
            return script;
        }
    }

    // todo: Support for worker thread pool unlock allowance requires a Godot change :(
    //  In GDScriptCache, this is allowed when THREADS_ENABLED is defined. We should see if it
    //  makes sense to expose this or if we can somehow implement our own?
    #ifdef WORKERTHREAD_POOL_UNLOCK_ALLOWANCE_AVAILABLE
    // Allowing lifting the lock might cause a script to be reloaded multiple times,
    // which, as a last resort deadlock prevention strategy, is a good tradeoff.
    uint32_t allowance_id = WorkerThreadPool::thread_enter_unlock_allowance_zone(singleton->mutex);
    r_error = script->reload(true);
    WorkerThreadPool::thread_exit_unlock_allowance_zone(allowance_id);
    #else
    r_error = script->reload(true);
    #endif

    if (r_error) {
        return script;
    }

    _singleton->_full_cache[p_path] = script;
    _singleton->_shallow_cache.erase(p_path);

    // Add the script to the resource cache. Usually ResourceLoader would take care of it, but cyclic references can break that sometimes so we do it ourselves.
    // Resources don't know whether they are cached, so using `set_path()` after `set_path_cache()` does not add the resource to the cache if the path is the same.
    // We reset the cached path from `get_shallow_script()` so that the subsequent call to `set_path()` caches everything correctly.
    script->set_path_cache(String());
    script->take_over_path(p_path);

    return script;
}

Ref<OScript> OScriptCache::get_cached_script(const String& p_path) {
    MutexLock lock(get_cache_mutex());

    if (_singleton->_full_cache.has(p_path)) {
        return _singleton->_full_cache[p_path];
    }

    if (_singleton->_shallow_cache.has(p_path)) {
        return _singleton->_shallow_cache[p_path];
    }

    return {};
}

Error OScriptCache::finish_compiling(const String& p_path) {
    ERR_FAIL_COND_V_MSG(p_path.is_empty(), ERR_COMPILATION_FAILED, "Cannot finish compiling due to invalid path");

    MutexLock lock(get_cache_mutex());

    // Mark this as compiled.
    Ref<OScript> script = get_cached_script(p_path);
    _singleton->_full_cache[p_path] = script;
    _singleton->_shallow_cache.erase(p_path);

    HashSet<String> depends = _singleton->_dependencies[p_path];

    Error err = OK;
    for (const String &E : depends) {
        Error this_err = OK;
        // No need to save the script. We assume it's already referenced in the owner.
        get_full_script(E, this_err);

        if (this_err != OK) {
            err = this_err;
        }
    }

    _singleton->_dependencies.erase(p_path);

    return err;
}

void OScriptCache::add_static_script(Ref<OScript> p_script) { // NOLINT
    ERR_FAIL_COND_MSG(p_script.is_null(), "Trying to cache empty script as static.");
    ERR_FAIL_COND_MSG(!p_script->_is_valid(), "Trying to cache non-compiled script as static.");
    _singleton->_static_cache[p_script->get_fully_qualified_class_name()] = p_script;
}

void OScriptCache::remove_static_script(const String& p_fully_qualified_class_name) {
    _singleton->_static_cache.erase(p_fully_qualified_class_name);
}

void OScriptCache::clear() {
    if (_singleton == nullptr) {
        return;
    }

    MutexLock lock(get_cache_mutex());

    if (_singleton->_cleared) {
        return;
    }
    _singleton->_cleared = true;

    _singleton->_parser_inverse_dependencies.clear();

    for (const KeyValue<String, Vector<ObjectID>>& KV : _singleton->_abandoned_parser_map) {
        for (ObjectID parser_ref_id : KV.value) {
            Ref<OScriptParserRef> parser_ref = { ObjectDB::get_instance(parser_ref_id) };
            if (parser_ref.is_valid()) {
                parser_ref->clear();
            }
        }
    }

    _singleton->_abandoned_parser_map.clear();

    RBSet<Ref<OScriptParserRef>> parser_map_refs;
    for (KeyValue<String, OScriptParserRef *>& E : _singleton->_parser_map) {
        parser_map_refs.insert(E.value);
    }

    _singleton->_parser_map.clear();

    for (Ref<OScriptParserRef>& E : parser_map_refs) {
        if (E.is_valid()) {
            E->clear();
        }
    }

    parser_map_refs.clear();
    _singleton->_shallow_cache.clear();
    _singleton->_full_cache.clear();
    _singleton->_static_cache.clear();
}

OScriptCache::OScriptCache() {
    _singleton = this;
    _mutex.instantiate();
}

OScriptCache::~OScriptCache() {
    if (!_cleared) {
        clear();
    }
    _singleton = nullptr;
}