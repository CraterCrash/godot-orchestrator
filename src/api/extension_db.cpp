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
#include "api/extension_db.h"

// THIS FILE IS GENERATED. EDITS WILL BE LOST.

namespace godot
{
    ExtensionDB* ExtensionDB::_singleton = nullptr;

    namespace internal
    {
        MethodInfo _make_method(const StringName& p_name, int32_t p_flags, Variant::Type p_rtype, const std::vector<PropertyInfo>& p_args, bool p_nil_is_variant = false)
        {
            int32_t return_flags = PROPERTY_USAGE_DEFAULT;
            if (p_nil_is_variant)
                return_flags |= PROPERTY_USAGE_NIL_IS_VARIANT;

            MethodInfo mi;
            mi.name = p_name;
            mi.flags = p_flags;
            mi.return_val.type = p_rtype;
            mi.return_val.usage = return_flags;
            mi.arguments.insert(mi.arguments.begin(), p_args.begin(), p_args.end());
            return mi;
        }

        String _resolve_enum_prefix(const Vector<EnumValue>& p_values)
        {
            if (p_values.size() == 0)
                return {};

            String prefix = p_values[0].name;
            // Some Godot enums are prefixed with a trailing underscore, those are our target.
            if (!prefix.contains("_"))
                return {};

            for (const EnumValue& value : p_values)
            {
                while (value.name.find(prefix) != 0)
                {
                    prefix = prefix.substr(0, prefix.length() - 1);
                    if (prefix.is_empty())
                        return {};
                }
            }
            return prefix;
        }

        bool _is_enum_values_upper_cased(const EnumInfo& p_enum)
        {
            return p_enum.name.match("EulerOrder");
        }

        void _sanitize_enum(EnumInfo& p_enum)
        {
            const bool is_key = p_enum.name.match("Key");
            const bool is_error = p_enum.name.match("Error");
            const bool is_method_flags = p_enum.name.match("MethodFlags");
            const bool is_upper = _is_enum_values_upper_cased(p_enum);

            const String prefix = _resolve_enum_prefix(p_enum.values);
            for (EnumValue& value : p_enum.values)
            {
                value.friendly_name = value.name.replace(prefix, "").capitalize();

                // Handle unique fix-ups for enum friendly names
                if (is_key && value.friendly_name.begins_with("Kp "))
                    value.friendly_name = value.friendly_name.substr(3, value.friendly_name.length()) + " (Keypad)";
                else if(is_key && value.friendly_name.begins_with("F "))
                    value.friendly_name = value.friendly_name.replace(" ", "");
                else if (is_error && value.friendly_name.begins_with("Err "))
                    value.friendly_name = value.friendly_name.substr(4, value.friendly_name.length());
                else if (is_method_flags && value.name.match("METHOD_FLAGS_DEFAULT"))
                    value.friendly_name = ""; // forces it to be skipped by some nodes (same as normal)

                if (is_upper)
                    value.friendly_name = value.friendly_name.to_upper();
            }
        }

        void _sanitize_enums(Vector<EnumInfo>& p_enums)
        {
            for (EnumInfo& ei : p_enums)
                _sanitize_enum(ei);
        }

		void ExtensionDBLoader::prime_math_constants()
		{
			// Math Constants
			ExtensionDB::_singleton->_math_constant_names.push_back("One");
			ExtensionDB::_singleton->_math_constants["One"] = { "One", Variant::FLOAT, 1.0 };
			ExtensionDB::_singleton->_math_constant_names.push_back("PI");
			ExtensionDB::_singleton->_math_constants["PI"] = { "PI", Variant::FLOAT, Math_PI };
			ExtensionDB::_singleton->_math_constant_names.push_back("PI/2");
			ExtensionDB::_singleton->_math_constants["PI/2"] = { "PI/2", Variant::FLOAT, Math_PI * 0.5 };
			ExtensionDB::_singleton->_math_constant_names.push_back("LN(2)");
			ExtensionDB::_singleton->_math_constants["LN(2)"] = { "LN(2)", Variant::FLOAT, Math_LN2 };
			ExtensionDB::_singleton->_math_constant_names.push_back("TAU");
			ExtensionDB::_singleton->_math_constants["TAU"] = { "TAU", Variant::FLOAT, Math_TAU };
			ExtensionDB::_singleton->_math_constant_names.push_back("E");
			ExtensionDB::_singleton->_math_constants["E"] = { "E", Variant::FLOAT, Math_E };
			ExtensionDB::_singleton->_math_constant_names.push_back("Sqrt1/2");
			ExtensionDB::_singleton->_math_constants["Sqrt1/2"] = { "Sqrt1/2", Variant::FLOAT, Math_SQRT12 };
			ExtensionDB::_singleton->_math_constant_names.push_back("Sqrt2");
			ExtensionDB::_singleton->_math_constants["Sqrt2"] = { "Sqrt2", Variant::FLOAT, Math_SQRT2 };
			ExtensionDB::_singleton->_math_constant_names.push_back("INF");
			ExtensionDB::_singleton->_math_constants["INF"] = { "INF", Variant::FLOAT, Math_INF };
			ExtensionDB::_singleton->_math_constant_names.push_back("NAN");
			ExtensionDB::_singleton->_math_constants["NAN"] = { "NAN", Variant::FLOAT, Math_NAN };
		}
		
