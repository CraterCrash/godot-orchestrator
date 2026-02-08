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
#ifndef ORCHESTRATOR_SCRIPT_CACHE_H
#define ORCHESTRATOR_SCRIPT_CACHE_H

#include "script/script.h"
#include "script/script_source.h"

using namespace godot;

/// Forward Declaration
class OScriptAnalyzer;
class OScriptParser;

class OScriptParserRef : public RefCounted {
    GDCLASS(OScriptParserRef, RefCounted);

public:
    enum Status {
        EMPTY,
        PARSED,
        INHERITANCE_SOLVED,
        INTERFACE_SOLVED,
        FULLY_SOLVED
    };

protected:
    static void _bind_methods() {}

private:
    OScriptParser* _parser = nullptr;
    OScriptAnalyzer* _analyzer = nullptr;
    Status _status = EMPTY;
    Error _result = OK;
    String _path;
    int64_t _source_hash = 0;
    bool _clearing = false;
    bool _abandoned = false;

    friend class OScriptCache;
    friend class OScript;

public:
    Status get_status() const;
    String get_path() const;
    uint32_t get_source_hash() const;
    OScriptParser* get_parser();
    OScriptAnalyzer* get_analyzer();
    Error raise_status(Status p_new_status);
    void clear();

    OScriptParserRef() = default;
    ~OScriptParserRef() override;
};

class OScriptCache {
    // String key is the full path
    HashMap<String, OScriptParserRef*> _parser_map;
    HashMap<String, Vector<ObjectID>> _abandoned_parser_map;
    HashMap<String, Ref<OScript>> _shallow_cache;
    HashMap<String, Ref<OScript>> _full_cache;
    HashMap<String, Ref<OScript>> _static_cache;
    HashMap<String, HashSet<String>> _dependencies;
    HashMap<String, HashSet<String>> _parser_inverse_dependencies;

    friend class OScript;
    friend class OScriptParserRef;
    friend class OScriptInstance;

    static OScriptCache* _singleton;
    bool _cleared = false;

    Ref<Mutex> _mutex;
    static Mutex& get_cache_mutex();

public:
    static void move_script(const String& p_source, const String& p_target);
    static void remove_script(const String& p_path);

    static bool has_parser(const String& p_path);
    static Ref<OScriptParserRef> get_parser(const String& p_path, OScriptParserRef::Status p_status, Error& r_error, const String& p_owner = String());
    static void remove_parser(const String& p_path);

    static OScriptSource get_source_code(const String& p_path);

    static Ref<Orchestration> get_orchestration(const String& p_path);
    static uint32_t get_source_code_hash(const String& p_path);

    static Ref<OScript> get_shallow_script(const String& p_path, Error& r_error, const String& p_owner = String());
    static Ref<OScript> get_full_script(const String& p_path, Error& r_error, const String& p_owner = String(), bool p_update_from_disk = false);
    static Ref<OScript> get_cached_script(const String& p_path);

    static Error finish_compiling(const String& p_path);

    static void add_static_script(Ref<OScript> p_script);
    static void remove_static_script(const String& p_fully_qualified_class_name);

    static void clear();

    OScriptCache();
    ~OScriptCache();
};

#endif // ORCHESTRATOR_SCRIPT_CACHE_H