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
#include "editor/gui/select_type_dialog.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/file_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "orchestration/variable.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/templates/rb_set.hpp>

PropertyInfo OrchestratorSelectTypeSearchDialog::_string_to_property(const String& p_value) const {
    return DictionaryUtils::to_property(UtilityFunctions::str_to_var(p_value));
}

String OrchestratorSelectTypeSearchDialog::_property_to_string(const PropertyInfo& p_property) const {
    return UtilityFunctions::var_to_str(DictionaryUtils::from_property(p_property)).replace("\n", " ");
}

String OrchestratorSelectTypeSearchDialog::_decode_property_line(const String& p_value) const {
    return OScriptVariable::decode_property(p_value);
}

String OrchestratorSelectTypeSearchDialog::_get_icon_name(const String& p_name, const String& p_fallback) {
    if (SceneUtils::has_editor_icon(p_name)) {
        return p_name;
    }
    if (!p_fallback.is_empty()) {
        return p_fallback;
    }
    return _fallback_icon;
}

String OrchestratorSelectTypeSearchDialog::_create_class_hierarchy_path(const String& p_class) {
    return StringUtils::join("/", _get_class_hierarchy(p_class));
}

PackedStringArray OrchestratorSelectTypeSearchDialog::_get_class_hierarchy(const String& p_class) {
    PackedStringArray hierarchy;
    if (ScriptServer::is_global_class(p_class)) {
        hierarchy = ScriptServer::get_class_hierarchy(p_class, true);
    } else {
        hierarchy.push_back(p_class);

        String clazz = ClassDB::get_parent_class(p_class);
        while (!clazz.is_empty()) {
            hierarchy.push_back(clazz);
            clazz = ClassDB::get_parent_class(clazz);
        }
    }
    hierarchy.reverse();
    return hierarchy;
}

void OrchestratorSelectTypeSearchDialog::_build_basic_type_items(Vector<Ref<SearchItem>>& r_items, const Ref<SearchItem>& p_parent) {
    // Walks each variant type except NIL and OBJECT.
    // OBJECT lives in its own category to deal with child types.
    for (int i = 0; i < Variant::VARIANT_MAX; i++) {
        const Variant::Type type = VariantUtils::to_type(i);
        if (type == Variant::OBJECT) {
            continue;
        }

        const String name = Variant::get_type_name(type);
        if (_exclusions.has(name)) {
            continue;
        }

        r_items.push_back(_make_item(
            name,
            type == Variant::NIL ? "Any" : VariantUtils::get_friendly_type_name(type),
            p_parent->path + "/" + name,
            p_parent,
            type == Variant::NIL ? PropertyUtils::make_variant("") : PropertyInfo(type, ""),
            type == Variant::NIL ? "Variant" : name));
    }
}

void OrchestratorSelectTypeSearchDialog::_build_object_type_items(Vector<Ref<SearchItem>>& r_items, const Ref<SearchItem>& p_parent) {
    // Compute children-by-parent for the entire class graph in a single pass.
    // This is cheaper than calling ClassDB::get_inheriters_from_class repeatedly at each depth.
    const PackedStringArray classes = ClassDB::get_class_list();
    HashMap<String, PackedStringArray> children_by_parent;
    for (const String& class_name : classes) {
        const String parent_class = ClassDB::get_parent_class(class_name);
        if (parent_class.is_empty()) {
            continue;
        }
        if (!children_by_parent.has(parent_class)) {
            children_by_parent[parent_class] = PackedStringArray();
        }
        children_by_parent[parent_class].push_back(class_name);
    }

    // Add named script classes into the same map keyed by their base type.
    const PackedStringArray global_classes = ScriptServer::get_global_class_list();
    for (const String& class_name : global_classes) {
        const String base_type = ScriptServer::get_global_class(class_name).base_type;
        if (base_type.is_empty()) {
            continue;
        }
        if (!children_by_parent.has(base_type)) {
            children_by_parent[base_type] = PackedStringArray();
        }
        children_by_parent[base_type].push_back(class_name);
    }

    _build_class_children("Object", p_parent, children_by_parent, r_items);
}

