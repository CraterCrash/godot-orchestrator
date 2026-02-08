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
#include "script/utility_functions.h"

#include "common/settings.h"
#include "common/version.h"
#include "core/godot/variant/variant.h"
#include "script/language.h"
#include "script/nodes/utilities/print_string.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>

// These methods assume (p_num + p_den) doesn't overflow.
_ALWAYS_INLINE_ int32_t division_round_up(int32_t p_num, int32_t p_den) {
    int32_t offset = (p_num < 0 && p_den < 0) ? 1 : -1;
    return (p_num + p_den + offset) / p_den;
}
_ALWAYS_INLINE_ uint32_t division_round_up(uint32_t p_num, uint32_t p_den) {
    return (p_num + p_den - 1) / p_den;
}
_ALWAYS_INLINE_ int64_t division_round_up(int64_t p_num, int64_t p_den) {
    int32_t offset = (p_num < 0 && p_den < 0) ? 1 : -1;
    return (p_num + p_den + offset) / p_den;
}
_ALWAYS_INLINE_ uint64_t division_round_up(uint64_t p_num, uint64_t p_den) {
    return (p_num + p_den - 1) / p_den;
}

#ifdef DEBUG_ENABLED
#define DEBUG_VALIDATE_ARG_COUNT(m_min_count, m_max_count)                          \
    if (unlikely(p_arg_count < (m_min_count))) {                                    \
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;                   \
        r_error.expected = m_min_count;                                             \
        return;                                                                     \
    }                                                                               \
    if (unlikely(p_arg_count > (m_max_count))) {                                    \
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;                  \
        r_error.expected = m_max_count;                                             \
        return;                                                                     \
    }

#define DEBUG_VALIDATE_ARG_TYPE(m_arg, m_type)                                      \
    if (unlikely(!Variant::can_convert_strict(p_args[m_arg]->get_type(), m_type))) {\
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;                    \
        r_error.argument = m_arg;                                                   \
        r_error.expected = m_type;                                                  \
        return;                                                                     \
    }
#else
#define DEBUG_VALIDATE_ARG_COUNT(m_min_count, m_max_count)
#define DEBUG_VALIDATE_ARG_TYPE(m_arg, m_type)
#endif // DEBUG_ENABLED

#define VALIDATE_ARG_CUSTOM(m_arg, m_type, m_cond, m_msg)                 \
    if (unlikely(m_cond)) {                                               \
        *r_ret = m_msg;                                                   \
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;          \
        r_error.argument = m_arg;                                         \
        r_error.expected = m_type;                                        \
        return;                                                           \
    }

#define OSFUNC_FAIL_COND_MSG(m_cond, m_msg)                             \
    if (unlikely(m_cond)) {                                             \
        *r_ret = m_msg;                                                 \
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;          \
        return;                                                         \
    }

#define RTR(x) x

struct OScriptUtilityFunctionsDefinitions
{
    /// Checks whether the specified file path exists, handling <code>remap</code> export file names.
    /// This function is intentionally not exposed to the API.
    static bool _file_exists(const String& p_path) {
        if (FileAccess::file_exists(p_path)) {
            return true;
        }

        // In export builds, files will be converted by binary by default, and will have .scn extensions.
        // This handles checking for the file remap.
        return FileAccess::file_exists(vformat("%s.remap", p_path));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OScript Public Functions
    ///
    /// These are functions that we expose as part of the `@OScript` family of methods to the user as nodes that can
    /// be directly called from scripts. These call into the C++ engine directly.

    /// Checks whether the specified class name exists, returning <code>true</code> or <code>false</code>.
    static void type_exists(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::STRING_NAME);
        *r_ret = ClassDB::class_exists(*p_args[0]);
    }

    #if GODOT_VERSION >= 0x040300
    static void print_debug(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        String s;
        for (int i = 0; i < p_arg_count; i++) {
            s += p_args[i]->operator String();
        }

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread == OS::get_singleton()->get_main_thread_id()) {
            OScriptLanguage* script = OScriptLanguage::get_singleton();
            if (script->_debug_get_stack_level_count() > 0) {
                s += "\n    At: "
                    + script->_debug_get_stack_level_source(0)
                    + ":"
                    + itos(script->_debug_get_stack_level_line(0))
                    + ":"
                    + script->_debug_get_stack_level_function(0)
                    + "()";
            }
        } else {
            s += "\n   At: Cannot retrieve debug info outside the main thread. Thread ID: " + itos(caller_thread);
        }

