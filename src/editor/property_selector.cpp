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
#include "editor/property_selector.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/variant_utils.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorPropertySelector::_text_changed(const String& p_new_text)
{
    _update_search();
}

void OrchestratorPropertySelector::_sbox_input(const Ref<InputEvent>& p_event)
{
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid())
    {
        switch (key->get_keycode())
        {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN:
            {
                _search_options->_gui_input(key);
                _search_box->accept_event();

                TreeItem* root = _search_options->get_root();
                if (!root->get_first_child())
                    break;

                TreeItem* current = _search_options->get_selected();

                TreeItem* item = _search_options->get_next_selected(root);
                while (item)
                {
                    item->deselect(0);
                    item = _search_options->get_next_selected(item);
                }

                current->select(0);
                break;
            }
            default:
                break;
        }
    }
}

void OrchestratorPropertySelector::_confirmed()
{
    TreeItem* item = _search_options->get_selected();
    if (!item)
        return;

    emit_signal("selected", item->get_metadata(0));
    hide();
}

void OrchestratorPropertySelector::_item_selected()
{
    // Nothing to do since we have no EditorHelpBit
    // Leaving for now in case EditorHelpBit gets exposed
}

void OrchestratorPropertySelector::_update_search()
{
    _search_options->clear();

    TreeItem* root = _search_options->create_item();

    const String search_text = _search_box->get_text().replace(" ", "_");

    List<PropertyInfo> props;
    if (_instance)
    {
        TypedArray<Dictionary> list = _instance->get_property_list();
        for (int i = 0; i < list.size(); i++)
            props.push_back(DictionaryUtils::to_property(list[i]));
    }

    TreeItem* category = nullptr;
    bool found = false;
    for (const PropertyInfo& E : props)
    {
        if (E.usage == PROPERTY_USAGE_CATEGORY)
        {
            if (category && category->get_first_child() == nullptr)
                memdelete(category);

            category = _search_options->create_item(root);
            category->set_text(0, E.name);
            category->set_selectable(0, false);

            Ref<Texture2D> icon;
            if (E.name.match("Script Variables"))
                icon = SceneUtils::get_editor_icon("Script");
            else
                icon = SceneUtils::get_class_icon(E.name);

            category->set_icon(0, icon);
            continue;
        }

        if (!(E.usage & PROPERTY_USAGE_EDITOR) && !(E.usage & PROPERTY_USAGE_SCRIPT_VARIABLE))
            continue;

        if (!_search_box->get_text().is_empty() && !E.name.containsn(search_text))
            continue;

        if (_type_filter.size() && !_type_filter.has(E.type))
            continue;

        TreeItem* item = _search_options->create_item(category ? category : root);
        item->set_text(0, E.name);
        item->set_metadata(0, E.name);
        item->set_icon(0, SceneUtils::get_class_icon(PropertyUtils::get_variant_type_name(E)));

        if (!found & !_search_box->get_text().is_empty() && E.name.containsn(search_text))
        {
            item->select(0);
            found = true;
        }
        else if (!found && _search_box->get_text().is_empty() && E.name.containsn(_selected))
        {
            item->select(0);
            found = true;
        }

        item->set_selectable(0, true);
    }

    if (category && category->get_first_child() == nullptr)
        memdelete(category);

    get_ok_button()->set_disabled(root->get_first_child() == nullptr);
}

void OrchestratorPropertySelector::select_property_from_instance(Object* p_instance, const String& p_current)
{
    _base_type = "";
    _selected = p_current;
    _type = Variant::NIL;
    _script = ObjectID();
    _instance = p_instance;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::set_type_filter(const Vector<Variant::Type>& p_type_filter)
{
    _type_filter = p_type_filter;
}

void OrchestratorPropertySelector::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        OCONNECT(_search_box, "text_changed", callable_mp(this, &OrchestratorPropertySelector::_text_changed));
        OCONNECT(_search_box, "gui_input", callable_mp(this, &OrchestratorPropertySelector::_sbox_input));
        OCONNECT(_search_options,  "item_activated", callable_mp(this, &OrchestratorPropertySelector::_confirmed));
        OCONNECT(_search_options, "cell_selected", callable_mp(this, &OrchestratorPropertySelector::_item_selected));
        OCONNECT(this, "confirmed", callable_mp(this, &OrchestratorPropertySelector::_confirmed));
    }
}

void OrchestratorPropertySelector::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("selected", PropertyInfo(Variant::STRING, "name")));
}

OrchestratorPropertySelector::OrchestratorPropertySelector()
{
    VBoxContainer* vbox = memnew(VBoxContainer);
    add_child(vbox);

    _search_box = memnew(LineEdit);
    SceneUtils::add_margin_child(vbox, "Search:", _search_box);

    _search_options = memnew(Tree);
    _search_options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    _search_options->set_hide_root(true);
    _search_options->set_hide_folding(true);
    SceneUtils::add_margin_child(vbox, "Matches:", _search_options, true);

    set_ok_button_text("Open");
    get_ok_button()->set_disabled((true));
    register_text_enter(_search_box);
    set_hide_on_ok(false);

    set_title("Select Property");
}

