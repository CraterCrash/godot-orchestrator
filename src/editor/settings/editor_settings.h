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

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Manages the editor-only settings for the plugin.
///
/// This class deliberately stores its values in Godot's <code>EditorSettings</code> (which is per-user,
/// machine-local, and excluded from project exports) rather than in <code>ProjectSettings</code>.
/// Shortcuts in particular are editor-global, and have no business in shipped games.
///
/// Godot's shortcut registry was only exposed through <code>EditorSettings</code> in 4.6+. To remain
/// compatible back to older versions, this class reimplements that small registry on top of the
/// generic <code>set_setting</code> / <code>get_setting</code> accessors, while still providing real
/// <code>Ref<Shortcut></code> objects to the rest of the editor UI is version-agnostic.
///
class OrchestratorEditorSettings : public Object {
    GDCLASS(OrchestratorEditorSettings, Object);

    static OrchestratorEditorSettings* _singleton;

    /// In-memory registry of shortcuts, keyed by their logical path (e.g. "graph_editor/toggle_bookmark").
    HashMap<String, Ref<Shortcut>> _shortcuts;

    /// Decoded persisted overrides (logical path -> event array), loaded lazily from the single
    /// settings entry and consulted while registering shortcuts.
    HashMap<String, Array> _overrides;
    bool _overrides_loaded = false;

    /// The single EditorSettings key under which all Orchestrator shortcut overrides are persisted,
    /// as a TypedArray of <code>{ "name": <path>, "shortcuts": [InputEvent...] }</code> (engine-shaped).
    static String _settings_key();

    /// Encodes/decodes one shortcut's event list.
    /// A single seam for the on-disk representation.
    static Variant _encode_events(const Array& p_events);
    static Array _decode_events(const Variant& p_value);

    /// Lazily loads persisted overrides from <code>_settings_key()</code> into <code>_overrides</code>.
    void _load_overrides();

    /// Rebuilds the single settings array from the registry (override-only) and writes or erases it.
    void _update_editor_settings_shortcuts();

    //~ Begin Signal Handlers
    void _shortcut_changed();
    //~ End Signal Handlers

protected:
    static void _bind_methods();

public:
    static OrchestratorEditorSettings* get_singleton() { return _singleton; }

    static void create();
    static void destroy();

    /// Registers a shortcut with its default event(s).
    ///
    /// Returns a shared Shortcut instance; if the path was already registered the existing instance is
    /// returned. The current events are loaded from <code>EditorSettings</code> when present, otherwise
    /// the supplied defaults are used. The defaults are stamped on the <code>Shortcut</code> as the
    /// "original" meta, so shortcuts can easily be reverted to their default.
    ///
    /// A <code>transient</code> shortcut is a binding that this plugin does not own, e.g. a native Godot
    /// editor shortcut). It is never persisted and is excluded from the <code>get_shortcut_list()</code>;
    /// and only holds the hardcoded default used as a fallback when checking native shortcuts.
    Ref<Shortcut> register_shortcut(const String& p_path, const String& p_name, const Array& p_default_events, bool p_transient = false);
    Ref<Shortcut> register_shortcut(const String& p_path, const String& p_name, const Ref<InputEvent>& p_default_event, bool p_transient = false);
    Ref<Shortcut> register_shortcut(const String& p_path, const String& p_name, int p_keycode, bool p_transient = false);

    bool has_shortcut(const String& p_path) const;
    Ref<Shortcut> get_shortcut(const String& p_path) const;
    PackedStringArray get_shortcut_list() const;

    /// True if the shortcut at <code>p_path</code> is transient (a shadow of a non-Orchestrator binding).
    bool is_transient(const String& p_path) const;

    /// Returns true if <code>p_event</code> matches any of the events bound to the shortcut at <code>>p_path</code>.
    bool is_shortcut(const String& p_path, const Ref<InputEvent>& p_event) const;

    /// Resolves the live events for a native (transient) shortcut: the Godot <code>EditorSettings</code>
    /// "shortcuts" override if the user has customized it, otherwise the registered transient default.
    Array get_native_shortcut_events(const String& p_path) const;

