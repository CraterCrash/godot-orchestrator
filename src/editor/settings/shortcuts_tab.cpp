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
#include "editor/settings/shortcuts_tab.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/input_event_configuration_dialog.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>

void OrchestratorEditorEventSearchBar::_event_changed(const Ref<InputEvent>& p_event) {
    if (p_event.is_valid() && (!p_event->is_pressed() || p_event->is_echo())) {
        return;
    }
    _value_changed();
}

void OrchestratorEditorEventSearchBar::_clear() {
    _search_by_name->set_block_signals(true);
    _search_by_name->clear();
    _search_by_name->set_block_signals(false);

    _search_by_event->set_block_signals(true);
    _search_by_event->clear_event();
    _search_by_event->set_block_signals(false);

    _value_changed();
}

void OrchestratorEditorEventSearchBar::_value_changed() {
    _clear_all->set_disabled(!is_searching());
    emit_signal(SceneStringName(value_changed));
}

bool OrchestratorEditorEventSearchBar::is_searching() const {
    return !get_name().is_empty() || get_event().is_valid();
}

String OrchestratorEditorEventSearchBar::get_name() const {
    return _search_by_name->get_text().strip_edges();
}

Ref<InputEvent> OrchestratorEditorEventSearchBar::get_event() const {
    return _search_by_event->get_event();
}

void OrchestratorEditorEventSearchBar::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _search_by_name->set_right_icon(SceneUtils::get_editor_icon("Search"));
            break;
        }
    }
}

void OrchestratorEditorEventSearchBar::_bind_methods() {
    ADD_SIGNAL(MethodInfo("value_changed"));
}

