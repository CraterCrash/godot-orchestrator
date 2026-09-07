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
#include "editor/inspector/annotation_presentation.h"

#include "script/script_warning.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace {
    struct Entry {
        const char* name;
        const char* label;
        const char* description;
    };

    // Descriptions follow the GDScript reference so the tooltips read the same as Godot's own docs.
    const Entry ENTRIES[] = {
        { "@export", "Export", "Expose the variable in the inspector using its declared type." },
        { "@export_enum", "Export Enum", "Expose an int or String as a dropdown of the given names. Use \"Name:value\" to set explicit values." },
        { "@export_file", "Export File", "Expose a String as a file path, optionally limited to the given filters such as \"*.png\"." },
        { "@export_dir", "Export Directory", "Expose a String as a directory path inside the project." },
        { "@export_global_file", "Export Global File", "Expose a String as a file path anywhere on disk, optionally limited to the given filters." },
        { "@export_global_dir", "Export Global Directory", "Expose a String as a directory path anywhere on disk." },
        { "@export_multiline", "Export Multiline", "Expose a String or Dictionary with a multiline text editor." },
        { "@export_placeholder", "Export Placeholder", "Expose a String with placeholder text shown while the field is empty." },
        { "@export_range", "Export Range", "Expose an int or float with a minimum, maximum and optional step. Extra hints: or_greater, or_less, hide_slider, exp, radians_as_degrees, degrees, suffix:unit." },
        { "@export_exp_easing", "Export Exp Easing", "Expose a float as an easing curve. Hints: attenuation, positive_only." },
        { "@export_color_no_alpha", "Export Color No Alpha", "Expose a Color without the alpha channel." },
        { "@export_node_path", "Export Node Path", "Expose a NodePath, optionally limited to the given node types." },
        { "@export_flags", "Export Flags", "Expose an int as a set of named bit flags. Use \"Name:value\" to set explicit values." },
        { "@export_flags_2d_render", "Export 2D Render Layers", "Expose an int as 2D render layers." },
        { "@export_flags_2d_physics", "Export 2D Physics Layers", "Expose an int as 2D physics layers." },
        { "@export_flags_2d_navigation", "Export 2D Navigation Layers", "Expose an int as 2D navigation layers." },
        { "@export_flags_3d_render", "Export 3D Render Layers", "Expose an int as 3D render layers." },
        { "@export_flags_3d_physics", "Export 3D Physics Layers", "Expose an int as 3D physics layers." },
        { "@export_flags_3d_navigation", "Export 3D Navigation Layers", "Expose an int as 3D navigation layers." },
        { "@export_flags_avoidance", "Export Avoidance Layers", "Expose an int as navigation avoidance layers." },
        { "@export_storage", "Export Storage", "Store the variable in the scene without showing it in the inspector." },
        { "@export_custom", "Export Custom", "Expose the variable with an explicit property hint, hint string and usage flags." },
        { "@export_tool_button", "Export Tool Button", "Expose a Callable as a button in the inspector with the given text and optional icon." },
        { "@rpc", "RPC", "Mark the function as a remote procedure call. Choose who may call it, whether it also runs locally, the transfer mode and the channel." },
        { "@onready", "On Ready", "Assign the default value when the owner enters the scene tree, just before _ready. Required when the initializer is a node path." },
        { "@warning_ignore", "Ignore Warnings", "Suppress the named compiler warnings for this member." },
    };

    struct Choice {
        const char* label;
        const char* value;
    };

    const Choice RPC_MODES[] = {
        { "Authority", "authority" },
        { "Any Peer", "any_peer" },
    };

    const Choice RPC_SYNCS[] = {
        { "Call Remote", "call_remote" },
        { "Call Local", "call_local" },
    };

    const Choice RPC_TRANSFER_MODES[] = {
        { "Unreliable", "unreliable" },
        { "Unreliable Ordered", "unreliable_ordered" },
        { "Reliable", "reliable" },
    };

    template <size_t N>
    void add_choices(const Choice (&p_choices)[N], PackedStringArray& r_labels, Array& r_values) {
        for (const Choice& choice : p_choices) {
            r_labels.push_back(choice.label);
            r_values.push_back(choice.value);
        }
    }

    struct HintEntry {
        PropertyHint hint;
        const char* label;
    };

    // PropertyHint values that make sense as an @export_custom hint, in enum order.
    const HintEntry HINTS[] = {
        { PROPERTY_HINT_NONE, "None" },
        { PROPERTY_HINT_RANGE, "Range" },
        { PROPERTY_HINT_ENUM, "Enum" },
        { PROPERTY_HINT_ENUM_SUGGESTION, "Enum Suggestion" },
        { PROPERTY_HINT_EXP_EASING, "Exp Easing" },
        { PROPERTY_HINT_LINK, "Link" },
        { PROPERTY_HINT_FLAGS, "Flags" },
        { PROPERTY_HINT_LAYERS_2D_RENDER, "Layers 2D Render" },
        { PROPERTY_HINT_LAYERS_2D_PHYSICS, "Layers 2D Physics" },
        { PROPERTY_HINT_LAYERS_2D_NAVIGATION, "Layers 2D Navigation" },
        { PROPERTY_HINT_LAYERS_3D_RENDER, "Layers 3D Render" },
        { PROPERTY_HINT_LAYERS_3D_PHYSICS, "Layers 3D Physics" },
        { PROPERTY_HINT_LAYERS_3D_NAVIGATION, "Layers 3D Navigation" },
        { PROPERTY_HINT_FILE, "File" },
        { PROPERTY_HINT_DIR, "Directory" },
        { PROPERTY_HINT_GLOBAL_FILE, "Global File" },
        { PROPERTY_HINT_GLOBAL_DIR, "Global Directory" },
        { PROPERTY_HINT_RESOURCE_TYPE, "Resource Type" },
        { PROPERTY_HINT_MULTILINE_TEXT, "Multiline Text" },
        { PROPERTY_HINT_EXPRESSION, "Expression" },
        { PROPERTY_HINT_PLACEHOLDER_TEXT, "Placeholder Text" },
        { PROPERTY_HINT_COLOR_NO_ALPHA, "Color No Alpha" },
        { PROPERTY_HINT_TYPE_STRING, "Type String" },
        { PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node Path Valid Types" },
        { PROPERTY_HINT_SAVE_FILE, "Save File" },
        { PROPERTY_HINT_GLOBAL_SAVE_FILE, "Global Save File" },
        { PROPERTY_HINT_ARRAY_TYPE, "Array Type" },
        { PROPERTY_HINT_LOCALE_ID, "Locale ID" },
        { PROPERTY_HINT_LOCALIZABLE_STRING, "Localizable String" },
        { PROPERTY_HINT_NODE_TYPE, "Node Type" },
        { PROPERTY_HINT_HIDE_QUATERNION_EDIT, "Hide Quaternion Edit" },
        { PROPERTY_HINT_PASSWORD, "Password" },
        { PROPERTY_HINT_LAYERS_AVOIDANCE, "Layers Avoidance" },
        { PROPERTY_HINT_DICTIONARY_TYPE, "Dictionary Type" },
        { PROPERTY_HINT_TOOL_BUTTON, "Tool Button" },
        { PROPERTY_HINT_ONESHOT, "Oneshot" },
        { PROPERTY_HINT_GROUP_ENABLE, "Group Enable" },
        { PROPERTY_HINT_INPUT_NAME, "Input Name" },
        { PROPERTY_HINT_FILE_PATH, "File Path" },
    };
}

