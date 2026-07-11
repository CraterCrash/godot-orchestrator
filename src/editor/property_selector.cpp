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
#include "editor/property_selector.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/object/class_db.h"
#include "core/godot/scene_string_names.h"
#include "core/godot/variant/variant.h"
#include "editor/doc/editor_help.h"
#include "editor/gui/filter_line_edit.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorPropertySelector::_text_changed(const String& p_new_text) {
    _update_search();
}

void OrchestratorPropertySelector::_confirmed() {
    TreeItem* item = _search_options->get_selected();
    if (!item) {
        return;
    }

    emit_signal("selected", item->get_metadata(0));
    hide();
}

void OrchestratorPropertySelector::_item_selected() {
    _help_bit->set_custom_text(String(), String(), String());

    TreeItem* item = _search_options->get_selected();

    get_ok_button()->set_disabled(item == nullptr);
    if (!item) {
        return;
    }

    String name = item->get_metadata(0);

    String class_type;
    if (_type != Variant::NIL) {
        class_type = Variant::get_type_name(_type);
    } else if (!_base_type.is_empty()) {
        class_type = _base_type;
    } else if (_instance) {
        class_type = _instance->get_class();
    }

    String text;
    while (!class_type.is_empty()) {
        if (_properties) {
            if (GDE::ClassDB::has_property(class_type, name, true)) {
                _help_bit->parse_symbol("property|" + class_type + "|" + name);
                break;
            }
        } else {
            if (ClassDB::class_has_method(class_type, name, true)) {
                _help_bit->parse_symbol("method|" + class_type + "|" + name);
                break;
            }
        }

        // It may be from a parent class, keep looking.
        class_type = ClassDB::get_parent_class(class_type);
    }
}

void OrchestratorPropertySelector::_hide_requested() {
    get_cancel_button()->set_pressed(true);
}

bool OrchestratorPropertySelector::_contains_ignore_case(const String& p_text, const String& p_what) const {
    return p_text.containsn(p_what);
}