OrchestratorEditorEventSearchBar::OrchestratorEditorEventSearchBar() {
    set_h_size_flags(SIZE_EXPAND_FILL);

    _search_by_name = memnew(LineEdit);
    _search_by_name->set_h_size_flags(SIZE_EXPAND_FILL);
    _search_by_name->set_placeholder("Filter by Name");
    #if GODOT_VERSION >= 0x040500
    _search_by_name->set_accessibility_name("Filter by Name");
    #endif
    _search_by_name->set_clear_button_enabled(true);
    _search_by_name->connect(SceneStringName(text_changed), callable_mp_this(_value_changed).unbind(1));
    add_child(_search_by_name);

    _search_by_event = memnew(OrchestratorEditorInputEventListenerLineEdit);
    _search_by_event->set_h_size_flags(SIZE_EXPAND_FILL);
    _search_by_event->set_stretch_ratio(0.75);
    #if GODOT_VERSION >= 0x040500
    _search_by_event->set_accessibility_name("Action Event");
    #endif
    _search_by_event->connect("event_changed", callable_mp_this(_event_changed));
    add_child(_search_by_event);

    _clear_all = memnew(Button);
    _clear_all->set_text("Clear All");
    _clear_all->set_tooltip_text("Clear all search filters.");
    _clear_all->connect(SceneStringName(pressed), callable_mp_this(_clear));
    _clear_all->set_disabled(true);
    add_child(_clear_all);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OrchestratorEditorSettingsShortcutsTab
///

void OrchestratorEditorSettingsShortcutsTab::_settings_changed() {
    _timer->start();
}

void OrchestratorEditorSettingsShortcutsTab::_settings_save() {
    if (!_timer->is_stopped()) {
        _timer->stop();
    }

    OrchestratorEditorSettings::get_singleton()->notify_changes();
    OrchestratorEditorSettings::get_singleton()->save();
}

void OrchestratorEditorSettingsShortcutsTab::_shortcut_button_pressed(TreeItem* p_item, int p_column, int p_idx, int p_button) {
    if (p_button != MOUSE_BUTTON_LEFT) {
        return;
    }

    ERR_FAIL_NULL_MSG(p_item, "Object passed is not a TreeItem");

    _is_editing_action = p_item->get_meta("is_action");

    String type = p_item->get_meta("type");
    if (type == "event") {
        _current_edited_identifier = p_item->get_parent()->get_meta("shortcut_identifier");
        _current_events = p_item->get_parent()->get_meta("events");
        _current_event_index = p_item->get_meta("event_index");
    } else {
        _current_edited_identifier = p_item->get_meta("shortcut_identifier");
        _current_events = p_item->get_meta("events");
        _current_event_index = -1;
    }

    switch (p_idx) {
        case SHORTCUT_ADD: {
            // Only for "shortcut" types
            _shortcut_editor->popup_and_configure();
            break;
        }
        case SHORTCUT_EDIT: {
            // Only for "event" types
            _shortcut_editor->popup_and_configure(_current_events[_current_event_index]);
            break;
        }
        case SHORTCUT_ERASE: {
            if (type == "shortcut") {
                if (_is_editing_action) {
                    _update_builtin_action(_current_edited_identifier, Array());
                } else {
                    _update_shortcut_events(_current_edited_identifier, Array());
                }
            } else if (type == "event") {
                _current_events.remove_at(_current_event_index);
                if (_is_editing_action) {
                    _update_builtin_action(_current_edited_identifier, _current_events);
                } else {
                    _update_shortcut_events(_current_edited_identifier, _current_events);
                }
            }
            break;
        }
        case SHORTCUT_REVERT: {
            // Only for "shortcut" types
            if (_is_editing_action) {
                // todo: currently no way to get actions?
            } else {
                Ref<Shortcut> sc = OrchestratorEditorSettings::get_singleton()->get_shortcut(_current_edited_identifier);
                Array original = sc->get_meta("original");
                _update_shortcut_events(_current_edited_identifier, original);
            }
            break;
        }
    }
}

void OrchestratorEditorSettingsShortcutsTab::_shortcut_cell_double_clicked() {
    // If the cell has children and is in the bindings column, and if its first child is editable,
    // then uncollapse the cell and if the first child is the only child, edit that child.
    // If the cell is in the bindings column and can be edited, edit it.
    // If the cell is in the name column, toggle collapse.
    TreeItem* item = _shortcuts->get_selected();
    if (!item) {
        return;
    }

    const int EDIT_BUTTON_COLUMN = 1;

    String type = item->get_meta("type");
    int column = _shortcuts->get_selected_column();

    if (type == "shortcut" && column == 0) {
        if (item->get_first_child()) {
            item->set_collapsed(!item->is_collapsed());
        }
    } else if (type == "shortcut" && column == 1) {
        if (item->get_first_child()) {
            TreeItem* child = item->get_first_child();
            if (child->get_button_by_id(EDIT_BUTTON_COLUMN, SHORTCUT_EDIT) != -1) {
                item->set_collapsed(false);
                if (item->get_child_count() == 1) {
                    _shortcut_button_pressed(child, EDIT_BUTTON_COLUMN, SHORTCUT_EDIT);
                }
            }
        }
    } else if (type == "event" && column == 1) {
        if (item->get_button_by_id(EDIT_BUTTON_COLUMN, SHORTCUT_EDIT) != -1) {
            _shortcut_button_pressed(item, EDIT_BUTTON_COLUMN, SHORTCUT_EDIT);
        }
    }
}

void OrchestratorEditorSettingsShortcutsTab::_event_config_confirmed() {
    Ref<InputEvent> event = _shortcut_editor->get_event();
    if (event.is_null()) {
        return;
    }

    // Adds (-1) or events (!= -1)
    if (_current_event_index == -1) {
        _current_events.push_back(event);
    } else {
        _current_events[_current_event_index] = event;
    }

    if (_is_editing_action) {
        _update_builtin_action(_current_edited_identifier, _current_events);
    } else {
        _update_shortcut_events(_current_edited_identifier, _current_events);
    }
}

Variant OrchestratorEditorSettingsShortcutsTab::get_drag_data_fw(const Point2& p_point, Control* p_from) {
    TreeItem* selected = _shortcuts->get_selected();

    // Only allow drag for events
    if (!selected || String(selected->get_meta("type", "")) != "event") {
        return Variant();
    }

    Label* label = memnew(Label);
    label->set_text(vformat("Event %d", selected->get_meta("event_index")));
    label->set_modulate(Color(1, 1, 1, 1.0f));
    _shortcuts->set_drag_preview(label);
    _shortcuts->set_drop_mode_flags(Tree::DROP_MODE_INBETWEEN);

    return Dictionary();
}

bool OrchestratorEditorSettingsShortcutsTab::can_drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) const {
    TreeItem* selected = _shortcuts->get_selected();
    TreeItem* item = (p_point == Vector2(Math_INF, Math_INF)) ? _shortcuts->get_selected() : _shortcuts->get_item_at_position(p_point);
    if (!selected || !item || item == selected || String(item->get_meta("type", "")) != "event") {
        return false;
    }

    // Don't allow moving an event in-between shortcuts
    if (selected->get_parent()->get_meta("shortcut_identifier") != item->get_parent()->get_meta("shortcut_identifier")) {
        return false;
    }

    return true;
}