void OrchestratorSelectTypeSearchDialog::_build_class_children(
    const String& p_class_name, const Ref<SearchItem>& p_parent,
    const HashMap<String, PackedStringArray>& p_children_by_parent, Vector<Ref<SearchItem>>& r_items) {

    // Enumerations/bitfields are listed first under their class node.
    if (ClassDB::class_exists(p_class_name)) {
        PackedStringArray class_enumerations = ClassDB::class_get_enum_list(p_class_name, true);
        class_enumerations.sort();

        for (const String& enumeration : class_enumerations) {
            const String qualified_name = vformat("%s.%s", p_class_name, enumeration);
            if (_exclusions.has(qualified_name)) {
                continue;
            }

            r_items.push_back(_make_item(
                enumeration,
                enumeration,
                p_parent->path + "/" + enumeration,
                p_parent,
                ClassDB::is_class_enum_bitfield(p_class_name, enumeration)
                    ? PropertyInfo(Variant::INT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD, qualified_name)
                    : PropertyInfo(Variant::INT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, qualified_name),
                _get_icon_name(enumeration, "Enum")));
        }
    } else if (ScriptServer::is_global_class(p_class_name)) {
        // GDScript enums are Dictionary-valued entries in the constants map.
        const Dictionary constants = ScriptServer::get_global_class(p_class_name).get_constants_list();
        PackedStringArray enum_names;
        const Array keys = constants.keys();
        for (int i = 0; i < keys.size(); i++) {
            if (constants[keys[i]].get_type() == Variant::DICTIONARY) {
                enum_names.push_back(keys[i]);
            }
        }
        enum_names.sort();

        for (const String& enum_name : enum_names) {
            const String qualified_name = vformat("%s.%s", p_class_name, enum_name);
            if (_exclusions.has(qualified_name)) {
                continue;
            }

            r_items.push_back(_make_item(
                enum_name,
                enum_name,
                p_parent->path + "/" + enum_name,
                p_parent,
                PropertyInfo(Variant::INT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, qualified_name),
                _get_icon_name(enum_name, "Enum")));
        }
    }

    // Subclasses alphabetical, recursive
    if (!p_children_by_parent.has(p_class_name)) {
        return;
    }

    PackedStringArray child_classes = p_children_by_parent[p_class_name];
    child_classes.sort();

    for (const String& child_class_name : child_classes) {
        if (_exclusions.has(child_class_name)) {
            continue;
        }

        const bool is_native = ClassDB::class_exists(child_class_name);
        bool selectable;

        if (is_native) {
            // Respect base-type filter: only include classes that are descendants of _base_type
            // along with the _base_type itself. When _base_type is empty, all classes are qualified.
            if (!_base_type.is_empty() &&
                    child_class_name != _base_type &&
                    !ClassDB::is_parent_class(child_class_name, _base_type) &&
                    !ClassDB::is_parent_class(_base_type, child_class_name)) {
                continue;
            }
            // Abstract Filter: best-effort via can_instantiate().
            // This also filters out singletons, which may or may not be desirable
            const bool can_instantiate = ClassDB::can_instantiate(child_class_name);
            selectable = _allow_abstract_types || can_instantiate;
        } else {
            if (!_base_type.is_empty() &&
                    child_class_name != _base_type &&
                    !ScriptServer::is_parent_class(child_class_name, _base_type)) {
                continue;
            }
            selectable = true;
        }

        const Ref<SearchItem> item = _make_item(
            child_class_name,
            child_class_name,
            p_parent->path + "/" + child_class_name,
            p_parent,
            PropertyInfo(Variant::OBJECT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, child_class_name),
            is_native ? _get_icon_name(child_class_name) : _get_icon_name(child_class_name, "Script"),
            selectable);

        if (!is_native) {
            item->script_filename = ScriptServer::get_global_class_path(child_class_name).get_file();
        }

        if (!selectable) {
            item->disabled = true;
        }

        r_items.push_back(item);
        _build_class_children(child_class_name, item, p_children_by_parent, r_items);
    }
}

