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
#ifndef ORCHESTRATOR_SCRIPT_NODE_ARRAYS_H
#define ORCHESTRATOR_SCRIPT_NODE_ARRAYS_H

#include "script/script.h"
#include "script/nodes/editable_pin_node.h"

/// Creates a new array
class OScriptNodeMakeArray : public OScriptEditablePinNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeMakeArray, OScriptEditablePinNode);

    int _element_count = 0;

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "constants_and_literals"; }
    String get_icon() const override;
    void pin_default_value_changed(const Ref<OScriptNodePin>& p_pin) override;
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    void add_dynamic_pin() override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    String get_pin_prefix() const override { return "Element"; }
    //~ End OScriptEditablePinNode Interface
};

/// A node that gets a specific element at a given index within an array.
class OScriptNodeArrayGet : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayGet, OScriptNode);

    Variant::Type _collection_type = Variant::ARRAY;
    Variant::Type _index_type = Variant::NIL;
    String _collection_name = "Array";

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    bool is_pure() const override { return true; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

/// A node that sets a specific element at a given index within an array.
class OScriptNodeArraySet : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArraySet, OScriptNode);

    Variant::Type _collection_type = Variant::ARRAY;
    Variant::Type _index_type = Variant::NIL;
    String _collection_name = "Array";

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

/// A node that looks for a given item in an array returning the index if found.
class OScriptNodeArrayFind : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayFind, OScriptNode);

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface
};

/// A node that clears the contents of a given array.
class OScriptNodeArrayClear : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayClear, OScriptNode);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    //~ End OScriptNode Interface
};


/// A node that takes two nodes and appends the second into the first.
class OScriptNodeArrayAppend : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayAppend, OScriptNode);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    //~ End OScriptNode Interface
};

/// A node that adds an element to an existing array.
class OScriptNodeArrayAddElement : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayAddElement, OScriptNode);

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    //~ End OScriptNode Interface
};

/// A node that removes an element from an existing array.
class OScriptNodeArrayRemoveElement : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayRemoveElement, OScriptNode);

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    //~ End OScriptNode Interface
};

/// A node that removes an element by index from an existing array.
class OScriptNodeArrayRemoveIndex : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeArrayRemoveIndex, OScriptNode);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override;
    //~ End OScriptNode Interface
};

#endif // ORCHESTRATOR_SCRIPT_NODE_ARRAYS_H