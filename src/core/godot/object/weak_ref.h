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
#ifndef ORCHESTRATOR_WEAK_REF_H
#define ORCHESTRATOR_WEAK_REF_H

#include <godot_cpp/core/object.hpp>

using namespace godot;

/// A useful template class for managing weak references to Godot objects
template <typename T>
class WeakRef {
    ObjectID _id;

public:
    void set(T* p_object) {
        _id = p_object ? p_object->get_instance_id() : ObjectID();
    }

    T* get() {
        Object* object = ObjectDB::get_instance(_id);
        return Object::cast_to<T>(object);
    }

    bool is_valid() const { return ObjectDB::get_instance(_id) != nullptr; }

    ObjectID get_id() const { return _id; }

    void reset() { _id = ObjectID(); }

    T* operator->() { return get(); }
    T* operator->() const { return get(); }
    T& operator*() { return *get(); }

    explicit operator bool() const { return is_valid(); }

    operator T*() { return get(); } // NOLINT
    operator T*() const { return get(); } // NOLINT

    WeakRef& operator=(T* p_object) {
        set(p_object);
        return *this;
    }

    explicit WeakRef(T* p_object) {
        if (p_object) {
            _id = p_object->get_instance_id();
        }
    }

    WeakRef() : _id(ObjectID()) { }
};

#endif // ORCHESTRATOR_WEAK_REF_H