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
#include "editor/settings/editor_settings.h"

#include "common/macros.h"
#include "common/os_utils.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cctype>

OrchestratorEditorSettings* OrchestratorEditorSettings::_singleton = nullptr;

namespace {
    // Defined further below (with the engine-parity helpers). Decodes a Key value that has
    // KeyModifierMask bits OR'd in into a fully-populated InputEventKey.
    Ref<InputEventKey> _create_event_reference(Key p_keycode, bool p_physical);
}

String OrchestratorEditorSettings::_settings_key() {
    // Godot treats leading underscore properties, e.g. <code>_property</code> as storage-only.
    // We use this here to avoid Orchestrator's shortcuts from being visible in EditorSettings.
    return "_orchestrator/shortcuts";
}

Variant OrchestratorEditorSettings::_encode_events(const Array& p_events) {
    // Encodes a shortcut's events (a plain Array of InputEvent resources).
    // EditorSettings persists these as inline sub resources.
    return p_events.duplicate(true);
}

Array OrchestratorEditorSettings::_decode_events(const Variant& p_value) {
    if (p_value.get_type() == Variant::ARRAY) {
        return p_value;
    }
    return {};
}

void OrchestratorEditorSettings::_load_overrides() {
    if (_overrides_loaded) {
        return;
    }
    _overrides_loaded = true;

    const Ref<EditorSettings> es = EI->get_editor_settings();
    if (!es->has_setting(_settings_key())) {
        return;
    }

    const Variant value = es->get_setting(_settings_key());
    if (value.get_type() != Variant::ARRAY) {
        return;
    }

    // The engine uses a TypedArray<Dictionary> where the dictionary has the format:
    // { "name": <path>, "shortcuts": [InputEvent...] }
    const Array entries = value;
    for (int i = 0; i < entries.size(); i++) {
        const Dictionary entry = entries[i];
        if (!entry.has("name")) {
            continue;
        }
        _overrides[entry["name"]] = _decode_events(entry.get("shortcuts", Array()));
    }
}

void OrchestratorEditorSettings::_update_editor_settings_shortcuts() {
    // Deterministic order keeps the EditorSettings .tres stable across writes.
    PackedStringArray names;
    for (const KeyValue<String, Ref<Shortcut>>& E : _shortcuts) {
        names.push_back(E.key);
    }
    names.sort();

    Array entries;
    for (const String& name : names) {
        const Ref<Shortcut> sc = _shortcuts[name];
        if (sc.is_null()) {
            continue;
        }

        // Transient shortcuts are shadow bindings, and we do not own them.
        // Their source is truth is always Godot's EditorSettings, so they are not persisted.
        if (sc->has_meta("transient")) {
            continue;
        }

        // If the default is not customized, it is not serialized.
        if (is_event_array_equal(sc->get_events(), sc->get_meta("original"))) {
            continue;
        }

        Dictionary entry;
        entry["name"] = name;
        entry["shortcuts"] = _encode_events(sc->get_events());
        entries.push_back(entry);
    }

    Ref<EditorSettings> es = EI->get_editor_settings();
    if (entries.is_empty()) {
        if (es->has_setting(_settings_key())) {
            es->erase(_settings_key());
        }
    } else {
        es->set_setting(_settings_key(), entries);
    }
}

void OrchestratorEditorSettings::_shortcut_changed() {
    _update_editor_settings_shortcuts();
}

Ref<Shortcut> OrchestratorEditorSettings::register_shortcut(const String& p_path, const String& p_name, const Array& p_default_events, bool p_transient) {
    ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<Shortcut>(), "Cannot register a shortcut with an empty path.");

    const Array defaults = p_default_events.duplicate(true);

    // Already registered (e.g. re-declared via ED_SHORTCUT)
    // Keep the current events but refresh the display name and "original" defaults to compare against.
    if (HashMap<String, Ref<Shortcut>>::Iterator it = _shortcuts.find(p_path); it != _shortcuts.end()) {
        if (it->value.is_valid()) {
            it->value->set_name(p_name);
            it->value->set_meta("original", defaults);
        }
        return it->value;
    }

    Ref<Shortcut> sc;
    sc.instantiate();
    sc->set_name(p_name);
    sc->set_meta("original", defaults);
    if (p_transient) {
        sc->set_meta("transient", true);
    }

    // Transient shortcuts hold hardcoded defaults, live values come from EditorSettings.
    // Non-transient ones prefer a persisted override over the code default.
    _load_overrides();
    const Array events = !p_transient && _overrides.has(p_path)
        ? _overrides[p_path].duplicate(true)
        : defaults.duplicate(true);

    sc->connect("changed", callable_mp_this(_shortcut_changed));

    // Populate the initial events without triggering the write-through. Blocking signals on the
    // shortcut keeps the connect-vs-populate order from mattering.
    sc->set_block_signals(true);
    sc->set_events(events);
    sc->set_block_signals(false);

    _shortcuts[p_path] = sc;
    return sc;
}