OrchestratorEditorAnnotationPresentation OrchestratorEditorAnnotationPresentation::get(const StringName& p_name) {
    for (const Entry& entry : ENTRIES) {
        if (p_name == StringName(entry.name)) {
            return { entry.label, entry.description };
        }
    }
    return { String(p_name), String() };
}

bool OrchestratorEditorAnnotationPresentation::get_argument_choices(const StringName& p_name, int p_argument_index, PackedStringArray& r_labels, Array& r_values) {
    if (p_name == StringName("@export_custom") && p_argument_index == 0) {
        for (const HintEntry& entry : HINTS) {
            r_labels.push_back(entry.label);
            r_values.push_back(Variant(static_cast<int64_t>(entry.hint)));
        }
        return true;
    }

    if (p_name == StringName("@rpc")) {
        switch (p_argument_index) {
            case 0:
                add_choices(RPC_MODES, r_labels, r_values);
                return true;
            case 1:
                add_choices(RPC_SYNCS, r_labels, r_values);
                return true;
            case 2:
                add_choices(RPC_TRANSFER_MODES, r_labels, r_values);
                return true;
            default:
                return false;
        }
    }

    #ifdef DEBUG_ENABLED
    // Warnings only exist in debug builds; elsewhere the name stays a free-text argument.
    if (p_name == StringName("@warning_ignore")) {
        // GDScript spells warning names in lower case inside the annotation.
        for (int i = 0; i < OScriptWarning::WARNING_MAX; i++) {
            const String name = OScriptWarning::get_name_from_code(static_cast<OScriptWarning::Code>(i));
            r_labels.push_back(name.capitalize());
            r_values.push_back(name.to_lower());
        }
        return true;
    }
    #endif

    return false;
}