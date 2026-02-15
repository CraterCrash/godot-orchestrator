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
#ifndef ORCHESTRATOR_ABOUT_DIALOG_H
#define ORCHESTRATOR_ABOUT_DIALOG_H

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/link_button.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// The plug-in about dialog window
class OrchestratorAboutDialog : public AcceptDialog {
    GDCLASS(OrchestratorAboutDialog, AcceptDialog);

    LinkButton* _version_btn = nullptr;
    LinkButton* _patreon_btn = nullptr;
    RichTextLabel* _license_text = nullptr;
    TextureRect* _logo = nullptr;
    bool _theme_changing = false;
    Vector<ItemList*> _name_lists;

    ScrollContainer* _populate_list(const String& p_name, const List<String>& p_sections, const char* const* const p_src[],
        int p_flag_single_column = 0, bool p_donor = false, bool p_allow_website = false);

    //~ Signal Handlers
    void _theme_changed();
    void _version_pressed();
    void _patreon_pressed();
    void _website_selected(int p_id, ItemList* p_list);
    void _item_list_resized(ItemList* p_list);
    //~ End Signal Handlers

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    OrchestratorAboutDialog();
};

#endif  // ORCHESTRATOR_ABOUT_DIALOG_H