Ref<Shortcut> OrchestratorEditorSettings::register_shortcut(const String& p_path, const String& p_name, const Ref<InputEvent>& p_default_event, bool p_transient) {
    Array events;
    if (p_default_event.is_valid()) {
        events.push_back(p_default_event);
    }
    return register_shortcut(p_path, p_name, events, p_transient);
}

Ref<Shortcut> OrchestratorEditorSettings::register_shortcut(const String& p_path, const String& p_name, int p_keycode, bool p_transient) {
    if (p_keycode == KEY_NONE) {
        return register_shortcut(p_path, p_name, Array(), p_transient);
    }

    // p_keycode is a Key value with KeyModifierMask bits OR'd in (e.g. via OACCEL_KEY /`KEY_MASK_* | KEY_*`).
    // Decode it the same way the ED_SHORTCUT path does, so the modifier bits and the keycode map to the
    // right InputEventKey fields.
    const Ref<InputEventKey> key = _create_event_reference(static_cast<Key>(p_keycode), false);
    return register_shortcut(p_path, p_name, key, p_transient);
}

bool OrchestratorEditorSettings::has_shortcut(const String& p_path) const {
    return _shortcuts.has(p_path);
}

Ref<Shortcut> OrchestratorEditorSettings::get_shortcut(const String& p_path) const {
    HashMap<String, Ref<Shortcut>>::ConstIterator it = _shortcuts.find(p_path);
    return it != _shortcuts.end() ? it->value : Ref<Shortcut>();
}

PackedStringArray OrchestratorEditorSettings::get_shortcut_list() const {
    PackedStringArray list;
    for (const KeyValue<String, Ref<Shortcut>>& E : _shortcuts) {
        // Transient shortcuts shadow native bindings and are not editable here, so skip them
        if (E.value.is_valid() && E.value->has_meta("transient")) {
            continue;
        }
        list.push_back(E.key);
    }
    return list;
}

bool OrchestratorEditorSettings::is_transient(const String& p_path) const {
    const Ref<Shortcut> sc = get_shortcut(p_path);
    return sc.is_valid() && sc->has_meta("transient");
}

bool OrchestratorEditorSettings::is_shortcut(const String& p_path, const Ref<InputEvent>& p_event) const {
    // An Orchestrator-registered shortcut takes precedence.
    if (const Ref<Shortcut> sc = get_shortcut(p_path); sc.is_valid()) {
        return sc->matches_event(p_event);
    }

    // Fallback to an InputMap action if the same name (e.g. the built-in ui_undo action).
    // Exact match so modifier variants don't bleed across actions (Ctrl+Z vs Ctrl+Shift+Z).
    InputMap* input_map = InputMap::get_singleton();
    return input_map->has_action(p_path) && input_map->event_is_action(p_event, p_path, true);
}

Array OrchestratorEditorSettings::get_native_shortcut_events(const String& p_path) const {
    // Live override from Godot's EditorSettings "shortcuts" array. That property is override-only,
    // so an entry is present here only if the user has rebound this native shortcut from its default.
    const Variant value = EI->get_editor_settings()->get_setting("shortcuts");
    if (value.get_type() == Variant::ARRAY) {
        const Array entries = value;
        for (int i = 0; i < entries.size(); i++) {
            const Dictionary entry = entries[i];
            if (String(entry.get("name", String())) == p_path) {
                return _decode_events(entry.get("shortcuts", Array()));
            }
        }
    }

    // Not overridden: fall back to the registered transient default (the immutable "original" meta).
    const Ref<Shortcut> def = get_shortcut(p_path);
    if (def.is_valid() && def->has_meta("original")) {
        return _decode_events(def->get_meta("original"));
    }
    return {};
}