void OrchestratorEditorSettingsShortcutsTab::drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) {
    if (!can_drop_data_fw(p_point, p_data, p_from)) {
        return;
    }

    TreeItem* selected = _shortcuts->get_selected();
    TreeItem* target = (p_point == Vector2(Math_INF, Math_INF)) ? _shortcuts->get_selected() : _shortcuts->get_item_at_position(p_point);
    if (!target) {
        return;
    }

    int target_event_index = target->get_meta("event_index");
    int index_moving_from = selected->get_meta("event_index");

    Array events = selected->get_parent()->get_meta("events");

    Variant event_moved = events[index_moving_from];
    events.remove_at(index_moving_from);
    events.insert(target_event_index, event_moved);

    String identifier = selected->get_parent()->get_meta("shortcut_identifier");
    if (selected->get_meta("is_action")) {
        _update_builtin_action(identifier, events);
    } else {
        _update_shortcut_events(identifier, events);
    }
}

bool OrchestratorEditorSettingsShortcutsTab::_should_display_shortcut(const String& p_path, const Array& p_events, const String& p_name) const {
    const Ref<InputEvent> event = _shortcuts_search_bar->get_event();
    if (event.is_valid()) {
        bool event_match = false;
        for (int i = 0; i < p_events.size(); ++i) {
            const Ref<InputEvent> ev = p_events[i];
            if (ev.is_valid() && ev->is_match(event, true)) {
                event_match = true;
                break;
            }
        }
        if (!event_match) {
            return false;
        }
    }

    const String& search_text = _shortcuts_search_bar->get_name();
    if (search_text.is_empty()) {
        return true;
    }
    if (search_text.is_subsequence_ofn(p_name)) {
        return true;
    }
    if (search_text.is_subsequence_ofn(p_name)) {
        return true;
    }
    if (search_text.is_subsequence_ofn(p_path)) {
        return true;
    }

    return false;
}

TreeItem* OrchestratorEditorSettingsShortcutsTab::_create_shortcut_item(TreeItem* p_parent, const String& p_identifier, const String& p_display, Array& p_events, bool p_allow_revert, bool p_is_action, bool p_is_collapsed) {
    TreeItem* shortcut_item = _shortcuts->create_item(p_parent);
	shortcut_item->set_collapsed(p_is_collapsed);
	shortcut_item->set_text(0, p_display);
	shortcut_item->set_tooltip_text(0, p_identifier);

	Ref<InputEvent> primary = p_events.size() > 0 ? Ref<InputEvent>(p_events[0]) : Ref<InputEvent>();
	Ref<InputEvent> secondary = p_events.size() > 1 ? Ref<InputEvent>(p_events[1]) : Ref<InputEvent>();

	String sc_text = "None";
	if (primary.is_valid()) {
		sc_text = primary->as_text();

		if (secondary.is_valid()) {
			sc_text += ", " + secondary->as_text();

			if (p_events.size() > 2) {
				sc_text += " (+" + itos(p_events.size() - 2) + ")";
			}
		}
		shortcut_item->set_auto_translate_mode(1, AUTO_TRANSLATE_MODE_DISABLED);
	}

	shortcut_item->set_text(1, sc_text);
	if (sc_text == "None") {
		// Fade out unassigned shortcut labels for easier visual grepping.
		shortcut_item->set_custom_color(1, get_theme_color(SceneStringName(font_color), "Label") * Color(1, 1, 1, 0.5));
	}

	if (p_allow_revert) {
		shortcut_item->add_button(1, SceneUtils::get_editor_icon("Reload"), SHORTCUT_REVERT);
	}

	shortcut_item->add_button(1, SceneUtils::get_editor_icon("Add"), SHORTCUT_ADD);
	shortcut_item->add_button(1, SceneUtils::get_editor_icon("Close"), SHORTCUT_ERASE, p_events.is_empty());

	shortcut_item->set_meta("is_action", p_is_action);
	shortcut_item->set_meta("type", "shortcut");
	shortcut_item->set_meta("shortcut_identifier", p_identifier);
	shortcut_item->set_meta("events", p_events);

	// Shortcut Input Events
	for (int i = 0; i < p_events.size(); i++) {
		Ref<InputEvent> ie = p_events[i];
		if (ie.is_null()) {
			continue;
		}

		TreeItem* event_item = _shortcuts->create_item(shortcut_item);

		// TRANSLATORS: This is the label for the main input event of a shortcut.
		event_item->set_text(0, shortcut_item->get_child_count() == 1 ? "Primary" : "");
		event_item->set_text(1, ie->as_text());
		event_item->set_auto_translate_mode(1, AUTO_TRANSLATE_MODE_DISABLED);

		event_item->add_button(1, SceneUtils::get_editor_icon("Edit"), SHORTCUT_EDIT);
		event_item->add_button(1, SceneUtils::get_editor_icon("Close"), SHORTCUT_ERASE);

		event_item->set_custom_bg_color(0, SceneUtils::get_editor_color("dark_color_3"));
		event_item->set_custom_bg_color(1, SceneUtils::get_editor_color("dark_color_3"));

		event_item->set_meta("is_action", p_is_action);
		event_item->set_meta("type", "event");
		event_item->set_meta("event_index", i);
	}

	return shortcut_item;
}

