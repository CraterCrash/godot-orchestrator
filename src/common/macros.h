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
#ifndef ORCHESTRATOR_MACROS_H
#define ORCHESTRATOR_MACROS_H

#define OACCEL_KEY(mask,key) (Key(static_cast<int>(mask) | static_cast<int>(key)))

#define OCONNECT(obj, signal, method)               \
        if (!obj->is_connected(signal, method)) {   \
            obj->connect(signal, method);           \
        }

#define ODISCONNECT(obj, signal, method)            \
        if (obj->is_connected(signal, method)) {    \
            obj->disconnect(signal, method);        \
        }

#define GUARD_NULL(x) if (!(x)) return;

#define EI godot::EditorInterface::get_singleton()
#define EDSCALE EI->get_editor_scale()
#define EditorNode get_tree()->get_root()->get_child(0)
#define EDITOR_GET(x) EI->get_editor_settings()->get(x)
#define PROJECT_GET(x,y,z) EI->get_editor_settings()->get_project_metadata(x,y,z)
#define PROJECT_SET(x,y,z) EI->get_editor_settings()->set_project_metadata(x,y,z)

#define callable_mp_parent(method) callable_mp(static_cast<parent_type*>(this), &parent_type::method)
#define callable_mp_this(method) callable_mp(this, &self_type::method)
#define callable_mp_this_parent(method) callable_mp(this, &parent_type::method)
#define callable_mp_cast(obj, type, method) callable_mp(static_cast<type*>(obj), &type::method)

#endif // ORCHESTRATOR_MACROS_H