bool OrchestratorEditorSettings::is_native_shortcut(const String& p_path, const Ref<InputEvent>& p_event) const {
    const Array events = get_native_shortcut_events(p_path);
    for (int i = 0; i < events.size(); i++) {
        const Ref<InputEvent> ie = events[i];
        if (ie.is_valid() && ie->is_match(p_event)) {
            return true;
        }
    }
    return false;
}

void OrchestratorEditorSettings::refresh_transient_shortcuts() {
    // Re-sync each transient shadow Shortcut's events to its live native value. The menus that bound
    // these Refs (via ED_GET_SHORTCUT) reflect the change automatically for both display and trigger.
    // Transient shortcuts are skipped by persistence, so the resulting `changed` signal is a no-op.
    for (const KeyValue<String, Ref<Shortcut>>& E : _shortcuts) {
        const Ref<Shortcut>& sc = E.value;
        if (sc.is_null() || !sc->has_meta("transient")) {
            continue;
        }
        const Array live = get_native_shortcut_events(E.key);
        if (!is_event_array_equal(sc->get_events(), live)) {
            sc->set_events(live);
        }
    }
}

bool OrchestratorEditorSettings::is_event_array_equal(const Array& p_a, const Array& p_b) {
    if (p_a.size() != p_b.size()) {
        return false;
    }

    for (int i = 0; i < p_a.size(); i++) {
        const Ref<InputEvent> ie_a = p_a[i];
        const Ref<InputEvent> ie_b = p_b[i];
        if (ie_a.is_null() || ie_b.is_null() || !ie_a->is_match(ie_b)) {
            return false;
        }
    }

    return true;
}

void OrchestratorEditorSettings::notify_changes() { // NOLINT
    SceneTree* tree = cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (!tree) {
        return;
    }

    Node* root = tree->get_root();
    if (!root) {
        return;
    }

    root->propagate_notification(EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED);
}

void OrchestratorEditorSettings::save() {
    // Ensure the latest registry state is written into EditorSettings
    _update_editor_settings_shortcuts();

    // Given that EditorSettings::save() is not directly exposed to GDExtension, we do this to flush
    // the resource directly, only if the resource path is known.
    Ref<EditorSettings> es = EI->get_editor_settings();
    if (es.is_valid() && !es->get_path().is_empty()) {
        ResourceSaver::get_singleton()->save(es, es->get_path());
    }
}

void OrchestratorEditorSettings::create() {
    ERR_FAIL_COND_MSG(_singleton != nullptr, "OrchestratorEditorSettings already created.");
    _singleton = memnew(OrchestratorEditorSettings);
}

void OrchestratorEditorSettings::destroy() {
    if (_singleton) {
        memdelete(_singleton);
        _singleton = nullptr;
    }
}

void OrchestratorEditorSettings::_bind_methods() {
}

//~ Begin engine-parity shortcut helpers

namespace {
    /// Replicates the (unexposed) InputEventKey::create_reference: decodes a Key value whose upper
    /// bits may carry KeyModifierMask flags into a fully-populated InputEventKey.
    Ref<InputEventKey> _create_event_reference(Key p_keycode, bool p_physical) {
        Ref<InputEventKey> ie;
        ie.instantiate();

        const int packed = p_keycode;
        const Key keycode = static_cast<Key>(static_cast<int>(packed & KEY_CODE_MASK));

        if (p_physical) {
            ie->set_physical_keycode(keycode);
        } else {
            ie->set_keycode(keycode);
        }

        const char32_t ch = static_cast<char32_t>(keycode);
        if (ch < 0xd800 || (ch > 0xdfff && ch <= 0x10ffff)) {
            ie->set_unicode(ch < 0x80 ? static_cast<char32_t>(std::tolower(static_cast<int>(ch))) : ch);
        }

        if (packed & KEY_MASK_SHIFT) {
            ie->set_shift_pressed(true);
        }
        if (packed & KEY_MASK_ALT) {
            ie->set_alt_pressed(true);
        }
        if (packed & KEY_MASK_CTRL) {
            ie->set_ctrl_pressed(true);
        }
        if (packed & KEY_MASK_CMD_OR_CTRL) {
            ie->set_command_or_control_autoremap(true);
        }
        if (packed & KEY_MASK_META) {
            ie->set_meta_pressed(true);
        }

        return ie;
    }