    /// This works just like <code>is_shortcut</code>, except its for native (transient) shortcuts where
    /// the source of truth is Godot's own <code>EditorSettings</code>.
    ///
    /// This resolves from the live <code>EditorSettings</code> shortcuts array if present; otherwise, it
    /// falls back to the registered transient default, which should be the editor's default. This is the
    /// only way to honor native editor shortcuts before Godot 4.6.
    bool is_native_shortcut(const String& p_path, const Ref<InputEvent>& p_event) const;

    /// Re-syncs every transient shortcut's events to its live native value (override -> default). Call
    /// when editor settings change so menus bound to the shadow Shortcuts reflect native rebinds.
    void refresh_transient_shortcuts();

    /// Checks element-wise equality between two arrays.
    /// Exposed because <code>Shortcut::is_event_array_equal</code> is not available to GDExtensions.
    static bool is_event_array_equal(const Array& p_a, const Array& p_b);

    /// Propagates NOTIFICATION_EDITOR_SETTINGS_CHANGED through the scene tree so editor UI that
    /// depends on these settings (e.g. shortcut displays) refreshes. Mirrors EditorSettings.
    void notify_changes();

    /// Writes the current registry state and best-effort flushes the backing editor settings
    /// resource to disk. Mirrors EditorSettings::save() (which is not exposed to GDExtension).
    void save();
};

// Engine-parity shortcut helpers.
//
// These mirror the editor's ED_* free functions/macros so call sites read identically to upstream Godot,
// but route through OrchestratorEditorSettings instead of EditorSettings (whose typed shortcut API is
// 4.6+ only).
//
// p_keycode accepts a Key value with KeyModifierMask bits OR'd in, e.g. KEY_MASK_CMD_OR_CTRL | KEY_S

/// Fetches a previously registered shortcut; errors if the path is unknown.
Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path);

/// Builds an ad-hoc Shortcut from a Godot InputMap action's current events, for menu items that
/// should mirror an action's binding (e.g. "ui_graph_delete"). It is not registered or persisted;
/// because it reads the live action events, call it when the (typically transient) menu is built so
/// it always reflects the current binding.
Ref<Shortcut> ED_ACTION_SHORTCUT(const StringName& p_action, const String& p_name);

/// Reader form of ED_ACTION_SHORTCUT for when only the binding matters (e.g. get_as_text() or
/// matching), not a menu label. Returns the action's current binding as an unnamed Shortcut.
Ref<Shortcut> ED_GET_ACTION_SHORTCUT(const StringName& p_action);

/// True if p_event matches the Godot InputMap action p_action (exact match). ED_IS_SHORTCUT already
/// falls back to this for unregistered paths; prefer this when the name is explicitly an action, to
/// state intent and skip the shortcut-registry lookup.
bool ED_IS_ACTION_SHORTCUT(const StringName& p_action, const Ref<InputEvent>& p_event);

/// Defines (or refreshes the defaults of) a shortcut with a single default chord.
Ref<Shortcut> ED_SHORTCUT(const String& p_path, const String& p_name, Key p_keycode = KEY_NONE, bool p_physical = false);

/// Defines (or refreshes the defaults of) a shortcut with multiple default chords.
Ref<Shortcut> ED_SHORTCUT_ARRAY(const String& p_path, const String& p_name, const PackedInt32Array& p_keycodes, bool p_physical = false);

/// Replaces a shortcut's default with a platform-specific chord, but only on <code>p_feature</code> platforms
/// and only when the user hasn't customized it.
void ED_SHORTCUT_OVERRIDE(const String& p_path, const String& p_feature, Key p_keycode = KEY_NONE, bool p_physical = false);
void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature, const PackedInt32Array& p_keycodes, bool p_physical = false);

#define ED_IS_SHORTCUT(p_path, p_ev) (OrchestratorEditorSettings::get_singleton()->is_shortcut((p_path), (p_ev)))
#define ED_IS_NATIVE_SHORTCUT(p_path, p_ev) (OrchestratorEditorSettings::get_singleton()->is_native_shortcut((p_path), (p_ev)))
#define ED_HAS_SHORTCUT(p_path) (OrchestratorEditorSettings::get_singleton()->has_shortcut((p_path)))