void OrchestratorPropertySelector::_update_search() {
    if (_properties) {
        set_title("Select Property");
    } else if (_virtuals_only) {
        set_title("Select Virtual Method");
    } else {
        set_title("Select Method");
    }

    _search_options->clear();
    _help_bit->set_custom_text(String(), String(), String());

    TreeItem* root = _search_options->create_item();

    const String search_text = _search_box->get_text().replace(" ", "_");

    // Set up font.
    bool use_monospace_font = EDITOR_GET("interface/theme/use_monospace_font_for_editor_symbols");
    Ref<Font> monospace_font = SceneUtils::get_editor_font("source");

    if (_properties) {
        List<PropertyInfo> props;
        if (_instance) {
            props = DictionaryUtils::to_properties(_instance->get_property_list());
        } else if (_type != Variant::NIL) {
            for (const PropertyInfo& pi : ExtensionDB::get_builtin_type(_type).properties) {
                props.push_back(pi);
            }
        } else {
            Object* obj = ObjectDB::get_instance(_script);

            Script* scr = cast_to<Script>(obj);
            if (scr) {
                if (OScript* oscr = cast_to<OScript>(scr)) {
                    props = DictionaryUtils::to_properties(oscr->get_orchestration_property_list());
                    props.push_front(PropertyInfo(Variant::NIL, "Script Variables", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
                } else {
                    props = DictionaryUtils::to_properties(scr->get_script_property_list());
                    props.push_front(PropertyInfo(Variant::NIL, "Script Variables", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
                }
            }

            StringName base = _base_type;
            while (!base.is_empty()) {
                props.push_back(PropertyInfo(Variant::NIL, base, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
                const TypedArray<Dictionary> property_list = ClassDB::class_get_property_list(base, true);
                for (int i = 0; i < property_list.size(); i++) {
                    props.push_back(DictionaryUtils::to_property(property_list[i]));
                }
                base = ClassDB::get_parent_class(base);
            }
        }

        TreeItem* category = nullptr;

        bool found = false;
        for (const PropertyInfo& E : props) {
            if (E.usage == PROPERTY_USAGE_CATEGORY) {
                if (category && category->get_first_child() == nullptr) {
                    memdelete(category);
                }

                category = _search_options->create_item(root);
                category->set_text(0, E.name);
                category->set_selectable(0, false);

                Ref<Texture2D> icon;
                if (E.name.match("Script Variables")) {
                    icon = SceneUtils::get_editor_icon("Script");
                } else {
                    icon = SceneUtils::get_class_icon(E.name);
                }

                category->set_icon(0, icon);
                continue;
            }

            if (!(E.usage & PROPERTY_USAGE_EDITOR) && !(E.usage & PROPERTY_USAGE_SCRIPT_VARIABLE)) {
                continue;
            }

            if (!_search_box->get_text().is_empty() && !_contains_ignore_case(E.name, search_text)) {
                continue;
            }

            if (_type_filter.size() && !_type_filter.has(E.type)) {
                continue;
            }

            TreeItem* item = _search_options->create_item(category ? category : root);
            item->set_text(0, E.name);
            if (use_monospace_font) {
                item->set_custom_font(0, monospace_font);
            }
            item->set_metadata(0, E.name);
            item->set_icon(0, SceneUtils::get_class_icon(PropertyUtils::get_variant_type_name(E)));

            if (!found & !_search_box->get_text().is_empty() && _contains_ignore_case(E.name, search_text)) {
                item->select(0);
                found = true;
            } else if (!found && _search_box->get_text().is_empty() && _contains_ignore_case(E.name, _selected)) {
                item->select(0);
                found = true;
            }

            item->set_selectable(0, true);
            _create_subproperties(item, E.type);
            item->set_collapsed(true);
        }


        if (category && category->get_first_child() == nullptr) {
            memdelete(category);
        }

        if (found) {
            callable_mp(_search_options, &Tree::scroll_to_item).call_deferred(_search_options->get_selected(), true);
        }
    } else {
        List<MethodInfo> methods;

		if (_type != Variant::NIL) {
		    for (const KeyValue<StringName, FunctionInfo>& E : ExtensionDB::get_builtin_type(_type).methods) {
		        methods.push_back(E.value.method);
		    }
		} else {
		    Ref<Script> script = cast_to<Script>(ObjectDB::get_instance(_script));
		    if (script.is_valid()) {
				if (script->is_built_in()) {
					script->reload(true);
				}

				List<MethodInfo> script_methods = DictionaryUtils::to_methods(script->get_script_method_list());

				methods.push_back(MethodInfo("*Script Methods")); // TODO: Split by inheritance.

				for (const MethodInfo &mi : script_methods) {
					if (mi.name.begins_with("@")) {
						// GH-92782. GDScript inline setters/getters are historically present in `get_method_list()`
						// and can be called using `Object.call()`. However, these functions are meant to be internal
						// and their names are not valid identifiers, so let's hide them from the user.
						continue;
					}
					methods.push_back(mi);
				}
			}

			StringName base = _base_type;
			while (!base.is_empty()) {
				methods.push_back(MethodInfo("*" + String(base)));
			    // todo: support exclude_from_properties
				const TypedArray<Dictionary> method_list = ClassDB::class_get_method_list(base, true);
                for (int i = 0; i < method_list.size(); i++) {
                    methods.push_back(DictionaryUtils::to_method(method_list[i]));
                }
				base = ClassDB::get_parent_class(base);
			}
		}

		TreeItem *category = nullptr;

		bool found = false;
		bool script_methods = false;

		for (MethodInfo& mi : methods) {
			if (mi.name.begins_with("*")) {
				if (category && category->get_first_child() == nullptr) {
					memdelete(category); //old category was unused
				}
				category = _search_options->create_item(root);
                category->set_text(0, StringUtils::replace_first(mi.name, "*", ""));
				category->set_selectable(0, false);

				Ref<Texture2D> icon;
				script_methods = false;
				String rep = mi.name.remove_char('*');
				if (mi.name.match("*Script Methods")) {
					icon = SceneUtils::get_editor_icon("Script");
					script_methods = true;
				} else {
					icon = SceneUtils::get_class_icon(rep);
				}
				category->set_icon(0, icon);

				continue;
			}

			String name = mi.name.get_slicec(':', 0);
			if (!script_methods && name.begins_with("_") && !(mi.flags & METHOD_FLAG_VIRTUAL)) {
				continue;
			}

			if (_virtuals_only && !(mi.flags & METHOD_FLAG_VIRTUAL)) {
				continue;
			}

			if (!_virtuals_only && (mi.flags & METHOD_FLAG_VIRTUAL)) {
				continue;
			}

			if (!_search_box->get_text().is_empty() && !name.containsn(search_text)) {
				continue;
			}

			TreeItem *item = _search_options->create_item(category ? category : root);

			String desc;
			if (mi.name.contains(":")) {
				desc = mi.name.get_slicec(':', 1) + " ";
				mi.name = mi.name.get_slicec(':', 0);
			} else if (mi.return_val.type != Variant::NIL) {
				desc = Variant::get_type_name(mi.return_val.type);
			} else {
				desc = "void";
			}

			desc += vformat(" %s(", mi.name);

			for (int64_t i = 0; i < mi.arguments.size(); ++i) {
				PropertyInfo& arg = mi.arguments[i];
				if (i > 0) {
					desc += ", ";
				}

				desc += arg.name;

				if (arg.type == Variant::NIL) {
					desc += ": Variant";
				} else if (arg.name.contains(":")) {
					desc += vformat(": %s", arg.name.get_slicec(':', 1));
					arg.name = arg.name.get_slicec(':', 0);
				} else {
					desc += vformat(": %s", Variant::get_type_name(arg.type));
				}
			}

			if (mi.flags & METHOD_FLAG_VARARG) {
				desc += mi.arguments.is_empty() ? "..." : ", ...";
			}

			desc += ")";

			if (mi.flags & METHOD_FLAG_VARARG) {
				desc += " vararg";
			}

			if (mi.flags & METHOD_FLAG_CONST) {
				desc += " const";
			}

			if (mi.flags & METHOD_FLAG_VIRTUAL) {
				desc += " virtual";
			}

			item->set_text(0, desc);
			if (use_monospace_font) {
				item->set_custom_font(0, monospace_font);
			}
			item->set_metadata(0, name);
			item->set_selectable(0, true);

			if (!found && !_search_box->get_text().is_empty() && name.containsn(search_text)) {
				item->select(0);
				found = true;
			} else if (!found && _search_box->get_text().is_empty() && name == _selected) {
				item->select(0);
				found = true;
			}
		}

		if (category && category->get_first_child() == nullptr) {
			memdelete(category); //old category was unused
		}

		if (found) {
			// As we call this while adding items, defer until list is completely populated.
			callable_mp(_search_options, &Tree::scroll_to_item).call_deferred(_search_options->get_selected(), true);
		}
    }

    get_ok_button()->set_disabled(_search_options->get_selected() == nullptr);
}

void OrchestratorPropertySelector::_create_subproperties(TreeItem* p_parent_item, Variant::Type p_type) {
    switch (p_type) {
        case Variant::VECTOR2: {
            _create_subproperty(p_parent_item, "x", Variant::FLOAT);
            _create_subproperty(p_parent_item, "y", Variant::FLOAT);
            break;
        }
        case Variant::VECTOR2I: {
            _create_subproperty(p_parent_item, "x", Variant::INT);
            _create_subproperty(p_parent_item, "y", Variant::INT);
            break;
        }
        case Variant::RECT2: {
            _create_subproperty(p_parent_item, "position", Variant::VECTOR2);
            _create_subproperty(p_parent_item, "size", Variant::VECTOR2);
            _create_subproperty(p_parent_item, "end", Variant::VECTOR2);
            break;
        }
        case Variant::RECT2I: {
            _create_subproperty(p_parent_item, "position", Variant::VECTOR2I);
            _create_subproperty(p_parent_item, "size", Variant::VECTOR2I);
            _create_subproperty(p_parent_item, "end", Variant::VECTOR2I);
            break;
        }
        case Variant::VECTOR3: {
            _create_subproperty(p_parent_item, "x", Variant::FLOAT);
            _create_subproperty(p_parent_item, "y", Variant::FLOAT);
            _create_subproperty(p_parent_item, "z", Variant::FLOAT);
            break;
        }
        case Variant::VECTOR3I: {
            _create_subproperty(p_parent_item, "x", Variant::INT);
            _create_subproperty(p_parent_item, "y", Variant::INT);
            _create_subproperty(p_parent_item, "z", Variant::INT);
            break;
        }
        case Variant::TRANSFORM2D: {
            _create_subproperty(p_parent_item, "origin", Variant::VECTOR2);
            _create_subproperty(p_parent_item, "x", Variant::VECTOR2);
            _create_subproperty(p_parent_item, "y", Variant::VECTOR2);
            break;
        }
        case Variant::VECTOR4: {
            _create_subproperty(p_parent_item, "x", Variant::FLOAT);
            _create_subproperty(p_parent_item, "y", Variant::FLOAT);
            _create_subproperty(p_parent_item, "z", Variant::FLOAT);
            _create_subproperty(p_parent_item, "w", Variant::FLOAT);
            break;
        }
        case Variant::VECTOR4I: {
            _create_subproperty(p_parent_item, "x", Variant::INT);
            _create_subproperty(p_parent_item, "y", Variant::INT);
            _create_subproperty(p_parent_item, "z", Variant::INT);
            _create_subproperty(p_parent_item, "w", Variant::INT);
            break;
        }
        case Variant::PLANE: {
            _create_subproperty(p_parent_item, "x", Variant::FLOAT);
            _create_subproperty(p_parent_item, "y", Variant::FLOAT);
            _create_subproperty(p_parent_item, "z", Variant::FLOAT);
            _create_subproperty(p_parent_item, "normal", Variant::VECTOR3);
            _create_subproperty(p_parent_item, "d", Variant::FLOAT);
            break;
        }
        case Variant::QUATERNION: {
            _create_subproperty(p_parent_item, "x", Variant::FLOAT);
            _create_subproperty(p_parent_item, "y", Variant::FLOAT);
            _create_subproperty(p_parent_item, "z", Variant::FLOAT);
            _create_subproperty(p_parent_item, "w", Variant::FLOAT);
            break;
        }
        case Variant::AABB: {
            _create_subproperty(p_parent_item, "position", Variant::VECTOR3);
            _create_subproperty(p_parent_item, "size", Variant::VECTOR3);
            _create_subproperty(p_parent_item, "end", Variant::VECTOR3);
            break;
        }
        case Variant::BASIS: {
            _create_subproperty(p_parent_item, "x", Variant::VECTOR3);
            _create_subproperty(p_parent_item, "y", Variant::VECTOR3);
            _create_subproperty(p_parent_item, "z", Variant::VECTOR3);
            break;
        }
        case Variant::TRANSFORM3D: {
            _create_subproperty(p_parent_item, "basis", Variant::BASIS);
            _create_subproperty(p_parent_item, "origin", Variant::VECTOR3);
            break;
        }
        case Variant::PROJECTION: {
            _create_subproperty(p_parent_item, "x", Variant::VECTOR4);
            _create_subproperty(p_parent_item, "y", Variant::VECTOR4);
            _create_subproperty(p_parent_item, "z", Variant::VECTOR4);
            _create_subproperty(p_parent_item, "w", Variant::VECTOR4);
            break;
        }
        case Variant::COLOR: {
            _create_subproperty(p_parent_item, "r", Variant::FLOAT);
            _create_subproperty(p_parent_item, "g", Variant::FLOAT);
            _create_subproperty(p_parent_item, "b", Variant::FLOAT);
            _create_subproperty(p_parent_item, "a", Variant::FLOAT);
            _create_subproperty(p_parent_item, "r8", Variant::INT);
            _create_subproperty(p_parent_item, "g8", Variant::INT);
            _create_subproperty(p_parent_item, "b8", Variant::INT);
            _create_subproperty(p_parent_item, "a8", Variant::INT);
            _create_subproperty(p_parent_item, "h", Variant::FLOAT);
            _create_subproperty(p_parent_item, "s", Variant::FLOAT);
            _create_subproperty(p_parent_item, "v", Variant::FLOAT);
            break;
        }
        default: {
        }
    }
}

void OrchestratorPropertySelector::select_method_from_base_type(const String &p_base, const String &p_current, bool p_virtuals_only) {
    _base_type = p_base;
    _selected = p_current;
    _type = Variant::NIL;
    _script = ObjectID();
    _properties = false;
    _instance = nullptr;
    _virtuals_only = p_virtuals_only;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_method_from_script(const Ref<Script> &p_script, const String &p_current) {
    ERR_FAIL_COND(p_script.is_null());
    _base_type = p_script->get_instance_base_type();
    _selected = p_current;
    _type = Variant::NIL;
    _script = p_script->get_instance_id();
    _properties = false;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_method_from_basic_type(Variant::Type p_type, const String &p_current) {
    ERR_FAIL_COND(p_type == Variant::NIL);
    _base_type = "";
    _selected = p_current;
    _type = p_type;
    _script = ObjectID();
    _properties = false;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_method_from_instance(Object *p_instance, const String &p_current) {
    _base_type = p_instance->get_class();
    _selected = p_current;
    _type = Variant::NIL;
    _script = ObjectID();
    {
        Ref<Script> scr = p_instance->get_script();
        if (scr.is_valid()) {
            _script = scr->get_instance_id();
        }
    }
    _properties = false;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::_create_subproperty(TreeItem *p_parent_item, const String &p_name, Variant::Type p_type) { // NOLINT
    if (!_type_filter.is_empty() && !_type_filter.has(p_type)) {
        return;
    }

    TreeItem *item = _search_options->create_item(p_parent_item);
    item->set_text(0, p_name);

    bool use_monospace_font = EDITOR_GET("interface/theme/use_monospace_font_for_editor_symbols");
    if (use_monospace_font) {
        item->set_custom_font(0, SceneUtils::get_editor_font("source"));
    }

    item->set_metadata(0, String(p_parent_item->get_metadata(0)) + ":" + p_name);
    item->set_icon(0, SceneUtils::get_editor_icon(Variant::get_type_name(p_type)));

    _create_subproperties(item, p_type);
}

void OrchestratorPropertySelector::select_property_from_base_type(const String& p_base_type, const String& p_current) {
    _base_type = p_base_type;
    _selected = p_current;
    _type = Variant::NIL;
    _script = ObjectID();
    _properties = true;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_property_from_script(const Ref<Script>& p_script, const String& p_current) {
    ERR_FAIL_COND(p_script.is_null());

    _base_type = p_script->get_instance_base_type();
    _selected = p_current;
    _type = Variant::NIL;
    _script = p_script->get_instance_id();
    _properties = true;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_property_from_basic_type(Variant::Type p_type, const String& p_current) {
    ERR_FAIL_COND(p_type == Variant::NIL);

    _base_type = "";
    _selected = p_current;
    _type = p_type;
    _script = ObjectID();
    _properties = true;
    _instance = nullptr;
    _virtuals_only = false;

    popup_centered_ratio(0.6);
    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::select_property_from_instance(Object* p_instance, const String& p_current) {
    _base_type = "";
    _selected = p_current;
    _type = Variant::NIL;
    _script = ObjectID();
    _properties = true;
    _instance = p_instance;
    _virtuals_only = false;

    popup_centered_ratio(0.6);

    _search_box->set_text("");
    _search_box->grab_focus();

    _update_search();
}

void OrchestratorPropertySelector::set_type_filter(const Vector<Variant::Type>& p_type_filter) {
    _type_filter = p_type_filter;
}

void OrchestratorPropertySelector::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            connect(SceneStringName(confirmed), callable_mp_this(_confirmed));
            break;
        }
        case NOTIFICATION_EXIT_TREE: {
            disconnect(SceneStringName(confirmed), callable_mp_this(_confirmed));
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _search_box->set_right_icon(SceneUtils::get_editor_icon("Search"));
            break;
        }
    }
}

void OrchestratorPropertySelector::_bind_methods() {
    ADD_SIGNAL(MethodInfo("selected", PropertyInfo(Variant::STRING, "name")));
}

OrchestratorPropertySelector::OrchestratorPropertySelector() : _type(Variant::NIL) {
    VBoxContainer* vbox = memnew(VBoxContainer);
    add_child(vbox);

    _search_box = memnew(OrchestratorEditorFilterLineEdit);
    _search_box->set_accessibility_name("Search:");
    SceneUtils::add_margin_child(vbox, "Search:", _search_box);
    _search_box->connect(SceneStringName(text_changed), callable_mp_this(_text_changed));

    _search_options = memnew(Tree);
    _search_box->set_forward_control(_search_options);
    _search_options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    _search_options->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTH);
    SceneUtils::set_theme_type_variation(_search_options, "Tree");

    MarginContainer* mc = SceneUtils::add_margin_child(vbox, "Matches:", _search_options, true);
    mc->set_theme_type_variation("NoBorderHorizontalWindow");

    _search_options->connect("item_activated", callable_mp_this(_confirmed));
    _search_options->connect("cell_selected", callable_mp_this(_item_selected));
    _search_options->set_hide_root(true);

    _help_bit = memnew(OrchestratorEditorHelpBit);
    _help_bit->set_content_help_limits(80 * EDSCALE, 80 * EDSCALE);
    _help_bit->connect("request_hide", callable_mp_this(_hide_requested));
    SceneUtils::add_margin_child(vbox, "Description:", _help_bit);

    set_ok_button_text("Open");
    get_ok_button()->set_disabled(true);
    register_text_enter(_search_box);
    set_hide_on_ok(false);
}