    Array _build_events(const PackedInt32Array& p_keycodes, bool p_physical) {
        const bool prefer_meta = OSUtils::prefer_meta_over_ctrl();

        Array events;
        for (int i = 0; i < p_keycodes.size(); i++) {
            Key keycode = static_cast<Key>(static_cast<int>(p_keycodes[i]));

            // Use Cmd+Backspace as a general replacement for Delete shortcuts on macOS-like platforms.
            if (prefer_meta && keycode == KEY_DELETE) {
                keycode = static_cast<Key>(static_cast<int>(KEY_MASK_META) | static_cast<int>(KEY_BACKSPACE));
            }

            if (keycode != KEY_NONE) {
                events.push_back(_create_event_reference(keycode, p_physical));
            }
        }
        return events;
    }
}

Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path) {
    OrchestratorEditorSettings* es = OrchestratorEditorSettings::get_singleton();
    ERR_FAIL_NULL_V_MSG(es, Ref<Shortcut>(), "OrchestratorEditorSettings not instantiated yet.");

    Ref<Shortcut> sc = es->get_shortcut(p_path);
    ERR_FAIL_COND_V_MSG(sc.is_null(), sc, "Used ED_GET_SHORTCUT with invalid shortcut: " + p_path);
    return sc;
}

Ref<Shortcut> ED_ACTION_SHORTCUT(const StringName& p_action, const String& p_name) {
    Ref<Shortcut> sc;
    sc.instantiate();
    sc->set_name(p_name);
    sc->set_events(InputMap::get_singleton()->action_get_events(p_action));
    return sc;
}

Ref<Shortcut> ED_GET_ACTION_SHORTCUT(const StringName& p_action) {
    return ED_ACTION_SHORTCUT(p_action, String());
}

bool ED_IS_ACTION_SHORTCUT(const StringName& p_action, const Ref<InputEvent>& p_event) {
    // Exact match, consistent with ED_IS_SHORTCUT (so e.g. Shift+Delete doesn't satisfy a plain
    // Delete action). InputEvent::is_action delegates to the InputMap internally.
    return p_event.is_valid() && p_event->is_action(p_action, true);
}

Ref<Shortcut> ED_SHORTCUT(const String& p_path, const String& p_name, Key p_keycode, bool p_physical) {
    PackedInt32Array arr;
    arr.push_back(p_keycode);
    return ED_SHORTCUT_ARRAY(p_path, p_name, arr, p_physical);
}

Ref<Shortcut> ED_SHORTCUT_ARRAY(const String& p_path, const String& p_name, const PackedInt32Array& p_keycodes, bool p_physical) {
    const Array events = _build_events(p_keycodes, p_physical);

    OrchestratorEditorSettings* es = OrchestratorEditorSettings::get_singleton();
    if (!es) {
        // No registry yet: hand back a detached shortcut so callers still get a usable object.
        Ref<Shortcut> sc;
        sc.instantiate();
        sc->set_name(p_name);
        sc->set_events(events);
        sc->set_meta("original", events.duplicate(true));
        return sc;
    }

    return es->register_shortcut(p_path, p_name, events);
}

void ED_SHORTCUT_OVERRIDE(const String& p_path, const String& p_feature, Key p_keycode, bool p_physical) {
    PackedInt32Array arr;
    arr.push_back(p_keycode);
    ED_SHORTCUT_OVERRIDE_ARRAY(p_path, p_feature, arr, p_physical);
}

void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature, const PackedInt32Array& p_keycodes, bool p_physical) {
    OrchestratorEditorSettings* es = OrchestratorEditorSettings::get_singleton();
    if (!es) {
        return;
    }

    Ref<Shortcut> sc = es->get_shortcut(p_path);
    ERR_FAIL_COND_MSG(sc.is_null(), "Used ED_SHORTCUT_OVERRIDE_ARRAY with invalid shortcut: " + p_path);

    // Only apply the override on platforms that support the feature (with the web-macOS/iOS caveat).
    if (!OS::get_singleton()->has_feature(p_feature)) {
        if (!(p_feature == "macos" && (OS::get_singleton()->has_feature("web_macos") || OS::get_singleton()->has_feature("web_ios")))) {
            return;
        }
    }

    const Array events = _build_events(p_keycodes, p_physical);

    // Override the events only if the user hasn't customized them away from the previous default.
    if (OrchestratorEditorSettings::is_event_array_equal(sc->get_events(), sc->get_meta("original"))) {
        sc->set_events(events);
    }
    sc->set_meta("original", events.duplicate(true));
}

//~ End engine-parity shortcut helpers