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

#include "common/version.h"

#define OACCEL_KEY(mask,key) (Key(static_cast<int>(mask) | static_cast<int>(key)))

#define OCONNECT(obj, signal, method)               \
        if (!obj->is_connected(signal, method)) {   \
            obj->connect(signal, method);           \
        }

#define ODISCONNECT(obj, signal, method)            \
        if (obj->is_connected(signal, method)) {    \
            obj->disconnect(signal, method);        \
        }

#if GODOT_VERSION < 0x040202
#define GDE_NOTIFICATION(p, x) p::_notification(x);
#else
#define GDE_NOTIFICATION(p, x)
#endif

#define BEGIN_NOTIFICATION_HANDLER(x)               \
    switch(x) {

#define NOTIFICATION_HANDLER(code, method)          \
    case code: method(); break;

#define END_NOTIFICATION_HANDLER                    \
    default: break;                                 \
    }

#define BEGIN_NOTIFICATION_HANDLER_WITH_PARENT(x)   \
    GDE_NOTIFICATION(x)                             \
    BEGIN_NOTIFICATION_HANDLER

#define CAST_INT_TO_ENUM(t, x) static_cast<t>(static_cast<int>(x))

#define EI godot::EditorInterface::get_singleton()
#define EDSCALE EI->get_editor_scale()
#define EditorNode get_tree()->get_root()->get_child(0)
#define EDITOR_GET(x) EI->get_editor_settings()->get(x)
#define PROJECT_GET(x,y,z) EI->get_editor_settings()->get_project_metadata(x,y,z)
#define PROJECT_SET(x,y,z) EI->get_editor_settings()->set_project_metadata(x,y,z)
#define EDITOR_GET_ENUM(t, x) static_cast<t>(static_cast<int>(EDITOR_GET(x)))

// Taken from control.h
#define SET_DRAG_FORWARDING_GCD(from, to)                                           \
    from->set_drag_forwarding(callable_mp(this, &to::get_drag_data_fw).bind(from),  \
    callable_mp(this, &to::can_drop_data_fw).bind(from),                            \
    callable_mp(this, &to::drop_data_fw).bind(from));

#define GUARD_NULL(x) if (!(x)) return;

#define SAFE_MEMDELETE(obj) { memdelete(obj); obj = nullptr; }

#define SAFE_REMOVE_CHILDREN(obj)                                       \
    for (int i = obj->get_child_count() - 1; i >= 0; i--) {             \
        Node* child = obj->get_child(i);                                \
        obj->remove_child(child);                                       \
        child->queue_free();                                            \
    }

#define callable_mp_parent(method) callable_mp(static_cast<parent_type*>(this), &parent_type::method)
#define callable_mp_this(method) callable_mp(this, &self_type::method)
#define callable_mp_this_parent(method) callable_mp(this, &parent_type::method)
#define callable_mp_cast(obj, type, method) callable_mp(static_cast<type*>(obj), &type::method)

// Takes the {@code evt} and propagates the event from {@code source} to {@code target}.
#define push_and_accept_event(evt, source, target)  \
    (target)->grab_focus();                         \
    (target)->get_viewport()->push_input(evt);      \
    (source)->accept_event();                       \
    (source)->grab_focus();

#endif // ORCHESTRATOR_MACROS_H