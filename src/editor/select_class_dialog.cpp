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
#include "editor/select_class_dialog.h"

#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/templates/rb_set.hpp>

struct OrchestratorSelectClassSearchDialog::SearchItemSortPath
{
    _FORCE_INLINE_ bool operator()(const Ref<SearchItem>& a, const Ref<SearchItem>& b) const
    {
        return a->path.to_lower() < b->path.to_lower();
    }
};

bool OrchestratorSelectClassSearchDialog::_is_preferred(const String& p_item) const
{
    if (ClassDB::class_exists(p_item))
        return ClassDB::is_parent_class(p_item, _preferred_search_result_type);

    return OrchestratorEditorSearchDialog::_is_preferred(p_item);
}

bool OrchestratorSelectClassSearchDialog::_should_collapse_on_empty_search() const
{
    return true;
}

bool OrchestratorSelectClassSearchDialog::_get_search_item_collapse_suggestion(TreeItem* p_item) const
{
    if (p_item != nullptr && p_item->get_parent())
    {
        const bool can_instantiate = p_item->get_meta("__instantiable", false);
        return p_item->get_text(0) != _base_type && (p_item->get_parent()->get_text(0) != _base_type || can_instantiate);
    }
    return false;
}

void OrchestratorSelectClassSearchDialog::_update_help(const Ref<SearchItem>& p_item)
{
    // todo: consider looking at Godot to expose this information via an API
    _help_bit->set_text(vformat("No description available for [b]%s[/b]", p_item->text));
    _help_bit->set_disabled(true);
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectClassSearchDialog::_get_search_items()
{
    Vector<Ref<SearchItem>> items;

    // This class-based implementation always makes the base type the root
    Ref<SearchItem> root(memnew(SearchItem));
    root->path = _base_type;
    root->name = _base_type;
    root->text = _base_type;
    root->selectable = true;
    root->collapsed = false;
    root->set_meta("can_instantiate", ClassDB::can_instantiate(_base_type));
    items.push_back(root);

    HashMap<String, Ref<SearchItem>> hierarchy_cache;

    // Classes
    for (const String& class_name : ClassDB::get_class_list())
    {
        // Exclude Orchestrator Types
        if (class_name.begins_with("OScript") || class_name.begins_with("Orchestrator"))
            continue;

        if (_is_base_type_node && class_name.begins_with("Editor"))
            continue;

        // An internal class for the editor
        if (class_name.match("MissingNode") || class_name.match("MissingResource"))
            continue;

        items.append_array(_get_class_hierarchy_search_items(class_name, hierarchy_cache, root));
    }

    // Global Classes
    for (const String& class_name : ScriptServer::get_global_class_list())
        items.append_array(_get_class_hierarchy_search_items(class_name, hierarchy_cache, root));

    items.sort_custom<SearchItemSortPath>();

    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectClassSearchDialog::_get_recent_items() const
{
    Vector<Ref<SearchItem>> items;
    for (const String& recent_item : _read_file_lines(vformat("orchestrator_recent_history.%s", _data_suffix)))
    {
        const Ref<SearchItem> item = _get_search_item_by_name(recent_item);
        if (item.is_valid())
            items.push_back(item);
    }
    return items;
}

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectClassSearchDialog::_get_favorite_items() const
{
    Vector<Ref<SearchItem>> items;
    for (const String& recent_item : _read_file_lines(vformat("orchestrator_favorites.%s", _data_suffix)))
    {
        const Ref<SearchItem> item = _get_search_item_by_name(recent_item);
        if (item.is_valid())
            items.push_back(item);
    }
    return items;
}

void OrchestratorSelectClassSearchDialog::_save_recent_items(const Vector<Ref<SearchItem>>& p_recents)
{
    PackedStringArray items;
    for (const Ref<SearchItem>& item : p_recents)
    {
        const String name = String(item->name).strip_edges();
        if (!items.has(name))
            items.push_back(name);
    }

    _write_file_lines(vformat("orchestrator_recent_history.%s", _data_suffix), items);
}

void OrchestratorSelectClassSearchDialog::_save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites)
{
    PackedStringArray items;
    for (const Ref<SearchItem>& item : p_favorites)
    {
        const String name = String(item->name).strip_edges();
        if (!items.has(name))
            items.push_back(name);
    }

    _write_file_lines(vformat("orchestrator_favorites.%s", _data_suffix), items);
}

String OrchestratorSelectClassSearchDialog::_create_class_hierarchy_path(const String& p_class)
{
    return StringUtils::join("?", _get_class_hierarchy(p_class));
}

PackedStringArray OrchestratorSelectClassSearchDialog::_get_class_hierarchy(const String& p_class)
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

Vector<Ref<OrchestratorEditorSearchDialog::SearchItem>> OrchestratorSelectClassSearchDialog::_get_class_hierarchy_search_items(const String& p_class, HashMap<String, Ref<SearchItem>>& r_cache, const Ref<SearchItem>& p_root)
{
    Vector<Ref<SearchItem>> items;

    // Generate class hierarchy
    const PackedStringArray hierarchy = _get_class_hierarchy(p_class);

    // Remove most immediate known parent
    Ref<SearchItem> parent;

    int class_index = 0;
    for (; class_index < hierarchy.size() - 1; class_index++)
    {
        const String& class_name = hierarchy[class_index];
        if (r_cache.has(class_name))
            parent = r_cache[class_name];
        else
            break;
    }

    if (!parent.is_valid())
        parent = p_root;

    for (; class_index < hierarchy.size(); class_index++)
    {
        const String& class_name = hierarchy[class_index];

        Ref<SearchItem> item(memnew(SearchItem));
        item->path = vformat("%s", StringUtils::join("/", hierarchy.slice(0, class_index + 1)));
        item->name = class_name;
        item->text = class_name;
        item->icon = SceneUtils::get_class_icon(class_name);
        item->parent = parent;

        if (!_allow_abstract_types)
        {
            if (!ClassDB::can_instantiate(class_name))
            {
                item->selectable = false;
                item->disabled = true;
            }
            else if (Engine::get_singleton()->get_singleton_list().has(class_name))
            {
                item->selectable = false;
                item->disabled = true;
            }
        }
        else
        {
            item->selectable = true;
            item->disabled = false;
        }

        if (ScriptServer::is_global_class(class_name))
            item->script_filename = ScriptServer::get_global_class(class_name).path.get_file();

        items.push_back(item);

        r_cache[class_name] = item;
        parent = item;
    }

    return items;
}

void OrchestratorSelectClassSearchDialog::popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name)
{
    _fallback_icon = SceneUtils::has_editor_icon(_base_type) ? _base_type : "Object";

    set_title(_title);
    set_ok_button_text("Change");

    OrchestratorEditorSearchDialog::popup_create(p_dont_clear, p_replace_mode, p_current_type, p_current_name);
}

String OrchestratorSelectClassSearchDialog::get_selected() const
{
    if (TreeItem* selected = _search_options->get_selected())
    {
        Ref<SearchItem> item = selected->get_meta("__item", {});
        if (item.is_valid())
            return item->name;
    }

    return "";
}

void OrchestratorSelectClassSearchDialog::set_base_type(const String& p_base_type)
{
    _base_type = p_base_type;
    _is_base_type_node = ClassDB::is_parent_class(p_base_type, "Node");
}

void OrchestratorSelectClassSearchDialog::set_data_suffix(const String& p_data_suffix)
{
    _data_suffix = p_data_suffix;
}

void OrchestratorSelectClassSearchDialog::set_allow_abstract_types(bool p_allow_abstracts)
{
    _allow_abstract_types = p_allow_abstracts;
}

void OrchestratorSelectClassSearchDialog::set_popup_title(const String& p_title)
{
    _title = p_title;
}

void OrchestratorSelectClassSearchDialog::_notification(int p_what)
{
}

void OrchestratorSelectClassSearchDialog::_bind_methods()
{
}

OrchestratorSelectClassSearchDialog::OrchestratorSelectClassSearchDialog()
{
}