void OrchestratorSelectTypeSearchDialog::_build_global_enumeration_items(Vector<Ref<SearchItem>>& r_items, Ref<SearchItem>& r_parent, const Ref<SearchItem>& p_root) {
    const PackedStringArray enumerations = ExtensionDB::get_global_enum_names();
    PackedStringArray names;
    for (const String& enumeration : enumerations) {
        const EnumInfo& enum_info = ExtensionDB::get_global_enum(enumeration);
        if (enum_info.is_bitfield || _exclusions.has(enumeration)) {
            continue;
        }
        names.push_back(enumeration);
    }

    if (names.is_empty()) {
        r_parent = Ref<SearchItem>();
        return;
    }

    names.sort();

    r_parent = _make_item(
        "Enumeration",
        "Global Enumerations",
        p_root->path + "/Global Enumerations",
        p_root,
        PropertyInfo(),
        "Enum",
        false);

    for (const String& name : names) {
        r_items.push_back(_make_item(name,
            name,
            r_parent->path + "/" + name,
            r_parent,
            PropertyInfo(Variant::INT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, name),
            _get_icon_name(name, "Enum")));
    }
}

void OrchestratorSelectTypeSearchDialog::_build_global_bitfield_items(Vector<Ref<SearchItem>>& r_items, Ref<SearchItem>& r_parent, const Ref<SearchItem>& p_root) {
    const PackedStringArray enumerations = ExtensionDB::get_global_enum_names();
    PackedStringArray names;
    for (const String& enumeration : enumerations) {
        const EnumInfo& enum_info = ExtensionDB::get_global_enum(enumeration);
        if (!enum_info.is_bitfield || _exclusions.has(enumeration)) {
            continue;
        }
        names.push_back(enumeration);
    }

    if (names.is_empty()) {
        r_parent = Ref<SearchItem>();
        return;
    }

    names.sort();

    r_parent = _make_item(
        "Enumeration",
        "Global Bitfields",
        p_root->path + "/Global Bitfields",
        p_root,
        PropertyInfo(),
        "Enum",
        false);

    for (const String& name : names) {
        r_items.push_back(_make_item(name,
            name,
            r_parent->path + "/" + name,
            r_parent,
            PropertyInfo(Variant::INT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD, name),
            _get_icon_name(name, "Enum")));
    }
}


Ref<OrchestratorEditorSearchDialog::SearchItem> OrchestratorSelectTypeSearchDialog::_make_item(
    const String& p_name, const String& p_text, const String& p_path, const Ref<SearchItem>& p_parent,
    const PropertyInfo&& p_property, const String& p_icon, bool p_selectable) {

    Ref<SearchItem> item;
    item.instantiate();

    item->name = p_name;
    item->text = p_text;
    item->path = p_path;
    item->parent = p_parent;
    item->selectable = p_selectable;
    item->property = p_property;

    if (!p_icon.is_empty()) {
        item->icon = SceneUtils::get_editor_icon(p_icon);
    }

    return item;
}

Ref<OrchestratorEditorSearchDialog::SearchItem> OrchestratorSelectTypeSearchDialog::_get_search_item_by_property(const PropertyInfo& p_property) const {
    for (const Ref<SearchItem>& item : _search_items) {
        if (PropertyUtils::are_equal(item->property, p_property)) {
            return item;
        }
    }
    return {};
}

bool OrchestratorSelectTypeSearchDialog::_is_preferred(const String& p_type) const {
    if (ClassDB::class_exists(p_type)) {
        return ClassDB::is_parent_class(p_type, _preferred_search_result_type);
    }
    return OrchestratorEditorSearchDialog::_is_preferred(p_type);
}

bool OrchestratorSelectTypeSearchDialog::_should_collapse_on_empty_search() const {
    return _filters->get_selected_id() == FT_ALL_TYPES;
}

bool OrchestratorSelectTypeSearchDialog::_get_search_item_collapse_suggestion(TreeItem* p_item) const {
    if (p_item->get_parent()) {
        const bool can_instantiate = p_item->get_meta("__instantiable", false);
        return p_item->get_text(0) != _base_type && (p_item->get_parent()->get_text(0) != _base_type || can_instantiate);
    }
    return false;
}

