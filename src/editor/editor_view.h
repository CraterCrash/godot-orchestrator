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
#ifndef ORCHESTRATOR_EDITOR_VIEW_H
#define ORCHESTRATOR_EDITOR_VIEW_H

#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

/// Virtual base class for all Orchestrator editor viewports
class OrchestratorEditorView : public VBoxContainer
{
    GDCLASS(OrchestratorEditorView, VBoxContainer);

protected:
    static void _bind_methods();

public:

    struct EditedFileData {
        String path;
        uint64_t last_modified_time = -1;
    } edited_file_data;

    virtual Ref<Resource> get_edited_resource() const { return {}; }
    virtual void set_edited_resource(const Ref<Resource>& p_resource) { }

    virtual Control* get_editor() const { return nullptr; }

    virtual Variant get_edit_state() { return {}; }
    virtual void set_edit_state(const Variant& p_state) { }
    virtual void store_previous_state() { } // ScriptTextEditor?

    virtual void apply_code() { }
    virtual void enable_editor(Control* p_shortcut_context = nullptr) { }
    virtual void reload_text() { }
    virtual String get_name() { return {}; }
    virtual Ref<Texture2D> get_theme_icon() { return {}; }
    virtual Ref<Texture2D> get_indicator_icon() { return {}; }
    virtual bool is_unsaved() { return false; }

    virtual void add_callback(const String& p_function, const PackedStringArray& p_args) { }

    virtual PackedInt32Array get_breakpoints() { return {}; }
    virtual void set_breakpoint(int p_node, bool p_enabled) { }
    virtual void clear_breakpoints() { }
    virtual void set_debugger_active(bool p_active) { }

    virtual Control* get_edit_menu() { return nullptr; }
    virtual void clear_edit_menu() { }

    virtual void tag_saved_version() { }
    virtual void validate() { }

    virtual void update_settings() { }
    virtual void update_toggle_scripts_button() { }
    virtual void update_toggle_components_button() { }
    virtual void ensure_focus() { }

    virtual void goto_node(int p_node) { }

    virtual bool can_lose_focus_on_node_selection() const { return false; }

    ~OrchestratorEditorView() override;
};


#endif // ORCHESTRATOR_EDITOR_VIEW_H