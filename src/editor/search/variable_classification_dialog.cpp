// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "editor/search/variable_classification_dialog.h"

#include "api/extension_db.h"
#include "common/file_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/templates/rb_set.hpp>

struct OrchestratorVariableTypeSearchDialog::SearchItemSortPath
{
    _FORCE_INLINE_ bool operator()(const Ref<SearchItem>& a, const Ref<SearchItem>& b) const
    {
        return a->path.to_lower() < b->path.to_lower();
    }
};

String OrchestratorVariableTypeSearchDialog::_create_class_hierarchy_path(const String& p_class)
{
    return StringUtils::join("/", _get_class_hiearchy(p_class));
}

PackedStringArray OrchestratorVariableTypeSearchDialog::_get_class_hiearchy(const String& p_class)
{
    PackedStringArray hierarchy;
    if (ScriptServer::is_global_class(p_class))
    {
        hierarchy = ScriptServer::get_class_hierarchy(p_class, true);
    }
    else
    {
        hierarchy.push_back(p_class);

        String clazz = ClassDB::get_parent_class(p_class);
        while (!clazz.is_empty())
        {
            hierarchy.push_back(clazz);
            clazz = ClassDB::get_parent_class(clazz);
        }
    }
    hierarchy.reverse();
    return hierarchy;
}

void OrchestratorVariableTypeSearchDialog::_update_help(const Ref<SearchItem>& p_item)
{
    _help_bit->set_text(vformat("No description available for [b]%s[/b].", p_item->text));
    _help_bit->set_disabled(true);
}

bool OrchestratorVariableTypeSearchDialog::_is_preferred(const String& p_type) const
{
    if (ClassDB::class_exists(p_type))
        return ClassDB::is_parent_class(p_type, _preferred_search_result_type);

    return OrchestratorEditorSearchDialog::_is_preferred(p_type);
}