        UtilityFunctions::print(s);
        *r_ret = Variant();
    }

    static void print_stack(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(0, 0);

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread != OS::get_singleton()->get_main_thread_id()) {
            UtilityFunctions::print("Cannot retrieve debug info outside the main thread. Thread ID: " + itos(caller_thread));
            return;
        }

        OScriptLanguage* script = OScriptLanguage::get_singleton();
        for (int i = 0; i < script->_debug_get_stack_level_count(); i++) {
            UtilityFunctions::print(vformat("Frame %d - %s:%d in function '%s'",
                i,
                script->_debug_get_stack_level_source(i),
                script->_debug_get_stack_level_line(i),
                script->_debug_get_stack_level_function(i)));
        }

        *r_ret = Variant();
    }

    static void get_stack(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(0, 0);

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread != OS::get_singleton()->get_main_thread_id()) {
            *r_ret = TypedArray<Dictionary>();
            return;
        }

        OScriptLanguage* script = OScriptLanguage::get_singleton();

        TypedArray<Dictionary> ret;
        for (int i = 0; i < script->_debug_get_stack_level_count(); i++) {
            Dictionary frame;
            frame["source"] = script->_debug_get_stack_level_source(i);
            frame["function"] = script->_debug_get_stack_level_function(i);
            frame["line"] = script->_debug_get_stack_level_line(i);
            ret.push_back(frame);
        }

        *r_ret = ret;
    }
    #endif

    /// Returns the length of the specified Variant input argument.
    static void len(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        switch (p_args[0]->get_type()) {
            case Variant::STRING:
            case Variant::STRING_NAME: {
                String d= *p_args[0];
                *r_ret = d.length();
                break;
            }
            case Variant::DICTIONARY: {
                Dictionary d= *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::ARRAY: {
                Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_BYTE_ARRAY: {
                PackedByteArray d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_INT32_ARRAY: {
                PackedInt32Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_INT64_ARRAY: {
                PackedInt64Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_FLOAT32_ARRAY: {
                PackedFloat32Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_FLOAT64_ARRAY: {
                PackedFloat64Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_STRING_ARRAY: {
                PackedStringArray d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_VECTOR2_ARRAY: {
                PackedVector2Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_VECTOR3_ARRAY: {
                PackedVector3Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            #if GODOT_VERSION >= 0x040300
            case Variant::PACKED_VECTOR4_ARRAY: {
                PackedVector4Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            #endif
            default: {
                *r_ret = vformat("Value of type '%s' cannot provide a length", Variant::get_type_name(p_args[0]->get_type()));
                r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
                r_error.argument = 0;
                r_error.expected = Variant::NIL;
                break;
            }
        }
    }

    /// OScript implementation of GDScript's <code>load</code> keyword, which allows for loading a resource
    /// on-demand. The returned value is the loaded resource reference, if valid.
    static void load(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::STRING);
        *r_ret = ResourceLoader::get_singleton()->load(*p_args[0]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OScript Internal Functions
    ///
    /// These are functions that are used by several node implementations to make building the AST easier. These will
    /// eventually be ported into the AST model so that if the user wishes to see the script in another target
    /// like GDScript, all the information is available. But for now, this simplified the transition to AST.

    /// This is OScript's implementation of the <code>range</code> keyword, that allows for iterating over a
    /// value list. This is primarily used by the <code>ForEach</code> and <code>ForLoop</code> nodes.
    static void _oscript_internal_range(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
		DEBUG_VALIDATE_ARG_COUNT(1, 3);
		switch (p_arg_count) {
			case 1: {
				DEBUG_VALIDATE_ARG_TYPE(0, Variant::INT);

				int64_t count = *p_args[0];

				Array arr;
				if (count <= 0) {
					*r_ret = arr;
					return;
				}

				OSFUNC_FAIL_COND_MSG(count > INT32_MAX, RTR("Range too big."));
				Error err = static_cast<Error>(arr.resize(count));
				OSFUNC_FAIL_COND_MSG(err != OK, RTR("Cannot resize array."));

				for (int64_t i = 0; i < count; i++) {
					arr[i] = i;
				}

				*r_ret = arr;
			    break;
			}
			case 2: {
				DEBUG_VALIDATE_ARG_TYPE(0, Variant::INT);
				DEBUG_VALIDATE_ARG_TYPE(1, Variant::INT);

				int64_t from = *p_args[0];
				int64_t to = *p_args[1];

				Array arr;
				if (from >= to) {
					*r_ret = arr;
					return;
				}

				OSFUNC_FAIL_COND_MSG(to - from > INT32_MAX, RTR("Range too big."));
				Error err = static_cast<Error>(arr.resize(to - from));
				OSFUNC_FAIL_COND_MSG(err != OK, RTR("Cannot resize array."));

				for (int64_t i = from; i < to; i++) {
					arr[i - from] = i;
				}

				*r_ret = arr;
			    break;
			}
			case 3: {
				DEBUG_VALIDATE_ARG_TYPE(0, Variant::INT);
				DEBUG_VALIDATE_ARG_TYPE(1, Variant::INT);
				DEBUG_VALIDATE_ARG_TYPE(2, Variant::INT);

				int64_t from = *p_args[0];
				int64_t to = *p_args[1];
				int64_t incr = *p_args[2];

				VALIDATE_ARG_CUSTOM(2, Variant::INT, incr == 0, RTR("Step argument is zero!"));

				Array arr;
				if (from >= to && incr > 0) {
					*r_ret = arr;
					return;
				}
				if (from <= to && incr < 0) {
					*r_ret = arr;
					return;
				}

				// Calculate how many.
				int64_t count = 0;
				if (incr > 0) {
					count = division_round_up(to - from, incr);
				} else {
					count = division_round_up(from - to, -incr);
				}

				OSFUNC_FAIL_COND_MSG(count > INT32_MAX, RTR("Range too big."));
				Error err = static_cast<Error>(arr.resize(count));
				OSFUNC_FAIL_COND_MSG(err != OK, RTR("Cannot resize array."));

				if (incr > 0) {
					int64_t idx = 0;
					for (int64_t i = from; i < to; i += incr) {
						arr[idx++] = i;
					}
				} else {
					int64_t idx = 0;
					for (int64_t i = from; i > to; i += incr) {
						arr[idx++] = i;
					}
				}

				*r_ret = arr;
			    break;
			}
		}
	}

    /// A call used by te <code>OScriptNodeInstantiateScene</code> class to load the specified scene from disk, and
    /// if successful, instantiates an instance of it and returns the instance to the caller.
    static void _oscript_internal_instantiate_scene(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::STRING);

        const Ref<PackedScene> resource = ResourceLoader::get_singleton()->load(*p_args[0]);
        if (resource.is_valid()) {
            Node* root = resource->instantiate();
            if (root) {
                *r_ret = root;
                return;
            }
        }

        *r_ret = vformat("Could not find '%s' as a resource or is not a PackedScene.", *p_args[0]);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
    }

    /// A call used by the <code>OScriptNodePrintString</code> class to not only write text to the console output, but
    /// to also print the text as part of the debug UI.
    static void _oscript_internal_print_string(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(6, 6);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::BOOL);
        DEBUG_VALIDATE_ARG_TYPE(1, Variant::NIL);
        DEBUG_VALIDATE_ARG_TYPE(2, Variant::BOOL);
        DEBUG_VALIDATE_ARG_TYPE(3, Variant::BOOL);
        DEBUG_VALIDATE_ARG_TYPE(4, Variant::COLOR);
        DEBUG_VALIDATE_ARG_TYPE(5, Variant::FLOAT);

        if (!*p_args[0] && *p_args[2]) {
            // Overlays are only applicable when print screen is enabled and not in tool scripts.
            OScriptNodePrintStringOverlay* overlay = OScriptLanguage::get_singleton()->get_or_create_overlay();
            if (overlay) {
                overlay->add_text(*p_args[1], "None", *p_args[5], *p_args[4]);
            }
        }

        if (*p_args[3]) {
            // Write to the log
            UtilityFunctions::print(*p_args[1]);
        }

        *r_ret = nullptr;
        r_error.error = GDEXTENSION_CALL_OK;
    }

    /// A call used by the <code>OScriptNodeDialogueMessage</code> class to load and display the dialogue message scene,
    /// allowing the user to select from no or 1 or more options, allowing for deviating conversation flows easily.
    static void _oscript_internal_show_dialogue(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) {
        DEBUG_VALIDATE_ARG_COUNT(3, 3);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::NIL);
        DEBUG_VALIDATE_ARG_TYPE(1, Variant::STRING);
        DEBUG_VALIDATE_ARG_TYPE(2, Variant::DICTIONARY);

        static String default_scene = ORCHESTRATOR_GET("settings/dialogue/default_message_scene", "");

        String scene_path = *p_args[1];
        if (scene_path.is_empty()) {
            scene_path = default_scene;
        }

        if (!_file_exists(scene_path)) {
            *r_ret = vformat("Scene path %s could not be found.", scene_path);
            r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
            r_error.argument = 1;
            return;
        }

        const Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(scene_path);
        if (!scene.is_valid()) {
            *r_ret = vformat("Scene path %s could not be loaded, is it a packed scene?", scene_path);
            r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
            return;
        }
        else if (!scene->can_instantiate()) {
            *r_ret = vformat("Scene path %s could not be instantiated.", scene_path);
            r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
            return;
        }

        Node* parent_node = Object::cast_to<Node>(*p_args[0]);
        if (!parent_node) {
            *r_ret = vformat("Unable to locate parent scene node. The dialogue message cannot be shown.");
            r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
            return;
        }

        Node* parent_node_root = parent_node->get_tree()->get_current_scene();
        if (!parent_node_root) {
            parent_node_root = parent_node->get_tree()->get_root()->get_child(0);
        }

        if (!parent_node_root) {
            *r_ret = vformat("Unable to locate scene root node. The dialogue message cannot be shown.");
            r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
            return;
        }

        Dictionary data = *p_args[2];
        if (data.has("options")) {
            Dictionary options = data["options"];
            const Array keys = options.keys();
            for (uint32_t i = 0; i < keys.size(); i++) {
                const Variant& key = keys[i];
                Dictionary choice = options[key];
                if (choice.has("visible") && choice["visible"]) {
                    options[key] = choice["text"];
                } else {
                    options.erase(key);
                }
            }
            data["options"] = options;
        } else {
            data["options"] = Dictionary();
        }

        Node* scene_root = scene->instantiate();
        scene_root->set("dialogue_data", data);

        if (!parent_node_root->is_node_ready()) {
            parent_node_root->call_deferred("add_child", scene_root);
        } else {
            parent_node_root->add_child(scene_root);
        }

        *r_ret = scene_root;
        r_error.error = GDEXTENSION_CALL_OK;
    }
};

struct OScriptUtilityFunctionInfo {
    OScriptUtilityFunctions::FunctionPtr function{ nullptr };
    MethodInfo info;
    bool is_const{ false };
    bool is_internal{ false };
};

static HashMap<StringName, OScriptUtilityFunctionInfo> utility_function_table;
static List<StringName> utility_function_name_table;

static void _register_function(const StringName& p_name, const MethodInfo& p_method, OScriptUtilityFunctions::FunctionPtr p_function, bool p_is_const, bool p_is_internal) {
    ERR_FAIL_COND(utility_function_table.has(p_name));

    OScriptUtilityFunctionInfo function;
    function.function = p_function;
    function.info = p_method;
    function.is_const = p_is_const;
    function.is_internal = p_is_internal;

    utility_function_table.insert(p_name, function);
    utility_function_name_table.push_back(p_name);
}

#define REGISTER_FUNC(m_func, m_is_const, m_return, m_args, m_is_vararg, m_default_args, m_is_internal) {       \
        String name(#m_func);                                                                                   \
                                                                                                                \
        MethodInfo info = m_args;                                                                               \
        info.name = name;                                                                                       \
        info.return_val = m_return;                                                                             \
        for (uint32_t i = 0; i < m_default_args.size(); i++) {                                                  \
            info.default_arguments.push_back(m_default_args[i]);                                                \
        }                                                                                                       \
        if (m_is_vararg) {                                                                                      \
            info.flags |= METHOD_FLAG_VARARG;                                                                   \
        }                                                                                                       \
        _register_function(name, info, OScriptUtilityFunctionsDefinitions::m_func, m_is_const, m_is_internal);  \
    }

#define RET(m_type)             PropertyInfo(Variant::m_type, "")
#define RETVAR                  PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)
#define RETCLS(m_class)         PropertyInfo(Variant::OBJECT, "", PROPERTY_HINT_RESOURCE_TYPE, m_class)

#define NOARGS                  MethodInfo()
#define ARGS(...)               MethodInfo("", __VA_ARGS__)
#define ARG(m_name, m_type)     PropertyInfo(Variant::m_type, m_name)
#define ARGVAR(m_name)          PropertyInfo(Variant::NIL, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)
#define ARGTYPE(m_name)         PropertyInfo(Variant::INT, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CLASS_IS_ENUM, "Variant.Type")

void OScriptUtilityFunctions::register_functions() {

    #if GODOT_VERSION >= 0x040300
    REGISTER_FUNC(print_debug, false, RET(NIL), NOARGS, true, varray(), false);
    REGISTER_FUNC(print_stack, false, RET(NIL), NOARGS, false, varray(), false);
    REGISTER_FUNC(get_stack, false, RET(ARRAY), NOARGS, false, varray(), false);
    #endif

    REGISTER_FUNC(type_exists, true, RET(BOOL), ARGS(ARG("type", STRING_NAME)), false, varray(), false);
    REGISTER_FUNC(len, true, RET(INT), ARGS(ARGVAR("var")), false, varray(), false);
    REGISTER_FUNC(load, false, RETCLS("Resource"), ARGS( ARG("path", STRING) ), false, varray(), false);

    // Internal methods
    // These are used typically by visual script nodes to make the creation of the AST easier, and
    // users should never expect these functions to exist indefinitely.
    REGISTER_FUNC(_oscript_internal_range, false, RET(ARRAY), NOARGS, true, varray(), true);
    REGISTER_FUNC(_oscript_internal_instantiate_scene, false, PropertyInfo(Variant::OBJECT, "", PROPERTY_HINT_NODE_TYPE, "Node", PROPERTY_USAGE_DEFAULT, "Node"), ARGS( ARG("path", STRING) ), false, varray(), true);
    REGISTER_FUNC(_oscript_internal_print_string, false, RET(NIL), ARGS( ARG("is_tool", BOOL), ARGVAR("text"), ARG("print_to_screen", BOOL), ARG("print_to_log", BOOL), ARG("text_color", COLOR), ARG("duration", FLOAT)), false, varray(), true);
    REGISTER_FUNC(_oscript_internal_show_dialogue, false, RETCLS("Node"), ARGS( ARGVAR("parent"), ARG("scene_path", STRING), ARGVAR("options") ), false, varray(), true);
}

void OScriptUtilityFunctions::unregister_functions() {
    utility_function_name_table.clear();
    utility_function_table.clear();
}

OScriptUtilityFunctions::FunctionPtr OScriptUtilityFunctions::get_function(const StringName& p_function_name) {
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, nullptr);

    return info->function;
}

bool OScriptUtilityFunctions::function_exists(const StringName& p_function_name) {
    return utility_function_table.has(p_function_name);
}

List<StringName> OScriptUtilityFunctions::get_function_list() {
    List<StringName> result;
    for (const StringName& E : utility_function_name_table) {
        result.push_back(E);
    }
    return result;
}

MethodInfo OScriptUtilityFunctions::get_function_info(const StringName& p_function_name) {
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, MethodInfo());

    return info->info;
}

int OScriptUtilityFunctions::get_function_argument_count(const StringName& p_function_name) {
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, 0);
    return info->info.arguments.size();
}

bool OScriptUtilityFunctions::is_function_constant(const StringName& p_function_name) {
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, false);
    return info->is_const;
}

bool OScriptUtilityFunctions::is_function_internal(const StringName& p_function_name) {
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, false);
    return info->is_internal;
}
