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
#include "scene_utils.h"

#include "plugin/plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace SceneUtils
{
    Ref<Texture2D> _get_class_or_script_icon(const String& p_class_name, const Ref<Script>& p_script, const String& p_fallback, bool p_fallback_script_to_theme)
    {
        ERR_FAIL_COND_V_MSG(p_class_name.is_empty(), nullptr, "Class name cannot be empty.");

        VBoxContainer* vbox = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_main_screen();
        if (vbox->has_theme_icon(p_class_name, "EditorIcons"))
            return vbox->get_theme_icon(p_class_name, "EditorIcons");

        if (!p_fallback.is_empty() && vbox->has_theme_icon(p_fallback, "EditorIcons"))
            return vbox->get_theme_icon(p_fallback, "EditorIcons");

        if (ClassDB::class_exists(p_class_name))
        {
            const bool instantiable = ClassDB::can_instantiate(p_class_name);
            if (ClassDB::is_parent_class(p_class_name, "Node"))
                return vbox->get_theme_icon(instantiable ? "Node" : "NodeDisabled", "EditorIcons");
            else
                return vbox->get_theme_icon(instantiable ? "Object" : "ObjectDisabled", "EditorIcons");
        }

        return nullptr;
    }

    Ref<Texture2D> get_editor_icon(const String& p_icon_name)
    {
        VBoxContainer* vbox = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_main_screen();
        return vbox->get_theme_icon(p_icon_name, "EditorIcons");
    }

    Ref<Texture2D> get_class_icon(const String& p_class_name, const String& p_fallback)
    {
        Ref<Script> script;
        return _get_class_or_script_icon(p_class_name, script, p_fallback, true);
    }

    String create_wrapped_tooltip_text(const String& p_tooltip_text, int p_width)
    {
        String wrapped = ""; // the wrapped text
        String current = ""; // current line

        Ref<Theme> theme = ThemeDB::get_singleton()->get_default_theme();
        if (!theme.is_valid())
            return p_tooltip_text;

        Ref<Font> font = theme->get_default_font();
        if (!font.is_valid())
            return p_tooltip_text;

        const PackedStringArray words = p_tooltip_text.split(" ", false);
        for (int i = 0; i < words.size(); i++)
        {
            const String word = words[i];

            // Check if word contains a new line
            int nl_pos = word.find("\n");
            if (nl_pos != -1)
            {
                // Split the word at the new line
                String before = word.substr(0, nl_pos);
                String after = word.substr(nl_pos + 1, word.length() - nl_pos - 1);
                if (font->get_string_size(current + before).width > p_width)
                {
                    wrapped += current + "\n";
                    current = "";
                }
                current += before + "\n";
                wrapped += current;
                current = after + " ";
            }
            else
            {
                // Checks if adding the next word exceeds the width
                if (font->get_string_size(current + word).width > p_width)
                {
                    wrapped += current + "\n";
                    current = "";
                }
                current += word + String(" ");
            }
        }

        // Add last line
        wrapped += current;
        return wrapped;
    }

    Node* get_node_with_script(const Ref<Script>& p_script, Node* p_node, Node* p_root)
    {
        // Non-instanced scene children
        if (p_node == p_root || p_node->get_owner() == p_root)
        {
            Ref<Script> node_script = p_node->get_script();
            if (node_script == p_script)
                return p_node;

            for (int i = 0; i < p_node->get_child_count(); i++)
            {
                Node* result = get_node_with_script(p_script, p_node->get_child(i), p_root);
                if (result)
                    return result;
            }
        }
        return nullptr;
    }

    Node* get_relative_scene_root(Node* p_node)
    {
        // Check if node is top level scene root
        if (!p_node->get_owner())
            return p_node;

        // Check if Node is top-level of a nested scene
        const String node_scene_file = p_node->get_scene_file_path();
        const String node_owner_scene_file = p_node->get_owner()->get_scene_file_path();
        if (!node_scene_file.is_empty()
            && !node_owner_scene_file.is_empty()
            && node_scene_file != node_owner_scene_file)
            return p_node;

        // Traverse node's owner
        return get_relative_scene_root(p_node->get_owner());
    }

    Vector<Node*> find_all_nodes_for_script(Node* p_base, Node* p_current, const Ref<Script>& p_script)
    {
        Vector<Node*> nodes;
        if (!p_current || (p_current->get_owner() != p_base && p_base != p_current))
            return nodes;

        Ref<Script> c = p_current->get_script();
        if (c == p_script)
            nodes.push_back(p_current);

        for (int i = 0; i < p_current->get_child_count(); i++)
        {
            Vector<Node*> found = find_all_nodes_for_script(p_base, p_current->get_child(i), p_script);
            nodes.append_array(found);
        }

        return nodes;
    }

    Vector<Node*> find_all_nodes_for_script_in_edited_scene(const Ref<Script>& p_script)
    {
        SceneTree* scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (scene_tree)
        {
            Node* scene_root = scene_tree->get_edited_scene_root();
            return find_all_nodes_for_script(scene_root, scene_root, p_script);
        }
        return {};
    }

    bool has_any_signals_connected_to_function(const String& p_function_name, const String& p_base_type,
                                               const Vector<Node*>& p_nodes)
    {
        for (int i = 0; i < p_nodes.size(); i++)
        {
            Node* node = p_nodes[i];
            TypedArray<Dictionary> incoming_connections = node->get_incoming_connections();
            for (int j = 0; j < incoming_connections.size(); ++j)
            {
                const Dictionary& connection = incoming_connections[j];
                const int connection_flags = connection["flags"];
                if (!(connection_flags & Node::CONNECT_PERSIST))
                    continue;

                const Signal signal = connection["signal"];

                // As deleted nodes are still accessible via the undo/redo system, check if they're in the tree
                Node* source = Object::cast_to<Node>(ObjectDB::get_instance(signal.get_object_id()));
                if (source && !source->is_inside_tree())
                    continue;

                const Callable callable = connection["callable"];
                const StringName method = callable.get_method();

                if (!ClassDB::class_has_method(p_base_type, method))
                {
                    if (p_function_name == method)
                        return true;
                }
            }
        }
        return false;
    }
}