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
#ifndef ORCHESTRATOR_CORE_GODOT_STRING_H
#define ORCHESTRATOR_CORE_GODOT_STRING_H

#include <godot_cpp/variant/string.hpp>

namespace godot {

    /// Provides a dynamic allocation wrapper for a String.
    /// This is useful when placing a Godot String in static initialization in GDExtension.
    class StringPtr {
        String* _data = nullptr;

    public:
        StringPtr& operator=(const String& p_value);
        StringPtr& operator=(const char* p_value);

        const String& get() const;

        ~StringPtr();
    };

}
#endif // ORCHESTRATOR_CORE_GODOT_STRING_H