TreeItem* OrchestratorEditorSettingsShortcutsTab::_ensure_section(const String& p_section_path, HashMap<String, TreeItem*>& r_sections, TreeItem* p_root, const HashMap<String, bool>& p_collapsed) {
    if (HashMap<String, TreeItem*>::Iterator it = r_sections.find(p_section_path); it != r_sections.end()) {
        return it->value;
    }

    // Parent = section path minus its last segment; the leaf segment is this section's label.
    TreeItem* parent = p_root;
    String label = p_section_path;
    if (const int slash = p_section_path.rfind("/"); slash != -1) {
        parent = _ensure_section(p_section_path.substr(0, slash), r_sections, p_root, p_collapsed);
        label = p_section_path.substr(slash + 1);
    }

    TreeItem* section = _shortcuts->create_item(parent);
    section->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
    section->set_text(0, label.capitalize());
    section->set_tooltip_text(0, p_section_path);
    section->set_selectable(0, false);
    section->set_selectable(1, false);
    section->set_custom_bg_color(0, get_theme_color("prop_subsection", "Editor"));
    section->set_custom_bg_color(1, get_theme_color("prop_subsection", "Editor"));

    // Stash the full path so the collapse save/restore can key by it (see _update_shortcuts).
    section->set_meta("section_path", p_section_path);
    if (p_collapsed.has(p_section_path)) {
        section->set_collapsed(p_collapsed[p_section_path]);
    }

    r_sections[p_section_path] = section;
    return section;
}

void OrchestratorEditorSettingsShortcutsTab::_update_shortcuts() {
    // Before the tree is cleared, take note of which categories are collapsed
    // This state will be retained when the tree is repopulated
    HashMap<String, bool> collapsed;
    if (_shortcuts->get_root() && _shortcuts->get_root()->get_first_child()) {
        TreeItem* item = _shortcuts->get_root()->get_first_child();
        while (item) {
            if (item->get_first_child() && item->has_meta("shortcut_identifier")) {
                collapsed[item->get_meta("shortcut_identifier")] = item->is_collapsed();
            } else if (item->has_meta("section_path")) {
                // Sections are keyed by their full path so identically-named subsections don't collide.
                collapsed[item->get_meta("section_path")] = item->is_collapsed();
            } else {
                collapsed[item->get_text(0)] = item->is_collapsed();
            }

            TreeItem* next = item->get_first_child();
            if (!next) {
                next = item;
                while (next && !next->get_next()) {
                    next = next->get_parent();
                }
                if (next) {
                    next = next->get_next();
                }
            }

            item = next;
        }
    }

    String prev_selected_shortcut;
    if (_shortcuts->get_selected()) {
        prev_selected_shortcut = _shortcuts->get_selected()->get_text(0);
    }

    _shortcuts->clear();

    TreeItem* root = _shortcuts->create_item();

    HashMap<String, TreeItem*> sections;

    //~ Begin Actions
    // todo:
    // EditorSettings uses common to add InputMap actions
    // This might be useful for modifying custom orchestrator actions?

    //~ Begin Shortcuts

    PackedStringArray slist = OrchestratorEditorSettings::get_singleton()->get_shortcut_list();
    slist.sort();

    for (const String& E : slist) {
        const Ref<Shortcut> sc = OrchestratorEditorSettings::get_singleton()->get_shortcut(E);
        if (sc.is_null() || !sc->has_meta("original")) {
            continue;
        }
        if (!_should_display_shortcut(E, sc->get_events(), sc->get_name())) {
            continue;
        }

        // Section path = everything before the last segment; a slash-less path lands under "Common".
        // Sections nest to any depth (e.g. "graph_editor/alignment" -> Graph editor > Alignment).
        const int slash = E.rfind("/");
        const String section_path = (slash == -1) ? String("Common") : E.substr(0, slash);
        TreeItem* section = _ensure_section(section_path, sections, root, collapsed);

        const Array original = sc->get_meta("original");
        Array events = sc->get_events().duplicate(true);
        const bool same_as_defaults = OrchestratorEditorSettings::is_event_array_equal(original, events);
        const bool collapse = !collapsed.has(E) || collapsed[E];

        TreeItem* shortcut_item = _create_shortcut_item(section, E, sc->get_name(), events, !same_as_defaults, false, collapse);
        if (!prev_selected_shortcut.is_empty() && sc->get_name() == prev_selected_shortcut) {
            shortcut_item->select(0);
        }
    }

    if (!prev_selected_shortcut.is_empty()) {
        _shortcuts->ensure_cursor_is_visible();
    }
}

