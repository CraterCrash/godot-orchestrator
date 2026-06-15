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
#include "editor/gui/sectioned_inspector.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"
#include "editor/doc/editor_help.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace {

String _process_name(const String& p_name, int p_style, const String& p_property = "", const StringName& p_class = "") {
    return p_name.capitalize();
}

bool _property_path_matches(const String& p_path, const String& p_filter, int p_style) {
    if (p_path.containsn(p_filter)) {
        return true;
    }

    const PackedStringArray sections = p_path.split("/");
    for (const String& part : sections) {
        if (p_filter.is_subsequence_ofn(_process_name(part, p_style, p_path))) {
            return true;
        }
    }

    return false;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OrchestratorEditorSectionedInspectorFilter
///

void OrchestratorEditorSectionedInspectorFilter::_get_property_list(List<PropertyInfo>* r_list) const {
    if (!_edited) {
        return;
    }

    List<PropertyInfo> properties = DictionaryUtils::to_properties(_edited->get_property_list());
    for (PropertyInfo& property : properties) {
        const int sp = property.name.find("/");
        if (property.name == StringName("resource_path")
                || property.name == StringName("resource_name")
                || property.name == StringName("resource_local_to_scene")
                || property.name.begins_with("script/")
                || property.name.begins_with("_global/script")) {
            continue;
        }

        if (sp == -1) {
            property.name = "global/" + property.name;
        }

        if (!_search_text.is_empty() && !property.name.containsn(_search_text)) {
            continue;
        }

        const String lookup = vformat("%s/", _section);
        if (property.name.begins_with(lookup)) {
            property.name = property.name.replace(lookup, "");
            if (!_allow_sub && property.name.contains("/")) {
                continue;
            }
            r_list->push_back(property);
        }
    }
}

bool OrchestratorEditorSectionedInspectorFilter::_set(const StringName& p_name, const Variant& p_value) {
    if (!_edited) {
        return false;
    }

    String name = p_name;
    if (!_section.is_empty()) {
        name = _section + "/" + name;
    }

    bool valid;
    Variant object = _edited;
    object.set(name, p_value, &valid);

    return valid;
}

bool OrchestratorEditorSectionedInspectorFilter::_get(const StringName& p_name, Variant& r_value) const {
    if (!_edited) {
        return false;
    }

    String name = p_name;
    if (!_section.is_empty()) {
        name = vformat("%s/%s", _section, name);
    }

    bool valid = false;
    Variant object = _edited;
    r_value = object.get(name, &valid);
    return valid;
}

bool OrchestratorEditorSectionedInspectorFilter::_property_can_revert(const StringName& p_name) const {
    return _edited->property_can_revert(vformat("%s/%s", _section, p_name));
}

bool OrchestratorEditorSectionedInspectorFilter::_property_get_revert(const StringName& p_name, Variant& r_value) const {
    r_value = _edited->property_get_revert(vformat("%s/%s", _section, p_name));
    return true;
}

void OrchestratorEditorSectionedInspectorFilter::set_section(const String& p_section, bool p_allow_sub) {
    _section = p_section;
    _allow_sub = p_allow_sub;
    notify_property_list_changed();
}

void OrchestratorEditorSectionedInspectorFilter::set_edited(Object* p_edited) {
    _edited = p_edited;
    notify_property_list_changed();
}

void OrchestratorEditorSectionedInspectorFilter::set_text(const String& p_text) {
    _search_text = p_text;
    notify_property_list_changed();
}

void OrchestratorEditorSectionedInspectorFilter::_bind_methods() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OrchestratorEditorSectionedInspector
///

void OrchestratorEditorSectionedInspector::_wire_property_tooltips(Node* p_node) {
    if (EditorProperty* ep = cast_to<EditorProperty>(p_node)) {
        if (!ep->has_meta("_oc_tooltip_wired")) {
            ep->set_meta("_oc_tooltip_wired", true);
            ep->connect("mouse_entered", callable_mp_this(_property_mouse_entered).bind(ep));
            ep->connect("mouse_exited", callable_mp_this(_property_mouse_exited).bind(ep));
        }
    }

    const int count = p_node->get_child_count();
    for (int i = 0; i < count; i++) {
        _wire_property_tooltips(p_node->get_child(i));
    }
}

void OrchestratorEditorSectionedInspector::_schedule_decorate() {
    if (!is_inside_tree()) {
        return;
    }

    _decorate_budget = 3;

    const Callable handler = callable_mp_this(_decorate_tooltips);
    if (!get_tree()->is_connected("process_frame", handler)) {
        get_tree()->connect("process_frame", handler);
    }
}

void OrchestratorEditorSectionedInspector::_decorate_tooltips() {
    if (is_visible_in_tree()) {
        _wire_property_tooltips(_inspector);
    }

    if (--_decorate_budget <= 0) {
        get_tree()->disconnect("process_frame", callable_mp_this(_decorate_tooltips));
    }
}

void OrchestratorEditorSectionedInspector::_property_mouse_entered(const EditorProperty* p_property) {
    _hovered_property_id = p_property->get_instance_id();

    // Dismiss any current tooltip and restart the hover delay
    // Moving between properties waits the delay rather than swapping the tooltip instantly.
    _tooltip->cancel_dismiss();
    _tooltip->hide();
    _hover_timer->start();
}

void OrchestratorEditorSectionedInspector::_property_mouse_exited(const EditorProperty* p_property) {
    _hover_timer->stop();
    if (_hovered_property_id == p_property->get_instance_id()) {
        _hovered_property_id = ObjectID();
    }

    _tooltip->request_dismiss();
}

void OrchestratorEditorSectionedInspector::_hover_timeout() {
    if (EditorProperty* ep = cast_to<EditorProperty>(ObjectDB::get_instance(_hovered_property_id))) {
        _show_tooltip_for(ep);
    }
}

void OrchestratorEditorSectionedInspector::_show_tooltip_for(EditorProperty* p_property) {
    // The inspector edits the filter proxy, so resolve the class from the real edited object.
    const Object* object = ObjectDB::get_instance(_object_id);
    if (!object) {
        return;
    }

    const String leaf = p_property->get_edited_property();
    const String full_path = _selected_category.is_empty() ? leaf : (_selected_category + "/" + leaf);

    _tooltip->set_content(vformat("property|%s|%s", object->get_class(), full_path));
    _tooltip->popup_at_mouse();
}

void OrchestratorEditorSectionedInspector::_section_selected() {
    if (!_sections->get_selected()) {
        return;
    }

    _selected_category = _sections->get_selected()->get_metadata(0);
    _filter->set_section(_selected_category, _sections->get_selected()->get_first_child() == nullptr);
    callable_mp_cast(_inspector, ScrollContainer, set_v_scroll).call_deferred(0);

    emit_signal("category_changed", _selected_category);

    _schedule_decorate();
}

void OrchestratorEditorSectionedInspector::_search_changed(const String& p_text) {
    if (_advanced_toggle) {
        if (p_text.is_empty()) {
            _advanced_toggle->set_pressed_no_signal(!_restrict_to_basic);
            _advanced_toggle->set_disabled(false);
            _advanced_toggle->set_tooltip_text(String());
        } else {
            _advanced_toggle->set_pressed_no_signal(true);
            _advanced_toggle->set_disabled(true);
            _advanced_toggle->set_tooltip_text("Advanced settings are always shown when searching.");
        }
    }
    update_category_list();
    _filter->set_text(p_text);
    _schedule_decorate();
}

void OrchestratorEditorSectionedInspector::_advanced_toggled(bool p_toggled) {
    _restrict_to_basic = !p_toggled;
    update_category_list();

    // todo: Would be nice to expose this on EditorInspector
    // _inspector->set_restrict_to_basic_settings(_restrict_to_basic);

    _schedule_decorate();
}

void OrchestratorEditorSectionedInspector::register_search_box(LineEdit* p_search_box) {
    _search_box = p_search_box;

    // todo: This can only be done when using EditorInspector::create_default_inspector
    // But if an implementation doesn't want the same layout as the InspectorDock, this limits.
    // _inspector->register_text_enter(p_search_box);

    _search_box->connect(SceneStringName(text_changed), callable_mp_this(_search_changed));
}

void OrchestratorEditorSectionedInspector::register_advanced_toggle(CheckButton* p_advanced_toggle) {
    _advanced_toggle = p_advanced_toggle;
    _advanced_toggle->connect(SceneStringName(toggled), callable_mp_this(_advanced_toggled));
    _advanced_toggled(_advanced_toggle->is_pressed());
}

EditorInspector* OrchestratorEditorSectionedInspector::get_inspector() {
    return _inspector;
}

void OrchestratorEditorSectionedInspector::edit(Object* p_object) {
    if (!p_object) {
        _object_id = ObjectID();
        _sections->clear();

        _filter->set_edited(nullptr);
        _inspector->edit(nullptr);

        return;
    }

    ObjectID id(p_object->get_instance_id());

    // todo: this is only used when use_doc_hints can be set, to build the doc hint tooltip
    // Given that we build our own tooltips, we can forgo this for now.
    //_inspector->set_object_class(p_object->get_class());

    if (_object_id != id) {
        _object_id = id;
        update_category_list();

        _filter->set_edited(p_object);
        _inspector->edit(_filter);

        TreeItem* first_item = _sections->get_root();
        if (first_item) {
            while (first_item->get_first_child()) {
                first_item = first_item->get_first_child();
            }

            first_item->select(0);
            _selected_category = first_item->get_metadata(0);
        }
    } else {
        update_category_list();
    }

    _schedule_decorate();
}

String OrchestratorEditorSectionedInspector::get_full_item_path(const String& p_item) {
    String base = get_current_section();
    if (!base.is_empty()) {
        return base + "/" + p_item;
    } else {
        return p_item;
    }
}

String OrchestratorEditorSectionedInspector::get_current_section() const {
    if (_sections->get_selected()) {
        return _sections->get_selected()->get_metadata(0);
    }
    return "";
}

void OrchestratorEditorSectionedInspector::set_current_section(const String& p_section) {
    if (_sections_map.has(p_section)) {
        TreeItem* item = _sections_map[p_section];
        item->select(0);
        _sections->scroll_to_item(item);
    }
}

void OrchestratorEditorSectionedInspector::update_category_list() {
    _sections->clear();

    Object* object = ObjectDB::get_instance(_object_id);
    if (!object) {
        return;
    }

    _sections_map.clear();
    TreeItem* root = _sections->create_item();
    _sections_map[""] = root;

    String filter_text;
    if (_search_box) {
        filter_text = _search_box->get_text();
    }

    List<PropertyInfo> properties = DictionaryUtils::to_properties(object->get_property_list());
    for (PropertyInfo& property : properties) {
        if (property.usage & PROPERTY_USAGE_CATEGORY) {
            continue;
        }
        if (!(property.usage & PROPERTY_USAGE_EDITOR)
                || (filter_text.is_empty() && _restrict_to_basic && !(property.usage & PROPERTY_USAGE_EDITOR_BASIC_SETTING))) {
            continue;
        }
        if (property.name.contains(":")
                || property.name == StringName("script")
                || property.name == StringName("resource_name")
                || property.name == StringName("resource_path")
                || property.name == StringName("resource_local_to_scene")
                || property.name.begins_with("_global_script")) {
            continue;
        }
        if (!filter_text.is_empty() && !_property_path_matches(property.name, filter_text, 1)) {
            continue;
        }

        int sp = property.name.find("/");
        if (sp == -1) {
            property.name = "global/" + property.name;
        }

        const PackedStringArray section_array = property.name.split("/");
        String meta_section;

        int sc = MIN(2, section_array.size() - 1);
        for (int i = 0; i < sc; i++) {
            TreeItem* parent = _sections_map[meta_section];
            parent->set_custom_font(0, SceneUtils::get_editor_font("bold"));

            if (i > 0) {
                meta_section += "/" + section_array[i];
            } else {
                meta_section = section_array[i];
            }

            if (!_sections_map.has(meta_section)) {
                TreeItem* ms = _sections->create_item(parent);
                _sections_map[meta_section] = ms;

                const String text = _process_name(section_array[i], 1, property.name);
                const String tooltip = _process_name(section_array[i], 1, property.name);

                ms->set_text(0, text);
                ms->set_tooltip_text(0, tooltip);
                ms->set_metadata(0, meta_section);
                ms->set_selectable(0, false);
            }

            if (i == sc - 1) {
                // if it has children, make selectable
                _sections_map[meta_section]->set_selectable(0, true);
            }
        }
    }

    if (_sections_map.has(_selected_category)) {
        _sections_map[_selected_category]->select(0);
    }

    // _inspector->update_tree();
}

void OrchestratorEditorSectionedInspector::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_TRANSLATION_CHANGED: {
            if (_sections->get_root()) {
                callable_mp_this(update_category_list).call_deferred();
            }
            break;
        }
    }
}

void OrchestratorEditorSectionedInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("update_category_list"), &OrchestratorEditorSectionedInspector::update_category_list);
    ADD_SIGNAL(MethodInfo("category_changed", PropertyInfo(Variant::STRING, "new_category")));
}

OrchestratorEditorSectionedInspector::OrchestratorEditorSectionedInspector() {
    _sections = memnew(Tree);
    _filter = memnew(OrchestratorEditorSectionedInspectorFilter);

    // There are a few methods that would be useful to expose
    // set_object_class
    // set_restrict_to_basic_settings
    // set_property_prefix
    // set_scroll_offset
    // set_use_doc_hints
    // update_tree
    _inspector = memnew(EditorInspector);

    add_theme_constant_override("autohide", 1);

    VBoxContainer* left_vb = memnew(VBoxContainer);
    left_vb->set_custom_minimum_size(Size2(190, 0) * EDSCALE);
    add_child(left_vb);

    _sections->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    _sections->set_v_size_flags(SIZE_EXPAND_FILL);
    _sections->set_hide_root(true);
    _sections->set_theme_type_variation("TreeSecondary");
    left_vb->add_child(_sections, true);

    VBoxContainer* right_vb = memnew(VBoxContainer);
    right_vb->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
    right_vb->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(right_vb);

    _inspector->set_v_size_flags(SIZE_EXPAND_FILL);
    right_vb->add_child(_inspector, true);
    // Not exposed to GDExtension
    // _inspector->set_use_doc_hints(true);
    _inspector->set_theme_type_variation("TreeSecondary");

    _sections->connect("cell_selected", callable_mp_this(_section_selected));

    _tooltip = memnew(OrchestratorEditorHelpBitTooltip);
    add_child(_tooltip);

    _hover_timer = memnew(Timer);
    _hover_timer->set_one_shot(true);
    _hover_timer->set_wait_time(0.5);
    _hover_timer->connect("timeout", callable_mp_this(_hover_timeout));
    add_child(_hover_timer);
}

OrchestratorEditorSectionedInspector::~OrchestratorEditorSectionedInspector() {
    memdelete(_filter);
}