void OrchestratorSelectTypeSearchDialog::_update_help(const Ref<SearchItem>& p_item) {
    const PropertyInfo property = p_item->property;
    const String symbol = property.class_name.is_empty()
        ? vformat("type|%s|", VariantUtils::get_friendly_type_name(property.type, true))
        : vformat("class|%s|", property.class_name);

    _help_bit->parse_symbol(symbol);
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectTypeSearchDialog::_get_search_items() {
    Vector<Ref<SearchItem>> items;

    const Ref<SearchItem> types = _make_item("Types", "Types", "Types", Ref<SearchItem>(), PropertyInfo(), "", false);

    Vector<Ref<SearchItem>> basic_types;
    _build_basic_type_items(basic_types, types);

    Ref<SearchItem> global_bitfields;
    Ref<SearchItem> global_enumerations;
    Vector<Ref<SearchItem>> descendants;
    _build_global_bitfield_items(descendants, global_bitfields, types);
    _build_global_enumeration_items(descendants, global_enumerations, types);

    Ref<SearchItem> object = _make_item(
        "Object",
        "Object",
        types->path + "/Object",
        types,
        PropertyInfo(Variant::OBJECT, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Object"),
        _get_icon_name("Object"));
    _build_object_type_items(descendants, object);

    struct TopLevelEntry {
        String sort_key;
        Ref<SearchItem> item;
    };

    Vector<TopLevelEntry> top_level;
    for (const Ref<SearchItem>& item : basic_types) {
        top_level.push_back({ item->text.to_lower(), item });
    }

    top_level.push_back({ "object", object });

    if (global_bitfields.is_valid()) {
        top_level.push_back({ "global bitfields", global_bitfields });
    }

    if (global_enumerations.is_valid()) {
        top_level.push_back({ "global enumerations", global_enumerations });
    }

    // Bubble sort by sort_key
    for (int i = 0; i < top_level.size(); i++) {
        for (int j = i + 1; j < top_level.size(); j++) {
            if (top_level[j].sort_key < top_level[i].sort_key) {
                TopLevelEntry tmp = top_level[i];
                top_level.write[i] = top_level[j];
                top_level.write[j] = tmp;
            }
        }
    }

    items.push_back(types);
    for (const TopLevelEntry& entry : top_level) {
        items.push_back(entry.item);
    }
    items.append_array(descendants);

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectTypeSearchDialog::_get_recent_items() const {
    Vector<Ref<SearchItem>> items;

    RBSet<String> recent_items;
    const Ref<FileAccess> recents = FileUtils::open_project_settings_file(vformat("orchestrator_recent_history.%s", _data_suffix), FileAccess::READ);
    FileUtils::for_each_line(recents, [&](const String& line) {
        if (const String trimmed = line.strip_edges(); !trimmed.is_empty()) {
            if (recent_items.has(trimmed)) {
                return;
            }

            const String value = _decode_property_line(trimmed);
            if (value.is_empty()) {
                return;
            }

            recent_items.insert(value);

            const Ref<SearchItem> search_item = _get_search_item_by_property(_string_to_property(value));
            if (search_item.is_valid()) {
                items.push_back(search_item);
            }
        }
    });

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectTypeSearchDialog::_get_favorite_items() const {
    Vector<Ref<SearchItem>> items;

    const Ref<FileAccess> recents = FileUtils::open_project_settings_file(vformat("orchestrator_favorites.%s", _data_suffix), FileAccess::READ);
    FileUtils::for_each_line(recents, [&](const String& line) {
        if (const String trimmed = line.strip_edges(); !trimmed.is_empty()) {
            const String value = _decode_property_line(trimmed);
            if (value.is_empty()) {
                return;
            }

            const Ref<SearchItem> search_item = _get_search_item_by_property(_string_to_property(value));
            if (search_item.is_valid()) {
                items.push_back(search_item);
            }
        }
    });

    return items;
}

void OrchestratorSelectTypeSearchDialog::_save_recent_items(const Vector<Ref<SearchItem>>& p_recents) {
    RBSet<String> recent_items;
    Ref<FileAccess> file = FileUtils::open_project_settings_file(vformat("orchestrator_recent_history.%s", _data_suffix), FileAccess::WRITE);
    for (const Ref<SearchItem>& item : p_recents) {
        // Always have a single line per property
        const String value = _property_to_string(item->property);
        if (recent_items.has(value)) {
            continue;
        }
        if (!value.is_empty()) {
            file->store_line(value);
        }
    }
}

void OrchestratorSelectTypeSearchDialog::_save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites) {
    Ref<FileAccess> file = FileUtils::open_project_settings_file(vformat("orchestrator_favorites.%s", _data_suffix), FileAccess::WRITE);
    for (const Ref<SearchItem>& item : p_favorites) {
        // Always have a single line per property
        const String value = _property_to_string(item->property);
        if (!value.is_empty()) {
            file->store_line(value);
        }
    }
}

Vector<OrchestratorEditorSearchDialog::FilterOption> OrchestratorSelectTypeSearchDialog::_get_filters() const {
    Vector<FilterOption> options;
    options.push_back({ FT_ALL_TYPES, "All Types" });
    options.push_back({ FT_BASIC_TYPES, "Basic Types" });
    options.push_back({ FT_BITFIELDS, "Bitfields" });
    options.push_back({ FT_ENUMERATIONS, "Enumerations" });
    options.push_back({ FT_NODES, "Nodes" });
    options.push_back({ FT_OBJECTS, "Objects" });
    options.push_back({ FT_RESOURCES, "Resources" });
    return options;
}

bool OrchestratorSelectTypeSearchDialog::_is_filtered(const Ref<SearchItem>& p_item, const String& p_text) const {
    if (!_filters) {
        return false;
    }

    const int filter_id = _filters->get_selected_id();
    if (filter_id == FT_ALL_TYPES) {
        return false;
    }

    const Ref<SearchItem> typed = p_item;
    if (!typed.is_valid()) {
        return false;
    }

    const PropertyInfo& property = typed->property;
    if (PropertyUtils::is_nil_no_variant(property)) {
        return true;
    }

    switch (filter_id) {
        case FT_BASIC_TYPES: {
            if (property.usage & (PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
                return true;
            }
            if (property.type == Variant::OBJECT) {
                return property.class_name != Object::get_class_static();
            }
            return false;
        }
        case FT_OBJECTS: {
            return property.type != Variant::OBJECT;
        }
        case FT_NODES: {
            if (property.type != Variant::OBJECT) {
                return true;
            }
            if (ClassDB::class_exists(property.class_name)) {
                return !ClassDB::is_parent_class(property.class_name, Node::get_class_static());
            }
            if (ScriptServer::is_global_class(property.class_name)) {
                return !ScriptServer::is_parent_class(property.class_name, Node::get_class_static());
            }
            return true;
        }
        case FT_ENUMERATIONS: {
            return !(property.usage & PROPERTY_USAGE_CLASS_IS_ENUM);
        }
        case FT_BITFIELDS: {
            return !(property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD);
        }
        case FT_RESOURCES: {
            if (property.type != Variant::OBJECT) {
                return true;
            }
            if (ClassDB::class_exists(property.class_name)) {
                return !ClassDB::is_parent_class(property.class_name, Resource::get_class_static());
            }
            if (ScriptServer::is_global_class(property.class_name)) {
                return !ScriptServer::is_parent_class(property.class_name, Resource::get_class_static());
            }
            return true;
        }
        default: {
            return false;
        }
    }
}

int OrchestratorSelectTypeSearchDialog::_get_default_filter() const {
    Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    return metadata->get_value("variable_type_search", "filter", 0);
}

void OrchestratorSelectTypeSearchDialog::_filter_type_changed(int p_index) {
    Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    metadata->set_value("variable_type_search", "filter", p_index);
    OrchestratorPlugin::get_singleton()->save_metadata(metadata);
}

PropertyInfo OrchestratorSelectTypeSearchDialog::get_selected() const {
    TreeItem* selected = _search_options->get_selected();
    if (!selected) {
        return {};
    }

    Ref<SearchItem> item = selected->get_meta("__item", {});
    return item.is_valid() ? item->property : PropertyInfo();
}

void OrchestratorSelectTypeSearchDialog::set_base_type(const String& p_base_type) {
    _base_type = p_base_type;
    _is_base_type_node = ClassDB::is_parent_class(p_base_type, "Node");
}

void OrchestratorSelectTypeSearchDialog::popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name) {
    _fallback_icon = SceneUtils::has_editor_icon(_base_type) ? _base_type : "Object";

    set_title(_title);
    set_ok_button_text("Change");

    OrchestratorEditorSearchDialog::popup_create(p_dont_clear, p_replace_mode, p_current_type, p_current_name);
}

void OrchestratorSelectTypeSearchDialog::_bind_methods() {

}