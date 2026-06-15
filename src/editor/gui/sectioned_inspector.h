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
#pragma once

#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/hash_map.hpp>

class OrchestratorEditorHelpBitTooltip;

using namespace godot;

class OrchestratorEditorSectionedInspectorFilter : public Object {
    GDCLASS(OrchestratorEditorSectionedInspectorFilter, Object);

    Object* _edited = nullptr;
    String _section;
    bool _allow_sub = false;
    String _search_text;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_value) const;
    //~ End Wrapped Interface

public:
    void set_section(const String& p_section, bool p_allow_sub);
    void set_edited(Object* p_edited);
    void set_text(const String& p_text);
};

class OrchestratorEditorSectionedInspector : public HSplitContainer {
    GDCLASS(OrchestratorEditorSectionedInspector, HSplitContainer);

    ObjectID _object_id;

    Tree* _sections = nullptr;
    HashMap<String, TreeItem*> _sections_map;

    OrchestratorEditorSectionedInspectorFilter* _filter = nullptr;
    EditorInspector* _inspector = nullptr;
    LineEdit* _search_box = nullptr;
    CheckButton* _advanced_toggle = nullptr;

    String _selected_category;
    bool _restrict_to_basic = false;

    int _decorate_budget = 0;

    OrchestratorEditorHelpBitTooltip* _tooltip = nullptr;
    Timer* _hover_timer = nullptr;
    uint64_t _hovered_property_id;

    void _wire_property_tooltips(Node* p_node);
    void _schedule_decorate();
    void _decorate_tooltips();

    void _property_mouse_entered(const EditorProperty* p_property);
    void _property_mouse_exited(const EditorProperty* p_property);
    void _hover_timeout();
    void _show_tooltip_for(EditorProperty* p_property);

    //~ Begin Signal Handlers
    void _section_selected();
    void _search_changed(const String& p_text);
    void _advanced_toggled(bool p_toggled);
    //~ End Signal Handlers

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    void register_search_box(LineEdit* p_search_box);
    void register_advanced_toggle(CheckButton* p_advanced_toggle);

    EditorInspector* get_inspector();
    void edit(Object* p_object);

    String get_full_item_path(const String& p_item);

    String get_current_section() const;
    void set_current_section(const String& p_section);

    void update_category_list();

    OrchestratorEditorSectionedInspector();
    ~OrchestratorEditorSectionedInspector() override;
};