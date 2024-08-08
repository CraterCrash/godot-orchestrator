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
#ifndef ORCHESTRATOR_SCENE_UTILS_H
#define ORCHESTRATOR_SCENE_UTILS_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

namespace SceneUtils
{
    /// Check whether there is an editor icon with the given name.
    /// @param p_icon_name the editor icon to check
    /// @return true if the icon exists; false otherwise
    bool has_editor_icon(const String& p_icon_name);

    /// Get the editor theme named color
    /// @param p_color_name the color name
    /// @param p_category the color category, defaults to "Editor"
    /// @return the editor color
    Color get_editor_color(const String& p_color_name, const String& p_category = "Editor");

    /// Gets an Orchestrator editor icon
    /// @param p_name the icon name
    /// @return a reference to the icon
    Ref<Texture2D> get_icon(const String& p_name);
    
    /// Load an icon.
    ///
    /// @param p_icon_name the editor icon to load
    /// @return a reference to the texture or an invalid reference if the texture isn't loaded
    Ref<Texture2D> get_editor_icon(const String& p_icon_name);

    /// Gets an editor style by name
    /// @param p_style_name the style name
    /// @return a reference to the editor stylebox or an invalid reference if not found
    Ref<StyleBox> get_editor_style(const String& p_style_name);

    /// Get an editor font
    /// @param p_font_name the font name
    /// @return the font reference or an invalid reference if the font isn't found
    Ref<Font> get_editor_font(const String& p_font_name);

    /// Get an editor font size
    /// @param p_font_name the font name
    /// @return the font size
    int get_editor_font_size(const String& p_font_name);

    /// Get an editor stylebox
    /// @return the stylebox or an invalid reference if it wasn't found
    Ref<StyleBox> get_editor_stylebox(const String& p_stylebox_name, const String& p_class_type);

    /// Loads the class icon
    ///
    /// @param p_class_name the class name
    /// @param p_fallback the fallback icon to use
    /// @return a reference to the texture or an invalid reference if the texture isn't loaded
    Ref<Texture2D> get_class_icon(const String& p_class_name, const String& p_fallback = "");

    /// Creates tooltip text that will automatically be wrapped at word boundaries and will not
    /// exceed the specified width. If text contains new lines, those will be preserved.
    ///
    /// @param p_tooltip_text the text to be wrapped
    /// @param p_width the maximum width of a line of text before wrapping
    /// @return the wrapped tooltip text
    String create_wrapped_tooltip_text(const String& p_tooltip_text, int p_width = 512);

    /// Finds the first node with the specified script attached
    /// @param p_script the script instance, should be valid
    /// @param p_node the node being inspected, should not be <code>null</code>
    /// @param p_root the root node of the scene being inspected, should not be <code>null</code>
    /// @return first node that has the script attached or <code>null</code> if not found
    Node* get_node_with_script(const Ref<Script>& p_script, Node* p_node, Node* p_root);

    /// Finds the specified node's relative scene root.
    /// @param p_node the node to find the nearest scene root for
    /// @return the nearest relative scene root to the given node
    Node* get_relative_scene_root(Node* p_node);

    /// Find all nodes associated with the specified script
    /// @param p_base the base node to start from, should not be <code>null</code>
    /// @param p_current the current node, should not be <code>null</code>
    /// @param p_script the script instance, should be valid
    /// @return vector list of node instances or an empty vector if none found
    Vector<Node*> find_all_nodes_for_script(Node* p_base, Node* p_current, const Ref<Script>& p_script);

    /// Calls the @link find_all_nodes_for_script method for the specified script in the current edited scene.
    /// @param p_script the script instance, should be valid
    /// @return vector list of node instances or an empty vector if none found
    Vector<Node*> find_all_nodes_for_script_in_edited_scene(const Ref<Script>& p_script);

    /// Returns whether any signals of the specified <code>p_nodes</code> are linked with the specified
    /// function or the base type.
    /// @param p_function_name the function name
    /// @param p_base_type the base type
    /// @param p_nodes the vector of nodes
    /// @return true if the function is a target of a signal; false otherwise
    bool has_any_signals_connected_to_function(const String& p_function_name, const String& p_base_type, const Vector<Node*>& p_nodes);

    // taken from VBoxContainer in the engine
    MarginContainer* add_margin_child(Node* p_parent, const String& p_label, Control* p_control, bool p_expand = false);
}

#endif  // ORCHESTRATOR_SCENE_UTILS_H
