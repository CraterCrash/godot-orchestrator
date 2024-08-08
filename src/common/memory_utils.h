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
#ifndef ORCHESTRATOR_MEMORY_UTILS_H
#define ORCHESTRATOR_MEMORY_UTILS_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace MemoryUtils
{
    /// Create a new instance of T with the specified value.
    /// @tparam T
    /// @param p_value the value to assign to the pointer
    /// @return the new instance pointer, should never be null
    template<typename T> T* memnew_ptr(const T& p_value)
    {
        T* ptr = memnew(T);
        *ptr = p_value;
        return ptr;
    }

    /// Create a new instance of a StringName
    /// @param p_value the value to be assigned to the new StringName pointer
    /// @return a new StringName pointer with the given value
    _FORCE_INLINE_ StringName* memnew_stringname(const StringName& p_value = StringName())
    {
        return memnew_ptr(p_value);
    }

    /// Create a new instance of a String
    /// @param p_value the value to be assigned to the new String pointer
    /// @return a new String pointer with the given value
    _FORCE_INLINE_ String* memnew_string(const String& p_value = String())
    {
        return memnew_ptr(p_value);
    }

    /// Allocates an array of T with the specified number of elements that includes a single
    /// spot at the start of the array that includes the size.
    ///
    /// @tparam T the element type
    /// @param p_size the number elements
    /// @return the pointer to the constructed array
    template<typename T> T* memnew_with_size(int p_size)
    {
        // Allocate a buffer with the specified number of elements plus an extra spot
        // at the start with the list size prepended.
        uint64_t size = sizeof(T) * p_size;
        void* ptr = memalloc(size + sizeof(int));

        // Write the list size
        *((int*) ptr) = p_size;

        // Return the pointer
        return (T*)((int*)ptr + 1);
    }

    /// Deallocates the array of T that was created using "memnew_with_size".
    ///
    /// @tparam T the element type
    /// @param p_ptr the array pointer
    template<typename T> void memdelete_with_size(const T* p_ptr)
    {
        // Deallocates the pointer created by the memnew_with_size function.
        memfree((int*)p_ptr - 1);
    }

    /// Read the array of T size.
    ///
    /// @tparam T the element type
    /// @param p_ptr the array pointer
    /// @return the array size, number of elements contained within
    template<typename T> int memnew_ptr_size(const T* p_ptr)
    {
        // Read the size from a pre-allocated pointer from memnew_with_size function.
        return !p_ptr ? 0 : *((int*)p_ptr - 1);
    }

    /// Deallocates a singular GDExtensionMethodInfo
    /// @param p_method the method info
    void free_method_info(const GDExtensionMethodInfo& p_method);

    /// Deallocates a GDExtensionPropertyInfo
    /// @param p_property the property info
    void free_property_info(const GDExtensionPropertyInfo& p_property);
}

#endif  // ORCHESTRATOR_MEMORY_UTILS_H