		void ExtensionDBLoader::prime_global_enumerations()
		{
			// Global enumerations
			{
				EnumInfo ei;
				ei.name = "Side";
				ei.is_bitfield = false;
				ei.values.push_back({ "SIDE_LEFT", "", 0 });
				ei.values.push_back({ "SIDE_TOP", "", 1 });
				ei.values.push_back({ "SIDE_RIGHT", "", 2 });
				ei.values.push_back({ "SIDE_BOTTOM", "", 3 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Side"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Side");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Corner";
				ei.is_bitfield = false;
				ei.values.push_back({ "CORNER_TOP_LEFT", "", 0 });
				ei.values.push_back({ "CORNER_TOP_RIGHT", "", 1 });
				ei.values.push_back({ "CORNER_BOTTOM_RIGHT", "", 2 });
				ei.values.push_back({ "CORNER_BOTTOM_LEFT", "", 3 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Corner"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Corner");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Orientation";
				ei.is_bitfield = false;
				ei.values.push_back({ "VERTICAL", "", 1 });
				ei.values.push_back({ "HORIZONTAL", "", 0 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Orientation"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Orientation");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "ClockDirection";
				ei.is_bitfield = false;
				ei.values.push_back({ "CLOCKWISE", "", 0 });
				ei.values.push_back({ "COUNTERCLOCKWISE", "", 1 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["ClockDirection"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("ClockDirection");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "HorizontalAlignment";
				ei.is_bitfield = false;
				ei.values.push_back({ "HORIZONTAL_ALIGNMENT_LEFT", "", 0 });
				ei.values.push_back({ "HORIZONTAL_ALIGNMENT_CENTER", "", 1 });
				ei.values.push_back({ "HORIZONTAL_ALIGNMENT_RIGHT", "", 2 });
				ei.values.push_back({ "HORIZONTAL_ALIGNMENT_FILL", "", 3 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["HorizontalAlignment"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("HorizontalAlignment");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "VerticalAlignment";
				ei.is_bitfield = false;
				ei.values.push_back({ "VERTICAL_ALIGNMENT_TOP", "", 0 });
				ei.values.push_back({ "VERTICAL_ALIGNMENT_CENTER", "", 1 });
				ei.values.push_back({ "VERTICAL_ALIGNMENT_BOTTOM", "", 2 });
				ei.values.push_back({ "VERTICAL_ALIGNMENT_FILL", "", 3 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["VerticalAlignment"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("VerticalAlignment");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "InlineAlignment";
				ei.is_bitfield = false;
				ei.values.push_back({ "INLINE_ALIGNMENT_TOP_TO", "", 0 });
				ei.values.push_back({ "INLINE_ALIGNMENT_CENTER_TO", "", 1 });
				ei.values.push_back({ "INLINE_ALIGNMENT_BASELINE_TO", "", 3 });
				ei.values.push_back({ "INLINE_ALIGNMENT_BOTTOM_TO", "", 2 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TO_TOP", "", 0 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TO_CENTER", "", 4 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TO_BASELINE", "", 8 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TO_BOTTOM", "", 12 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TOP", "", 0 });
				ei.values.push_back({ "INLINE_ALIGNMENT_CENTER", "", 5 });
				ei.values.push_back({ "INLINE_ALIGNMENT_BOTTOM", "", 14 });
				ei.values.push_back({ "INLINE_ALIGNMENT_IMAGE_MASK", "", 3 });
				ei.values.push_back({ "INLINE_ALIGNMENT_TEXT_MASK", "", 12 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["InlineAlignment"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("InlineAlignment");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "EulerOrder";
				ei.is_bitfield = false;
				ei.values.push_back({ "EULER_ORDER_XYZ", "", 0 });
				ei.values.push_back({ "EULER_ORDER_XZY", "", 1 });
				ei.values.push_back({ "EULER_ORDER_YXZ", "", 2 });
				ei.values.push_back({ "EULER_ORDER_YZX", "", 3 });
				ei.values.push_back({ "EULER_ORDER_ZXY", "", 4 });
				ei.values.push_back({ "EULER_ORDER_ZYX", "", 5 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["EulerOrder"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("EulerOrder");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Key";
				ei.is_bitfield = false;
				ei.values.push_back({ "KEY_NONE", "", 0 });
				ei.values.push_back({ "KEY_SPECIAL", "", 4194304 });
				ei.values.push_back({ "KEY_ESCAPE", "", 4194305 });
				ei.values.push_back({ "KEY_TAB", "", 4194306 });
				ei.values.push_back({ "KEY_BACKTAB", "", 4194307 });
				ei.values.push_back({ "KEY_BACKSPACE", "", 4194308 });
				ei.values.push_back({ "KEY_ENTER", "", 4194309 });
				ei.values.push_back({ "KEY_KP_ENTER", "", 4194310 });
				ei.values.push_back({ "KEY_INSERT", "", 4194311 });
				ei.values.push_back({ "KEY_DELETE", "", 4194312 });
				ei.values.push_back({ "KEY_PAUSE", "", 4194313 });
				ei.values.push_back({ "KEY_PRINT", "", 4194314 });
				ei.values.push_back({ "KEY_SYSREQ", "", 4194315 });
				ei.values.push_back({ "KEY_CLEAR", "", 4194316 });
				ei.values.push_back({ "KEY_HOME", "", 4194317 });
				ei.values.push_back({ "KEY_END", "", 4194318 });
				ei.values.push_back({ "KEY_LEFT", "", 4194319 });
				ei.values.push_back({ "KEY_UP", "", 4194320 });
				ei.values.push_back({ "KEY_RIGHT", "", 4194321 });
				ei.values.push_back({ "KEY_DOWN", "", 4194322 });
				ei.values.push_back({ "KEY_PAGEUP", "", 4194323 });
				ei.values.push_back({ "KEY_PAGEDOWN", "", 4194324 });
				ei.values.push_back({ "KEY_SHIFT", "", 4194325 });
				ei.values.push_back({ "KEY_CTRL", "", 4194326 });
				ei.values.push_back({ "KEY_META", "", 4194327 });
				ei.values.push_back({ "KEY_ALT", "", 4194328 });
				ei.values.push_back({ "KEY_CAPSLOCK", "", 4194329 });
				ei.values.push_back({ "KEY_NUMLOCK", "", 4194330 });
				ei.values.push_back({ "KEY_SCROLLLOCK", "", 4194331 });
				ei.values.push_back({ "KEY_F1", "", 4194332 });
				ei.values.push_back({ "KEY_F2", "", 4194333 });
				ei.values.push_back({ "KEY_F3", "", 4194334 });
				ei.values.push_back({ "KEY_F4", "", 4194335 });
				ei.values.push_back({ "KEY_F5", "", 4194336 });
				ei.values.push_back({ "KEY_F6", "", 4194337 });
				ei.values.push_back({ "KEY_F7", "", 4194338 });
				ei.values.push_back({ "KEY_F8", "", 4194339 });
				ei.values.push_back({ "KEY_F9", "", 4194340 });
				ei.values.push_back({ "KEY_F10", "", 4194341 });
				ei.values.push_back({ "KEY_F11", "", 4194342 });
				ei.values.push_back({ "KEY_F12", "", 4194343 });
				ei.values.push_back({ "KEY_F13", "", 4194344 });
				ei.values.push_back({ "KEY_F14", "", 4194345 });
				ei.values.push_back({ "KEY_F15", "", 4194346 });
				ei.values.push_back({ "KEY_F16", "", 4194347 });
				ei.values.push_back({ "KEY_F17", "", 4194348 });
				ei.values.push_back({ "KEY_F18", "", 4194349 });
				ei.values.push_back({ "KEY_F19", "", 4194350 });
				ei.values.push_back({ "KEY_F20", "", 4194351 });
				ei.values.push_back({ "KEY_F21", "", 4194352 });
				ei.values.push_back({ "KEY_F22", "", 4194353 });
				ei.values.push_back({ "KEY_F23", "", 4194354 });
				ei.values.push_back({ "KEY_F24", "", 4194355 });
				ei.values.push_back({ "KEY_F25", "", 4194356 });
				ei.values.push_back({ "KEY_F26", "", 4194357 });
				ei.values.push_back({ "KEY_F27", "", 4194358 });
				ei.values.push_back({ "KEY_F28", "", 4194359 });
				ei.values.push_back({ "KEY_F29", "", 4194360 });
				ei.values.push_back({ "KEY_F30", "", 4194361 });
				ei.values.push_back({ "KEY_F31", "", 4194362 });
				ei.values.push_back({ "KEY_F32", "", 4194363 });
				ei.values.push_back({ "KEY_F33", "", 4194364 });
				ei.values.push_back({ "KEY_F34", "", 4194365 });
				ei.values.push_back({ "KEY_F35", "", 4194366 });
				ei.values.push_back({ "KEY_KP_MULTIPLY", "", 4194433 });
				ei.values.push_back({ "KEY_KP_DIVIDE", "", 4194434 });
				ei.values.push_back({ "KEY_KP_SUBTRACT", "", 4194435 });
				ei.values.push_back({ "KEY_KP_PERIOD", "", 4194436 });
				ei.values.push_back({ "KEY_KP_ADD", "", 4194437 });
				ei.values.push_back({ "KEY_KP_0", "", 4194438 });
				ei.values.push_back({ "KEY_KP_1", "", 4194439 });
				ei.values.push_back({ "KEY_KP_2", "", 4194440 });
				ei.values.push_back({ "KEY_KP_3", "", 4194441 });
				ei.values.push_back({ "KEY_KP_4", "", 4194442 });
				ei.values.push_back({ "KEY_KP_5", "", 4194443 });
				ei.values.push_back({ "KEY_KP_6", "", 4194444 });
				ei.values.push_back({ "KEY_KP_7", "", 4194445 });
				ei.values.push_back({ "KEY_KP_8", "", 4194446 });
				ei.values.push_back({ "KEY_KP_9", "", 4194447 });
				ei.values.push_back({ "KEY_MENU", "", 4194370 });
				ei.values.push_back({ "KEY_HYPER", "", 4194371 });
				ei.values.push_back({ "KEY_HELP", "", 4194373 });
				ei.values.push_back({ "KEY_BACK", "", 4194376 });
				ei.values.push_back({ "KEY_FORWARD", "", 4194377 });
				ei.values.push_back({ "KEY_STOP", "", 4194378 });
				ei.values.push_back({ "KEY_REFRESH", "", 4194379 });
				ei.values.push_back({ "KEY_VOLUMEDOWN", "", 4194380 });
				ei.values.push_back({ "KEY_VOLUMEMUTE", "", 4194381 });
				ei.values.push_back({ "KEY_VOLUMEUP", "", 4194382 });
				ei.values.push_back({ "KEY_MEDIAPLAY", "", 4194388 });
				ei.values.push_back({ "KEY_MEDIASTOP", "", 4194389 });
				ei.values.push_back({ "KEY_MEDIAPREVIOUS", "", 4194390 });
				ei.values.push_back({ "KEY_MEDIANEXT", "", 4194391 });
				ei.values.push_back({ "KEY_MEDIARECORD", "", 4194392 });
				ei.values.push_back({ "KEY_HOMEPAGE", "", 4194393 });
				ei.values.push_back({ "KEY_FAVORITES", "", 4194394 });
				ei.values.push_back({ "KEY_SEARCH", "", 4194395 });
				ei.values.push_back({ "KEY_STANDBY", "", 4194396 });
				ei.values.push_back({ "KEY_OPENURL", "", 4194397 });
				ei.values.push_back({ "KEY_LAUNCHMAIL", "", 4194398 });
				ei.values.push_back({ "KEY_LAUNCHMEDIA", "", 4194399 });
				ei.values.push_back({ "KEY_LAUNCH0", "", 4194400 });
				ei.values.push_back({ "KEY_LAUNCH1", "", 4194401 });
				ei.values.push_back({ "KEY_LAUNCH2", "", 4194402 });
				ei.values.push_back({ "KEY_LAUNCH3", "", 4194403 });
				ei.values.push_back({ "KEY_LAUNCH4", "", 4194404 });
				ei.values.push_back({ "KEY_LAUNCH5", "", 4194405 });
				ei.values.push_back({ "KEY_LAUNCH6", "", 4194406 });
				ei.values.push_back({ "KEY_LAUNCH7", "", 4194407 });
				ei.values.push_back({ "KEY_LAUNCH8", "", 4194408 });
				ei.values.push_back({ "KEY_LAUNCH9", "", 4194409 });
				ei.values.push_back({ "KEY_LAUNCHA", "", 4194410 });
				ei.values.push_back({ "KEY_LAUNCHB", "", 4194411 });
				ei.values.push_back({ "KEY_LAUNCHC", "", 4194412 });
				ei.values.push_back({ "KEY_LAUNCHD", "", 4194413 });
				ei.values.push_back({ "KEY_LAUNCHE", "", 4194414 });
				ei.values.push_back({ "KEY_LAUNCHF", "", 4194415 });
				ei.values.push_back({ "KEY_GLOBE", "", 4194416 });
				ei.values.push_back({ "KEY_KEYBOARD", "", 4194417 });
				ei.values.push_back({ "KEY_JIS_EISU", "", 4194418 });
				ei.values.push_back({ "KEY_JIS_KANA", "", 4194419 });
				ei.values.push_back({ "KEY_UNKNOWN", "", 8388607 });
				ei.values.push_back({ "KEY_SPACE", "", 32 });
				ei.values.push_back({ "KEY_EXCLAM", "", 33 });
				ei.values.push_back({ "KEY_QUOTEDBL", "", 34 });
				ei.values.push_back({ "KEY_NUMBERSIGN", "", 35 });
				ei.values.push_back({ "KEY_DOLLAR", "", 36 });
				ei.values.push_back({ "KEY_PERCENT", "", 37 });
				ei.values.push_back({ "KEY_AMPERSAND", "", 38 });
				ei.values.push_back({ "KEY_APOSTROPHE", "", 39 });
				ei.values.push_back({ "KEY_PARENLEFT", "", 40 });
				ei.values.push_back({ "KEY_PARENRIGHT", "", 41 });
				ei.values.push_back({ "KEY_ASTERISK", "", 42 });
				ei.values.push_back({ "KEY_PLUS", "", 43 });
				ei.values.push_back({ "KEY_COMMA", "", 44 });
				ei.values.push_back({ "KEY_MINUS", "", 45 });
				ei.values.push_back({ "KEY_PERIOD", "", 46 });
				ei.values.push_back({ "KEY_SLASH", "", 47 });
				ei.values.push_back({ "KEY_0", "", 48 });
				ei.values.push_back({ "KEY_1", "", 49 });
				ei.values.push_back({ "KEY_2", "", 50 });
				ei.values.push_back({ "KEY_3", "", 51 });
				ei.values.push_back({ "KEY_4", "", 52 });
				ei.values.push_back({ "KEY_5", "", 53 });
				ei.values.push_back({ "KEY_6", "", 54 });
				ei.values.push_back({ "KEY_7", "", 55 });
				ei.values.push_back({ "KEY_8", "", 56 });
				ei.values.push_back({ "KEY_9", "", 57 });
				ei.values.push_back({ "KEY_COLON", "", 58 });
				ei.values.push_back({ "KEY_SEMICOLON", "", 59 });
				ei.values.push_back({ "KEY_LESS", "", 60 });
				ei.values.push_back({ "KEY_EQUAL", "", 61 });
				ei.values.push_back({ "KEY_GREATER", "", 62 });
				ei.values.push_back({ "KEY_QUESTION", "", 63 });
				ei.values.push_back({ "KEY_AT", "", 64 });
				ei.values.push_back({ "KEY_A", "", 65 });
				ei.values.push_back({ "KEY_B", "", 66 });
				ei.values.push_back({ "KEY_C", "", 67 });
				ei.values.push_back({ "KEY_D", "", 68 });
				ei.values.push_back({ "KEY_E", "", 69 });
				ei.values.push_back({ "KEY_F", "", 70 });
				ei.values.push_back({ "KEY_G", "", 71 });
				ei.values.push_back({ "KEY_H", "", 72 });
				ei.values.push_back({ "KEY_I", "", 73 });
				ei.values.push_back({ "KEY_J", "", 74 });
				ei.values.push_back({ "KEY_K", "", 75 });
				ei.values.push_back({ "KEY_L", "", 76 });
				ei.values.push_back({ "KEY_M", "", 77 });
				ei.values.push_back({ "KEY_N", "", 78 });
				ei.values.push_back({ "KEY_O", "", 79 });
				ei.values.push_back({ "KEY_P", "", 80 });
				ei.values.push_back({ "KEY_Q", "", 81 });
				ei.values.push_back({ "KEY_R", "", 82 });
				ei.values.push_back({ "KEY_S", "", 83 });
				ei.values.push_back({ "KEY_T", "", 84 });
				ei.values.push_back({ "KEY_U", "", 85 });
				ei.values.push_back({ "KEY_V", "", 86 });
				ei.values.push_back({ "KEY_W", "", 87 });
				ei.values.push_back({ "KEY_X", "", 88 });
				ei.values.push_back({ "KEY_Y", "", 89 });
				ei.values.push_back({ "KEY_Z", "", 90 });
				ei.values.push_back({ "KEY_BRACKETLEFT", "", 91 });
				ei.values.push_back({ "KEY_BACKSLASH", "", 92 });
				ei.values.push_back({ "KEY_BRACKETRIGHT", "", 93 });
				ei.values.push_back({ "KEY_ASCIICIRCUM", "", 94 });
				ei.values.push_back({ "KEY_UNDERSCORE", "", 95 });
				ei.values.push_back({ "KEY_QUOTELEFT", "", 96 });
				ei.values.push_back({ "KEY_BRACELEFT", "", 123 });
				ei.values.push_back({ "KEY_BAR", "", 124 });
				ei.values.push_back({ "KEY_BRACERIGHT", "", 125 });
				ei.values.push_back({ "KEY_ASCIITILDE", "", 126 });
				ei.values.push_back({ "KEY_YEN", "", 165 });
				ei.values.push_back({ "KEY_SECTION", "", 167 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Key"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Key");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "KeyModifierMask";
				ei.is_bitfield = true;
				ei.values.push_back({ "KEY_CODE_MASK", "", 8388607 });
				ei.values.push_back({ "KEY_MODIFIER_MASK", "", 532676608 });
				ei.values.push_back({ "KEY_MASK_CMD_OR_CTRL", "", 16777216 });
				ei.values.push_back({ "KEY_MASK_SHIFT", "", 33554432 });
				ei.values.push_back({ "KEY_MASK_ALT", "", 67108864 });
				ei.values.push_back({ "KEY_MASK_META", "", 134217728 });
				ei.values.push_back({ "KEY_MASK_CTRL", "", 268435456 });
				ei.values.push_back({ "KEY_MASK_KPAD", "", 536870912 });
				ei.values.push_back({ "KEY_MASK_GROUP_SWITCH", "", 1073741824 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["KeyModifierMask"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("KeyModifierMask");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "KeyLocation";
				ei.is_bitfield = false;
				ei.values.push_back({ "KEY_LOCATION_UNSPECIFIED", "", 0 });
				ei.values.push_back({ "KEY_LOCATION_LEFT", "", 1 });
				ei.values.push_back({ "KEY_LOCATION_RIGHT", "", 2 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["KeyLocation"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("KeyLocation");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "MouseButton";
				ei.is_bitfield = false;
				ei.values.push_back({ "MOUSE_BUTTON_NONE", "", 0 });
				ei.values.push_back({ "MOUSE_BUTTON_LEFT", "", 1 });
				ei.values.push_back({ "MOUSE_BUTTON_RIGHT", "", 2 });
				ei.values.push_back({ "MOUSE_BUTTON_MIDDLE", "", 3 });
				ei.values.push_back({ "MOUSE_BUTTON_WHEEL_UP", "", 4 });
				ei.values.push_back({ "MOUSE_BUTTON_WHEEL_DOWN", "", 5 });
				ei.values.push_back({ "MOUSE_BUTTON_WHEEL_LEFT", "", 6 });
				ei.values.push_back({ "MOUSE_BUTTON_WHEEL_RIGHT", "", 7 });
				ei.values.push_back({ "MOUSE_BUTTON_XBUTTON1", "", 8 });
				ei.values.push_back({ "MOUSE_BUTTON_XBUTTON2", "", 9 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["MouseButton"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("MouseButton");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "MouseButtonMask";
				ei.is_bitfield = true;
				ei.values.push_back({ "MOUSE_BUTTON_MASK_LEFT", "", 1 });
				ei.values.push_back({ "MOUSE_BUTTON_MASK_RIGHT", "", 2 });
				ei.values.push_back({ "MOUSE_BUTTON_MASK_MIDDLE", "", 4 });
				ei.values.push_back({ "MOUSE_BUTTON_MASK_MB_XBUTTON1", "", 128 });
				ei.values.push_back({ "MOUSE_BUTTON_MASK_MB_XBUTTON2", "", 256 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["MouseButtonMask"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("MouseButtonMask");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "JoyButton";
				ei.is_bitfield = false;
				ei.values.push_back({ "JOY_BUTTON_INVALID", "", -1 });
				ei.values.push_back({ "JOY_BUTTON_A", "", 0 });
				ei.values.push_back({ "JOY_BUTTON_B", "", 1 });
				ei.values.push_back({ "JOY_BUTTON_X", "", 2 });
				ei.values.push_back({ "JOY_BUTTON_Y", "", 3 });
				ei.values.push_back({ "JOY_BUTTON_BACK", "", 4 });
				ei.values.push_back({ "JOY_BUTTON_GUIDE", "", 5 });
				ei.values.push_back({ "JOY_BUTTON_START", "", 6 });
				ei.values.push_back({ "JOY_BUTTON_LEFT_STICK", "", 7 });
				ei.values.push_back({ "JOY_BUTTON_RIGHT_STICK", "", 8 });
				ei.values.push_back({ "JOY_BUTTON_LEFT_SHOULDER", "", 9 });
				ei.values.push_back({ "JOY_BUTTON_RIGHT_SHOULDER", "", 10 });
				ei.values.push_back({ "JOY_BUTTON_DPAD_UP", "", 11 });
				ei.values.push_back({ "JOY_BUTTON_DPAD_DOWN", "", 12 });
				ei.values.push_back({ "JOY_BUTTON_DPAD_LEFT", "", 13 });
				ei.values.push_back({ "JOY_BUTTON_DPAD_RIGHT", "", 14 });
				ei.values.push_back({ "JOY_BUTTON_MISC1", "", 15 });
				ei.values.push_back({ "JOY_BUTTON_PADDLE1", "", 16 });
				ei.values.push_back({ "JOY_BUTTON_PADDLE2", "", 17 });
				ei.values.push_back({ "JOY_BUTTON_PADDLE3", "", 18 });
				ei.values.push_back({ "JOY_BUTTON_PADDLE4", "", 19 });
				ei.values.push_back({ "JOY_BUTTON_TOUCHPAD", "", 20 });
				ei.values.push_back({ "JOY_BUTTON_SDL_MAX", "", 21 });
				ei.values.push_back({ "JOY_BUTTON_MAX", "", 128 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["JoyButton"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("JoyButton");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "JoyAxis";
				ei.is_bitfield = false;
				ei.values.push_back({ "JOY_AXIS_INVALID", "", -1 });
				ei.values.push_back({ "JOY_AXIS_LEFT_X", "", 0 });
				ei.values.push_back({ "JOY_AXIS_LEFT_Y", "", 1 });
				ei.values.push_back({ "JOY_AXIS_RIGHT_X", "", 2 });
				ei.values.push_back({ "JOY_AXIS_RIGHT_Y", "", 3 });
				ei.values.push_back({ "JOY_AXIS_TRIGGER_LEFT", "", 4 });
				ei.values.push_back({ "JOY_AXIS_TRIGGER_RIGHT", "", 5 });
				ei.values.push_back({ "JOY_AXIS_SDL_MAX", "", 6 });
				ei.values.push_back({ "JOY_AXIS_MAX", "", 10 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["JoyAxis"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("JoyAxis");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "MIDIMessage";
				ei.is_bitfield = false;
				ei.values.push_back({ "MIDI_MESSAGE_NONE", "", 0 });
				ei.values.push_back({ "MIDI_MESSAGE_NOTE_OFF", "", 8 });
				ei.values.push_back({ "MIDI_MESSAGE_NOTE_ON", "", 9 });
				ei.values.push_back({ "MIDI_MESSAGE_AFTERTOUCH", "", 10 });
				ei.values.push_back({ "MIDI_MESSAGE_CONTROL_CHANGE", "", 11 });
				ei.values.push_back({ "MIDI_MESSAGE_PROGRAM_CHANGE", "", 12 });
				ei.values.push_back({ "MIDI_MESSAGE_CHANNEL_PRESSURE", "", 13 });
				ei.values.push_back({ "MIDI_MESSAGE_PITCH_BEND", "", 14 });
				ei.values.push_back({ "MIDI_MESSAGE_SYSTEM_EXCLUSIVE", "", 240 });
				ei.values.push_back({ "MIDI_MESSAGE_QUARTER_FRAME", "", 241 });
				ei.values.push_back({ "MIDI_MESSAGE_SONG_POSITION_POINTER", "", 242 });
				ei.values.push_back({ "MIDI_MESSAGE_SONG_SELECT", "", 243 });
				ei.values.push_back({ "MIDI_MESSAGE_TUNE_REQUEST", "", 246 });
				ei.values.push_back({ "MIDI_MESSAGE_TIMING_CLOCK", "", 248 });
				ei.values.push_back({ "MIDI_MESSAGE_START", "", 250 });
				ei.values.push_back({ "MIDI_MESSAGE_CONTINUE", "", 251 });
				ei.values.push_back({ "MIDI_MESSAGE_STOP", "", 252 });
				ei.values.push_back({ "MIDI_MESSAGE_ACTIVE_SENSING", "", 254 });
				ei.values.push_back({ "MIDI_MESSAGE_SYSTEM_RESET", "", 255 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["MIDIMessage"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("MIDIMessage");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Error";
				ei.is_bitfield = false;
				ei.values.push_back({ "OK", "", 0 });
				ei.values.push_back({ "FAILED", "", 1 });
				ei.values.push_back({ "ERR_UNAVAILABLE", "", 2 });
				ei.values.push_back({ "ERR_UNCONFIGURED", "", 3 });
				ei.values.push_back({ "ERR_UNAUTHORIZED", "", 4 });
				ei.values.push_back({ "ERR_PARAMETER_RANGE_ERROR", "", 5 });
				ei.values.push_back({ "ERR_OUT_OF_MEMORY", "", 6 });
				ei.values.push_back({ "ERR_FILE_NOT_FOUND", "", 7 });
				ei.values.push_back({ "ERR_FILE_BAD_DRIVE", "", 8 });
				ei.values.push_back({ "ERR_FILE_BAD_PATH", "", 9 });
				ei.values.push_back({ "ERR_FILE_NO_PERMISSION", "", 10 });
				ei.values.push_back({ "ERR_FILE_ALREADY_IN_USE", "", 11 });
				ei.values.push_back({ "ERR_FILE_CANT_OPEN", "", 12 });
				ei.values.push_back({ "ERR_FILE_CANT_WRITE", "", 13 });
				ei.values.push_back({ "ERR_FILE_CANT_READ", "", 14 });
				ei.values.push_back({ "ERR_FILE_UNRECOGNIZED", "", 15 });
				ei.values.push_back({ "ERR_FILE_CORRUPT", "", 16 });
				ei.values.push_back({ "ERR_FILE_MISSING_DEPENDENCIES", "", 17 });
				ei.values.push_back({ "ERR_FILE_EOF", "", 18 });
				ei.values.push_back({ "ERR_CANT_OPEN", "", 19 });
				ei.values.push_back({ "ERR_CANT_CREATE", "", 20 });
				ei.values.push_back({ "ERR_QUERY_FAILED", "", 21 });
				ei.values.push_back({ "ERR_ALREADY_IN_USE", "", 22 });
				ei.values.push_back({ "ERR_LOCKED", "", 23 });
				ei.values.push_back({ "ERR_TIMEOUT", "", 24 });
				ei.values.push_back({ "ERR_CANT_CONNECT", "", 25 });
				ei.values.push_back({ "ERR_CANT_RESOLVE", "", 26 });
				ei.values.push_back({ "ERR_CONNECTION_ERROR", "", 27 });
				ei.values.push_back({ "ERR_CANT_ACQUIRE_RESOURCE", "", 28 });
				ei.values.push_back({ "ERR_CANT_FORK", "", 29 });
				ei.values.push_back({ "ERR_INVALID_DATA", "", 30 });
				ei.values.push_back({ "ERR_INVALID_PARAMETER", "", 31 });
				ei.values.push_back({ "ERR_ALREADY_EXISTS", "", 32 });
				ei.values.push_back({ "ERR_DOES_NOT_EXIST", "", 33 });
				ei.values.push_back({ "ERR_DATABASE_CANT_READ", "", 34 });
				ei.values.push_back({ "ERR_DATABASE_CANT_WRITE", "", 35 });
				ei.values.push_back({ "ERR_COMPILATION_FAILED", "", 36 });
				ei.values.push_back({ "ERR_METHOD_NOT_FOUND", "", 37 });
				ei.values.push_back({ "ERR_LINK_FAILED", "", 38 });
				ei.values.push_back({ "ERR_SCRIPT_FAILED", "", 39 });
				ei.values.push_back({ "ERR_CYCLIC_LINK", "", 40 });
				ei.values.push_back({ "ERR_INVALID_DECLARATION", "", 41 });
				ei.values.push_back({ "ERR_DUPLICATE_SYMBOL", "", 42 });
				ei.values.push_back({ "ERR_PARSE_ERROR", "", 43 });
				ei.values.push_back({ "ERR_BUSY", "", 44 });
				ei.values.push_back({ "ERR_SKIP", "", 45 });
				ei.values.push_back({ "ERR_HELP", "", 46 });
				ei.values.push_back({ "ERR_BUG", "", 47 });
				ei.values.push_back({ "ERR_PRINTER_ON_FIRE", "", 48 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Error"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Error");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "PropertyHint";
				ei.is_bitfield = false;
				ei.values.push_back({ "PROPERTY_HINT_NONE", "", 0 });
				ei.values.push_back({ "PROPERTY_HINT_RANGE", "", 1 });
				ei.values.push_back({ "PROPERTY_HINT_ENUM", "", 2 });
				ei.values.push_back({ "PROPERTY_HINT_ENUM_SUGGESTION", "", 3 });
				ei.values.push_back({ "PROPERTY_HINT_EXP_EASING", "", 4 });
				ei.values.push_back({ "PROPERTY_HINT_LINK", "", 5 });
				ei.values.push_back({ "PROPERTY_HINT_FLAGS", "", 6 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_2D_RENDER", "", 7 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_2D_PHYSICS", "", 8 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_2D_NAVIGATION", "", 9 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_3D_RENDER", "", 10 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_3D_PHYSICS", "", 11 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_3D_NAVIGATION", "", 12 });
				ei.values.push_back({ "PROPERTY_HINT_LAYERS_AVOIDANCE", "", 37 });
				ei.values.push_back({ "PROPERTY_HINT_FILE", "", 13 });
				ei.values.push_back({ "PROPERTY_HINT_DIR", "", 14 });
				ei.values.push_back({ "PROPERTY_HINT_GLOBAL_FILE", "", 15 });
				ei.values.push_back({ "PROPERTY_HINT_GLOBAL_DIR", "", 16 });
				ei.values.push_back({ "PROPERTY_HINT_RESOURCE_TYPE", "", 17 });
				ei.values.push_back({ "PROPERTY_HINT_MULTILINE_TEXT", "", 18 });
				ei.values.push_back({ "PROPERTY_HINT_EXPRESSION", "", 19 });
				ei.values.push_back({ "PROPERTY_HINT_PLACEHOLDER_TEXT", "", 20 });
				ei.values.push_back({ "PROPERTY_HINT_COLOR_NO_ALPHA", "", 21 });
				ei.values.push_back({ "PROPERTY_HINT_OBJECT_ID", "", 22 });
				ei.values.push_back({ "PROPERTY_HINT_TYPE_STRING", "", 23 });
				ei.values.push_back({ "PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE", "", 24 });
				ei.values.push_back({ "PROPERTY_HINT_OBJECT_TOO_BIG", "", 25 });
				ei.values.push_back({ "PROPERTY_HINT_NODE_PATH_VALID_TYPES", "", 26 });
				ei.values.push_back({ "PROPERTY_HINT_SAVE_FILE", "", 27 });
				ei.values.push_back({ "PROPERTY_HINT_GLOBAL_SAVE_FILE", "", 28 });
				ei.values.push_back({ "PROPERTY_HINT_INT_IS_OBJECTID", "", 29 });
				ei.values.push_back({ "PROPERTY_HINT_INT_IS_POINTER", "", 30 });
				ei.values.push_back({ "PROPERTY_HINT_ARRAY_TYPE", "", 31 });
				ei.values.push_back({ "PROPERTY_HINT_LOCALE_ID", "", 32 });
				ei.values.push_back({ "PROPERTY_HINT_LOCALIZABLE_STRING", "", 33 });
				ei.values.push_back({ "PROPERTY_HINT_NODE_TYPE", "", 34 });
				ei.values.push_back({ "PROPERTY_HINT_HIDE_QUATERNION_EDIT", "", 35 });
				ei.values.push_back({ "PROPERTY_HINT_PASSWORD", "", 36 });
				ei.values.push_back({ "PROPERTY_HINT_MAX", "", 38 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["PropertyHint"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("PropertyHint");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "PropertyUsageFlags";
				ei.is_bitfield = true;
				ei.values.push_back({ "PROPERTY_USAGE_NONE", "", 0 });
				ei.values.push_back({ "PROPERTY_USAGE_STORAGE", "", 2 });
				ei.values.push_back({ "PROPERTY_USAGE_EDITOR", "", 4 });
				ei.values.push_back({ "PROPERTY_USAGE_INTERNAL", "", 8 });
				ei.values.push_back({ "PROPERTY_USAGE_CHECKABLE", "", 16 });
				ei.values.push_back({ "PROPERTY_USAGE_CHECKED", "", 32 });
				ei.values.push_back({ "PROPERTY_USAGE_GROUP", "", 64 });
				ei.values.push_back({ "PROPERTY_USAGE_CATEGORY", "", 128 });
				ei.values.push_back({ "PROPERTY_USAGE_SUBGROUP", "", 256 });
				ei.values.push_back({ "PROPERTY_USAGE_CLASS_IS_BITFIELD", "", 512 });
				ei.values.push_back({ "PROPERTY_USAGE_NO_INSTANCE_STATE", "", 1024 });
				ei.values.push_back({ "PROPERTY_USAGE_RESTART_IF_CHANGED", "", 2048 });
				ei.values.push_back({ "PROPERTY_USAGE_SCRIPT_VARIABLE", "", 4096 });
				ei.values.push_back({ "PROPERTY_USAGE_STORE_IF_NULL", "", 8192 });
				ei.values.push_back({ "PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED", "", 16384 });
				ei.values.push_back({ "PROPERTY_USAGE_SCRIPT_DEFAULT_VALUE", "", 32768 });
				ei.values.push_back({ "PROPERTY_USAGE_CLASS_IS_ENUM", "", 65536 });
				ei.values.push_back({ "PROPERTY_USAGE_NIL_IS_VARIANT", "", 131072 });
				ei.values.push_back({ "PROPERTY_USAGE_ARRAY", "", 262144 });
				ei.values.push_back({ "PROPERTY_USAGE_ALWAYS_DUPLICATE", "", 524288 });
				ei.values.push_back({ "PROPERTY_USAGE_NEVER_DUPLICATE", "", 1048576 });
				ei.values.push_back({ "PROPERTY_USAGE_HIGH_END_GFX", "", 2097152 });
				ei.values.push_back({ "PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT", "", 4194304 });
				ei.values.push_back({ "PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT", "", 8388608 });
				ei.values.push_back({ "PROPERTY_USAGE_KEYING_INCREMENTS", "", 16777216 });
				ei.values.push_back({ "PROPERTY_USAGE_DEFERRED_SET_RESOURCE", "", 33554432 });
				ei.values.push_back({ "PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT", "", 67108864 });
				ei.values.push_back({ "PROPERTY_USAGE_EDITOR_BASIC_SETTING", "", 134217728 });
				ei.values.push_back({ "PROPERTY_USAGE_READ_ONLY", "", 268435456 });
				ei.values.push_back({ "PROPERTY_USAGE_SECRET", "", 536870912 });
				ei.values.push_back({ "PROPERTY_USAGE_DEFAULT", "", 6 });
				ei.values.push_back({ "PROPERTY_USAGE_NO_EDITOR", "", 2 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["PropertyUsageFlags"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("PropertyUsageFlags");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "MethodFlags";
				ei.is_bitfield = true;
				ei.values.push_back({ "METHOD_FLAG_NORMAL", "", 1 });
				ei.values.push_back({ "METHOD_FLAG_EDITOR", "", 2 });
				ei.values.push_back({ "METHOD_FLAG_CONST", "", 4 });
				ei.values.push_back({ "METHOD_FLAG_VIRTUAL", "", 8 });
				ei.values.push_back({ "METHOD_FLAG_VARARG", "", 16 });
				ei.values.push_back({ "METHOD_FLAG_STATIC", "", 32 });
				ei.values.push_back({ "METHOD_FLAG_OBJECT_CORE", "", 64 });
				ei.values.push_back({ "METHOD_FLAGS_DEFAULT", "", 1 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["MethodFlags"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("MethodFlags");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Variant.Type";
				ei.is_bitfield = false;
				ei.values.push_back({ "TYPE_NIL", "", 0 });
				ei.values.push_back({ "TYPE_BOOL", "", 1 });
				ei.values.push_back({ "TYPE_INT", "", 2 });
				ei.values.push_back({ "TYPE_FLOAT", "", 3 });
				ei.values.push_back({ "TYPE_STRING", "", 4 });
				ei.values.push_back({ "TYPE_VECTOR2", "", 5 });
				ei.values.push_back({ "TYPE_VECTOR2I", "", 6 });
				ei.values.push_back({ "TYPE_RECT2", "", 7 });
				ei.values.push_back({ "TYPE_RECT2I", "", 8 });
				ei.values.push_back({ "TYPE_VECTOR3", "", 9 });
				ei.values.push_back({ "TYPE_VECTOR3I", "", 10 });
				ei.values.push_back({ "TYPE_TRANSFORM2D", "", 11 });
				ei.values.push_back({ "TYPE_VECTOR4", "", 12 });
				ei.values.push_back({ "TYPE_VECTOR4I", "", 13 });
				ei.values.push_back({ "TYPE_PLANE", "", 14 });
				ei.values.push_back({ "TYPE_QUATERNION", "", 15 });
				ei.values.push_back({ "TYPE_AABB", "", 16 });
				ei.values.push_back({ "TYPE_BASIS", "", 17 });
				ei.values.push_back({ "TYPE_TRANSFORM3D", "", 18 });
				ei.values.push_back({ "TYPE_PROJECTION", "", 19 });
				ei.values.push_back({ "TYPE_COLOR", "", 20 });
				ei.values.push_back({ "TYPE_STRING_NAME", "", 21 });
				ei.values.push_back({ "TYPE_NODE_PATH", "", 22 });
				ei.values.push_back({ "TYPE_RID", "", 23 });
				ei.values.push_back({ "TYPE_OBJECT", "", 24 });
				ei.values.push_back({ "TYPE_CALLABLE", "", 25 });
				ei.values.push_back({ "TYPE_SIGNAL", "", 26 });
				ei.values.push_back({ "TYPE_DICTIONARY", "", 27 });
				ei.values.push_back({ "TYPE_ARRAY", "", 28 });
				ei.values.push_back({ "TYPE_PACKED_BYTE_ARRAY", "", 29 });
				ei.values.push_back({ "TYPE_PACKED_INT32_ARRAY", "", 30 });
				ei.values.push_back({ "TYPE_PACKED_INT64_ARRAY", "", 31 });
				ei.values.push_back({ "TYPE_PACKED_FLOAT32_ARRAY", "", 32 });
				ei.values.push_back({ "TYPE_PACKED_FLOAT64_ARRAY", "", 33 });
				ei.values.push_back({ "TYPE_PACKED_STRING_ARRAY", "", 34 });
				ei.values.push_back({ "TYPE_PACKED_VECTOR2_ARRAY", "", 35 });
				ei.values.push_back({ "TYPE_PACKED_VECTOR3_ARRAY", "", 36 });
				ei.values.push_back({ "TYPE_PACKED_COLOR_ARRAY", "", 37 });
				ei.values.push_back({ "TYPE_PACKED_VECTOR4_ARRAY", "", 38 });
				ei.values.push_back({ "TYPE_MAX", "", 39 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Variant.Type"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Variant.Type");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
			{
				EnumInfo ei;
				ei.name = "Variant.Operator";
				ei.is_bitfield = false;
				ei.values.push_back({ "OP_EQUAL", "", 0 });
				ei.values.push_back({ "OP_NOT_EQUAL", "", 1 });
				ei.values.push_back({ "OP_LESS", "", 2 });
				ei.values.push_back({ "OP_LESS_EQUAL", "", 3 });
				ei.values.push_back({ "OP_GREATER", "", 4 });
				ei.values.push_back({ "OP_GREATER_EQUAL", "", 5 });
				ei.values.push_back({ "OP_ADD", "", 6 });
				ei.values.push_back({ "OP_SUBTRACT", "", 7 });
				ei.values.push_back({ "OP_MULTIPLY", "", 8 });
				ei.values.push_back({ "OP_DIVIDE", "", 9 });
				ei.values.push_back({ "OP_NEGATE", "", 10 });
				ei.values.push_back({ "OP_POSITIVE", "", 11 });
				ei.values.push_back({ "OP_MODULE", "", 12 });
				ei.values.push_back({ "OP_POWER", "", 13 });
				ei.values.push_back({ "OP_SHIFT_LEFT", "", 14 });
				ei.values.push_back({ "OP_SHIFT_RIGHT", "", 15 });
				ei.values.push_back({ "OP_BIT_AND", "", 16 });
				ei.values.push_back({ "OP_BIT_OR", "", 17 });
				ei.values.push_back({ "OP_BIT_XOR", "", 18 });
				ei.values.push_back({ "OP_BIT_NEGATE", "", 19 });
				ei.values.push_back({ "OP_AND", "", 20 });
				ei.values.push_back({ "OP_OR", "", 21 });
				ei.values.push_back({ "OP_XOR", "", 22 });
				ei.values.push_back({ "OP_NOT", "", 23 });
				ei.values.push_back({ "OP_IN", "", 24 });
				ei.values.push_back({ "OP_MAX", "", 25 });
				_sanitize_enum(ei);
				ExtensionDB::_singleton->_global_enums["Variant.Operator"] = ei;
				ExtensionDB::_singleton->_global_enum_names.push_back("Variant.Operator");
				for (const EnumValue& v : ei.values)
					ExtensionDB::_singleton->_global_enum_value_names.push_back(v.name);
			}
		}
		
		void ExtensionDBLoader::prime_builtin_classes()
		{
			// Builtin Data Types
			{
				BuiltInType type;
				type.name = "Nil";
				type.type = Variant::NIL;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::NIL, "Nil", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::NIL, "Nil", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::NIL, "Nil", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::NIL, "Nil", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::NIL, "Nil", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::NIL, "Nil", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::NIL, "Nil", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::NIL, "Nil", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::NIL, "Nil", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::NIL, "Nil", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::NIL, "Nil", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::RECT2, "Rect2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::RECT2, "Rect2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::RECT2I, "Rect2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::RECT2I, "Rect2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::TRANSFORM2D, "Transform2D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::TRANSFORM2D, "Transform2D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PLANE, "Plane", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PLANE, "Plane", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::QUATERNION, "Quaternion", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::QUATERNION, "Quaternion", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::AABB, "AABB", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::AABB, "AABB", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::BASIS, "Basis", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::BASIS, "Basis", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::TRANSFORM3D, "Transform3D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::TRANSFORM3D, "Transform3D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PROJECTION, "Projection", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PROJECTION, "Projection", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::COLOR, "Color", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::COLOR, "Color", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::NODE_PATH, "NodePath", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::NODE_PATH, "NodePath", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::NIL, "Nil", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::NIL, "Nil", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::NIL, "Nil", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::CALLABLE, "Callable", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::CALLABLE, "Callable", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::SIGNAL, "Signal", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::SIGNAL, "Signal", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::NIL, "Nil", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::NIL, "Nil", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NIL, "Nil", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::NIL, "from") } });
				ExtensionDB::_singleton->_builtin_types["Nil"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::NIL] = "Nil";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Nil");
			}
			{
				BuiltInType type;
				type.name = "bool";
				type.type = Variant::BOOL;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::BOOL, "bool", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::BOOL, "bool", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::BOOL, "bool", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::BOOL, "bool", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::BOOL, "bool", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::BOOL, "bool", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::BOOL, "bool", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::BOOL, "bool", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::BOOL, "bool", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::BOOL, "bool", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::BOOL, "bool", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::BOOL, "bool", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::BOOL, "bool", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::BOOL, "bool", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::BOOL, "bool", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::BOOL, "bool", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::BOOL, "bool", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::BOOL, "bool", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::BOOL, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "from") } });
				ExtensionDB::_singleton->_builtin_types["bool"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::BOOL] = "bool";
				ExtensionDB::_singleton->_builtin_type_names.push_back("bool");
			}
			{
				BuiltInType type;
				type.name = "int";
				type.type = Variant::INT;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::INT, "int", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::INT, "int", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::INT, "int", Variant::NIL, "", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::INT, "int", Variant::NIL, "", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_BIT_NEGATE, "~", "Bitwise Negate", Variant::INT, "int", Variant::NIL, "", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::INT, "int", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::INT, "int", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::INT, "int", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::INT, "int", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::INT, "int", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::INT, "int", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::INT, "int", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_POWER, "**", "Power", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_SHIFT_LEFT, "<<", "Shift Left", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_SHIFT_RIGHT, ">>", "Shift Right", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_BIT_AND, "&", "Bitwise And", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_BIT_OR, "|", "Bitwise Or", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_BIT_XOR, "^", "Bitwise Xor", Variant::INT, "int", Variant::INT, "int", Variant::INT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::INT, "int", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::INT, "int", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::INT, "int", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::INT, "int", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_POWER, "**", "Power", Variant::INT, "int", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::INT, "int", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::QUATERNION, "Quaternion", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::INT, "int", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::INT, "int", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::INT, "int", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::INT, "int", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::INT, "int", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::BOOL, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "from") } });
				ExtensionDB::_singleton->_builtin_types["int"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::INT] = "int";
				ExtensionDB::_singleton->_builtin_type_names.push_back("int");
			}
			{
				BuiltInType type;
				type.name = "float";
				type.type = Variant::FLOAT;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::FLOAT, "float", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::FLOAT, "float", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::FLOAT, "float", Variant::NIL, "", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::FLOAT, "float", Variant::NIL, "", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::FLOAT, "float", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::FLOAT, "float", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::FLOAT, "float", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::FLOAT, "float", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::FLOAT, "float", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::FLOAT, "float", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::FLOAT, "float", Variant::BOOL, "bool", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::FLOAT, "float", Variant::INT, "int", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::FLOAT, "float", Variant::INT, "int", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::INT, "int", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::FLOAT, "float", Variant::INT, "int", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_POWER, "**", "Power", Variant::FLOAT, "float", Variant::INT, "int", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::FLOAT, "float", Variant::INT, "int", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_POWER, "**", "Power", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::FLOAT });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::FLOAT, "float", Variant::FLOAT, "float", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::QUATERNION, "Quaternion", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::FLOAT, "float", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_AND, "and", "And", Variant::FLOAT, "float", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_OR, "or", "Or", Variant::FLOAT, "float", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_XOR, "xor", "Xor", Variant::FLOAT, "float", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::FLOAT, "float", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::BOOL, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "from") } });
				ExtensionDB::_singleton->_builtin_types["float"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::FLOAT] = "float";
				ExtensionDB::_singleton->_builtin_type_names.push_back("float");
			}
			{
				BuiltInType type;
				type.name = "String";
				type.type = Variant::STRING;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::STRING;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING, "String", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING, "String", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::NIL, "Variant", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::STRING, "String", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::BOOL, "bool", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::INT, "int", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::FLOAT, "float", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::STRING, "String", Variant::STRING, "String", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::STRING, "String", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR2, "Vector2", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR2I, "Vector2i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::RECT2, "Rect2", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::RECT2I, "Rect2i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR3, "Vector3", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR3I, "Vector3i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::TRANSFORM2D, "Transform2D", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR4, "Vector4", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::VECTOR4I, "Vector4i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PLANE, "Plane", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::QUATERNION, "Quaternion", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::AABB, "AABB", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::BASIS, "Basis", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::TRANSFORM3D, "Transform3D", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PROJECTION, "Projection", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::COLOR, "Color", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING, "String", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING, "String", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::STRING, "String", Variant::STRING_NAME, "StringName", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::STRING_NAME, "StringName", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::NODE_PATH, "NodePath", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::OBJECT, "Object", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::CALLABLE, "Callable", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::SIGNAL, "Signal", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::DICTIONARY, "Dictionary", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::ARRAY, "Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING, "String", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING, "String", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::STRING });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING_NAME, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::NODE_PATH, "from") } });
				type.methods.push_back(_make_method("casecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("nocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("naturalcasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("naturalnocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("filecasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("filenocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("substr", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "from" }, { Variant::INT, "len" } }));
				type.methods.push_back(_make_method("get_slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "delimiter" }, { Variant::INT, "slice" } }));
				type.methods.push_back(_make_method("get_slicec", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "delimiter" }, { Variant::INT, "slice" } }));
				type.methods.push_back(_make_method("get_slice_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "delimiter" } }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("findn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" }, { Variant::INT, "to" } }));
				type.methods.push_back(_make_method("countn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" }, { Variant::INT, "to" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfindn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("match", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "expr" } }));
				type.methods.push_back(_make_method("matchn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "expr" } }));
				type.methods.push_back(_make_method("begins_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("ends_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("is_subsequence_of", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("is_subsequence_ofn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("bigrams", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, {  }));
				type.methods.push_back(_make_method("similarity", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("format", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::NIL, "values" }, { Variant::STRING, "placeholder" } }));
				type.methods.push_back(_make_method("replace", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "what" }, { Variant::STRING, "forwhat" } }));
				type.methods.push_back(_make_method("replacen", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "what" }, { Variant::STRING, "forwhat" } }));
				type.methods.push_back(_make_method("repeat", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "count" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "position" }, { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("erase", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "position" }, { Variant::INT, "chars" } }));
				type.methods.push_back(_make_method("capitalize", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_camel_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_pascal_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_snake_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("split", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" }, { Variant::INT, "maxsplit" } }));
				type.methods.push_back(_make_method("rsplit", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" }, { Variant::INT, "maxsplit" } }));
				type.methods.push_back(_make_method("split_floats", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT64_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" } }));
				type.methods.push_back(_make_method("join", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::PACKED_STRING_ARRAY, "parts" } }));
				type.methods.push_back(_make_method("to_upper", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_lower", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("left", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "length" } }));
				type.methods.push_back(_make_method("right", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "length" } }));
				type.methods.push_back(_make_method("strip_edges", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::BOOL, "left" }, { Variant::BOOL, "right" } }));
				type.methods.push_back(_make_method("strip_escapes", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("lstrip", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "chars" } }));
				type.methods.push_back(_make_method("rstrip", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "chars" } }));
				type.methods.push_back(_make_method("get_extension", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_basename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("path_join", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "file" } }));
				type.methods.push_back(_make_method("unicode_at", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "at" } }));
				type.methods.push_back(_make_method("indent", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "prefix" } }));
				type.methods.push_back(_make_method("dedent", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("md5_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("sha1_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("sha256_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("md5_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sha1_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sha256_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("contains", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("containsn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("is_absolute_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_relative_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("simplify_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_base_dir", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_file", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("xml_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::BOOL, "escape_quotes" } }));
				type.methods.push_back(_make_method("xml_unescape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("uri_encode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("uri_decode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("c_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("c_unescape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("json_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("validate_node_name", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("validate_filename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("is_valid_identifier", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_float", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_hex_number", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::BOOL, "with_prefix" } }));
				type.methods.push_back(_make_method("is_valid_html_color", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_ip_address", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_filename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_float", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("hex_to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("bin_to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("lpad", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "min_length" }, { Variant::STRING, "character" } }));
				type.methods.push_back(_make_method("rpad", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "min_length" }, { Variant::STRING, "character" } }));
				type.methods.push_back(_make_method("pad_decimals", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "digits" } }));
				type.methods.push_back(_make_method("pad_zeros", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "digits" } }));
				type.methods.push_back(_make_method("trim_prefix", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "prefix" } }));
				type.methods.push_back(_make_method("trim_suffix", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "suffix" } }));
				type.methods.push_back(_make_method("to_ascii_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf8_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf16_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf32_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("hex_decode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_wchar_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("num_scientific", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::FLOAT, "number" } }));
				type.methods.push_back(_make_method("num", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::FLOAT, "number" }, { Variant::INT, "decimals" } }));
				type.methods.push_back(_make_method("num_int64", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::INT, "number" }, { Variant::INT, "base" }, { Variant::BOOL, "capitalize_hex" } }));
				type.methods.push_back(_make_method("num_uint64", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::INT, "number" }, { Variant::INT, "base" }, { Variant::BOOL, "capitalize_hex" } }));
				type.methods.push_back(_make_method("chr", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::INT, "char" } }));
				type.methods.push_back(_make_method("humanize_size", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::STRING, { { Variant::INT, "size" } }));
				ExtensionDB::_singleton->_builtin_types["String"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::STRING] = "String";
				ExtensionDB::_singleton->_builtin_type_names.push_back("String");
			}
			{
				BuiltInType type;
				type.name = "Vector2";
				type.type = Variant::VECTOR2;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR2, "Vector2", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR2, "Vector2", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR2, "Vector2", Variant::NIL, "", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR2, "Vector2", Variant::NIL, "", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR2, "Vector2", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2, "Vector2", Variant::INT, "int", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2, "Vector2", Variant::INT, "int", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2, "Vector2", Variant::FLOAT, "float", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2, "Vector2", Variant::FLOAT, "float", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2, "Vector2", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2, "Vector2", Variant::TRANSFORM2D, "Transform2D", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR2, "Vector2", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR2, "Vector2", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR2, "Vector2", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "x"), PropertyInfo(Variant::FLOAT, "y") } });
				type.properties.push_back({ Variant::FLOAT, "x" });
				type.properties.push_back({ Variant::FLOAT, "y" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "ZERO", Variant::VECTOR2, Vector2(0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR2, Vector2(1, 1) });
				type.constants.push_back({ "INF", Variant::VECTOR2, Vector2(INFINITY, INFINITY) });
				type.constants.push_back({ "LEFT", Variant::VECTOR2, Vector2(-1, 0) });
				type.constants.push_back({ "RIGHT", Variant::VECTOR2, Vector2(1, 0) });
				type.constants.push_back({ "UP", Variant::VECTOR2, Vector2(0, -1) });
				type.constants.push_back({ "DOWN", Variant::VECTOR2, Vector2(0, 1) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("angle", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("angle_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("angle_to_point", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("direction_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("limit_length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "length" } }));
				type.methods.push_back(_make_method("normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("is_normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("is_zero_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("posmod", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "mod" } }));
				type.methods.push_back(_make_method("posmodv", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "modv" } }));
				type.methods.push_back(_make_method("project", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "b" } }));
				type.methods.push_back(_make_method("lerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("slerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "b" }, { Variant::VECTOR2, "pre_a" }, { Variant::VECTOR2, "post_b" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate_in_time", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "b" }, { Variant::VECTOR2, "pre_a" }, { Variant::VECTOR2, "post_b" }, { Variant::FLOAT, "weight" }, { Variant::FLOAT, "b_t" }, { Variant::FLOAT, "pre_a_t" }, { Variant::FLOAT, "post_b_t" } }));
				type.methods.push_back(_make_method("bezier_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "control_1" }, { Variant::VECTOR2, "control_2" }, { Variant::VECTOR2, "end" }, { Variant::FLOAT, "t" } }));
				type.methods.push_back(_make_method("bezier_derivative", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "control_1" }, { Variant::VECTOR2, "control_2" }, { Variant::VECTOR2, "end" }, { Variant::FLOAT, "t" } }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("move_toward", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "to" }, { Variant::FLOAT, "delta" } }));
				type.methods.push_back(_make_method("rotated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("orthogonal", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("floor", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("ceil", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("round", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("aspect", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("dot", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "with" } }));
				type.methods.push_back(_make_method("slide", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "n" } }));
				type.methods.push_back(_make_method("bounce", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "n" } }));
				type.methods.push_back(_make_method("reflect", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "line" } }));
				type.methods.push_back(_make_method("cross", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2, "with" } }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "min" }, { Variant::VECTOR2, "max" } }));
				type.methods.push_back(_make_method("clampf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "min" }, { Variant::FLOAT, "max" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "step" } }));
				type.methods.push_back(_make_method("snappedf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "step" } }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "with" } }));
				type.methods.push_back(_make_method("minf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "with" } }));
				type.methods.push_back(_make_method("maxf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::FLOAT, "with" } }));
				type.methods.push_back(_make_method("from_angle", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::VECTOR2, { { Variant::FLOAT, "angle" } }));
				ExtensionDB::_singleton->_builtin_types["Vector2"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR2] = "Vector2";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector2");
			}
			{
				BuiltInType type;
				type.name = "Vector2i";
				type.type = Variant::VECTOR2I;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR2I, "Vector2i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR2I, "Vector2i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR2I, "Vector2i", Variant::NIL, "", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR2I, "Vector2i", Variant::NIL, "", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR2I, "Vector2i", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2I, "Vector2i", Variant::INT, "int", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2I, "Vector2i", Variant::INT, "int", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR2I, "Vector2i", Variant::INT, "int", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2I, "Vector2i", Variant::FLOAT, "float", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2I, "Vector2i", Variant::FLOAT, "float", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I, "Vector2i", Variant::VECTOR2I });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR2I, "Vector2i", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR2I, "Vector2i", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "x"), PropertyInfo(Variant::INT, "y") } });
				type.properties.push_back({ Variant::INT, "x" });
				type.properties.push_back({ Variant::INT, "y" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "ZERO", Variant::VECTOR2I, Vector2i(0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR2I, Vector2i(1, 1) });
				type.constants.push_back({ "MIN", Variant::VECTOR2I, Vector2i(-2147483648, -2147483648) });
				type.constants.push_back({ "MAX", Variant::VECTOR2I, Vector2i(2147483647, 2147483647) });
				type.constants.push_back({ "LEFT", Variant::VECTOR2I, Vector2i(-1, 0) });
				type.constants.push_back({ "RIGHT", Variant::VECTOR2I, Vector2i(1, 0) });
				type.constants.push_back({ "UP", Variant::VECTOR2I, Vector2i(0, -1) });
				type.constants.push_back({ "DOWN", Variant::VECTOR2I, Vector2i(0, 1) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("aspect", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR2I, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR2I, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, {  }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, {  }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::VECTOR2I, "min" }, { Variant::VECTOR2I, "max" } }));
				type.methods.push_back(_make_method("clampi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::INT, "min" }, { Variant::INT, "max" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::VECTOR2I, "step" } }));
				type.methods.push_back(_make_method("snappedi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::INT, "step" } }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::VECTOR2I, "with" } }));
				type.methods.push_back(_make_method("mini", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::INT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::VECTOR2I, "with" } }));
				type.methods.push_back(_make_method("maxi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, { { Variant::INT, "with" } }));
				ExtensionDB::_singleton->_builtin_types["Vector2i"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR2I] = "Vector2i";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector2i");
			}
			{
				BuiltInType type;
				type.name = "Rect2";
				type.type = Variant::RECT2;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RECT2, "Rect2", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RECT2, "Rect2", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::RECT2, "Rect2", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RECT2, "Rect2", Variant::RECT2, "Rect2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RECT2, "Rect2", Variant::RECT2, "Rect2", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::RECT2, "Rect2", Variant::TRANSFORM2D, "Transform2D", Variant::RECT2 });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::RECT2, "Rect2", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::RECT2, "Rect2", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::RECT2, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::RECT2I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2, "position"), PropertyInfo(Variant::VECTOR2, "size") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "x"), PropertyInfo(Variant::FLOAT, "y"), PropertyInfo(Variant::FLOAT, "width"), PropertyInfo(Variant::FLOAT, "height") } });
				type.properties.push_back({ Variant::VECTOR2, "position" });
				type.properties.push_back({ Variant::VECTOR2, "size" });
				type.properties.push_back({ Variant::VECTOR2, "end" });
				type.methods.push_back(_make_method("get_center", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("get_area", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("has_area", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("has_point", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR2, "point" } }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::RECT2, "rect" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("intersects", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::RECT2, "b" }, { Variant::BOOL, "include_borders" } }));
				type.methods.push_back(_make_method("encloses", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::RECT2, "b" } }));
				type.methods.push_back(_make_method("intersection", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::RECT2, "b" } }));
				type.methods.push_back(_make_method("merge", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::RECT2, "b" } }));
				type.methods.push_back(_make_method("expand", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::VECTOR2, "to" } }));
				type.methods.push_back(_make_method("grow", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::FLOAT, "amount" } }));
				type.methods.push_back(_make_method("grow_side", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::INT, "side" }, { Variant::FLOAT, "amount" } }));
				type.methods.push_back(_make_method("grow_individual", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, { { Variant::FLOAT, "left" }, { Variant::FLOAT, "top" }, { Variant::FLOAT, "right" }, { Variant::FLOAT, "bottom" } }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2, {  }));
				ExtensionDB::_singleton->_builtin_types["Rect2"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::RECT2] = "Rect2";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Rect2");
			}
			{
				BuiltInType type;
				type.name = "Rect2i";
				type.type = Variant::RECT2I;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RECT2I, "Rect2i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RECT2I, "Rect2i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::RECT2I, "Rect2i", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RECT2I, "Rect2i", Variant::RECT2I, "Rect2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RECT2I, "Rect2i", Variant::RECT2I, "Rect2i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::RECT2I, "Rect2i", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::RECT2I, "Rect2i", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::RECT2I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::RECT2, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2I, "position"), PropertyInfo(Variant::VECTOR2I, "size") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "x"), PropertyInfo(Variant::INT, "y"), PropertyInfo(Variant::INT, "width"), PropertyInfo(Variant::INT, "height") } });
				type.properties.push_back({ Variant::VECTOR2I, "position" });
				type.properties.push_back({ Variant::VECTOR2I, "size" });
				type.properties.push_back({ Variant::VECTOR2I, "end" });
				type.methods.push_back(_make_method("get_center", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2I, {  }));
				type.methods.push_back(_make_method("get_area", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("has_area", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("has_point", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR2I, "point" } }));
				type.methods.push_back(_make_method("intersects", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::RECT2I, "b" } }));
				type.methods.push_back(_make_method("encloses", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::RECT2I, "b" } }));
				type.methods.push_back(_make_method("intersection", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::RECT2I, "b" } }));
				type.methods.push_back(_make_method("merge", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::RECT2I, "b" } }));
				type.methods.push_back(_make_method("expand", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::VECTOR2I, "to" } }));
				type.methods.push_back(_make_method("grow", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::INT, "amount" } }));
				type.methods.push_back(_make_method("grow_side", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::INT, "side" }, { Variant::INT, "amount" } }));
				type.methods.push_back(_make_method("grow_individual", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, { { Variant::INT, "left" }, { Variant::INT, "top" }, { Variant::INT, "right" }, { Variant::INT, "bottom" } }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::RECT2I, {  }));
				ExtensionDB::_singleton->_builtin_types["Rect2i"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::RECT2I] = "Rect2i";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Rect2i");
			}
			{
				BuiltInType type;
				type.name = "Vector3";
				type.type = Variant::VECTOR3;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR3, "Vector3", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR3, "Vector3", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR3, "Vector3", Variant::NIL, "", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR3, "Vector3", Variant::NIL, "", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR3, "Vector3", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::INT, "int", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3, "Vector3", Variant::INT, "int", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::FLOAT, "float", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3, "Vector3", Variant::FLOAT, "float", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3, "Vector3", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::QUATERNION, "Quaternion", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::BASIS, "Basis", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3, "Vector3", Variant::TRANSFORM3D, "Transform3D", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR3, "Vector3", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR3, "Vector3", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR3, "Vector3", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "x"), PropertyInfo(Variant::FLOAT, "y"), PropertyInfo(Variant::FLOAT, "z") } });
				type.properties.push_back({ Variant::FLOAT, "x" });
				type.properties.push_back({ Variant::FLOAT, "y" });
				type.properties.push_back({ Variant::FLOAT, "z" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "AXIS_Z", Variant::INT, 2 });
				type.constants.push_back({ "ZERO", Variant::VECTOR3, Vector3(0, 0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR3, Vector3(1, 1, 1) });
				type.constants.push_back({ "INF", Variant::VECTOR3, Vector3(INFINITY, INFINITY, INFINITY) });
				type.constants.push_back({ "LEFT", Variant::VECTOR3, Vector3(-1, 0, 0) });
				type.constants.push_back({ "RIGHT", Variant::VECTOR3, Vector3(1, 0, 0) });
				type.constants.push_back({ "UP", Variant::VECTOR3, Vector3(0, 1, 0) });
				type.constants.push_back({ "DOWN", Variant::VECTOR3, Vector3(0, -1, 0) });
				type.constants.push_back({ "FORWARD", Variant::VECTOR3, Vector3(0, 0, -1) });
				type.constants.push_back({ "BACK", Variant::VECTOR3, Vector3(0, 0, 1) });
				type.constants.push_back({ "MODEL_LEFT", Variant::VECTOR3, Vector3(1, 0, 0) });
				type.constants.push_back({ "MODEL_RIGHT", Variant::VECTOR3, Vector3(-1, 0, 0) });
				type.constants.push_back({ "MODEL_TOP", Variant::VECTOR3, Vector3(0, 1, 0) });
				type.constants.push_back({ "MODEL_BOTTOM", Variant::VECTOR3, Vector3(0, -1, 0) });
				type.constants.push_back({ "MODEL_FRONT", Variant::VECTOR3, Vector3(0, 0, 1) });
				type.constants.push_back({ "MODEL_REAR", Variant::VECTOR3, Vector3(0, 0, -1) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 }, { "AXIS_Z", "", 2 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("angle_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "to" } }));
				type.methods.push_back(_make_method("signed_angle_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "to" }, { Variant::VECTOR3, "axis" } }));
				type.methods.push_back(_make_method("direction_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "to" } }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("limit_length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "length" } }));
				type.methods.push_back(_make_method("normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("is_normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR3, "to" } }));
				type.methods.push_back(_make_method("is_zero_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "min" }, { Variant::VECTOR3, "max" } }));
				type.methods.push_back(_make_method("clampf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "min" }, { Variant::FLOAT, "max" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "step" } }));
				type.methods.push_back(_make_method("snappedf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "step" } }));
				type.methods.push_back(_make_method("rotated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "axis" }, { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("lerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("slerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "b" }, { Variant::VECTOR3, "pre_a" }, { Variant::VECTOR3, "post_b" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate_in_time", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "b" }, { Variant::VECTOR3, "pre_a" }, { Variant::VECTOR3, "post_b" }, { Variant::FLOAT, "weight" }, { Variant::FLOAT, "b_t" }, { Variant::FLOAT, "pre_a_t" }, { Variant::FLOAT, "post_b_t" } }));
				type.methods.push_back(_make_method("bezier_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "control_1" }, { Variant::VECTOR3, "control_2" }, { Variant::VECTOR3, "end" }, { Variant::FLOAT, "t" } }));
				type.methods.push_back(_make_method("bezier_derivative", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "control_1" }, { Variant::VECTOR3, "control_2" }, { Variant::VECTOR3, "end" }, { Variant::FLOAT, "t" } }));
				type.methods.push_back(_make_method("move_toward", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "to" }, { Variant::FLOAT, "delta" } }));
				type.methods.push_back(_make_method("dot", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("cross", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("outer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("floor", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("ceil", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("round", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("posmod", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "mod" } }));
				type.methods.push_back(_make_method("posmodv", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "modv" } }));
				type.methods.push_back(_make_method("project", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "b" } }));
				type.methods.push_back(_make_method("slide", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "n" } }));
				type.methods.push_back(_make_method("bounce", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "n" } }));
				type.methods.push_back(_make_method("reflect", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "n" } }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("octahedron_encode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("minf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("maxf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::FLOAT, "with" } }));
				type.methods.push_back(_make_method("octahedron_decode", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::VECTOR3, { { Variant::VECTOR2, "uv" } }));
				ExtensionDB::_singleton->_builtin_types["Vector3"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR3] = "Vector3";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector3");
			}
			{
				BuiltInType type;
				type.name = "Vector3i";
				type.type = Variant::VECTOR3I;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR3I, "Vector3i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR3I, "Vector3i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR3I, "Vector3i", Variant::NIL, "", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR3I, "Vector3i", Variant::NIL, "", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR3I, "Vector3i", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3I, "Vector3i", Variant::INT, "int", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3I, "Vector3i", Variant::INT, "int", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR3I, "Vector3i", Variant::INT, "int", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3I, "Vector3i", Variant::FLOAT, "float", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3I, "Vector3i", Variant::FLOAT, "float", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I, "Vector3i", Variant::VECTOR3I });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR3I, "Vector3i", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR3I, "Vector3i", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "x"), PropertyInfo(Variant::INT, "y"), PropertyInfo(Variant::INT, "z") } });
				type.properties.push_back({ Variant::INT, "x" });
				type.properties.push_back({ Variant::INT, "y" });
				type.properties.push_back({ Variant::INT, "z" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "AXIS_Z", Variant::INT, 2 });
				type.constants.push_back({ "ZERO", Variant::VECTOR3I, Vector3i(0, 0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR3I, Vector3i(1, 1, 1) });
				type.constants.push_back({ "MIN", Variant::VECTOR3I, Vector3i(-2147483648, -2147483648, -2147483648) });
				type.constants.push_back({ "MAX", Variant::VECTOR3I, Vector3i(2147483647, 2147483647, 2147483647) });
				type.constants.push_back({ "LEFT", Variant::VECTOR3I, Vector3i(-1, 0, 0) });
				type.constants.push_back({ "RIGHT", Variant::VECTOR3I, Vector3i(1, 0, 0) });
				type.constants.push_back({ "UP", Variant::VECTOR3I, Vector3i(0, 1, 0) });
				type.constants.push_back({ "DOWN", Variant::VECTOR3I, Vector3i(0, -1, 0) });
				type.constants.push_back({ "FORWARD", Variant::VECTOR3I, Vector3i(0, 0, -1) });
				type.constants.push_back({ "BACK", Variant::VECTOR3I, Vector3i(0, 0, 1) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 }, { "AXIS_Z", "", 2 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3I, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR3I, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, {  }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, {  }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::VECTOR3I, "min" }, { Variant::VECTOR3I, "max" } }));
				type.methods.push_back(_make_method("clampi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::INT, "min" }, { Variant::INT, "max" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::VECTOR3I, "step" } }));
				type.methods.push_back(_make_method("snappedi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::INT, "step" } }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::VECTOR3I, "with" } }));
				type.methods.push_back(_make_method("mini", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::INT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::VECTOR3I, "with" } }));
				type.methods.push_back(_make_method("maxi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3I, { { Variant::INT, "with" } }));
				ExtensionDB::_singleton->_builtin_types["Vector3i"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR3I] = "Vector3i";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector3i");
			}
			{
				BuiltInType type;
				type.name = "Transform2D";
				type.type = Variant::TRANSFORM2D;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::VECTOR2;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::TRANSFORM2D, "Transform2D", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::TRANSFORM2D, "Transform2D", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::TRANSFORM2D, "Transform2D", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::INT, "int", Variant::TRANSFORM2D });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::TRANSFORM2D, "Transform2D", Variant::INT, "int", Variant::TRANSFORM2D });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::FLOAT, "float", Variant::TRANSFORM2D });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::TRANSFORM2D, "Transform2D", Variant::FLOAT, "float", Variant::TRANSFORM2D });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::VECTOR2, "Vector2", Variant::VECTOR2 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::RECT2, "Rect2", Variant::RECT2 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::TRANSFORM2D, "Transform2D", Variant::TRANSFORM2D, "Transform2D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::TRANSFORM2D, "Transform2D", Variant::TRANSFORM2D, "Transform2D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::TRANSFORM2D, "Transform2D", Variant::TRANSFORM2D });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::TRANSFORM2D, "Transform2D", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::TRANSFORM2D, "Transform2D", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM2D, "Transform2D", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::PACKED_VECTOR2_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::TRANSFORM2D, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "rotation"), PropertyInfo(Variant::VECTOR2, "position") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "rotation"), PropertyInfo(Variant::VECTOR2, "scale"), PropertyInfo(Variant::FLOAT, "skew"), PropertyInfo(Variant::VECTOR2, "position") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR2, "x_axis"), PropertyInfo(Variant::VECTOR2, "y_axis"), PropertyInfo(Variant::VECTOR2, "origin") } });
				type.properties.push_back({ Variant::VECTOR2, "x" });
				type.properties.push_back({ Variant::VECTOR2, "y" });
				type.properties.push_back({ Variant::VECTOR2, "origin" });
				type.constants.push_back({ "IDENTITY", Variant::TRANSFORM2D, Transform2D(1, 0, 0, 1, 0, 0) });
				type.constants.push_back({ "FLIP_X", Variant::TRANSFORM2D, Transform2D(-1, 0, 0, 1, 0, 0) });
				type.constants.push_back({ "FLIP_Y", Variant::TRANSFORM2D, Transform2D(1, 0, 0, -1, 0, 0) });
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, {  }));
				type.methods.push_back(_make_method("affine_inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, {  }));
				type.methods.push_back(_make_method("get_rotation", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_origin", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("get_scale", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("get_skew", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("orthonormalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, {  }));
				type.methods.push_back(_make_method("rotated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("rotated_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("scaled", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::VECTOR2, "scale" } }));
				type.methods.push_back(_make_method("scaled_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::VECTOR2, "scale" } }));
				type.methods.push_back(_make_method("translated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::VECTOR2, "offset" } }));
				type.methods.push_back(_make_method("translated_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::VECTOR2, "offset" } }));
				type.methods.push_back(_make_method("determinant", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("basis_xform", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "v" } }));
				type.methods.push_back(_make_method("basis_xform_inv", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, { { Variant::VECTOR2, "v" } }));
				type.methods.push_back(_make_method("interpolate_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::TRANSFORM2D, "xform" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("is_conformal", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::TRANSFORM2D, "xform" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("looking_at", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM2D, { { Variant::VECTOR2, "target" } }));
				ExtensionDB::_singleton->_builtin_types["Transform2D"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::TRANSFORM2D] = "Transform2D";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Transform2D");
			}
			{
				BuiltInType type;
				type.name = "Vector4";
				type.type = Variant::VECTOR4;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR4, "Vector4", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR4, "Vector4", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR4, "Vector4", Variant::NIL, "", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR4, "Vector4", Variant::NIL, "", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR4, "Vector4", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4, "Vector4", Variant::INT, "int", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4, "Vector4", Variant::INT, "int", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4, "Vector4", Variant::FLOAT, "float", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4, "Vector4", Variant::FLOAT, "float", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4, "Vector4", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4, "Vector4", Variant::PROJECTION, "Projection", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR4, "Vector4", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR4, "Vector4", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR4, "Vector4", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR4, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR4I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "x"), PropertyInfo(Variant::FLOAT, "y"), PropertyInfo(Variant::FLOAT, "z"), PropertyInfo(Variant::FLOAT, "w") } });
				type.properties.push_back({ Variant::FLOAT, "x" });
				type.properties.push_back({ Variant::FLOAT, "y" });
				type.properties.push_back({ Variant::FLOAT, "z" });
				type.properties.push_back({ Variant::FLOAT, "w" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "AXIS_Z", Variant::INT, 2 });
				type.constants.push_back({ "AXIS_W", Variant::INT, 3 });
				type.constants.push_back({ "ZERO", Variant::VECTOR4, Vector4(0, 0, 0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR4, Vector4(1, 1, 1, 1) });
				type.constants.push_back({ "INF", Variant::VECTOR4, Vector4(INFINITY, INFINITY, INFINITY, INFINITY) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 }, { "AXIS_Z", "", 2 }, { "AXIS_W", "", 3 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("floor", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("ceil", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("round", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("lerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "b" }, { Variant::VECTOR4, "pre_a" }, { Variant::VECTOR4, "post_b" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("cubic_interpolate_in_time", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "b" }, { Variant::VECTOR4, "pre_a" }, { Variant::VECTOR4, "post_b" }, { Variant::FLOAT, "weight" }, { Variant::FLOAT, "b_t" }, { Variant::FLOAT, "pre_a_t" }, { Variant::FLOAT, "post_b_t" } }));
				type.methods.push_back(_make_method("posmod", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::FLOAT, "mod" } }));
				type.methods.push_back(_make_method("posmodv", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "modv" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "step" } }));
				type.methods.push_back(_make_method("snappedf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::FLOAT, "step" } }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "min" }, { Variant::VECTOR4, "max" } }));
				type.methods.push_back(_make_method("clampf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::FLOAT, "min" }, { Variant::FLOAT, "max" } }));
				type.methods.push_back(_make_method("normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("is_normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("direction_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "to" } }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR4, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR4, "to" } }));
				type.methods.push_back(_make_method("dot", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR4, "with" } }));
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR4, "to" } }));
				type.methods.push_back(_make_method("is_zero_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "with" } }));
				type.methods.push_back(_make_method("minf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::FLOAT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::VECTOR4, "with" } }));
				type.methods.push_back(_make_method("maxf", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4, { { Variant::FLOAT, "with" } }));
				ExtensionDB::_singleton->_builtin_types["Vector4"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR4] = "Vector4";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector4");
			}
			{
				BuiltInType type;
				type.name = "Vector4i";
				type.type = Variant::VECTOR4I;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR4I, "Vector4i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR4I, "Vector4i", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::VECTOR4I, "Vector4i", Variant::NIL, "", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::VECTOR4I, "Vector4i", Variant::NIL, "", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::VECTOR4I, "Vector4i", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4I, "Vector4i", Variant::INT, "int", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4I, "Vector4i", Variant::INT, "int", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR4I, "Vector4i", Variant::INT, "int", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4I, "Vector4i", Variant::FLOAT, "float", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4I, "Vector4i", Variant::FLOAT, "float", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I, "Vector4i", Variant::VECTOR4I });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR4I, "Vector4i", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::VECTOR4I, "Vector4i", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR4I, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR4, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::INT, "x"), PropertyInfo(Variant::INT, "y"), PropertyInfo(Variant::INT, "z"), PropertyInfo(Variant::INT, "w") } });
				type.properties.push_back({ Variant::INT, "x" });
				type.properties.push_back({ Variant::INT, "y" });
				type.properties.push_back({ Variant::INT, "z" });
				type.properties.push_back({ Variant::INT, "w" });
				type.constants.push_back({ "AXIS_X", Variant::INT, 0 });
				type.constants.push_back({ "AXIS_Y", Variant::INT, 1 });
				type.constants.push_back({ "AXIS_Z", Variant::INT, 2 });
				type.constants.push_back({ "AXIS_W", Variant::INT, 3 });
				type.constants.push_back({ "ZERO", Variant::VECTOR4I, Vector4i(0, 0, 0, 0) });
				type.constants.push_back({ "ONE", Variant::VECTOR4I, Vector4i(1, 1, 1, 1) });
				type.constants.push_back({ "MIN", Variant::VECTOR4I, Vector4i(-2147483648, -2147483648, -2147483648, -2147483648) });
				type.constants.push_back({ "MAX", Variant::VECTOR4I, Vector4i(2147483647, 2147483647, 2147483647, 2147483647) });
				type.enums.push_back({ "Axis", false, { { "AXIS_X", "", 0 }, { "AXIS_Y", "", 1 }, { "AXIS_Z", "", 2 }, { "AXIS_W", "", 3 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("min_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("max_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("sign", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, {  }));
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, {  }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::VECTOR4I, "min" }, { Variant::VECTOR4I, "max" } }));
				type.methods.push_back(_make_method("clampi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::INT, "min" }, { Variant::INT, "max" } }));
				type.methods.push_back(_make_method("snapped", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::VECTOR4I, "step" } }));
				type.methods.push_back(_make_method("snappedi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::INT, "step" } }));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::VECTOR4I, "with" } }));
				type.methods.push_back(_make_method("mini", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::INT, "with" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::VECTOR4I, "with" } }));
				type.methods.push_back(_make_method("maxi", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR4I, { { Variant::INT, "with" } }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR4I, "to" } }));
				type.methods.push_back(_make_method("distance_squared_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR4I, "to" } }));
				ExtensionDB::_singleton->_builtin_types["Vector4i"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::VECTOR4I] = "Vector4i";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Vector4i");
			}
			{
				BuiltInType type;
				type.name = "Plane";
				type.type = Variant::PLANE;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PLANE, "Plane", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PLANE, "Plane", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::PLANE, "Plane", Variant::NIL, "", Variant::PLANE });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::PLANE, "Plane", Variant::NIL, "", Variant::PLANE });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PLANE, "Plane", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PLANE, "Plane", Variant::PLANE, "Plane", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PLANE, "Plane", Variant::PLANE, "Plane", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::PLANE, "Plane", Variant::TRANSFORM3D, "Transform3D", Variant::PLANE });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PLANE, "Plane", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PLANE, "Plane", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PLANE, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "normal") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "normal"), PropertyInfo(Variant::FLOAT, "d") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "normal"), PropertyInfo(Variant::VECTOR3, "point") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "point1"), PropertyInfo(Variant::VECTOR3, "point2"), PropertyInfo(Variant::VECTOR3, "point3") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "a"), PropertyInfo(Variant::FLOAT, "b"), PropertyInfo(Variant::FLOAT, "c"), PropertyInfo(Variant::FLOAT, "d") } });
				type.properties.push_back({ Variant::FLOAT, "x" });
				type.properties.push_back({ Variant::FLOAT, "y" });
				type.properties.push_back({ Variant::FLOAT, "z" });
				type.properties.push_back({ Variant::FLOAT, "d" });
				type.properties.push_back({ Variant::VECTOR3, "normal" });
				type.constants.push_back({ "PLANE_YZ", Variant::PLANE, Plane(1, 0, 0, 0) });
				type.constants.push_back({ "PLANE_XZ", Variant::PLANE, Plane(0, 1, 0, 0) });
				type.constants.push_back({ "PLANE_XY", Variant::PLANE, Plane(0, 0, 1, 0) });
				type.methods.push_back(_make_method("normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PLANE, {  }));
				type.methods.push_back(_make_method("get_center", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::PLANE, "to_plane" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_point_over", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR3, "point" } }));
				type.methods.push_back(_make_method("distance_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "point" } }));
				type.methods.push_back(_make_method("has_point", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR3, "point" }, { Variant::FLOAT, "tolerance" } }));
				type.methods.push_back(_make_method("project", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "point" } }));
				type.methods.push_back(_make_method("intersect_3", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::PLANE, "b" }, { Variant::PLANE, "c" } }, true));
				type.methods.push_back(_make_method("intersects_ray", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::VECTOR3, "from" }, { Variant::VECTOR3, "dir" } }, true));
				type.methods.push_back(_make_method("intersects_segment", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::VECTOR3, "from" }, { Variant::VECTOR3, "to" } }, true));
				ExtensionDB::_singleton->_builtin_types["Plane"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PLANE] = "Plane";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Plane");
			}
			{
				BuiltInType type;
				type.name = "Quaternion";
				type.type = Variant::QUATERNION;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::QUATERNION, "Quaternion", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::QUATERNION, "Quaternion", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::QUATERNION, "Quaternion", Variant::NIL, "", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::QUATERNION, "Quaternion", Variant::NIL, "", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::QUATERNION, "Quaternion", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::QUATERNION, "Quaternion", Variant::INT, "int", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::QUATERNION, "Quaternion", Variant::INT, "int", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::QUATERNION, "Quaternion", Variant::FLOAT, "float", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::QUATERNION, "Quaternion", Variant::FLOAT, "float", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::QUATERNION, "Quaternion", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::QUATERNION, "Quaternion", Variant::QUATERNION, "Quaternion", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::QUATERNION, "Quaternion", Variant::QUATERNION, "Quaternion", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::QUATERNION, "Quaternion", Variant::QUATERNION, "Quaternion", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::QUATERNION, "Quaternion", Variant::QUATERNION, "Quaternion", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::QUATERNION, "Quaternion", Variant::QUATERNION, "Quaternion", Variant::QUATERNION });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::QUATERNION, "Quaternion", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::QUATERNION, "Quaternion", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::QUATERNION, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::BASIS, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "axis"), PropertyInfo(Variant::FLOAT, "angle") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "arc_from"), PropertyInfo(Variant::VECTOR3, "arc_to") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "x"), PropertyInfo(Variant::FLOAT, "y"), PropertyInfo(Variant::FLOAT, "z"), PropertyInfo(Variant::FLOAT, "w") } });
				type.properties.push_back({ Variant::FLOAT, "x" });
				type.properties.push_back({ Variant::FLOAT, "y" });
				type.properties.push_back({ Variant::FLOAT, "z" });
				type.properties.push_back({ Variant::FLOAT, "w" });
				type.constants.push_back({ "IDENTITY", Variant::QUATERNION, Quaternion(0, 0, 0, 1) });
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("length_squared", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, {  }));
				type.methods.push_back(_make_method("is_normalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::QUATERNION, "to" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, {  }));
				type.methods.push_back(_make_method("log", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, {  }));
				type.methods.push_back(_make_method("exp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, {  }));
				type.methods.push_back(_make_method("angle_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::QUATERNION, "to" } }));
				type.methods.push_back(_make_method("dot", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::QUATERNION, "with" } }));
				type.methods.push_back(_make_method("slerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, { { Variant::QUATERNION, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("slerpni", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, { { Variant::QUATERNION, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("spherical_cubic_interpolate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, { { Variant::QUATERNION, "b" }, { Variant::QUATERNION, "pre_a" }, { Variant::QUATERNION, "post_b" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("spherical_cubic_interpolate_in_time", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, { { Variant::QUATERNION, "b" }, { Variant::QUATERNION, "pre_a" }, { Variant::QUATERNION, "post_b" }, { Variant::FLOAT, "weight" }, { Variant::FLOAT, "b_t" }, { Variant::FLOAT, "pre_a_t" }, { Variant::FLOAT, "post_b_t" } }));
				type.methods.push_back(_make_method("get_euler", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::INT, "order" } }));
				type.methods.push_back(_make_method("from_euler", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::QUATERNION, { { Variant::VECTOR3, "euler" } }));
				type.methods.push_back(_make_method("get_axis", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("get_angle", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				ExtensionDB::_singleton->_builtin_types["Quaternion"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::QUATERNION] = "Quaternion";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Quaternion");
			}
			{
				BuiltInType type;
				type.name = "AABB";
				type.type = Variant::AABB;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::AABB, "AABB", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::AABB, "AABB", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::AABB, "AABB", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::AABB, "AABB", Variant::AABB, "AABB", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::AABB, "AABB", Variant::AABB, "AABB", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::AABB, "AABB", Variant::TRANSFORM3D, "Transform3D", Variant::AABB });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::AABB, "AABB", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::AABB, "AABB", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::AABB, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "position"), PropertyInfo(Variant::VECTOR3, "size") } });
				type.properties.push_back({ Variant::VECTOR3, "position" });
				type.properties.push_back({ Variant::VECTOR3, "size" });
				type.properties.push_back({ Variant::VECTOR3, "end" });
				type.methods.push_back(_make_method("abs", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::AABB, {  }));
				type.methods.push_back(_make_method("get_center", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("get_volume", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("has_volume", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("has_surface", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("has_point", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR3, "point" } }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::AABB, "aabb" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("intersects", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::AABB, "with" } }));
				type.methods.push_back(_make_method("encloses", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::AABB, "with" } }));
				type.methods.push_back(_make_method("intersects_plane", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::PLANE, "plane" } }));
				type.methods.push_back(_make_method("intersection", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::AABB, { { Variant::AABB, "with" } }));
				type.methods.push_back(_make_method("merge", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::AABB, { { Variant::AABB, "with" } }));
				type.methods.push_back(_make_method("expand", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::AABB, { { Variant::VECTOR3, "to_point" } }));
				type.methods.push_back(_make_method("grow", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::AABB, { { Variant::FLOAT, "by" } }));
				type.methods.push_back(_make_method("get_support", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::VECTOR3, "dir" } }));
				type.methods.push_back(_make_method("get_longest_axis", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("get_longest_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_longest_axis_size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_shortest_axis", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("get_shortest_axis_index", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_shortest_axis_size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_endpoint", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::INT, "idx" } }));
				type.methods.push_back(_make_method("intersects_segment", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::VECTOR3, "from" }, { Variant::VECTOR3, "to" } }, true));
				type.methods.push_back(_make_method("intersects_ray", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::VECTOR3, "from" }, { Variant::VECTOR3, "dir" } }, true));
				ExtensionDB::_singleton->_builtin_types["AABB"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::AABB] = "AABB";
				ExtensionDB::_singleton->_builtin_type_names.push_back("AABB");
			}
			{
				BuiltInType type;
				type.name = "Basis";
				type.type = Variant::BASIS;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::VECTOR3;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::BASIS, "Basis", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::BASIS, "Basis", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::BASIS, "Basis", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::BASIS, "Basis", Variant::INT, "int", Variant::BASIS });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::BASIS, "Basis", Variant::INT, "int", Variant::BASIS });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::BASIS, "Basis", Variant::FLOAT, "float", Variant::BASIS });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::BASIS, "Basis", Variant::FLOAT, "float", Variant::BASIS });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::BASIS, "Basis", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::BASIS, "Basis", Variant::BASIS, "Basis", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::BASIS, "Basis", Variant::BASIS, "Basis", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::BASIS, "Basis", Variant::BASIS, "Basis", Variant::BASIS });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::BASIS, "Basis", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::BASIS, "Basis", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::BASIS, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::QUATERNION, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "axis"), PropertyInfo(Variant::FLOAT, "angle") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "x_axis"), PropertyInfo(Variant::VECTOR3, "y_axis"), PropertyInfo(Variant::VECTOR3, "z_axis") } });
				type.properties.push_back({ Variant::VECTOR3, "x" });
				type.properties.push_back({ Variant::VECTOR3, "y" });
				type.properties.push_back({ Variant::VECTOR3, "z" });
				type.constants.push_back({ "IDENTITY", Variant::BASIS, Basis(1, 0, 0, 0, 1, 0, 0, 0, 1) });
				type.constants.push_back({ "FLIP_X", Variant::BASIS, Basis(-1, 0, 0, 0, 1, 0, 0, 0, 1) });
				type.constants.push_back({ "FLIP_Y", Variant::BASIS, Basis(1, 0, 0, 0, -1, 0, 0, 0, 1) });
				type.constants.push_back({ "FLIP_Z", Variant::BASIS, Basis(1, 0, 0, 0, 1, 0, 0, 0, -1) });
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, {  }));
				type.methods.push_back(_make_method("transposed", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, {  }));
				type.methods.push_back(_make_method("orthonormalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, {  }));
				type.methods.push_back(_make_method("determinant", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("rotated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, { { Variant::VECTOR3, "axis" }, { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("scaled", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, { { Variant::VECTOR3, "scale" } }));
				type.methods.push_back(_make_method("get_scale", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, {  }));
				type.methods.push_back(_make_method("get_euler", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR3, { { Variant::INT, "order" } }));
				type.methods.push_back(_make_method("tdotx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("tdoty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("tdotz", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::VECTOR3, "with" } }));
				type.methods.push_back(_make_method("slerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BASIS, { { Variant::BASIS, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("is_conformal", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::BASIS, "b" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_rotation_quaternion", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::QUATERNION, {  }));
				type.methods.push_back(_make_method("looking_at", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::BASIS, { { Variant::VECTOR3, "target" }, { Variant::VECTOR3, "up" }, { Variant::BOOL, "use_model_front" } }));
				type.methods.push_back(_make_method("from_scale", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::BASIS, { { Variant::VECTOR3, "scale" } }));
				type.methods.push_back(_make_method("from_euler", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::BASIS, { { Variant::VECTOR3, "euler" }, { Variant::INT, "order" } }));
				ExtensionDB::_singleton->_builtin_types["Basis"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::BASIS] = "Basis";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Basis");
			}
			{
				BuiltInType type;
				type.name = "Transform3D";
				type.type = Variant::TRANSFORM3D;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::TRANSFORM3D, "Transform3D", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::TRANSFORM3D, "Transform3D", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::TRANSFORM3D, "Transform3D", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::INT, "int", Variant::TRANSFORM3D });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::TRANSFORM3D, "Transform3D", Variant::INT, "int", Variant::TRANSFORM3D });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::FLOAT, "float", Variant::TRANSFORM3D });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::TRANSFORM3D, "Transform3D", Variant::FLOAT, "float", Variant::TRANSFORM3D });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::VECTOR3, "Vector3", Variant::VECTOR3 });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::PLANE, "Plane", Variant::PLANE });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::AABB, "AABB", Variant::AABB });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::TRANSFORM3D, "Transform3D", Variant::TRANSFORM3D, "Transform3D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::TRANSFORM3D, "Transform3D", Variant::TRANSFORM3D, "Transform3D", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::TRANSFORM3D, "Transform3D", Variant::TRANSFORM3D });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::TRANSFORM3D, "Transform3D", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::TRANSFORM3D, "Transform3D", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::TRANSFORM3D, "Transform3D", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::PACKED_VECTOR3_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::TRANSFORM3D, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::BASIS, "basis"), PropertyInfo(Variant::VECTOR3, "origin") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR3, "x_axis"), PropertyInfo(Variant::VECTOR3, "y_axis"), PropertyInfo(Variant::VECTOR3, "z_axis"), PropertyInfo(Variant::VECTOR3, "origin") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PROJECTION, "from") } });
				type.properties.push_back({ Variant::BASIS, "basis" });
				type.properties.push_back({ Variant::VECTOR3, "origin" });
				type.constants.push_back({ "IDENTITY", Variant::TRANSFORM3D, Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0) });
				type.constants.push_back({ "FLIP_X", Variant::TRANSFORM3D, Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0) });
				type.constants.push_back({ "FLIP_Y", Variant::TRANSFORM3D, Transform3D(1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 0, 0) });
				type.constants.push_back({ "FLIP_Z", Variant::TRANSFORM3D, Transform3D(1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0) });
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, {  }));
				type.methods.push_back(_make_method("affine_inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, {  }));
				type.methods.push_back(_make_method("orthonormalized", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, {  }));
				type.methods.push_back(_make_method("rotated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "axis" }, { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("rotated_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "axis" }, { Variant::FLOAT, "angle" } }));
				type.methods.push_back(_make_method("scaled", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "scale" } }));
				type.methods.push_back(_make_method("scaled_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "scale" } }));
				type.methods.push_back(_make_method("translated", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "offset" } }));
				type.methods.push_back(_make_method("translated_local", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "offset" } }));
				type.methods.push_back(_make_method("looking_at", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::VECTOR3, "target" }, { Variant::VECTOR3, "up" }, { Variant::BOOL, "use_model_front" } }));
				type.methods.push_back(_make_method("interpolate_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::TRANSFORM3D, { { Variant::TRANSFORM3D, "xform" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::TRANSFORM3D, "xform" } }));
				type.methods.push_back(_make_method("is_finite", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				ExtensionDB::_singleton->_builtin_types["Transform3D"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::TRANSFORM3D] = "Transform3D";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Transform3D");
			}
			{
				BuiltInType type;
				type.name = "Projection";
				type.type = Variant::PROJECTION;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::VECTOR4;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PROJECTION, "Projection", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PROJECTION, "Projection", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PROJECTION, "Projection", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::PROJECTION, "Projection", Variant::VECTOR4, "Vector4", Variant::VECTOR4 });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PROJECTION, "Projection", Variant::PROJECTION, "Projection", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PROJECTION, "Projection", Variant::PROJECTION, "Projection", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::PROJECTION, "Projection", Variant::PROJECTION, "Projection", Variant::PROJECTION });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PROJECTION, "Projection", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PROJECTION, "Projection", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PROJECTION, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::TRANSFORM3D, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::VECTOR4, "x_axis"), PropertyInfo(Variant::VECTOR4, "y_axis"), PropertyInfo(Variant::VECTOR4, "z_axis"), PropertyInfo(Variant::VECTOR4, "w_axis") } });
				type.properties.push_back({ Variant::VECTOR4, "x" });
				type.properties.push_back({ Variant::VECTOR4, "y" });
				type.properties.push_back({ Variant::VECTOR4, "z" });
				type.properties.push_back({ Variant::VECTOR4, "w" });
				type.constants.push_back({ "PLANE_NEAR", Variant::INT, 0 });
				type.constants.push_back({ "PLANE_FAR", Variant::INT, 1 });
				type.constants.push_back({ "PLANE_LEFT", Variant::INT, 2 });
				type.constants.push_back({ "PLANE_TOP", Variant::INT, 3 });
				type.constants.push_back({ "PLANE_RIGHT", Variant::INT, 4 });
				type.constants.push_back({ "PLANE_BOTTOM", Variant::INT, 5 });
				type.constants.push_back({ "IDENTITY", Variant::PROJECTION, Projection(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1)) });
				type.constants.push_back({ "ZERO", Variant::PROJECTION, Projection(Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0)) });
				type.enums.push_back({ "Planes", false, { { "PLANE_NEAR", "", 0 }, { "PLANE_FAR", "", 1 }, { "PLANE_LEFT", "", 2 }, { "PLANE_TOP", "", 3 }, { "PLANE_RIGHT", "", 4 }, { "PLANE_BOTTOM", "", 5 } } });
				_sanitize_enums(type.enums);
				type.methods.push_back(_make_method("create_depth_correction", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::BOOL, "flip_y" } }));
				type.methods.push_back(_make_method("create_light_atlas_rect", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::RECT2, "rect" } }));
				type.methods.push_back(_make_method("create_perspective", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "fovy" }, { Variant::FLOAT, "aspect" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" }, { Variant::BOOL, "flip_fov" } }));
				type.methods.push_back(_make_method("create_perspective_hmd", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "fovy" }, { Variant::FLOAT, "aspect" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" }, { Variant::BOOL, "flip_fov" }, { Variant::INT, "eye" }, { Variant::FLOAT, "intraocular_dist" }, { Variant::FLOAT, "convergence_dist" } }));
				type.methods.push_back(_make_method("create_for_hmd", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::INT, "eye" }, { Variant::FLOAT, "aspect" }, { Variant::FLOAT, "intraocular_dist" }, { Variant::FLOAT, "display_width" }, { Variant::FLOAT, "display_to_lens" }, { Variant::FLOAT, "oversample" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" } }));
				type.methods.push_back(_make_method("create_orthogonal", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "left" }, { Variant::FLOAT, "right" }, { Variant::FLOAT, "bottom" }, { Variant::FLOAT, "top" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" } }));
				type.methods.push_back(_make_method("create_orthogonal_aspect", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "size" }, { Variant::FLOAT, "aspect" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" }, { Variant::BOOL, "flip_fov" } }));
				type.methods.push_back(_make_method("create_frustum", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "left" }, { Variant::FLOAT, "right" }, { Variant::FLOAT, "bottom" }, { Variant::FLOAT, "top" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" } }));
				type.methods.push_back(_make_method("create_frustum_aspect", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::FLOAT, "size" }, { Variant::FLOAT, "aspect" }, { Variant::VECTOR2, "offset" }, { Variant::FLOAT, "z_near" }, { Variant::FLOAT, "z_far" }, { Variant::BOOL, "flip_fov" } }));
				type.methods.push_back(_make_method("create_fit_aabb", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::PROJECTION, { { Variant::AABB, "aabb" } }));
				type.methods.push_back(_make_method("determinant", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("perspective_znear_adjusted", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PROJECTION, { { Variant::FLOAT, "new_znear" } }));
				type.methods.push_back(_make_method("get_projection_plane", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PLANE, { { Variant::INT, "plane" } }));
				type.methods.push_back(_make_method("flipped_y", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PROJECTION, {  }));
				type.methods.push_back(_make_method("jitter_offseted", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PROJECTION, { { Variant::VECTOR2, "offset" } }));
				type.methods.push_back(_make_method("get_fovy", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::FLOAT, { { Variant::FLOAT, "fovx" }, { Variant::FLOAT, "aspect" } }));
				type.methods.push_back(_make_method("get_z_far", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_z_near", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_aspect", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("get_fov", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("is_orthogonal", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_viewport_half_extents", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("get_far_plane_half_extents", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::VECTOR2, {  }));
				type.methods.push_back(_make_method("inverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PROJECTION, {  }));
				type.methods.push_back(_make_method("get_pixels_per_meter", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "for_pixel_width" } }));
				type.methods.push_back(_make_method("get_lod_multiplier", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				ExtensionDB::_singleton->_builtin_types["Projection"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PROJECTION] = "Projection";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Projection");
			}
			{
				BuiltInType type;
				type.name = "Color";
				type.type = Variant::COLOR;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::COLOR, "Color", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::COLOR, "Color", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NEGATE, "unary-", "Unary- or Negate", Variant::COLOR, "Color", Variant::NIL, "", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_POSITIVE, "unary+", "Unary+", Variant::COLOR, "Color", Variant::NIL, "", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::COLOR, "Color", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::COLOR, "Color", Variant::INT, "int", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::COLOR, "Color", Variant::INT, "int", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::COLOR, "Color", Variant::FLOAT, "float", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::COLOR, "Color", Variant::FLOAT, "float", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_SUBTRACT, "-", "Subtract", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_DIVIDE, "/", "Division", Variant::COLOR, "Color", Variant::COLOR, "Color", Variant::COLOR });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::COLOR, "Color", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::COLOR, "Color", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::COLOR, "Color", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::COLOR, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::COLOR, "from"), PropertyInfo(Variant::FLOAT, "alpha") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "r"), PropertyInfo(Variant::FLOAT, "g"), PropertyInfo(Variant::FLOAT, "b") } });
				type.constructors.push_back({ { PropertyInfo(Variant::FLOAT, "r"), PropertyInfo(Variant::FLOAT, "g"), PropertyInfo(Variant::FLOAT, "b"), PropertyInfo(Variant::FLOAT, "a") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "code") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "code"), PropertyInfo(Variant::FLOAT, "alpha") } });
				type.properties.push_back({ Variant::FLOAT, "r" });
				type.properties.push_back({ Variant::FLOAT, "g" });
				type.properties.push_back({ Variant::FLOAT, "b" });
				type.properties.push_back({ Variant::FLOAT, "a" });
				type.properties.push_back({ Variant::INT, "r8" });
				type.properties.push_back({ Variant::INT, "g8" });
				type.properties.push_back({ Variant::INT, "b8" });
				type.properties.push_back({ Variant::INT, "a8" });
				type.properties.push_back({ Variant::FLOAT, "h" });
				type.properties.push_back({ Variant::FLOAT, "s" });
				type.properties.push_back({ Variant::FLOAT, "v" });
				type.constants.push_back({ "ALICE_BLUE", Variant::COLOR, Color(0.941176, 0.972549, 1, 1) });
				type.constants.push_back({ "ANTIQUE_WHITE", Variant::COLOR, Color(0.980392, 0.921569, 0.843137, 1) });
				type.constants.push_back({ "AQUA", Variant::COLOR, Color(0, 1, 1, 1) });
				type.constants.push_back({ "AQUAMARINE", Variant::COLOR, Color(0.498039, 1, 0.831373, 1) });
				type.constants.push_back({ "AZURE", Variant::COLOR, Color(0.941176, 1, 1, 1) });
				type.constants.push_back({ "BEIGE", Variant::COLOR, Color(0.960784, 0.960784, 0.862745, 1) });
				type.constants.push_back({ "BISQUE", Variant::COLOR, Color(1, 0.894118, 0.768627, 1) });
				type.constants.push_back({ "BLACK", Variant::COLOR, Color(0, 0, 0, 1) });
				type.constants.push_back({ "BLANCHED_ALMOND", Variant::COLOR, Color(1, 0.921569, 0.803922, 1) });
				type.constants.push_back({ "BLUE", Variant::COLOR, Color(0, 0, 1, 1) });
				type.constants.push_back({ "BLUE_VIOLET", Variant::COLOR, Color(0.541176, 0.168627, 0.886275, 1) });
				type.constants.push_back({ "BROWN", Variant::COLOR, Color(0.647059, 0.164706, 0.164706, 1) });
				type.constants.push_back({ "BURLYWOOD", Variant::COLOR, Color(0.870588, 0.721569, 0.529412, 1) });
				type.constants.push_back({ "CADET_BLUE", Variant::COLOR, Color(0.372549, 0.619608, 0.627451, 1) });
				type.constants.push_back({ "CHARTREUSE", Variant::COLOR, Color(0.498039, 1, 0, 1) });
				type.constants.push_back({ "CHOCOLATE", Variant::COLOR, Color(0.823529, 0.411765, 0.117647, 1) });
				type.constants.push_back({ "CORAL", Variant::COLOR, Color(1, 0.498039, 0.313726, 1) });
				type.constants.push_back({ "CORNFLOWER_BLUE", Variant::COLOR, Color(0.392157, 0.584314, 0.929412, 1) });
				type.constants.push_back({ "CORNSILK", Variant::COLOR, Color(1, 0.972549, 0.862745, 1) });
				type.constants.push_back({ "CRIMSON", Variant::COLOR, Color(0.862745, 0.0784314, 0.235294, 1) });
				type.constants.push_back({ "CYAN", Variant::COLOR, Color(0, 1, 1, 1) });
				type.constants.push_back({ "DARK_BLUE", Variant::COLOR, Color(0, 0, 0.545098, 1) });
				type.constants.push_back({ "DARK_CYAN", Variant::COLOR, Color(0, 0.545098, 0.545098, 1) });
				type.constants.push_back({ "DARK_GOLDENROD", Variant::COLOR, Color(0.721569, 0.52549, 0.0431373, 1) });
				type.constants.push_back({ "DARK_GRAY", Variant::COLOR, Color(0.662745, 0.662745, 0.662745, 1) });
				type.constants.push_back({ "DARK_GREEN", Variant::COLOR, Color(0, 0.392157, 0, 1) });
				type.constants.push_back({ "DARK_KHAKI", Variant::COLOR, Color(0.741176, 0.717647, 0.419608, 1) });
				type.constants.push_back({ "DARK_MAGENTA", Variant::COLOR, Color(0.545098, 0, 0.545098, 1) });
				type.constants.push_back({ "DARK_OLIVE_GREEN", Variant::COLOR, Color(0.333333, 0.419608, 0.184314, 1) });
				type.constants.push_back({ "DARK_ORANGE", Variant::COLOR, Color(1, 0.54902, 0, 1) });
				type.constants.push_back({ "DARK_ORCHID", Variant::COLOR, Color(0.6, 0.196078, 0.8, 1) });
				type.constants.push_back({ "DARK_RED", Variant::COLOR, Color(0.545098, 0, 0, 1) });
				type.constants.push_back({ "DARK_SALMON", Variant::COLOR, Color(0.913725, 0.588235, 0.478431, 1) });
				type.constants.push_back({ "DARK_SEA_GREEN", Variant::COLOR, Color(0.560784, 0.737255, 0.560784, 1) });
				type.constants.push_back({ "DARK_SLATE_BLUE", Variant::COLOR, Color(0.282353, 0.239216, 0.545098, 1) });
				type.constants.push_back({ "DARK_SLATE_GRAY", Variant::COLOR, Color(0.184314, 0.309804, 0.309804, 1) });
				type.constants.push_back({ "DARK_TURQUOISE", Variant::COLOR, Color(0, 0.807843, 0.819608, 1) });
				type.constants.push_back({ "DARK_VIOLET", Variant::COLOR, Color(0.580392, 0, 0.827451, 1) });
				type.constants.push_back({ "DEEP_PINK", Variant::COLOR, Color(1, 0.0784314, 0.576471, 1) });
				type.constants.push_back({ "DEEP_SKY_BLUE", Variant::COLOR, Color(0, 0.74902, 1, 1) });
				type.constants.push_back({ "DIM_GRAY", Variant::COLOR, Color(0.411765, 0.411765, 0.411765, 1) });
				type.constants.push_back({ "DODGER_BLUE", Variant::COLOR, Color(0.117647, 0.564706, 1, 1) });
				type.constants.push_back({ "FIREBRICK", Variant::COLOR, Color(0.698039, 0.133333, 0.133333, 1) });
				type.constants.push_back({ "FLORAL_WHITE", Variant::COLOR, Color(1, 0.980392, 0.941176, 1) });
				type.constants.push_back({ "FOREST_GREEN", Variant::COLOR, Color(0.133333, 0.545098, 0.133333, 1) });
				type.constants.push_back({ "FUCHSIA", Variant::COLOR, Color(1, 0, 1, 1) });
				type.constants.push_back({ "GAINSBORO", Variant::COLOR, Color(0.862745, 0.862745, 0.862745, 1) });
				type.constants.push_back({ "GHOST_WHITE", Variant::COLOR, Color(0.972549, 0.972549, 1, 1) });
				type.constants.push_back({ "GOLD", Variant::COLOR, Color(1, 0.843137, 0, 1) });
				type.constants.push_back({ "GOLDENROD", Variant::COLOR, Color(0.854902, 0.647059, 0.12549, 1) });
				type.constants.push_back({ "GRAY", Variant::COLOR, Color(0.745098, 0.745098, 0.745098, 1) });
				type.constants.push_back({ "GREEN", Variant::COLOR, Color(0, 1, 0, 1) });
				type.constants.push_back({ "GREEN_YELLOW", Variant::COLOR, Color(0.678431, 1, 0.184314, 1) });
				type.constants.push_back({ "HONEYDEW", Variant::COLOR, Color(0.941176, 1, 0.941176, 1) });
				type.constants.push_back({ "HOT_PINK", Variant::COLOR, Color(1, 0.411765, 0.705882, 1) });
				type.constants.push_back({ "INDIAN_RED", Variant::COLOR, Color(0.803922, 0.360784, 0.360784, 1) });
				type.constants.push_back({ "INDIGO", Variant::COLOR, Color(0.294118, 0, 0.509804, 1) });
				type.constants.push_back({ "IVORY", Variant::COLOR, Color(1, 1, 0.941176, 1) });
				type.constants.push_back({ "KHAKI", Variant::COLOR, Color(0.941176, 0.901961, 0.54902, 1) });
				type.constants.push_back({ "LAVENDER", Variant::COLOR, Color(0.901961, 0.901961, 0.980392, 1) });
				type.constants.push_back({ "LAVENDER_BLUSH", Variant::COLOR, Color(1, 0.941176, 0.960784, 1) });
				type.constants.push_back({ "LAWN_GREEN", Variant::COLOR, Color(0.486275, 0.988235, 0, 1) });
				type.constants.push_back({ "LEMON_CHIFFON", Variant::COLOR, Color(1, 0.980392, 0.803922, 1) });
				type.constants.push_back({ "LIGHT_BLUE", Variant::COLOR, Color(0.678431, 0.847059, 0.901961, 1) });
				type.constants.push_back({ "LIGHT_CORAL", Variant::COLOR, Color(0.941176, 0.501961, 0.501961, 1) });
				type.constants.push_back({ "LIGHT_CYAN", Variant::COLOR, Color(0.878431, 1, 1, 1) });
				type.constants.push_back({ "LIGHT_GOLDENROD", Variant::COLOR, Color(0.980392, 0.980392, 0.823529, 1) });
				type.constants.push_back({ "LIGHT_GRAY", Variant::COLOR, Color(0.827451, 0.827451, 0.827451, 1) });
				type.constants.push_back({ "LIGHT_GREEN", Variant::COLOR, Color(0.564706, 0.933333, 0.564706, 1) });
				type.constants.push_back({ "LIGHT_PINK", Variant::COLOR, Color(1, 0.713726, 0.756863, 1) });
				type.constants.push_back({ "LIGHT_SALMON", Variant::COLOR, Color(1, 0.627451, 0.478431, 1) });
				type.constants.push_back({ "LIGHT_SEA_GREEN", Variant::COLOR, Color(0.12549, 0.698039, 0.666667, 1) });
				type.constants.push_back({ "LIGHT_SKY_BLUE", Variant::COLOR, Color(0.529412, 0.807843, 0.980392, 1) });
				type.constants.push_back({ "LIGHT_SLATE_GRAY", Variant::COLOR, Color(0.466667, 0.533333, 0.6, 1) });
				type.constants.push_back({ "LIGHT_STEEL_BLUE", Variant::COLOR, Color(0.690196, 0.768627, 0.870588, 1) });
				type.constants.push_back({ "LIGHT_YELLOW", Variant::COLOR, Color(1, 1, 0.878431, 1) });
				type.constants.push_back({ "LIME", Variant::COLOR, Color(0, 1, 0, 1) });
				type.constants.push_back({ "LIME_GREEN", Variant::COLOR, Color(0.196078, 0.803922, 0.196078, 1) });
				type.constants.push_back({ "LINEN", Variant::COLOR, Color(0.980392, 0.941176, 0.901961, 1) });
				type.constants.push_back({ "MAGENTA", Variant::COLOR, Color(1, 0, 1, 1) });
				type.constants.push_back({ "MAROON", Variant::COLOR, Color(0.690196, 0.188235, 0.376471, 1) });
				type.constants.push_back({ "MEDIUM_AQUAMARINE", Variant::COLOR, Color(0.4, 0.803922, 0.666667, 1) });
				type.constants.push_back({ "MEDIUM_BLUE", Variant::COLOR, Color(0, 0, 0.803922, 1) });
				type.constants.push_back({ "MEDIUM_ORCHID", Variant::COLOR, Color(0.729412, 0.333333, 0.827451, 1) });
				type.constants.push_back({ "MEDIUM_PURPLE", Variant::COLOR, Color(0.576471, 0.439216, 0.858824, 1) });
				type.constants.push_back({ "MEDIUM_SEA_GREEN", Variant::COLOR, Color(0.235294, 0.701961, 0.443137, 1) });
				type.constants.push_back({ "MEDIUM_SLATE_BLUE", Variant::COLOR, Color(0.482353, 0.407843, 0.933333, 1) });
				type.constants.push_back({ "MEDIUM_SPRING_GREEN", Variant::COLOR, Color(0, 0.980392, 0.603922, 1) });
				type.constants.push_back({ "MEDIUM_TURQUOISE", Variant::COLOR, Color(0.282353, 0.819608, 0.8, 1) });
				type.constants.push_back({ "MEDIUM_VIOLET_RED", Variant::COLOR, Color(0.780392, 0.0823529, 0.521569, 1) });
				type.constants.push_back({ "MIDNIGHT_BLUE", Variant::COLOR, Color(0.0980392, 0.0980392, 0.439216, 1) });
				type.constants.push_back({ "MINT_CREAM", Variant::COLOR, Color(0.960784, 1, 0.980392, 1) });
				type.constants.push_back({ "MISTY_ROSE", Variant::COLOR, Color(1, 0.894118, 0.882353, 1) });
				type.constants.push_back({ "MOCCASIN", Variant::COLOR, Color(1, 0.894118, 0.709804, 1) });
				type.constants.push_back({ "NAVAJO_WHITE", Variant::COLOR, Color(1, 0.870588, 0.678431, 1) });
				type.constants.push_back({ "NAVY_BLUE", Variant::COLOR, Color(0, 0, 0.501961, 1) });
				type.constants.push_back({ "OLD_LACE", Variant::COLOR, Color(0.992157, 0.960784, 0.901961, 1) });
				type.constants.push_back({ "OLIVE", Variant::COLOR, Color(0.501961, 0.501961, 0, 1) });
				type.constants.push_back({ "OLIVE_DRAB", Variant::COLOR, Color(0.419608, 0.556863, 0.137255, 1) });
				type.constants.push_back({ "ORANGE", Variant::COLOR, Color(1, 0.647059, 0, 1) });
				type.constants.push_back({ "ORANGE_RED", Variant::COLOR, Color(1, 0.270588, 0, 1) });
				type.constants.push_back({ "ORCHID", Variant::COLOR, Color(0.854902, 0.439216, 0.839216, 1) });
				type.constants.push_back({ "PALE_GOLDENROD", Variant::COLOR, Color(0.933333, 0.909804, 0.666667, 1) });
				type.constants.push_back({ "PALE_GREEN", Variant::COLOR, Color(0.596078, 0.984314, 0.596078, 1) });
				type.constants.push_back({ "PALE_TURQUOISE", Variant::COLOR, Color(0.686275, 0.933333, 0.933333, 1) });
				type.constants.push_back({ "PALE_VIOLET_RED", Variant::COLOR, Color(0.858824, 0.439216, 0.576471, 1) });
				type.constants.push_back({ "PAPAYA_WHIP", Variant::COLOR, Color(1, 0.937255, 0.835294, 1) });
				type.constants.push_back({ "PEACH_PUFF", Variant::COLOR, Color(1, 0.854902, 0.72549, 1) });
				type.constants.push_back({ "PERU", Variant::COLOR, Color(0.803922, 0.521569, 0.247059, 1) });
				type.constants.push_back({ "PINK", Variant::COLOR, Color(1, 0.752941, 0.796078, 1) });
				type.constants.push_back({ "PLUM", Variant::COLOR, Color(0.866667, 0.627451, 0.866667, 1) });
				type.constants.push_back({ "POWDER_BLUE", Variant::COLOR, Color(0.690196, 0.878431, 0.901961, 1) });
				type.constants.push_back({ "PURPLE", Variant::COLOR, Color(0.627451, 0.12549, 0.941176, 1) });
				type.constants.push_back({ "REBECCA_PURPLE", Variant::COLOR, Color(0.4, 0.2, 0.6, 1) });
				type.constants.push_back({ "RED", Variant::COLOR, Color(1, 0, 0, 1) });
				type.constants.push_back({ "ROSY_BROWN", Variant::COLOR, Color(0.737255, 0.560784, 0.560784, 1) });
				type.constants.push_back({ "ROYAL_BLUE", Variant::COLOR, Color(0.254902, 0.411765, 0.882353, 1) });
				type.constants.push_back({ "SADDLE_BROWN", Variant::COLOR, Color(0.545098, 0.270588, 0.0745098, 1) });
				type.constants.push_back({ "SALMON", Variant::COLOR, Color(0.980392, 0.501961, 0.447059, 1) });
				type.constants.push_back({ "SANDY_BROWN", Variant::COLOR, Color(0.956863, 0.643137, 0.376471, 1) });
				type.constants.push_back({ "SEA_GREEN", Variant::COLOR, Color(0.180392, 0.545098, 0.341176, 1) });
				type.constants.push_back({ "SEASHELL", Variant::COLOR, Color(1, 0.960784, 0.933333, 1) });
				type.constants.push_back({ "SIENNA", Variant::COLOR, Color(0.627451, 0.321569, 0.176471, 1) });
				type.constants.push_back({ "SILVER", Variant::COLOR, Color(0.752941, 0.752941, 0.752941, 1) });
				type.constants.push_back({ "SKY_BLUE", Variant::COLOR, Color(0.529412, 0.807843, 0.921569, 1) });
				type.constants.push_back({ "SLATE_BLUE", Variant::COLOR, Color(0.415686, 0.352941, 0.803922, 1) });
				type.constants.push_back({ "SLATE_GRAY", Variant::COLOR, Color(0.439216, 0.501961, 0.564706, 1) });
				type.constants.push_back({ "SNOW", Variant::COLOR, Color(1, 0.980392, 0.980392, 1) });
				type.constants.push_back({ "SPRING_GREEN", Variant::COLOR, Color(0, 1, 0.498039, 1) });
				type.constants.push_back({ "STEEL_BLUE", Variant::COLOR, Color(0.27451, 0.509804, 0.705882, 1) });
				type.constants.push_back({ "TAN", Variant::COLOR, Color(0.823529, 0.705882, 0.54902, 1) });
				type.constants.push_back({ "TEAL", Variant::COLOR, Color(0, 0.501961, 0.501961, 1) });
				type.constants.push_back({ "THISTLE", Variant::COLOR, Color(0.847059, 0.74902, 0.847059, 1) });
				type.constants.push_back({ "TOMATO", Variant::COLOR, Color(1, 0.388235, 0.278431, 1) });
				type.constants.push_back({ "TRANSPARENT", Variant::COLOR, Color(1, 1, 1, 0) });
				type.constants.push_back({ "TURQUOISE", Variant::COLOR, Color(0.25098, 0.878431, 0.815686, 1) });
				type.constants.push_back({ "VIOLET", Variant::COLOR, Color(0.933333, 0.509804, 0.933333, 1) });
				type.constants.push_back({ "WEB_GRAY", Variant::COLOR, Color(0.501961, 0.501961, 0.501961, 1) });
				type.constants.push_back({ "WEB_GREEN", Variant::COLOR, Color(0, 0.501961, 0, 1) });
				type.constants.push_back({ "WEB_MAROON", Variant::COLOR, Color(0.501961, 0, 0, 1) });
				type.constants.push_back({ "WEB_PURPLE", Variant::COLOR, Color(0.501961, 0, 0.501961, 1) });
				type.constants.push_back({ "WHEAT", Variant::COLOR, Color(0.960784, 0.870588, 0.701961, 1) });
				type.constants.push_back({ "WHITE", Variant::COLOR, Color(1, 1, 1, 1) });
				type.constants.push_back({ "WHITE_SMOKE", Variant::COLOR, Color(0.960784, 0.960784, 0.960784, 1) });
				type.constants.push_back({ "YELLOW", Variant::COLOR, Color(1, 1, 0, 1) });
				type.constants.push_back({ "YELLOW_GREEN", Variant::COLOR, Color(0.603922, 0.803922, 0.196078, 1) });
				type.methods.push_back(_make_method("to_argb32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_abgr32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_rgba32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_argb64", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_abgr64", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_rgba64", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_html", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::BOOL, "with_alpha" } }));
				type.methods.push_back(_make_method("clamp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, { { Variant::COLOR, "min" }, { Variant::COLOR, "max" } }));
				type.methods.push_back(_make_method("inverted", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, {  }));
				type.methods.push_back(_make_method("lerp", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, { { Variant::COLOR, "to" }, { Variant::FLOAT, "weight" } }));
				type.methods.push_back(_make_method("lightened", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, { { Variant::FLOAT, "amount" } }));
				type.methods.push_back(_make_method("darkened", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, { { Variant::FLOAT, "amount" } }));
				type.methods.push_back(_make_method("blend", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, { { Variant::COLOR, "over" } }));
				type.methods.push_back(_make_method("get_luminance", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("srgb_to_linear", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, {  }));
				type.methods.push_back(_make_method("linear_to_srgb", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::COLOR, {  }));
				type.methods.push_back(_make_method("is_equal_approx", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::COLOR, "to" } }));
				type.methods.push_back(_make_method("hex", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::INT, "hex" } }));
				type.methods.push_back(_make_method("hex64", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::INT, "hex" } }));
				type.methods.push_back(_make_method("html", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::STRING, "rgba" } }));
				type.methods.push_back(_make_method("html_is_valid", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::BOOL, { { Variant::STRING, "color" } }));
				type.methods.push_back(_make_method("from_string", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::STRING, "str" }, { Variant::COLOR, "default" } }));
				type.methods.push_back(_make_method("from_hsv", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::FLOAT, "h" }, { Variant::FLOAT, "s" }, { Variant::FLOAT, "v" }, { Variant::FLOAT, "alpha" } }));
				type.methods.push_back(_make_method("from_ok_hsl", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::FLOAT, "h" }, { Variant::FLOAT, "s" }, { Variant::FLOAT, "l" }, { Variant::FLOAT, "alpha" } }));
				type.methods.push_back(_make_method("from_rgbe9995", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::COLOR, { { Variant::INT, "rgbe" } }));
				ExtensionDB::_singleton->_builtin_types["Color"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::COLOR] = "Color";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Color");
			}
			{
				BuiltInType type;
				type.name = "StringName";
				type.type = Variant::STRING_NAME;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING_NAME, "StringName", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING_NAME, "StringName", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::NIL, "Variant", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::STRING_NAME, "StringName", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::BOOL, "bool", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::INT, "int", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::FLOAT, "float", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING_NAME, "StringName", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING_NAME, "StringName", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::STRING_NAME, "StringName", Variant::STRING, "String", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::STRING, "String", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::STRING, "String", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR2, "Vector2", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR2I, "Vector2i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::RECT2, "Rect2", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::RECT2I, "Rect2i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR3, "Vector3", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR3I, "Vector3i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::TRANSFORM2D, "Transform2D", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR4, "Vector4", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::VECTOR4I, "Vector4i", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PLANE, "Plane", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::QUATERNION, "Quaternion", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::AABB, "AABB", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::BASIS, "Basis", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::TRANSFORM3D, "Transform3D", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PROJECTION, "Projection", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::COLOR, "Color", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::STRING_NAME, "StringName", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::NODE_PATH, "NodePath", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::OBJECT, "Object", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::OBJECT, "Object", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::CALLABLE, "Callable", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::SIGNAL, "Signal", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::DICTIONARY, "Dictionary", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::ARRAY, "Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::STRING_NAME, "StringName", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::STRING });
				type.operators.push_back({ VariantOperators::OP_MODULE, "%", "Module", Variant::STRING_NAME, "StringName", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::STRING });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING_NAME, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "from") } });
				type.methods.push_back(_make_method("casecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("nocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("naturalcasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("naturalnocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("filecasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("filenocasecmp_to", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "to" } }));
				type.methods.push_back(_make_method("length", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("substr", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "from" }, { Variant::INT, "len" } }));
				type.methods.push_back(_make_method("get_slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "delimiter" }, { Variant::INT, "slice" } }));
				type.methods.push_back(_make_method("get_slicec", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "delimiter" }, { Variant::INT, "slice" } }));
				type.methods.push_back(_make_method("get_slice_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "delimiter" } }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("findn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" }, { Variant::INT, "to" } }));
				type.methods.push_back(_make_method("countn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" }, { Variant::INT, "to" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfindn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("match", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "expr" } }));
				type.methods.push_back(_make_method("matchn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "expr" } }));
				type.methods.push_back(_make_method("begins_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("ends_with", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("is_subsequence_of", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("is_subsequence_ofn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("bigrams", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, {  }));
				type.methods.push_back(_make_method("similarity", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::STRING, "text" } }));
				type.methods.push_back(_make_method("format", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::NIL, "values" }, { Variant::STRING, "placeholder" } }));
				type.methods.push_back(_make_method("replace", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "what" }, { Variant::STRING, "forwhat" } }));
				type.methods.push_back(_make_method("replacen", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "what" }, { Variant::STRING, "forwhat" } }));
				type.methods.push_back(_make_method("repeat", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "count" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "position" }, { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("erase", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "position" }, { Variant::INT, "chars" } }));
				type.methods.push_back(_make_method("capitalize", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_camel_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_pascal_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_snake_case", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("split", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" }, { Variant::INT, "maxsplit" } }));
				type.methods.push_back(_make_method("rsplit", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" }, { Variant::INT, "maxsplit" } }));
				type.methods.push_back(_make_method("split_floats", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT64_ARRAY, { { Variant::STRING, "delimiter" }, { Variant::BOOL, "allow_empty" } }));
				type.methods.push_back(_make_method("join", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::PACKED_STRING_ARRAY, "parts" } }));
				type.methods.push_back(_make_method("to_upper", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("to_lower", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("left", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "length" } }));
				type.methods.push_back(_make_method("right", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "length" } }));
				type.methods.push_back(_make_method("strip_edges", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::BOOL, "left" }, { Variant::BOOL, "right" } }));
				type.methods.push_back(_make_method("strip_escapes", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("lstrip", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "chars" } }));
				type.methods.push_back(_make_method("rstrip", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "chars" } }));
				type.methods.push_back(_make_method("get_extension", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_basename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("path_join", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "file" } }));
				type.methods.push_back(_make_method("unicode_at", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "at" } }));
				type.methods.push_back(_make_method("indent", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "prefix" } }));
				type.methods.push_back(_make_method("dedent", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("md5_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("sha1_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("sha256_text", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("md5_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sha1_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sha256_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("contains", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("containsn", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "what" } }));
				type.methods.push_back(_make_method("is_absolute_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_relative_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("simplify_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_base_dir", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_file", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("xml_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::BOOL, "escape_quotes" } }));
				type.methods.push_back(_make_method("xml_unescape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("uri_encode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("uri_decode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("c_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("c_unescape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("json_escape", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("validate_node_name", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("validate_filename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("is_valid_identifier", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_float", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_hex_number", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::BOOL, "with_prefix" } }));
				type.methods.push_back(_make_method("is_valid_html_color", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_ip_address", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid_filename", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("to_float", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, {  }));
				type.methods.push_back(_make_method("hex_to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("bin_to_int", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("lpad", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "min_length" }, { Variant::STRING, "character" } }));
				type.methods.push_back(_make_method("rpad", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "min_length" }, { Variant::STRING, "character" } }));
				type.methods.push_back(_make_method("pad_decimals", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "digits" } }));
				type.methods.push_back(_make_method("pad_zeros", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::INT, "digits" } }));
				type.methods.push_back(_make_method("trim_prefix", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "prefix" } }));
				type.methods.push_back(_make_method("trim_suffix", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, { { Variant::STRING, "suffix" } }));
				type.methods.push_back(_make_method("to_ascii_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf8_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf16_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_utf32_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("hex_decode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("to_wchar_buffer", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				ExtensionDB::_singleton->_builtin_types["StringName"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::STRING_NAME] = "StringName";
				ExtensionDB::_singleton->_builtin_type_names.push_back("StringName");
			}
			{
				BuiltInType type;
				type.name = "NodePath";
				type.type = Variant::NODE_PATH;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NODE_PATH, "NodePath", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NODE_PATH, "NodePath", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::NODE_PATH, "NodePath", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::NODE_PATH, "NodePath", Variant::NODE_PATH, "NodePath", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::NODE_PATH, "NodePath", Variant::NODE_PATH, "NodePath", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::NODE_PATH, "NodePath", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::NODE_PATH, "NodePath", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::NODE_PATH, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::STRING, "from") } });
				type.methods.push_back(_make_method("is_absolute", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_name_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_name", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, { { Variant::INT, "idx" } }));
				type.methods.push_back(_make_method("get_subname_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_subname", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, { { Variant::INT, "idx" } }));
				type.methods.push_back(_make_method("get_concatenated_names", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, {  }));
				type.methods.push_back(_make_method("get_concatenated_subnames", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NODE_PATH, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("get_as_property_path", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NODE_PATH, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				ExtensionDB::_singleton->_builtin_types["NodePath"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::NODE_PATH] = "NodePath";
				ExtensionDB::_singleton->_builtin_type_names.push_back("NodePath");
			}
			{
				BuiltInType type;
				type.name = "RID";
				type.type = Variant::RID;
				type.keyed = false;
				type.has_destructor = false;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RID, "RID", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RID, "RID", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::RID, "RID", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::RID, "RID", Variant::RID, "RID", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::RID, "from") } });
				type.methods.push_back(_make_method("is_valid", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_id", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				ExtensionDB::_singleton->_builtin_types["RID"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::RID] = "RID";
				ExtensionDB::_singleton->_builtin_type_names.push_back("RID");
			}
			{
				BuiltInType type;
				type.name = "Callable";
				type.type = Variant::CALLABLE;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::CALLABLE, "Callable", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::CALLABLE, "Callable", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::CALLABLE, "Callable", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::CALLABLE, "Callable", Variant::CALLABLE, "Callable", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::CALLABLE, "Callable", Variant::CALLABLE, "Callable", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::CALLABLE, "Callable", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::CALLABLE, "Callable", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::CALLABLE, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::OBJECT, "object"), PropertyInfo(Variant::STRING_NAME, "method") } });
				type.methods.push_back(_make_method("create", METHOD_FLAG_NORMAL | METHOD_FLAG_STATIC, Variant::CALLABLE, { { Variant::NIL, "variant" }, { Variant::STRING_NAME, "method" } }));
				type.methods.push_back(_make_method("callv", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::ARRAY, "arguments" } }, true));
				type.methods.push_back(_make_method("is_null", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_custom", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_standard", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_valid", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_object", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::OBJECT, {  }));
				type.methods.push_back(_make_method("get_object_id", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_method", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, {  }));
				type.methods.push_back(_make_method("get_argument_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_bound_arguments_count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_bound_arguments", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, {  }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("bindv", METHOD_FLAG_NORMAL, Variant::CALLABLE, { { Variant::ARRAY, "arguments" } }));
				type.methods.push_back(_make_method("unbind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::CALLABLE, { { Variant::INT, "argcount" } }));
				type.methods.push_back(_make_method("call", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("call_deferred", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::NIL, {  }));
				type.methods.push_back(_make_method("rpc", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::NIL, {  }));
				type.methods.push_back(_make_method("rpc_id", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::NIL, { { Variant::INT, "peer_id" } }));
				type.methods.push_back(_make_method("bind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::CALLABLE, {  }));
				ExtensionDB::_singleton->_builtin_types["Callable"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::CALLABLE] = "Callable";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Callable");
			}
			{
				BuiltInType type;
				type.name = "Signal";
				type.type = Variant::SIGNAL;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::SIGNAL, "Signal", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::SIGNAL, "Signal", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::SIGNAL, "Signal", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::SIGNAL, "Signal", Variant::SIGNAL, "Signal", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::SIGNAL, "Signal", Variant::SIGNAL, "Signal", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::SIGNAL, "Signal", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::SIGNAL, "Signal", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::SIGNAL, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::OBJECT, "object"), PropertyInfo(Variant::STRING_NAME, "signal") } });
				type.methods.push_back(_make_method("is_null", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("get_object", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::OBJECT, {  }));
				type.methods.push_back(_make_method("get_object_id", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_name", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, {  }));
				type.methods.push_back(_make_method("connect", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::CALLABLE, "callable" }, { Variant::INT, "flags" } }));
				type.methods.push_back(_make_method("disconnect", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::CALLABLE, "callable" } }));
				type.methods.push_back(_make_method("is_connected", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::CALLABLE, "callable" } }));
				type.methods.push_back(_make_method("get_connections", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, {  }));
				type.methods.push_back(_make_method("emit", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST | METHOD_FLAG_VARARG, Variant::NIL, {  }));
				ExtensionDB::_singleton->_builtin_types["Signal"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::SIGNAL] = "Signal";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Signal");
			}
			{
				BuiltInType type;
				type.name = "Dictionary";
				type.type = Variant::DICTIONARY;
				type.keyed = true;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::DICTIONARY, "Dictionary", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::DICTIONARY, "Dictionary", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::DICTIONARY, "Dictionary", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::DICTIONARY, "Dictionary", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::DICTIONARY, "Dictionary", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::DICTIONARY, "Dictionary", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::DICTIONARY, "Dictionary", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::DICTIONARY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("merge", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::DICTIONARY, "dictionary" }, { Variant::BOOL, "overwrite" } }));
				type.methods.push_back(_make_method("merged", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::DICTIONARY, { { Variant::DICTIONARY, "dictionary" }, { Variant::BOOL, "overwrite" } }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::NIL, "key" } }));
				type.methods.push_back(_make_method("has_all", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::ARRAY, "keys" } }));
				type.methods.push_back(_make_method("find_key", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::NIL, "value" } }, true));
				type.methods.push_back(_make_method("erase", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::NIL, "key" } }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("keys", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, {  }));
				type.methods.push_back(_make_method("values", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, {  }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::DICTIONARY, { { Variant::BOOL, "deep" } }));
				type.methods.push_back(_make_method("get", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::NIL, "key" }, { Variant::NIL, "default" } }, true));
				type.methods.push_back(_make_method("get_or_add", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "key" }, { Variant::NIL, "default" } }, true));
				type.methods.push_back(_make_method("make_read_only", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("is_read_only", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("recursive_equal", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::DICTIONARY, "dictionary" }, { Variant::INT, "recursion_count" } }));
				ExtensionDB::_singleton->_builtin_types["Dictionary"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::DICTIONARY] = "Dictionary";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Dictionary");
			}
			{
				BuiltInType type;
				type.name = "Array";
				type.type = Variant::ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::NIL;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::ARRAY, "Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::ARRAY, "Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::ARRAY, "Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::ARRAY, "Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS, "<", "Less-than", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_LESS_EQUAL, "<=", "Less-than or Equal", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER, ">", "Greater-than", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_GREATER_EQUAL, ">=", "Greater-than or Equal", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::ARRAY });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::ARRAY, "Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "base"), PropertyInfo(Variant::INT, "type"), PropertyInfo(Variant::STRING_NAME, "class_name"), PropertyInfo(Variant::NIL, "script") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_BYTE_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_INT32_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_INT64_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_STRING_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_COLOR_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR4_ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("hash", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("assign", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::ARRAY, "array" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("push_front", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::ARRAY, "array" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "size" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "position" }, { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "position" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("erase", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("front", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("back", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("pick_random", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::NIL, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::NIL, "what" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::NIL, "value" } }));
				type.methods.push_back(_make_method("pop_back", METHOD_FLAG_NORMAL, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("pop_front", METHOD_FLAG_NORMAL, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("pop_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "position" } }, true));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("sort_custom", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::CALLABLE, "func" } }));
				type.methods.push_back(_make_method("shuffle", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::NIL, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("bsearch_custom", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::NIL, "value" }, { Variant::CALLABLE, "func" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, { { Variant::BOOL, "deep" } }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" }, { Variant::INT, "step" }, { Variant::BOOL, "deep" } }));
				type.methods.push_back(_make_method("filter", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, { { Variant::CALLABLE, "method" } }));
				type.methods.push_back(_make_method("map", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::ARRAY, { { Variant::CALLABLE, "method" } }));
				type.methods.push_back(_make_method("reduce", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::CALLABLE, "method" }, { Variant::NIL, "accum" } }, true));
				type.methods.push_back(_make_method("any", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::CALLABLE, "method" } }));
				type.methods.push_back(_make_method("all", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::CALLABLE, "method" } }));
				type.methods.push_back(_make_method("max", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("min", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("is_typed", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("is_same_typed", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::ARRAY, "array" } }));
				type.methods.push_back(_make_method("get_typed_builtin", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("get_typed_class_name", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING_NAME, {  }));
				type.methods.push_back(_make_method("get_typed_script", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, {  }, true));
				type.methods.push_back(_make_method("make_read_only", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("is_read_only", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				ExtensionDB::_singleton->_builtin_types["Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::ARRAY] = "Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("Array");
			}
			{
				BuiltInType type;
				type.name = "PackedByteArray";
				type.type = Variant::PACKED_BYTE_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::PACKED_BYTE_ARRAY, "PackedByteArray", Variant::PACKED_BYTE_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_BYTE_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_BYTE_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("get_string_from_ascii", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_string_from_utf8", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_string_from_utf16", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_string_from_utf32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("get_string_from_wchar", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("hex_encode", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::STRING, {  }));
				type.methods.push_back(_make_method("compress", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, { { Variant::INT, "compression_mode" } }));
				type.methods.push_back(_make_method("decompress", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, { { Variant::INT, "buffer_size" }, { Variant::INT, "compression_mode" } }));
				type.methods.push_back(_make_method("decompress_dynamic", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, { { Variant::INT, "max_output_size" }, { Variant::INT, "compression_mode" } }));
				type.methods.push_back(_make_method("decode_u8", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_s8", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_u16", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_s16", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_u32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_s32", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_u64", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_s64", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_half", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_float", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("decode_double", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::FLOAT, { { Variant::INT, "byte_offset" } }));
				type.methods.push_back(_make_method("has_encoded_var", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::INT, "byte_offset" }, { Variant::BOOL, "allow_objects" } }));
				type.methods.push_back(_make_method("decode_var", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::BOOL, "allow_objects" } }, true));
				type.methods.push_back(_make_method("decode_var_size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "byte_offset" }, { Variant::BOOL, "allow_objects" } }));
				type.methods.push_back(_make_method("to_int32_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_INT32_ARRAY, {  }));
				type.methods.push_back(_make_method("to_int64_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_INT64_ARRAY, {  }));
				type.methods.push_back(_make_method("to_float32_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT32_ARRAY, {  }));
				type.methods.push_back(_make_method("to_float64_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT64_ARRAY, {  }));
				type.methods.push_back(_make_method("encode_u8", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_s8", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_u16", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_s16", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_u32", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_s32", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_u64", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_s64", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("encode_half", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("encode_float", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("encode_double", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "byte_offset" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("encode_var", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "byte_offset" }, { Variant::NIL, "value" }, { Variant::BOOL, "allow_objects" } }));
				ExtensionDB::_singleton->_builtin_types["PackedByteArray"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_BYTE_ARRAY] = "PackedByteArray";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedByteArray");
			}
			{
				BuiltInType type;
				type.name = "PackedInt32Array";
				type.type = Variant::PACKED_INT32_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::PACKED_INT32_ARRAY, "PackedInt32Array", Variant::PACKED_INT32_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_INT32_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_INT32_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_INT32_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_INT32_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedInt32Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_INT32_ARRAY] = "PackedInt32Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedInt32Array");
			}
			{
				BuiltInType type;
				type.name = "PackedInt64Array";
				type.type = Variant::PACKED_INT64_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::INT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::PACKED_INT64_ARRAY, "PackedInt64Array", Variant::PACKED_INT64_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_INT64_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_INT64_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::INT, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_INT64_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_INT64_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::INT, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedInt64Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_INT64_ARRAY] = "PackedInt64Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedInt64Array");
			}
			{
				BuiltInType type;
				type.name = "PackedFloat32Array";
				type.type = Variant::PACKED_FLOAT32_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::PACKED_FLOAT32_ARRAY, "PackedFloat32Array", Variant::PACKED_FLOAT32_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_FLOAT32_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT32_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_FLOAT32_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedFloat32Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_FLOAT32_ARRAY] = "PackedFloat32Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedFloat32Array");
			}
			{
				BuiltInType type;
				type.name = "PackedFloat64Array";
				type.type = Variant::PACKED_FLOAT64_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::FLOAT;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::PACKED_FLOAT64_ARRAY, "PackedFloat64Array", Variant::PACKED_FLOAT64_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_FLOAT64_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::FLOAT, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_FLOAT64_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_FLOAT64_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::FLOAT, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedFloat64Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_FLOAT64_ARRAY] = "PackedFloat64Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedFloat64Array");
			}
			{
				BuiltInType type;
				type.name = "PackedStringArray";
				type.type = Variant::PACKED_STRING_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::STRING;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::PACKED_STRING_ARRAY, "PackedStringArray", Variant::PACKED_STRING_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_STRING_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_STRING_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::STRING, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_STRING_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::STRING, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_STRING_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::STRING, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedStringArray"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_STRING_ARRAY] = "PackedStringArray";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedStringArray");
			}
			{
				BuiltInType type;
				type.name = "PackedVector2Array";
				type.type = Variant::PACKED_VECTOR2_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::VECTOR2;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::TRANSFORM2D, "Transform2D", Variant::PACKED_VECTOR2_ARRAY });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::PACKED_VECTOR2_ARRAY, "PackedVector2Array", Variant::PACKED_VECTOR2_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_VECTOR2_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR2, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_VECTOR2_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::VECTOR2, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_VECTOR2_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR2, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR2, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR2, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedVector2Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_VECTOR2_ARRAY] = "PackedVector2Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedVector2Array");
			}
			{
				BuiltInType type;
				type.name = "PackedVector3Array";
				type.type = Variant::PACKED_VECTOR3_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::VECTOR3;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_MULTIPLY, "*", "Multiply", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::TRANSFORM3D, "Transform3D", Variant::PACKED_VECTOR3_ARRAY });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::PACKED_VECTOR3_ARRAY, "PackedVector3Array", Variant::PACKED_VECTOR3_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_VECTOR3_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR3, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_VECTOR3_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::VECTOR3, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_VECTOR3_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR3, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR3, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR3, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedVector3Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_VECTOR3_ARRAY] = "PackedVector3Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedVector3Array");
			}
			{
				BuiltInType type;
				type.name = "PackedColorArray";
				type.type = Variant::PACKED_COLOR_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::COLOR;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::PACKED_COLOR_ARRAY, "PackedColorArray", Variant::PACKED_COLOR_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_COLOR_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_COLOR_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::COLOR, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_COLOR_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::COLOR, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_COLOR_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::COLOR, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::COLOR, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::COLOR, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedColorArray"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_COLOR_ARRAY] = "PackedColorArray";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedColorArray");
			}
			{
				BuiltInType type;
				type.name = "PackedVector4Array";
				type.type = Variant::PACKED_VECTOR4_ARRAY;
				type.keyed = false;
				type.has_destructor = true;
				type.index_returning_type = Variant::VECTOR4;
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::NIL, "Variant", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT, "not", "Not", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::NIL, "", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::DICTIONARY, "Dictionary", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_IN, "in", "In", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::ARRAY, "Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_EQUAL, "==", "Equal", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_NOT_EQUAL, "!=", "Not Equal", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::BOOL });
				type.operators.push_back({ VariantOperators::OP_ADD, "+", "Addition", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::PACKED_VECTOR4_ARRAY, "PackedVector4Array", Variant::PACKED_VECTOR4_ARRAY });
				type.constructors.push_back({ {  } });
				type.constructors.push_back({ { PropertyInfo(Variant::PACKED_VECTOR4_ARRAY, "from") } });
				type.constructors.push_back({ { PropertyInfo(Variant::ARRAY, "from") } });
				type.methods.push_back(_make_method("size", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, {  }));
				type.methods.push_back(_make_method("is_empty", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, {  }));
				type.methods.push_back(_make_method("set", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" }, { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("push_back", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("append", METHOD_FLAG_NORMAL, Variant::BOOL, { { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("append_array", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::PACKED_VECTOR4_ARRAY, "array" } }));
				type.methods.push_back(_make_method("remove_at", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::INT, "index" } }));
				type.methods.push_back(_make_method("insert", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "at_index" }, { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("fill", METHOD_FLAG_NORMAL, Variant::NIL, { { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("resize", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::INT, "new_size" } }));
				type.methods.push_back(_make_method("clear", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("has", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::BOOL, { { Variant::VECTOR4, "value" } }));
				type.methods.push_back(_make_method("reverse", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("slice", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_VECTOR4_ARRAY, { { Variant::INT, "begin" }, { Variant::INT, "end" } }));
				type.methods.push_back(_make_method("to_byte_array", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::PACKED_BYTE_ARRAY, {  }));
				type.methods.push_back(_make_method("sort", METHOD_FLAG_NORMAL, Variant::NIL, {  }));
				type.methods.push_back(_make_method("bsearch", METHOD_FLAG_NORMAL, Variant::INT, { { Variant::VECTOR4, "value" }, { Variant::BOOL, "before" } }));
				type.methods.push_back(_make_method("duplicate", METHOD_FLAG_NORMAL, Variant::PACKED_VECTOR4_ARRAY, {  }));
				type.methods.push_back(_make_method("find", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR4, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("rfind", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR4, "value" }, { Variant::INT, "from" } }));
				type.methods.push_back(_make_method("count", METHOD_FLAG_NORMAL | METHOD_FLAG_CONST, Variant::INT, { { Variant::VECTOR4, "value" } }));
				ExtensionDB::_singleton->_builtin_types["PackedVector4Array"] = type;
				ExtensionDB::_singleton->_builtin_types_to_name[Variant::PACKED_VECTOR4_ARRAY] = "PackedVector4Array";
				ExtensionDB::_singleton->_builtin_type_names.push_back("PackedVector4Array");
			}
		}
		
		void ExtensionDBLoader::prime_utility_functions()
		{
			// Utility Functions
			{
				FunctionInfo fi;
				fi.name = "sin";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "angle_rad" });
				ExtensionDB::_singleton->_functions["sin"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("sin");
			}
			{
				FunctionInfo fi;
				fi.name = "cos";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "angle_rad" });
				ExtensionDB::_singleton->_functions["cos"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cos");
			}
			{
				FunctionInfo fi;
				fi.name = "tan";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "angle_rad" });
				ExtensionDB::_singleton->_functions["tan"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("tan");
			}
			{
				FunctionInfo fi;
				fi.name = "sinh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["sinh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("sinh");
			}
			{
				FunctionInfo fi;
				fi.name = "cosh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["cosh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cosh");
			}
			{
				FunctionInfo fi;
				fi.name = "tanh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["tanh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("tanh");
			}
			{
				FunctionInfo fi;
				fi.name = "asin";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["asin"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("asin");
			}
			{
				FunctionInfo fi;
				fi.name = "acos";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["acos"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("acos");
			}
			{
				FunctionInfo fi;
				fi.name = "atan";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["atan"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("atan");
			}
			{
				FunctionInfo fi;
				fi.name = "atan2";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "y" });
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["atan2"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("atan2");
			}
			{
				FunctionInfo fi;
				fi.name = "asinh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["asinh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("asinh");
			}
			{
				FunctionInfo fi;
				fi.name = "acosh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["acosh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("acosh");
			}
			{
				FunctionInfo fi;
				fi.name = "atanh";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["atanh"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("atanh");
			}
			{
				FunctionInfo fi;
				fi.name = "sqrt";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["sqrt"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("sqrt");
			}
			{
				FunctionInfo fi;
				fi.name = "fmod";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				fi.arguments.push_back({ Variant::FLOAT, "y" });
				ExtensionDB::_singleton->_functions["fmod"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("fmod");
			}
			{
				FunctionInfo fi;
				fi.name = "fposmod";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				fi.arguments.push_back({ Variant::FLOAT, "y" });
				ExtensionDB::_singleton->_functions["fposmod"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("fposmod");
			}
			{
				FunctionInfo fi;
				fi.name = "posmod";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "x" });
				fi.arguments.push_back({ Variant::INT, "y" });
				ExtensionDB::_singleton->_functions["posmod"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("posmod");
			}
			{
				FunctionInfo fi;
				fi.name = "floor";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				ExtensionDB::_singleton->_functions["floor"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("floor");
			}
			{
				FunctionInfo fi;
				fi.name = "floorf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["floorf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("floorf");
			}
			{
				FunctionInfo fi;
				fi.name = "floori";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["floori"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("floori");
			}
			{
				FunctionInfo fi;
				fi.name = "ceil";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				ExtensionDB::_singleton->_functions["ceil"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("ceil");
			}
			{
				FunctionInfo fi;
				fi.name = "ceilf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["ceilf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("ceilf");
			}
			{
				FunctionInfo fi;
				fi.name = "ceili";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["ceili"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("ceili");
			}
			{
				FunctionInfo fi;
				fi.name = "round";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				ExtensionDB::_singleton->_functions["round"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("round");
			}
			{
				FunctionInfo fi;
				fi.name = "roundf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["roundf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("roundf");
			}
			{
				FunctionInfo fi;
				fi.name = "roundi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["roundi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("roundi");
			}
			{
				FunctionInfo fi;
				fi.name = "abs";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				ExtensionDB::_singleton->_functions["abs"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("abs");
			}
			{
				FunctionInfo fi;
				fi.name = "absf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["absf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("absf");
			}
			{
				FunctionInfo fi;
				fi.name = "absi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "x" });
				ExtensionDB::_singleton->_functions["absi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("absi");
			}
			{
				FunctionInfo fi;
				fi.name = "sign";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				ExtensionDB::_singleton->_functions["sign"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("sign");
			}
			{
				FunctionInfo fi;
				fi.name = "signf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["signf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("signf");
			}
			{
				FunctionInfo fi;
				fi.name = "signi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "x" });
				ExtensionDB::_singleton->_functions["signi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("signi");
			}
			{
				FunctionInfo fi;
				fi.name = "snapped";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "x" });
				fi.arguments.push_back({ Variant::NIL, "step" });
				ExtensionDB::_singleton->_functions["snapped"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("snapped");
			}
			{
				FunctionInfo fi;
				fi.name = "snappedf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				fi.arguments.push_back({ Variant::FLOAT, "step" });
				ExtensionDB::_singleton->_functions["snappedf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("snappedf");
			}
			{
				FunctionInfo fi;
				fi.name = "snappedi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				fi.arguments.push_back({ Variant::INT, "step" });
				ExtensionDB::_singleton->_functions["snappedi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("snappedi");
			}
			{
				FunctionInfo fi;
				fi.name = "pow";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "base" });
				fi.arguments.push_back({ Variant::FLOAT, "exp" });
				ExtensionDB::_singleton->_functions["pow"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("pow");
			}
			{
				FunctionInfo fi;
				fi.name = "log";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["log"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("log");
			}
			{
				FunctionInfo fi;
				fi.name = "exp";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["exp"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("exp");
			}
			{
				FunctionInfo fi;
				fi.name = "is_nan";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["is_nan"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_nan");
			}
			{
				FunctionInfo fi;
				fi.name = "is_inf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["is_inf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_inf");
			}
			{
				FunctionInfo fi;
				fi.name = "is_equal_approx";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "a" });
				fi.arguments.push_back({ Variant::FLOAT, "b" });
				ExtensionDB::_singleton->_functions["is_equal_approx"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_equal_approx");
			}
			{
				FunctionInfo fi;
				fi.name = "is_zero_approx";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["is_zero_approx"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_zero_approx");
			}
			{
				FunctionInfo fi;
				fi.name = "is_finite";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["is_finite"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_finite");
			}
			{
				FunctionInfo fi;
				fi.name = "ease";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				fi.arguments.push_back({ Variant::FLOAT, "curve" });
				ExtensionDB::_singleton->_functions["ease"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("ease");
			}
			{
				FunctionInfo fi;
				fi.name = "step_decimals";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["step_decimals"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("step_decimals");
			}
			{
				FunctionInfo fi;
				fi.name = "lerp";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "from" });
				fi.arguments.push_back({ Variant::NIL, "to" });
				fi.arguments.push_back({ Variant::NIL, "weight" });
				ExtensionDB::_singleton->_functions["lerp"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("lerp");
			}
			{
				FunctionInfo fi;
				fi.name = "lerpf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				ExtensionDB::_singleton->_functions["lerpf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("lerpf");
			}
			{
				FunctionInfo fi;
				fi.name = "cubic_interpolate";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "pre" });
				fi.arguments.push_back({ Variant::FLOAT, "post" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				ExtensionDB::_singleton->_functions["cubic_interpolate"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cubic_interpolate");
			}
			{
				FunctionInfo fi;
				fi.name = "cubic_interpolate_angle";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "pre" });
				fi.arguments.push_back({ Variant::FLOAT, "post" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				ExtensionDB::_singleton->_functions["cubic_interpolate_angle"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cubic_interpolate_angle");
			}
			{
				FunctionInfo fi;
				fi.name = "cubic_interpolate_in_time";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "pre" });
				fi.arguments.push_back({ Variant::FLOAT, "post" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				fi.arguments.push_back({ Variant::FLOAT, "to_t" });
				fi.arguments.push_back({ Variant::FLOAT, "pre_t" });
				fi.arguments.push_back({ Variant::FLOAT, "post_t" });
				ExtensionDB::_singleton->_functions["cubic_interpolate_in_time"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cubic_interpolate_in_time");
			}
			{
				FunctionInfo fi;
				fi.name = "cubic_interpolate_angle_in_time";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "pre" });
				fi.arguments.push_back({ Variant::FLOAT, "post" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				fi.arguments.push_back({ Variant::FLOAT, "to_t" });
				fi.arguments.push_back({ Variant::FLOAT, "pre_t" });
				fi.arguments.push_back({ Variant::FLOAT, "post_t" });
				ExtensionDB::_singleton->_functions["cubic_interpolate_angle_in_time"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("cubic_interpolate_angle_in_time");
			}
			{
				FunctionInfo fi;
				fi.name = "bezier_interpolate";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "start" });
				fi.arguments.push_back({ Variant::FLOAT, "control_1" });
				fi.arguments.push_back({ Variant::FLOAT, "control_2" });
				fi.arguments.push_back({ Variant::FLOAT, "end" });
				fi.arguments.push_back({ Variant::FLOAT, "t" });
				ExtensionDB::_singleton->_functions["bezier_interpolate"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("bezier_interpolate");
			}
			{
				FunctionInfo fi;
				fi.name = "bezier_derivative";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "start" });
				fi.arguments.push_back({ Variant::FLOAT, "control_1" });
				fi.arguments.push_back({ Variant::FLOAT, "control_2" });
				fi.arguments.push_back({ Variant::FLOAT, "end" });
				fi.arguments.push_back({ Variant::FLOAT, "t" });
				ExtensionDB::_singleton->_functions["bezier_derivative"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("bezier_derivative");
			}
			{
				FunctionInfo fi;
				fi.name = "angle_difference";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				ExtensionDB::_singleton->_functions["angle_difference"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("angle_difference");
			}
			{
				FunctionInfo fi;
				fi.name = "lerp_angle";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				ExtensionDB::_singleton->_functions["lerp_angle"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("lerp_angle");
			}
			{
				FunctionInfo fi;
				fi.name = "inverse_lerp";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "weight" });
				ExtensionDB::_singleton->_functions["inverse_lerp"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("inverse_lerp");
			}
			{
				FunctionInfo fi;
				fi.name = "remap";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "value" });
				fi.arguments.push_back({ Variant::FLOAT, "istart" });
				fi.arguments.push_back({ Variant::FLOAT, "istop" });
				fi.arguments.push_back({ Variant::FLOAT, "ostart" });
				fi.arguments.push_back({ Variant::FLOAT, "ostop" });
				ExtensionDB::_singleton->_functions["remap"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("remap");
			}
			{
				FunctionInfo fi;
				fi.name = "smoothstep";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "x" });
				ExtensionDB::_singleton->_functions["smoothstep"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("smoothstep");
			}
			{
				FunctionInfo fi;
				fi.name = "move_toward";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "delta" });
				ExtensionDB::_singleton->_functions["move_toward"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("move_toward");
			}
			{
				FunctionInfo fi;
				fi.name = "rotate_toward";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				fi.arguments.push_back({ Variant::FLOAT, "delta" });
				ExtensionDB::_singleton->_functions["rotate_toward"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("rotate_toward");
			}
			{
				FunctionInfo fi;
				fi.name = "deg_to_rad";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "deg" });
				ExtensionDB::_singleton->_functions["deg_to_rad"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("deg_to_rad");
			}
			{
				FunctionInfo fi;
				fi.name = "rad_to_deg";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "rad" });
				ExtensionDB::_singleton->_functions["rad_to_deg"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("rad_to_deg");
			}
			{
				FunctionInfo fi;
				fi.name = "linear_to_db";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "lin" });
				ExtensionDB::_singleton->_functions["linear_to_db"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("linear_to_db");
			}
			{
				FunctionInfo fi;
				fi.name = "db_to_linear";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "db" });
				ExtensionDB::_singleton->_functions["db_to_linear"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("db_to_linear");
			}
			{
				FunctionInfo fi;
				fi.name = "wrap";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "value" });
				fi.arguments.push_back({ Variant::NIL, "min" });
				fi.arguments.push_back({ Variant::NIL, "max" });
				ExtensionDB::_singleton->_functions["wrap"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("wrap");
			}
			{
				FunctionInfo fi;
				fi.name = "wrapi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "value" });
				fi.arguments.push_back({ Variant::INT, "min" });
				fi.arguments.push_back({ Variant::INT, "max" });
				ExtensionDB::_singleton->_functions["wrapi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("wrapi");
			}
			{
				FunctionInfo fi;
				fi.name = "wrapf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "value" });
				fi.arguments.push_back({ Variant::FLOAT, "min" });
				fi.arguments.push_back({ Variant::FLOAT, "max" });
				ExtensionDB::_singleton->_functions["wrapf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("wrapf");
			}
			{
				FunctionInfo fi;
				fi.name = "max";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				fi.arguments.push_back({ Variant::NIL, "arg2" });
				ExtensionDB::_singleton->_functions["max"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("max");
			}
			{
				FunctionInfo fi;
				fi.name = "maxi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "a" });
				fi.arguments.push_back({ Variant::INT, "b" });
				ExtensionDB::_singleton->_functions["maxi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("maxi");
			}
			{
				FunctionInfo fi;
				fi.name = "maxf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "a" });
				fi.arguments.push_back({ Variant::FLOAT, "b" });
				ExtensionDB::_singleton->_functions["maxf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("maxf");
			}
			{
				FunctionInfo fi;
				fi.name = "min";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				fi.arguments.push_back({ Variant::NIL, "arg2" });
				ExtensionDB::_singleton->_functions["min"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("min");
			}
			{
				FunctionInfo fi;
				fi.name = "mini";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "a" });
				fi.arguments.push_back({ Variant::INT, "b" });
				ExtensionDB::_singleton->_functions["mini"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("mini");
			}
			{
				FunctionInfo fi;
				fi.name = "minf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "a" });
				fi.arguments.push_back({ Variant::FLOAT, "b" });
				ExtensionDB::_singleton->_functions["minf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("minf");
			}
			{
				FunctionInfo fi;
				fi.name = "clamp";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "value" });
				fi.arguments.push_back({ Variant::NIL, "min" });
				fi.arguments.push_back({ Variant::NIL, "max" });
				ExtensionDB::_singleton->_functions["clamp"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("clamp");
			}
			{
				FunctionInfo fi;
				fi.name = "clampi";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "value" });
				fi.arguments.push_back({ Variant::INT, "min" });
				fi.arguments.push_back({ Variant::INT, "max" });
				ExtensionDB::_singleton->_functions["clampi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("clampi");
			}
			{
				FunctionInfo fi;
				fi.name = "clampf";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "value" });
				fi.arguments.push_back({ Variant::FLOAT, "min" });
				fi.arguments.push_back({ Variant::FLOAT, "max" });
				ExtensionDB::_singleton->_functions["clampf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("clampf");
			}
			{
				FunctionInfo fi;
				fi.name = "nearest_po2";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "value" });
				ExtensionDB::_singleton->_functions["nearest_po2"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("nearest_po2");
			}
			{
				FunctionInfo fi;
				fi.name = "pingpong";
				fi.category = "math";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "value" });
				fi.arguments.push_back({ Variant::FLOAT, "length" });
				ExtensionDB::_singleton->_functions["pingpong"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("pingpong");
			}
			{
				FunctionInfo fi;
				fi.name = "randomize";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = false;
				ExtensionDB::_singleton->_functions["randomize"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randomize");
			}
			{
				FunctionInfo fi;
				fi.name = "randi";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				ExtensionDB::_singleton->_functions["randi"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randi");
			}
			{
				FunctionInfo fi;
				fi.name = "randf";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				ExtensionDB::_singleton->_functions["randf"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randf");
			}
			{
				FunctionInfo fi;
				fi.name = "randi_range";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "from" });
				fi.arguments.push_back({ Variant::INT, "to" });
				ExtensionDB::_singleton->_functions["randi_range"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randi_range");
			}
			{
				FunctionInfo fi;
				fi.name = "randf_range";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "from" });
				fi.arguments.push_back({ Variant::FLOAT, "to" });
				ExtensionDB::_singleton->_functions["randf_range"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randf_range");
			}
			{
				FunctionInfo fi;
				fi.name = "randfn";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::FLOAT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::FLOAT, "mean" });
				fi.arguments.push_back({ Variant::FLOAT, "deviation" });
				ExtensionDB::_singleton->_functions["randfn"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("randfn");
			}
			{
				FunctionInfo fi;
				fi.name = "seed";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "base" });
				ExtensionDB::_singleton->_functions["seed"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("seed");
			}
			{
				FunctionInfo fi;
				fi.name = "rand_from_seed";
				fi.category = "random";
				fi.return_val = PropertyInfo(Variant::PACKED_INT64_ARRAY, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "seed" });
				ExtensionDB::_singleton->_functions["rand_from_seed"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("rand_from_seed");
			}
			{
				FunctionInfo fi;
				fi.name = "weakref";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "obj" });
				ExtensionDB::_singleton->_functions["weakref"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("weakref");
			}
			{
				FunctionInfo fi;
				fi.name = "typeof";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variable" });
				ExtensionDB::_singleton->_functions["typeof"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("typeof");
			}
			{
				FunctionInfo fi;
				fi.name = "type_convert";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variant" });
				fi.arguments.push_back({ Variant::INT, "type" });
				ExtensionDB::_singleton->_functions["type_convert"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("type_convert");
			}
			{
				FunctionInfo fi;
				fi.name = "str";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::STRING, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["str"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("str");
			}
			{
				FunctionInfo fi;
				fi.name = "error_string";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::STRING, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "error" });
				ExtensionDB::_singleton->_functions["error_string"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("error_string");
			}
			{
				FunctionInfo fi;
				fi.name = "type_string";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::STRING, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "type" });
				ExtensionDB::_singleton->_functions["type_string"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("type_string");
			}
			{
				FunctionInfo fi;
				fi.name = "print";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["print"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("print");
			}
			{
				FunctionInfo fi;
				fi.name = "print_rich";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["print_rich"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("print_rich");
			}
			{
				FunctionInfo fi;
				fi.name = "printerr";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["printerr"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("printerr");
			}
			{
				FunctionInfo fi;
				fi.name = "printt";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["printt"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("printt");
			}
			{
				FunctionInfo fi;
				fi.name = "prints";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["prints"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("prints");
			}
			{
				FunctionInfo fi;
				fi.name = "printraw";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["printraw"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("printraw");
			}
			{
				FunctionInfo fi;
				fi.name = "print_verbose";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["print_verbose"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("print_verbose");
			}
			{
				FunctionInfo fi;
				fi.name = "push_error";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["push_error"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("push_error");
			}
			{
				FunctionInfo fi;
				fi.name = "push_warning";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "");
				fi.is_vararg = true;
				fi.arguments.push_back({ Variant::NIL, "arg1" });
				ExtensionDB::_singleton->_functions["push_warning"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("push_warning");
			}
			{
				FunctionInfo fi;
				fi.name = "var_to_str";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::STRING, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variable" });
				ExtensionDB::_singleton->_functions["var_to_str"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("var_to_str");
			}
			{
				FunctionInfo fi;
				fi.name = "str_to_var";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::STRING, "string" });
				ExtensionDB::_singleton->_functions["str_to_var"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("str_to_var");
			}
			{
				FunctionInfo fi;
				fi.name = "var_to_bytes";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::PACKED_BYTE_ARRAY, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variable" });
				ExtensionDB::_singleton->_functions["var_to_bytes"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("var_to_bytes");
			}
			{
				FunctionInfo fi;
				fi.name = "bytes_to_var";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::PACKED_BYTE_ARRAY, "bytes" });
				ExtensionDB::_singleton->_functions["bytes_to_var"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("bytes_to_var");
			}
			{
				FunctionInfo fi;
				fi.name = "var_to_bytes_with_objects";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::PACKED_BYTE_ARRAY, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variable" });
				ExtensionDB::_singleton->_functions["var_to_bytes_with_objects"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("var_to_bytes_with_objects");
			}
			{
				FunctionInfo fi;
				fi.name = "bytes_to_var_with_objects";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::PACKED_BYTE_ARRAY, "bytes" });
				ExtensionDB::_singleton->_functions["bytes_to_var_with_objects"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("bytes_to_var_with_objects");
			}
			{
				FunctionInfo fi;
				fi.name = "hash";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "variable" });
				ExtensionDB::_singleton->_functions["hash"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("hash");
			}
			{
				FunctionInfo fi;
				fi.name = "instance_from_id";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::OBJECT, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "instance_id" });
				ExtensionDB::_singleton->_functions["instance_from_id"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("instance_from_id");
			}
			{
				FunctionInfo fi;
				fi.name = "is_instance_id_valid";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "id" });
				ExtensionDB::_singleton->_functions["is_instance_id_valid"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_instance_id_valid");
			}
			{
				FunctionInfo fi;
				fi.name = "is_instance_valid";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "instance" });
				ExtensionDB::_singleton->_functions["is_instance_valid"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_instance_valid");
			}
			{
				FunctionInfo fi;
				fi.name = "rid_allocate_id";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::INT, "");
				fi.is_vararg = false;
				ExtensionDB::_singleton->_functions["rid_allocate_id"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("rid_allocate_id");
			}
			{
				FunctionInfo fi;
				fi.name = "rid_from_int64";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::RID, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::INT, "base" });
				ExtensionDB::_singleton->_functions["rid_from_int64"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("rid_from_int64");
			}
			{
				FunctionInfo fi;
				fi.name = "is_same";
				fi.category = "general";
				fi.return_val = PropertyInfo(Variant::BOOL, "");
				fi.is_vararg = false;
				fi.arguments.push_back({ Variant::NIL, "a" });
				fi.arguments.push_back({ Variant::NIL, "b" });
				ExtensionDB::_singleton->_functions["is_same"] = fi;
				ExtensionDB::_singleton->_function_names.push_back("is_same");
			}
		}
		
		void ExtensionDBLoader::prime_class_details()
		{
			// Class details
			// This currently only loads classes that have bitfield enums; use ClassDB otherwise.
			// Can eventually be replaced by: https://github.com/godotengine/godot/pull/90368
			
			// AudioStreamOggVorbis
			ExtensionDB::_singleton->_classes["AudioStreamOggVorbis"].name = "AudioStreamOggVorbis";
			ExtensionDB::_singleton->_classes["AudioStreamOggVorbis"].static_function_hashes["load_from_buffer"] = 354904730;
			ExtensionDB::_singleton->_classes["AudioStreamOggVorbis"].static_function_hashes["load_from_file"] = 797568536;
			
			// Control
			ExtensionDB::_singleton->_classes["Control"].name = "Control";
			ExtensionDB::_singleton->_classes["Control"].bitfield_enums.push_back("SizeFlags");
			
			// DirAccess
			ExtensionDB::_singleton->_classes["DirAccess"].name = "DirAccess";
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["open"] = 1923528528;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["get_open_error"] = 166280745;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["get_files_at"] = 3538744774;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["get_directories_at"] = 3538744774;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["get_drive_count"] = 2455072627;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["get_drive_name"] = 990163283;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["make_dir_absolute"] = 166001499;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["make_dir_recursive_absolute"] = 166001499;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["dir_exists_absolute"] = 2323990056;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["copy_absolute"] = 1063198817;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["rename_absolute"] = 852856452;
			ExtensionDB::_singleton->_classes["DirAccess"].static_function_hashes["remove_absolute"] = 166001499;
			
			// FileAccess
			ExtensionDB::_singleton->_classes["FileAccess"].name = "FileAccess";
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["open"] = 1247358404;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["open_encrypted"] = 1482131466;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["open_encrypted_with_pass"] = 790283377;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["open_compressed"] = 3686439335;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_open_error"] = 166280745;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_file_as_bytes"] = 659035735;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_file_as_string"] = 1703090593;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_md5"] = 1703090593;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_sha256"] = 1703090593;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["file_exists"] = 2323990056;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_modified_time"] = 1597066294;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_unix_permissions"] = 524341837;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["set_unix_permissions"] = 846038644;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_hidden_attribute"] = 2323990056;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["set_hidden_attribute"] = 2892558115;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["set_read_only_attribute"] = 2892558115;
			ExtensionDB::_singleton->_classes["FileAccess"].static_function_hashes["get_read_only_attribute"] = 2323990056;
			ExtensionDB::_singleton->_classes["FileAccess"].bitfield_enums.push_back("UnixPermissionFlags");
			
			// FramebufferCacheRD
			ExtensionDB::_singleton->_classes["FramebufferCacheRD"].name = "FramebufferCacheRD";
			ExtensionDB::_singleton->_classes["FramebufferCacheRD"].static_function_hashes["get_cache_multipass"] = 3437881813;
			
			// GLTFCamera
			ExtensionDB::_singleton->_classes["GLTFCamera"].name = "GLTFCamera";
			ExtensionDB::_singleton->_classes["GLTFCamera"].static_function_hashes["from_node"] = 237784;
			ExtensionDB::_singleton->_classes["GLTFCamera"].static_function_hashes["from_dictionary"] = 2495512509;
			
			// GLTFDocument
			ExtensionDB::_singleton->_classes["GLTFDocument"].name = "GLTFDocument";
			ExtensionDB::_singleton->_classes["GLTFDocument"].static_function_hashes["register_gltf_document_extension"] = 3752678331;
			ExtensionDB::_singleton->_classes["GLTFDocument"].static_function_hashes["unregister_gltf_document_extension"] = 2684415758;
			
			// GLTFLight
			ExtensionDB::_singleton->_classes["GLTFLight"].name = "GLTFLight";
			ExtensionDB::_singleton->_classes["GLTFLight"].static_function_hashes["from_node"] = 3907677874;
			ExtensionDB::_singleton->_classes["GLTFLight"].static_function_hashes["from_dictionary"] = 4057087208;
			
			// GLTFPhysicsBody
			ExtensionDB::_singleton->_classes["GLTFPhysicsBody"].name = "GLTFPhysicsBody";
			ExtensionDB::_singleton->_classes["GLTFPhysicsBody"].static_function_hashes["from_node"] = 420544174;
			ExtensionDB::_singleton->_classes["GLTFPhysicsBody"].static_function_hashes["from_dictionary"] = 1177544336;
			
			// GLTFPhysicsShape
			ExtensionDB::_singleton->_classes["GLTFPhysicsShape"].name = "GLTFPhysicsShape";
			ExtensionDB::_singleton->_classes["GLTFPhysicsShape"].static_function_hashes["from_node"] = 3613751275;
			ExtensionDB::_singleton->_classes["GLTFPhysicsShape"].static_function_hashes["from_resource"] = 3845569786;
			ExtensionDB::_singleton->_classes["GLTFPhysicsShape"].static_function_hashes["from_dictionary"] = 2390691823;
			
			// Image
			ExtensionDB::_singleton->_classes["Image"].name = "Image";
			ExtensionDB::_singleton->_classes["Image"].static_function_hashes["create"] = 986942177;
			ExtensionDB::_singleton->_classes["Image"].static_function_hashes["create_empty"] = 986942177;
			ExtensionDB::_singleton->_classes["Image"].static_function_hashes["create_from_data"] = 299398494;
			ExtensionDB::_singleton->_classes["Image"].static_function_hashes["load_from_file"] = 736337515;
			
			// ImageFormatLoader
			ExtensionDB::_singleton->_classes["ImageFormatLoader"].name = "ImageFormatLoader";
			ExtensionDB::_singleton->_classes["ImageFormatLoader"].bitfield_enums.push_back("LoaderFlags");
			
			// ImageTexture
			ExtensionDB::_singleton->_classes["ImageTexture"].name = "ImageTexture";
			ExtensionDB::_singleton->_classes["ImageTexture"].static_function_hashes["create_from_image"] = 2775144163;
			
			// JSON
			ExtensionDB::_singleton->_classes["JSON"].name = "JSON";
			ExtensionDB::_singleton->_classes["JSON"].static_function_hashes["stringify"] = 462733549;
			ExtensionDB::_singleton->_classes["JSON"].static_function_hashes["parse_string"] = 309047738;
			
			// Mesh
			ExtensionDB::_singleton->_classes["Mesh"].name = "Mesh";
			ExtensionDB::_singleton->_classes["Mesh"].bitfield_enums.push_back("ArrayFormat");
			
			// MovieWriter
			ExtensionDB::_singleton->_classes["MovieWriter"].name = "MovieWriter";
			ExtensionDB::_singleton->_classes["MovieWriter"].static_function_hashes["add_writer"] = 4023702871;
			
			// MultiplayerAPI
			ExtensionDB::_singleton->_classes["MultiplayerAPI"].name = "MultiplayerAPI";
			ExtensionDB::_singleton->_classes["MultiplayerAPI"].static_function_hashes["set_default_interface"] = 3304788590;
			ExtensionDB::_singleton->_classes["MultiplayerAPI"].static_function_hashes["get_default_interface"] = 2737447660;
			ExtensionDB::_singleton->_classes["MultiplayerAPI"].static_function_hashes["create_default_interface"] = 3294156723;
			
			// NavigationPathQueryParameters2D
			ExtensionDB::_singleton->_classes["NavigationPathQueryParameters2D"].name = "NavigationPathQueryParameters2D";
			ExtensionDB::_singleton->_classes["NavigationPathQueryParameters2D"].bitfield_enums.push_back("PathMetadataFlags");
			
			// NavigationPathQueryParameters3D
			ExtensionDB::_singleton->_classes["NavigationPathQueryParameters3D"].name = "NavigationPathQueryParameters3D";
			ExtensionDB::_singleton->_classes["NavigationPathQueryParameters3D"].bitfield_enums.push_back("PathMetadataFlags");
			
			// Node
			ExtensionDB::_singleton->_classes["Node"].name = "Node";
			ExtensionDB::_singleton->_classes["Node"].static_function_hashes["print_orphan_nodes"] = 3218959716;
			ExtensionDB::_singleton->_classes["Node"].bitfield_enums.push_back("ProcessThreadMessages");
			
			// OpenXRAPIExtension
			ExtensionDB::_singleton->_classes["OpenXRAPIExtension"].name = "OpenXRAPIExtension";
			ExtensionDB::_singleton->_classes["OpenXRAPIExtension"].static_function_hashes["openxr_is_enabled"] = 2703660260;
			
			// OpenXRInterface
			ExtensionDB::_singleton->_classes["OpenXRInterface"].name = "OpenXRInterface";
			ExtensionDB::_singleton->_classes["OpenXRInterface"].bitfield_enums.push_back("HandJointFlags");
			
			// PathFollow3D
			ExtensionDB::_singleton->_classes["PathFollow3D"].name = "PathFollow3D";
			ExtensionDB::_singleton->_classes["PathFollow3D"].static_function_hashes["correct_posture"] = 2686588690;
			
			// PhysicsRayQueryParameters2D
			ExtensionDB::_singleton->_classes["PhysicsRayQueryParameters2D"].name = "PhysicsRayQueryParameters2D";
			ExtensionDB::_singleton->_classes["PhysicsRayQueryParameters2D"].static_function_hashes["create"] = 3196569324;
			
			// PhysicsRayQueryParameters3D
			ExtensionDB::_singleton->_classes["PhysicsRayQueryParameters3D"].name = "PhysicsRayQueryParameters3D";
			ExtensionDB::_singleton->_classes["PhysicsRayQueryParameters3D"].static_function_hashes["create"] = 3110599579;
			
			// PortableCompressedTexture2D
			ExtensionDB::_singleton->_classes["PortableCompressedTexture2D"].name = "PortableCompressedTexture2D";
			ExtensionDB::_singleton->_classes["PortableCompressedTexture2D"].static_function_hashes["set_keep_all_compressed_buffers"] = 2586408642;
			ExtensionDB::_singleton->_classes["PortableCompressedTexture2D"].static_function_hashes["is_keeping_all_compressed_buffers"] = 2240911060;
			
			// RegEx
			ExtensionDB::_singleton->_classes["RegEx"].name = "RegEx";
			ExtensionDB::_singleton->_classes["RegEx"].static_function_hashes["create_from_string"] = 2150300909;
			
			// RenderingDevice
			ExtensionDB::_singleton->_classes["RenderingDevice"].name = "RenderingDevice";
			ExtensionDB::_singleton->_classes["RenderingDevice"].bitfield_enums.push_back("BarrierMask");
			ExtensionDB::_singleton->_classes["RenderingDevice"].bitfield_enums.push_back("TextureUsageBits");
			ExtensionDB::_singleton->_classes["RenderingDevice"].bitfield_enums.push_back("StorageBufferUsage");
			ExtensionDB::_singleton->_classes["RenderingDevice"].bitfield_enums.push_back("PipelineDynamicStateFlags");
			
			// RenderingServer
			ExtensionDB::_singleton->_classes["RenderingServer"].name = "RenderingServer";
			ExtensionDB::_singleton->_classes["RenderingServer"].bitfield_enums.push_back("ArrayFormat");
			
			// Resource
			ExtensionDB::_singleton->_classes["Resource"].name = "Resource";
			ExtensionDB::_singleton->_classes["Resource"].static_function_hashes["generate_scene_unique_id"] = 2841200299;
			
			// ResourceImporterOggVorbis
			ExtensionDB::_singleton->_classes["ResourceImporterOggVorbis"].name = "ResourceImporterOggVorbis";
			ExtensionDB::_singleton->_classes["ResourceImporterOggVorbis"].static_function_hashes["load_from_buffer"] = 354904730;
			ExtensionDB::_singleton->_classes["ResourceImporterOggVorbis"].static_function_hashes["load_from_file"] = 797568536;
			
			// ResourceSaver
			ExtensionDB::_singleton->_classes["ResourceSaver"].name = "ResourceSaver";
			ExtensionDB::_singleton->_classes["ResourceSaver"].bitfield_enums.push_back("SaverFlags");
			
			// RichTextLabel
			ExtensionDB::_singleton->_classes["RichTextLabel"].name = "RichTextLabel";
			ExtensionDB::_singleton->_classes["RichTextLabel"].bitfield_enums.push_back("ImageUpdateMask");
			
			// TLSOptions
			ExtensionDB::_singleton->_classes["TLSOptions"].name = "TLSOptions";
			ExtensionDB::_singleton->_classes["TLSOptions"].static_function_hashes["client"] = 3565000357;
			ExtensionDB::_singleton->_classes["TLSOptions"].static_function_hashes["client_unsafe"] = 2090251749;
			ExtensionDB::_singleton->_classes["TLSOptions"].static_function_hashes["server"] = 36969539;
			
			// TextServer
			ExtensionDB::_singleton->_classes["TextServer"].name = "TextServer";
			ExtensionDB::_singleton->_classes["TextServer"].bitfield_enums.push_back("JustificationFlag");
			ExtensionDB::_singleton->_classes["TextServer"].bitfield_enums.push_back("LineBreakFlag");
			ExtensionDB::_singleton->_classes["TextServer"].bitfield_enums.push_back("TextOverrunFlag");
			ExtensionDB::_singleton->_classes["TextServer"].bitfield_enums.push_back("GraphemeFlag");
			ExtensionDB::_singleton->_classes["TextServer"].bitfield_enums.push_back("FontStyle");
			
			// Thread
			ExtensionDB::_singleton->_classes["Thread"].name = "Thread";
			ExtensionDB::_singleton->_classes["Thread"].static_function_hashes["set_thread_safety_checks_enabled"] = 2586408642;
			
			// Tween
			ExtensionDB::_singleton->_classes["Tween"].name = "Tween";
			ExtensionDB::_singleton->_classes["Tween"].static_function_hashes["interpolate_value"] = 3452526450;
			
			// UniformSetCacheRD
			ExtensionDB::_singleton->_classes["UniformSetCacheRD"].name = "UniformSetCacheRD";
			ExtensionDB::_singleton->_classes["UniformSetCacheRD"].static_function_hashes["get_cache"] = 658571723;
			
			// WebRTCPeerConnection
			ExtensionDB::_singleton->_classes["WebRTCPeerConnection"].name = "WebRTCPeerConnection";
			ExtensionDB::_singleton->_classes["WebRTCPeerConnection"].static_function_hashes["set_default_extension"] = 3304788590;
			
			// XRBodyModifier3D
			ExtensionDB::_singleton->_classes["XRBodyModifier3D"].name = "XRBodyModifier3D";
			ExtensionDB::_singleton->_classes["XRBodyModifier3D"].bitfield_enums.push_back("BodyUpdate");
			
			// XRBodyTracker
			ExtensionDB::_singleton->_classes["XRBodyTracker"].name = "XRBodyTracker";
			ExtensionDB::_singleton->_classes["XRBodyTracker"].bitfield_enums.push_back("BodyFlags");
			ExtensionDB::_singleton->_classes["XRBodyTracker"].bitfield_enums.push_back("JointFlags");
			
			// XRHandTracker
			ExtensionDB::_singleton->_classes["XRHandTracker"].name = "XRHandTracker";
			ExtensionDB::_singleton->_classes["XRHandTracker"].bitfield_enums.push_back("HandJointFlags");
		}
		
		void ExtensionDBLoader::prime()
		{
			prime_math_constants();
			prime_global_enumerations();
			prime_builtin_classes();
			prime_utility_functions();
			prime_class_details();
		}

    }

    ExtensionDB::ExtensionDB() { _singleton = this; }
    ExtensionDB::~ExtensionDB() { _singleton = nullptr; }

    PackedStringArray ExtensionDB::get_builtin_type_names()
    {
        return ExtensionDB::_singleton->_builtin_type_names;
    }

    BuiltInType ExtensionDB::get_builtin_type(const StringName& p_name)
    {
        return ExtensionDB::_singleton->_builtin_types[p_name];
    }

    BuiltInType ExtensionDB::get_builtin_type(Variant::Type p_type)
    {
        return ExtensionDB::_singleton->_builtin_types[ExtensionDB::_singleton->_builtin_types_to_name[p_type]];
    }

    PackedStringArray ExtensionDB::get_global_enum_names()
    {
        return ExtensionDB::_singleton->_global_enum_names;
    }

    PackedStringArray ExtensionDB::get_global_enum_value_names()
    {
        return ExtensionDB::_singleton->_global_enum_value_names;
    }

    EnumInfo ExtensionDB::get_global_enum(const StringName& p_name)
    {
        return ExtensionDB::_singleton->_global_enums[p_name];
    }

    EnumValue ExtensionDB::get_global_enum_value(const StringName& p_name)
    {
        for (const KeyValue<StringName, EnumInfo>& E : ExtensionDB::_singleton->_global_enums)
            for (const EnumValue& ev : E.value.values)
                if (ev.name.match(p_name))
                    return ev;
        return {};
    }

    PackedStringArray ExtensionDB::get_math_constant_names()
    {
        return ExtensionDB::_singleton->_math_constant_names;
    }

    ConstantInfo ExtensionDB::get_math_constant(const StringName& p_name)
    {
        return ExtensionDB::_singleton->_math_constants[p_name];
    }

    PackedStringArray ExtensionDB::get_function_names()
    {
        return ExtensionDB::_singleton->_function_names;
    }

    FunctionInfo ExtensionDB::get_function(const StringName& p_name)
    {
        return ExtensionDB::_singleton->_functions[p_name];
    }

    bool ExtensionDB::is_class_enum_bitfield(const StringName& p_class_name, const String& p_enum_name)
    {
        if (ExtensionDB::_singleton->_classes.has(p_class_name))
            return ExtensionDB::_singleton->_classes[p_class_name].bitfield_enums.has(p_enum_name);
        return false;
    }

    PackedStringArray ExtensionDB::get_static_function_names(const StringName& p_class_name)
    {
        PackedStringArray values;
        if (ExtensionDB::_singleton->_classes.has(p_class_name))
        {
            for (const KeyValue<StringName, int64_t>& E : ExtensionDB::_singleton->_classes[p_class_name].static_function_hashes)
                values.push_back(E.key);
        }
        return values;
    }

    int64_t ExtensionDB::get_static_function_hash(const StringName& p_class_name, const StringName& p_function_name)
    {
        if (ExtensionDB::_singleton->_classes.has(p_class_name))
        {
            if (ExtensionDB::_singleton->_classes[p_class_name].static_function_hashes.has(p_function_name))
                return ExtensionDB::_singleton->_classes[p_class_name].static_function_hashes[p_function_name];
        }
        return 0;
    }
}

