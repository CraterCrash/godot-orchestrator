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
#ifndef ORCHESTRATOR_GODOT_UTILS_H
#define ORCHESTRATOR_GODOT_UTILS_H

#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_set.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace GodotUtils {

    template<typename E, typename C = godot::Comparator<E>, typename A = godot::DefaultAllocator>
    _FORCE_INLINE_ godot::RBSet<E, C, A> vector_to_rbset(const godot::Vector<E>& p_vector) {
        godot::RBSet<E, C, A> result;
        for (const E& item : p_vector)
            result.insert(item);

        return result;
    }

    template<typename E, typename C = godot::Comparator<E>, typename A= godot::DefaultAllocator>
    _FORCE_INLINE_ godot::Vector<E> rbset_to_vector(const godot::RBSet<E, C, A>& p_set) {
        godot::Vector<E> result;
        for (const E& item : p_set)
            result.push_back(item);

        return result;
    }

    template<typename T>
    _FORCE_INLINE_ godot::TypedArray<T> set_to_typed_array(const godot::HashSet<T*>& p_set) {
        godot::TypedArray<T> result;
        for (T* entry : p_set)
            result.push_back(entry);

        return result;
    }

    template<typename E, typename C = godot::Comparator<E>, typename A = godot::DefaultAllocator>
    _FORCE_INLINE_ godot::Vector<E> deduplicate(const godot::Vector<E>& p_vector) {
        return rbset_to_vector<E>(vector_to_rbset<E, C, A>(p_vector));
    }
}

#endif