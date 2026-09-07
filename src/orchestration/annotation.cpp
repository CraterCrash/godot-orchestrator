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
#include "orchestration/annotation.h"

#include "orchestration/annotation_registry.h"

bool OScriptAnnotation::operator==(const OScriptAnnotation& p_other) const {
    return name == p_other.name && arguments == p_other.arguments;
}

bool OScriptAnnotation::operator!=(const OScriptAnnotation& p_other) const {
    return !(*this == p_other);
}

Dictionary OScriptAnnotation::to_dict() const {
    Dictionary dict;
    dict["name"] = name;
    if (!arguments.is_empty()) {
        dict["args"] = arguments;
    }
    return dict;
}

OScriptAnnotation OScriptAnnotation::from_dict(const Dictionary& p_dict) {
    OScriptAnnotation annotation;
    annotation.name = p_dict.get("name", StringName());
    annotation.arguments = p_dict.get("args", Array());
    return annotation;
}

OScriptAnnotation::OScriptAnnotation(const StringName& p_name, const Array& p_arguments)
    : name(p_name)
    , arguments(p_arguments) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptAnnotationList

int OScriptAnnotationList::find(const StringName& p_name) const {
    for (int i = 0; i < _items.size(); i++) {
        if (_items[i].name == p_name) {
            return i;
        }
    }
    return -1;
}

bool OScriptAnnotationList::has(const StringName& p_name) const {
    return find(p_name) >= 0;
}

bool OScriptAnnotationList::has_family(const StringName& p_family) const {
    for (const OScriptAnnotation& item : _items) {
        if (OScriptAnnotationRegistry::is_family(item.name, p_family)) {
            return true;
        }
    }
    return false;
}

Error OScriptAnnotationList::add(uint32_t p_target, const PropertyInfo& p_owner, const OScriptAnnotation& p_annotation, String* r_reason) {
    const Error result = OScriptAnnotationRegistry::can_add(p_target, p_owner, _items, p_annotation.name, r_reason);
    if (result != OK) {
        return result;
    }

    _items.push_back(p_annotation);
    return OK;
}

bool OScriptAnnotationList::remove_at(int p_index) {
    if (p_index < 0 || p_index >= _items.size()) {
        return false;
    }

    _items.remove_at(p_index);
    return true;
}

int OScriptAnnotationList::remove_family(const StringName& p_family) {
    int removed = 0;
    for (int i = _items.size() - 1; i >= 0; i--) {
        if (OScriptAnnotationRegistry::is_family(_items[i].name, p_family)) {
            _items.remove_at(i);
            removed++;
        }
    }
    return removed;
}

int OScriptAnnotationList::prune(uint32_t p_target, const PropertyInfo& p_owner) {
    int removed = 0;
    for (int i = _items.size() - 1; i >= 0; i--) {
        const OScriptAnnotationDescriptor* descriptor = OScriptAnnotationRegistry::find(_items[i].name);
        if (descriptor && !OScriptAnnotationRegistry::applies_to(*descriptor, p_target, p_owner)) {
            _items.remove_at(i);
            removed++;
        }
    }
    return removed;
}

bool OScriptAnnotationList::set_arguments(int p_index, const Array& p_arguments) {
    if (p_index < 0 || p_index >= _items.size()) {
        return false;
    }

    if (_items[p_index].arguments == p_arguments) {
        return false;
    }

    _items.write[p_index].arguments = p_arguments;
    return true;
}

Array OScriptAnnotationList::to_array() const {
    Array result;
    for (const OScriptAnnotation& item : _items) {
        result.push_back(item.to_dict());
    }
    return result;
}

void OScriptAnnotationList::from_array(const Array& p_array) {
    _items.clear();
    for (int i = 0; i < p_array.size(); i++) {
        const Variant& entry = p_array[i];
        if (entry.get_type() != Variant::DICTIONARY) {
            continue;
        }

        const OScriptAnnotation annotation = OScriptAnnotation::from_dict(entry);
        if (annotation.name.is_empty()) {
            continue;
        }

        _items.push_back(annotation);
    }
}

bool OScriptAnnotationList::operator==(const OScriptAnnotationList& p_other) const {
    if (_items.size() != p_other._items.size()) {
        return false;
    }

    for (int i = 0; i < _items.size(); i++) {
        if (_items[i] != p_other._items[i]) {
            return false;
        }
    }
    return true;
}

bool OScriptAnnotationList::operator!=(const OScriptAnnotationList& p_other) const {
    return !(*this == p_other);
}