void OrchestratorEditorSettingsShortcutsTab::_update_builtin_action(const String& p_path, const Array& p_events) {
    // todo: cannot access builtin action overrides
}

void OrchestratorEditorSettingsShortcutsTab::_update_shortcut_events(const String& p_path, const Array& p_events) {
    Ref<Shortcut> sc = OrchestratorEditorSettings::get_singleton()->get_shortcut(p_path);

    EditorSettings* es = EI->get_editor_settings().ptr();
    EditorUndoRedoManager* undo_redo = EI->get_editor_undo_redo();
    undo_redo->create_action(vformat("Edit Shortcut: %s", p_path), UndoRedo::MERGE_DISABLE, es);
    undo_redo->force_fixed_history();
    undo_redo->add_do_method(sc.ptr(), "set_events", p_events);
    undo_redo->add_undo_method(sc.ptr(), "set_events", sc->get_events());
    undo_redo->add_do_method(es, "mark_setting_changed", "shortcuts");
    undo_redo->add_undo_method(es, "mark_setting_changed", "shortcuts");
    undo_redo->add_do_method(this, "_update_shortcuts");
    undo_redo->add_undo_method(this, "_update_shortcuts");
    undo_redo->add_do_method(this, "_settings_changed");
    undo_redo->add_undo_method(this, "_settings_changed");
    undo_redo->commit_action();
}

void OrchestratorEditorSettingsShortcutsTab::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (is_visible()) {
                _shortcuts_search_bar->get_name_search_box()->grab_focus();
                _shortcuts_search_bar->get_name_search_box()->select_all();
                break;
            }
            _update_shortcuts();
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _update_shortcuts();
            break;
        }
    }
}

void OrchestratorEditorSettingsShortcutsTab::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_update_shortcuts"), &OrchestratorEditorSettingsShortcutsTab::_update_shortcuts);
    ClassDB::bind_method(D_METHOD("_settings_changed"), &OrchestratorEditorSettingsShortcutsTab::_settings_changed);
}

OrchestratorEditorSettingsShortcutsTab::OrchestratorEditorSettingsShortcutsTab() {
    set_name("Shortcuts");

    _shortcuts_search_bar = memnew(OrchestratorEditorEventSearchBar);
    _shortcuts_search_bar->connect(SceneStringName(value_changed), callable_mp_this(_update_shortcuts));
    add_child(_shortcuts_search_bar);

    MarginContainer* mc = memnew(MarginContainer);
    mc->set_v_size_flags(SIZE_EXPAND_FILL);
    mc->set_theme_type_variation("NoBorderHorizontalBottom");
    add_child(mc);

    _shortcuts = memnew(Tree);
    #if GODOT_VERSION >= 0x040500
    _shortcuts->set_accessibility_name("Shortcuts");
    #endif
    _shortcuts->set_theme_type_variation("TreeTable");
    _shortcuts->set_columns(2);
    _shortcuts->set_hide_root(true);
    _shortcuts->set_column_titles_visible(true);
    _shortcuts->set_column_title(0, "Name");
    _shortcuts->set_column_title(1, "Binding");
    _shortcuts->connect("button_clicked", callable_mp_this(_shortcut_button_pressed));
    _shortcuts->connect("item_activated", callable_mp_this(_shortcut_cell_double_clicked));
    mc->add_child(_shortcuts);

    _shortcut_editor = memnew(OrchestratorEditorInputEventConfigurationDialog);
    _shortcut_editor->set_allowed_input_types(INPUT_ALL);
    _shortcut_editor->connect(SceneStringName(confirmed), callable_mp_this(_event_config_confirmed));
    add_child(_shortcut_editor);

    SET_DRAG_FORWARDING_GCD(_shortcuts, OrchestratorEditorSettingsShortcutsTab);

    _timer = memnew(Timer);
    _timer->set_wait_time(1.5);
    _timer->connect("timeout", callable_mp_this(_settings_save));
    _timer->set_one_shot(true);
    add_child(_timer);

    EI->get_editor_settings()->connect("settings_changed", callable_mp_this(_settings_changed));
}