bool OrchestratorVariableTypeSearchDialog::_get_search_item_collapse_suggestion(TreeItem* p_item) const
{
    if (p_item->get_parent())
    {
        const bool can_instantiate = p_item->get_meta("__instantiable", false);
        return p_item->get_text(0) != _base_type && (p_item->get_parent()->get_text(0) != _base_type || can_instantiate);
    }
    return false;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorVariableTypeSearchDialog::_get_search_items()
{
    Vector<Ref<SearchItem>> items;

    Ref<SearchItem> root(memnew(SearchItem));
    root->path = "Types";
    root->name = "Types";
    root->text = "Types";
    root->selectable = false;
    root->collapsed = false; // Root always expanded
    root->set_meta("can_instantiate", false);
    items.push_back(root);

    // Basic Types
    for (int i = 0; i < Variant::VARIANT_MAX; i++)
    {
        const String variant_type = Variant::get_type_name(VariantUtils::to_type(i));
        _variant_type_names.push_back(variant_type);

        if (_exclusions.has(variant_type))
            continue;

        Ref<SearchItem> item(memnew(SearchItem));
        item->path = vformat("Types/%s", i == 0 ? "Any" : variant_type);
        item->name = vformat("type:%s", variant_type);
        item->text = i == 0 ? "Any" : variant_type;
        item->icon = SceneUtils::get_editor_icon(i == 0 ? "Variant" : item->text);
        item->selectable = true;
        item->parent = root;
        items.push_back(item);
    }

    // Ref<SearchItem> custom_enum(memnew(SearchItem));
    // custom_enum->path = "Types/Custom Enum";
    // custom_enum->name = "custom_enum:Custom Enum";
    // custom_enum->text = "Custom Enum";
    // custom_enum->icon = SceneUtils::get_editor_icon("ImportCheck");
    // custom_enum->selectable = true;
    // custom_enum->parent = root;
    // items.push_back(custom_enum);
    //
    // Ref<SearchItem> custom_bitfield(memnew(SearchItem));
    // custom_bitfield->path = "Types/Custom Bitfield";
    // custom_bitfield->name = "custom_bitfield:Custom Bitfield";
    // custom_bitfield->text = "Custom Bitfield";
    // custom_bitfield->icon = SceneUtils::get_editor_icon("ImportCheck");
    // custom_bitfield->selectable = true;
    // custom_bitfield->parent = root;
    // items.push_back(custom_bitfield);

    // Global Enumerations
    for (const String& enum_name : ExtensionDB::get_global_enum_names())
    {
        if (_exclusions.has(enum_name))
            continue;

        // Automatically exclude Variant.Type and Variant.Operator
        if (enum_name.begins_with("Variant."))
            continue;

        const EnumInfo& ei = ExtensionDB::get_global_enum(enum_name);

        Ref<SearchItem> item(memnew(SearchItem));
        item->path = vformat("Types/%s", enum_name);
        item->name = vformat("%s:%s", ei.is_bitfield ? "bitfield" : "enum", enum_name);
        item->text = enum_name;
        item->icon = SceneUtils::get_editor_icon("Enum");
        item->selectable = true;
        item->parent = root;
        items.push_back(item);
    }

    HashMap<String, Ref<SearchItem>> hierarchy_lookup;

    // Classes
    for (const String& class_name : ClassDB::get_class_list())
    {
        // Exclude Orchestrator types
        if (class_name.begins_with("OScript") || class_name.begins_with("Orchestrator"))
            continue;

        if (_is_base_type_node && class_name.begins_with("Editor"))
            continue;

        // An internal class for the editor
        if (class_name.match("MissingNode") || class_name.match("MissingResource"))
            continue;

        bool excluded = false;
        for (const StringName& excluded_name : _exclusions)
        {
            if (ClassDB::is_parent_class(class_name, excluded_name))
            {
                excluded = true;
                break;
            }
        }
        if (excluded)
            continue;

        items.append_array(_get_class_hierarchy_search_items(class_name, hierarchy_lookup, root));
    }

    // Class/Type-based enumerations
    for (const String& class_name : ClassDB::get_class_list())
    {
        for (const String& enum_name : ClassDB::class_get_enum_list(class_name, true))
        {
            const String enum_full_name = vformat("%s.%s", class_name, enum_name);
            if (_exclusions.has(enum_full_name))
                continue;

            const bool bitfield = ExtensionDB::is_class_enum_bitfield(class_name, enum_name);

            // Create class hierarchy, if it doesn't exist
            items.append_array(_get_class_hierarchy_search_items(class_name, hierarchy_lookup, root));

            Ref<SearchItem> item(memnew(SearchItem));
            item->path = vformat("Types/%s/%s", _create_class_hierarchy_path(class_name), enum_name);
            item->name = vformat("%s:%s.%s", bitfield ? "class_bitfield" : "class_enum", class_name, enum_name);
            item->text = enum_name;
            item->icon = SceneUtils::get_editor_icon("Enum");
            item->parent = hierarchy_lookup[class_name];
            items.push_back(item);
        }
    }

    // Global Class Types
    for (const String& class_name : ScriptServer::get_global_class_list())
    {
        if (_exclusions.has(class_name))
            continue;

        // Create hierarchy if it doesn't exist.
        items.append_array(_get_class_hierarchy_search_items(class_name, hierarchy_lookup, root));
    }

    items.sort_custom<SearchItemSortPath>();

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorVariableTypeSearchDialog::_get_class_hierarchy_search_items(
    const String& p_class, HashMap<String, Ref<SearchItem>>& r_cache, const Ref<SearchItem>& p_root)
{
    Vector<Ref<SearchItem>> items;

    // Generate class hierarchy
    const PackedStringArray hierarchy = _get_class_hiearchy(p_class);

    // Resolve most immediate known parent
    int class_index = 0;
    Ref<SearchItem> parent;
    for (; class_index < hierarchy.size() - 1; class_index++)
    {
        const String& clazz = hierarchy[class_index];
        if (r_cache.has(clazz))
            parent = r_cache[clazz];
        else
            break;
    }

    if (!parent.is_valid())
        parent = p_root;

    for (; class_index < hierarchy.size(); class_index++)
    {
        const String& clazz_name = hierarchy[class_index];

        Ref<SearchItem> item(memnew(SearchItem));
        item->path = vformat("Types/%s", StringUtils::join("/", hierarchy.slice(0, class_index + 1)));
        item->name = vformat("class:%s", clazz_name);
        item->text = clazz_name;
        item->icon = SceneUtils::get_class_icon(clazz_name);
        item->parent = parent;

        if (!ClassDB::can_instantiate(clazz_name))
        {
            item->selectable = false;
            item->disabled = true;
        }
        else if (Engine::get_singleton()->get_singleton_list().has(clazz_name))
        {
            item->selectable = false;
            item->disabled = true;
        }

        items.push_back(item);

        r_cache[clazz_name] = item;
        parent = item;
    }

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorVariableTypeSearchDialog::_get_recent_items() const
{
    Vector<Ref<SearchItem>> items;

    RBSet<String> recent_items;
    const Ref<FileAccess> recents = FileUtils::open_project_settings_file("orchestrator_recent_history.variable_type", FileAccess::READ);
    FileUtils::for_each_line(recents, [&](const String& line) {
        if (const String trimmed = line.strip_edges(); !trimmed.is_empty())
        {
            if (recent_items.has(trimmed))
                return;

            recent_items.insert(trimmed);

            const Ref<SearchItem> search_item = _get_search_item_by_name(trimmed);
            if (search_item.is_valid())
                items.push_back(search_item);
        }
    });

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorVariableTypeSearchDialog::_get_favorite_items() const
{
    Vector<Ref<SearchItem>> items;

    const Ref<FileAccess> recents = FileUtils::open_project_settings_file("orchestrator_favorites.variable_type", FileAccess::READ);
    FileUtils::for_each_line(recents, [&](const String& line) {
        if (const String trimmed = line.strip_edges(); !trimmed.is_empty())
        {
            // const TypeEntry entry = _get_entry_from_name(trimmed);
            // if (_is_recognized(entry))
            const Ref<SearchItem> search_item = _get_search_item_by_name(trimmed);
            if (search_item.is_valid())
                items.push_back(search_item);
        }
    });

    return items;
}

void OrchestratorVariableTypeSearchDialog::_save_recent_items(const Vector<Ref<SearchItem>>& p_recents)
{
    RBSet<String> recent_items;
    Ref<FileAccess> file = FileUtils::open_project_settings_file("orchestrator_recent_history.variable_type", FileAccess::WRITE);
    for (const Ref<SearchItem>& item : p_recents)
    {
        const String name = String(item->name).strip_edges();
        if (recent_items.has(name))
            continue;

        if (!name.is_empty())
            file->store_line(name);
    }
}

void OrchestratorVariableTypeSearchDialog::_save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites)
{
    Ref<FileAccess> file = FileUtils::open_project_settings_file("orchestrator_favorites.variable_type", FileAccess::WRITE);
    for (const Ref<SearchItem>& item : p_favorites)
    {
        const String name = String(item->name).strip_edges();
        if (!name.is_empty())
            file->store_line(name);
    }
}

Vector<OrchestratorEditorSearchDialog::FilterOption> OrchestratorVariableTypeSearchDialog::_get_filters() const
{
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

bool OrchestratorVariableTypeSearchDialog::_is_filtered(const Ref<SearchItem>& p_item, const String& p_text) const
{
    if (!_filters)
        return false;

    switch (_filters->get_selected_id())
    {
        case FT_ALL_TYPES:
            return false;
        case FT_BASIC_TYPES:
        {
            if (p_item->name.begins_with("type:"))
                return false;
            break;
        }
        case FT_BITFIELDS:
        {
            if (p_item->name.begins_with("bitfield:") || p_item->name.begins_with("class_bitfield:"))
                return false;
            break;
        }
        case FT_ENUMERATIONS:
        {
            if (p_item->name.begins_with("enum:") || p_item->name.begins_with("class_enum:"))
                return false;
            break;
        }
        case FT_NODES:
        {
            if (p_item->name.begins_with("class:") && p_item->path.begins_with("Types/Object/Node"))
                return false;
            break;
        }
        case FT_OBJECTS:
        {
            if (p_item->name.begins_with("class:") && p_item->path.begins_with("Types/Object"))
                return false;
            break;
        }
        case FT_RESOURCES:
        {
            if (p_item->name.begins_with("class:") && p_item->path.begins_with("Types/Object/RefCounted/Resource/"))
                return false;
            break;
        }
        default:
            break;
    }
    return true;
}

int OrchestratorVariableTypeSearchDialog::_get_default_filter() const
{
    Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    return metadata->get_value("variable_type_search", "filter", 0);
}

void OrchestratorVariableTypeSearchDialog::_filter_type_changed(int p_index)
{
    Ref<ConfigFile> metadata = OrchestratorPlugin::get_singleton()->get_metadata();
    metadata->set_value("variable_type_search", "filter", p_index);
    OrchestratorPlugin::get_singleton()->save_metadata(metadata);
}

String OrchestratorVariableTypeSearchDialog::get_selected_type() const
{
    TreeItem* selected = _search_options->get_selected();
    if (!selected)
        return {};

    Ref<SearchItem> item = selected->get_meta("__item", {});
    return item.is_valid() ? item->name : String();
}

void OrchestratorVariableTypeSearchDialog::set_base_type(const String& p_base_type)
{
    _base_type = p_base_type;
    _is_base_type_node = ClassDB::is_parent_class(p_base_type, "Node");
}

void OrchestratorVariableTypeSearchDialog::popup_create(bool p_dont_clear, bool p_replace_mode,
                                                        const String& p_current_type, const String& p_current_name)
{
    _fallback_icon = SceneUtils::has_editor_icon(_base_type) ? _base_type : "Object";

    set_title("Select Variable Type");
    set_ok_button_text("Change");

    register_text_enter(_search_box);

    OrchestratorEditorSearchDialog::popup_create(p_dont_clear, p_replace_mode, p_current_type, p_current_name);
}
