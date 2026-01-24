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
#include "script/script_docgen.h"

#include "common/string_utils.h"
#include "core/godot/config/project_settings.h"
#include "core/godot/doc_data.h"
#include "core/godot/variant/variant.h"
#include "orchestration/serialization/text/variant_parser.h"

#ifdef TOOLS_ENABLED
HashMap<String, String> OScriptDocGen::_singletons;

String OScriptDocGen::_get_script_name(const String& p_path) {
    const HashMap<String, String>::ConstIterator E = _singletons.find(p_path);
    return E ? E->value : StringUtils::quote(p_path.trim_prefix("res://"));
}

String OScriptDocGen::_get_class_name(const Parser::ClassNode& p_class) {
    const Parser::ClassNode* current_class = &p_class;
    if (!current_class->identifier) { // All inner classes have identifiers, so this is the outer
        return _get_script_name(current_class->fqcn);
    }

    String full_name = current_class->identifier->name;
    while (current_class->outer) {
        current_class = current_class->outer;
        if (!current_class->identifier) {
            return vformat("%s.%s", _get_script_name(current_class->fqcn), full_name);
        }
        full_name = vformat("%s.%s", current_class->identifier->name, full_name);
    }
    return full_name;
}

void OScriptDocGen::_doctype_from_script_type(const Type& p_script_type, String& r_type, String& r_enum, bool p_is_return) {
    if (!p_script_type.is_hard_type()) {
        r_type = "Variant";
        return;
    }

    switch (p_script_type.kind) {
        case Type::BUILTIN: {
            if (p_script_type.builtin_type == Variant::NIL) {
                r_type = p_is_return ? "void" : "null";
                return;
            }
            if (p_script_type.builtin_type == Variant::ARRAY && p_script_type.has_container_element_type(0)) {
                _doctype_from_script_type(p_script_type.get_container_element_type(0), r_type, r_enum);
                if (!r_enum.is_empty()) {
                    r_type = "int[]";
                    r_enum += "[]";
                    return;
                }
                if (!r_type.is_empty() && r_type != "Variant") {
                    r_type += "[]";
                    return;
                }
            }
            if (p_script_type.builtin_type == Variant::DICTIONARY && p_script_type.has_container_element_types()) {
                String key, value;
                _doctype_from_script_type(p_script_type.get_container_element_type_or_variant(0), key, r_enum);
                _doctype_from_script_type(p_script_type.get_container_element_type_or_variant(1), value, r_enum);
                if (key != "Variant" || value != "Variant") {
                    r_type = "Dictionary[" + key + ", " + value + "]";
                    return;
                }
            }
            r_type = Variant::get_type_name(p_script_type.builtin_type);
            break;
        }
        case Type::NATIVE: {
            if (p_script_type.is_meta_type) {
                r_type = "Object";
                return;
            }
            r_type = p_script_type.native_type;
            break;
        }
        case Type::SCRIPT: {
            if (p_script_type.is_meta_type) {
                r_type = p_script_type.script_type.is_valid()
                    ? String(p_script_type.script_type->get_class())
                    : String(Script::get_class_static());
                return;
            }
            if (p_script_type.script_type.is_valid()) {
                if (p_script_type.script_type->get_global_name() != StringName()) {
                    r_type = p_script_type.script_type->get_global_name();
                    return;
                }
                if (!p_script_type.script_type->get_path().is_empty()) {
                    r_type = _get_script_name(p_script_type.script_type->get_path());
                    return;
                }
            }
            if (!p_script_type.script_path.is_empty()) {
                r_type = _get_script_name(p_script_type.script_path);
                return;
            }
            r_type = "Object";
            break;
        }
        case Type::CLASS: {
            if (p_script_type.is_meta_type) {
                r_type = OScript::get_class_static();
                return;
            }
            r_type = _get_class_name(*p_script_type.class_type);
            break;
        }
        case Type::ENUM: {
            if (p_script_type.is_meta_type) {
                r_type = "Dictionary";
                return;
            }
            r_type = "int";
            r_enum = String(p_script_type.native_type).replace("::", ".");
            if (r_enum.begins_with("res://")) {
                int dot_pos = r_enum.rfind(".");
                if (dot_pos >= 0) {
                    r_enum = _get_script_name(r_enum.left(dot_pos)) + r_enum.substr(dot_pos);
                } else {
                    r_enum = _get_script_name(r_enum);
                }
            }
            break;
        }
        case Type::VARIANT:
        case Type::RESOLVING:
        case Type::UNRESOLVED: {
            r_type = "Variant";
            break;
        }
    }
}

String OScriptDocGen::_docvalue_from_variant(const Variant& p_value, int p_recursion_level) {
    constexpr int MAX_RECURSION_LEVEL = 2;

	switch (p_value.get_type()) {
		case Variant::STRING: {
		    return StringUtils::quote(String(p_value).c_escape());
		}
		case Variant::OBJECT: {
		    return "<Object>";
		}
		case Variant::DICTIONARY: {
			const Dictionary dict = p_value;
			String result;

			if (dict.is_typed()) {
				result += "Dictionary[";

				Ref<Script> key_script = dict.get_typed_key_script();
				if (key_script.is_valid()) {
					if (key_script->get_global_name() != StringName()) {
						result += key_script->get_global_name();
					} else if (!key_script->get_path().get_file().is_empty()) {
						result += key_script->get_path().get_file();
					} else {
						result += dict.get_typed_key_class_name();
					}
				} else if (dict.get_typed_key_class_name() != StringName()) {
					result += dict.get_typed_key_class_name();
				} else if (dict.is_typed_key()) {
					result += Variant::get_type_name((Variant::Type)dict.get_typed_key_builtin());
				} else {
					result += "Variant";
				}

				result += ", ";

				Ref<Script> value_script = dict.get_typed_value_script();
				if (value_script.is_valid()) {
					if (value_script->get_global_name() != StringName()) {
						result += value_script->get_global_name();
					} else if (!value_script->get_path().get_file().is_empty()) {
						result += value_script->get_path().get_file();
					} else {
						result += dict.get_typed_value_class_name();
					}
				} else if (dict.get_typed_value_class_name() != StringName()) {
					result += dict.get_typed_value_class_name();
				} else if (dict.is_typed_value()) {
					result += Variant::get_type_name((Variant::Type)dict.get_typed_value_builtin());
				} else {
					result += "Variant";
				}

				result += "](";
			}

			if (dict.is_empty()) {
				result += "{}";
			} else if (p_recursion_level > MAX_RECURSION_LEVEL) {
				result += "{...}";
			} else {
				result += "{";

			    Vector<Variant> sorted_keys;
			    const Array keys = dict.keys();
			    for (uint32_t i = 0; i < keys.size(); i++) {
			        sorted_keys.push_back(keys[i]);
			    }
				sorted_keys.sort_custom<GDE::Variant::StringLikeVariantOrder>();

				for (uint32_t i = 0; i < sorted_keys.size(); i++) {
					const Variant &key = sorted_keys[i];
					if (i > 0) {
						result += ", ";
					}
					result += _docvalue_from_variant(key, p_recursion_level + 1) + ": " + _docvalue_from_variant(dict[key], p_recursion_level + 1);
				}

				result += "}";
			}

			if (dict.is_typed()) {
				result += ")";
			}

			return result;
		}
		case Variant::ARRAY: {
			const Array array = p_value;
			String result;

			if (array.is_typed()) {
				result += "Array[";

				Ref<Script> script = array.get_typed_script();
				if (script.is_valid()) {
					if (script->get_global_name() != StringName()) {
						result += script->get_global_name();
					} else if (!script->get_path().get_file().is_empty()) {
						result += script->get_path().get_file();
					} else {
						result += array.get_typed_class_name();
					}
				} else if (array.get_typed_class_name() != StringName()) {
					result += array.get_typed_class_name();
				} else {
					result += Variant::get_type_name((Variant::Type)array.get_typed_builtin());
				}

				result += "](";
			}

			if (array.is_empty()) {
				result += "[]";
			} else if (p_recursion_level > MAX_RECURSION_LEVEL) {
				result += "[...]";
			} else {
				result += "[";

				for (int i = 0; i < array.size(); i++) {
					if (i > 0) {
						result += ", ";
					}
					result += _docvalue_from_variant(array[i], p_recursion_level + 1);
				}

				result += "]";
			}

			if (array.is_typed()) {
				result += ")";
			}

			return result;
		}
	    default: {
		    String value;
		    OScriptVariantWriter::write_to_string(p_value, value);
		    return value;
		}
	}
}

void OScriptDocGen::_generate_docs(OScript* p_script, const Parser::ClassNode* p_class) {
    p_script->_clear_doc();

	DocData::ClassDoc &doc = p_script->doc;

	doc.is_script_doc = true;

	if (p_script->local_name == StringName()) {
		// This is an outer unnamed class.
		doc.name = _get_script_name(p_script->get_script_path());
	} else {
		// This is an inner or global outer class.
		doc.name = p_script->local_name;
		if (p_script->subclass_owner) {
			doc.name = p_script->subclass_owner->doc.name + "." + doc.name;
		}
	}

	doc.script_path = p_script->get_script_path();

	if (p_script->base.is_valid() && p_script->base->_is_valid()) {
		if (!p_script->base->doc.name.is_empty()) {
			doc.inherits = p_script->base->doc.name;
		} else {
			doc.inherits = p_script->base->get_instance_base_type();
		}
	} else if (p_script->native.is_valid()) {
		doc.inherits = p_script->native->get_name();
	}

	doc.brief_description = p_class->doc_data.brief;
	doc.description = p_class->doc_data.description;
	for (const Pair<String, String>& p : p_class->doc_data.tutorials) {
		DocData::TutorialDoc td;
		td.title = p.first;
		td.link = p.second;
		doc.tutorials.append(td);
	}
	doc.is_deprecated = p_class->doc_data.is_deprecated;
	doc.deprecated_message = p_class->doc_data.deprecated_message;
	doc.is_experimental = p_class->doc_data.is_experimental;
	doc.experimental_message = p_class->doc_data.experimental_message;

	for (const Parser::ClassNode::Member& member : p_class->members) {
		switch (member.type) {
			case Parser::ClassNode::Member::CLASS: {
				const Parser::ClassNode* inner_class = member.m_class;
				const StringName& class_name = inner_class->identifier->name;

			    p_script->member_node_ids[class_name] = inner_class->script_node_id;

				// Recursively generate inner class docs.
				// Needs inner GDScripts to exist: previously generated in GDScriptCompiler::make_scripts().
				_generate_docs(*p_script->subclasses[class_name], inner_class);
			    break;
			}
			case Parser::ClassNode::Member::CONSTANT: {
				const Parser::ConstantNode* m_const = member.constant;
				const StringName &const_name = member.constant->identifier->name;

				p_script->member_node_ids[const_name] = m_const->script_node_id;

				DocData::ConstantDoc const_doc;
				const_doc.name = const_name;
				const_doc.value = _docvalue_from_variant(m_const->initializer->reduced_value);
				const_doc.is_value_valid = true;
				_doctype_from_script_type(m_const->get_datatype(), const_doc.type, const_doc.enumeration);
				const_doc.description = m_const->doc_data.description;
				const_doc.is_deprecated = m_const->doc_data.is_deprecated;
				const_doc.deprecated_message = m_const->doc_data.deprecated_message;
				const_doc.is_experimental = m_const->doc_data.is_experimental;
				const_doc.experimental_message = m_const->doc_data.experimental_message;
				doc.constants.push_back(const_doc);
			    break;
			}
			case Parser::ClassNode::Member::FUNCTION: {
				const Parser::FunctionNode* m_func = member.function;
				const StringName& func_name = m_func->identifier->name;

				p_script->member_node_ids[func_name] = m_func->script_node_id;

				DocData::MethodDoc method_doc;
				method_doc.name = func_name;
				method_doc.description = m_func->doc_data.description;
				method_doc.is_deprecated = m_func->doc_data.is_deprecated;
				method_doc.deprecated_message = m_func->doc_data.deprecated_message;
				method_doc.is_experimental = m_func->doc_data.is_experimental;
				method_doc.experimental_message = m_func->doc_data.experimental_message;

				if (m_func->is_vararg()) {
					if (!method_doc.qualifiers.is_empty()) {
						method_doc.qualifiers += " ";
					}
					method_doc.qualifiers += "vararg";
					method_doc.rest_argument.name = m_func->rest_parameter->identifier->name;
					_doctype_from_script_type(m_func->rest_parameter->get_datatype(), method_doc.rest_argument.type, method_doc.rest_argument.enumeration);
				}

				if (m_func->is_abstract) {
					if (!method_doc.qualifiers.is_empty()) {
						method_doc.qualifiers += " ";
					}
					method_doc.qualifiers += "abstract";
				}

				if (m_func->is_static) {
					if (!method_doc.qualifiers.is_empty()) {
						method_doc.qualifiers += " ";
					}
					method_doc.qualifiers += "static";
				}

				if (func_name == StringName("_init")) {
					method_doc.return_type = "void";
				} else if (m_func->return_type) {
					// `m_func->return_type->get_datatype()` is a metatype.
					_doctype_from_script_type(m_func->get_datatype(), method_doc.return_type, method_doc.return_enum, true);
				} else if (!m_func->body->has_return) {
					// If no `return` statement, then return type is `void`, not `Variant`.
					method_doc.return_type = "void";
				} else {
					method_doc.return_type = "Variant";
				}

				for (const Parser::ParameterNode* p : m_func->parameters) {
					DocData::ArgumentDoc arg_doc;
					arg_doc.name = p->identifier->name;
					_doctype_from_script_type(p->get_datatype(), arg_doc.type, arg_doc.enumeration);

					if (p->initializer != nullptr) {
						arg_doc.default_value = doc_value_from_expression(p->initializer);
					}

					method_doc.arguments.push_back(arg_doc);
				}

				doc.methods.push_back(method_doc);
			    break;
			}
			case Parser::ClassNode::Member::SIGNAL: {
				const Parser::SignalNode* m_signal = member.signal;
				const StringName& signal_name = m_signal->identifier->name;

				p_script->member_node_ids[signal_name] = m_signal->script_node_id;

				DocData::MethodDoc signal_doc;
				signal_doc.name = signal_name;
				signal_doc.description = m_signal->doc_data.description;
				signal_doc.is_deprecated = m_signal->doc_data.is_deprecated;
				signal_doc.deprecated_message = m_signal->doc_data.deprecated_message;
				signal_doc.is_experimental = m_signal->doc_data.is_experimental;
				signal_doc.experimental_message = m_signal->doc_data.experimental_message;

				for (const Parser::ParameterNode* p : m_signal->parameters) {
					DocData::ArgumentDoc arg_doc;
					arg_doc.name = p->identifier->name;
					_doctype_from_script_type(p->get_datatype(), arg_doc.type, arg_doc.enumeration);
					signal_doc.arguments.push_back(arg_doc);
				}

				doc.signals.push_back(signal_doc);
			    break;
			}
			case Parser::ClassNode::Member::VARIABLE: {
				const Parser::VariableNode* m_var = member.variable;
				const StringName& var_name = m_var->identifier->name;

				p_script->member_node_ids[var_name] = m_var->script_node_id;

				DocData::PropertyDoc prop_doc;
				prop_doc.name = var_name;
				prop_doc.description = m_var->doc_data.description;
				prop_doc.is_deprecated = m_var->doc_data.is_deprecated;
				prop_doc.deprecated_message = m_var->doc_data.deprecated_message;
				prop_doc.is_experimental = m_var->doc_data.is_experimental;
				prop_doc.experimental_message = m_var->doc_data.experimental_message;
				_doctype_from_script_type(m_var->get_datatype(), prop_doc.type, prop_doc.enumeration);

				switch (m_var->style) {
					case Parser::VariableNode::NONE: {
					    break;
					}
				    case Parser::VariableNode::INLINE: {
				        if (m_var->setter != nullptr) {
				            prop_doc.setter = m_var->setter->identifier->name;
				        }
				        if (m_var->getter != nullptr) {
				            prop_doc.getter = m_var->getter->identifier->name;
				        }
				        break;
				    }
					case Parser::VariableNode::SETGET: {
					    if (m_var->setter_pointer != nullptr) {
					        prop_doc.setter = m_var->setter_pointer->name;
					    }
					    if (m_var->getter_pointer != nullptr) {
					        prop_doc.getter = m_var->getter_pointer->name;
					    }
					    break;
					}
				}

				if (m_var->initializer != nullptr) {
					prop_doc.default_value = doc_value_from_expression(m_var->initializer);
				}

				prop_doc.overridden = false;
				doc.properties.push_back(prop_doc);
			    break;
			}
			case Parser::ClassNode::Member::ENUM: {
				const Parser::EnumNode* m_enum = member.m_enum;
				StringName name = m_enum->identifier->name;

				p_script->member_node_ids[name] = m_enum->script_node_id;

				DocData::EnumDoc enum_doc;
				enum_doc.description = m_enum->doc_data.description;
				enum_doc.is_deprecated = m_enum->doc_data.is_deprecated;
				enum_doc.deprecated_message = m_enum->doc_data.deprecated_message;
				enum_doc.is_experimental = m_enum->doc_data.is_experimental;
				enum_doc.experimental_message = m_enum->doc_data.experimental_message;
				doc.enums[name] = enum_doc;

				for (const Parser::EnumNode::Value& val : m_enum->values) {
					DocData::ConstantDoc const_doc;
					const_doc.name = val.identifier->name;
					const_doc.value = _docvalue_from_variant(val.value);
					const_doc.is_value_valid = true;
					const_doc.type = "int";
					const_doc.enumeration = name;
					const_doc.description = val.doc_data.description;
					const_doc.is_deprecated = val.doc_data.is_deprecated;
					const_doc.deprecated_message = val.doc_data.deprecated_message;
					const_doc.is_experimental = val.doc_data.is_experimental;
					const_doc.experimental_message = val.doc_data.experimental_message;

					doc.constants.push_back(const_doc);
				}
			    break;
			}
			case Parser::ClassNode::Member::ENUM_VALUE: {
				const Parser::EnumNode::Value& m_enum_val = member.enum_value;
				const StringName& name = m_enum_val.identifier->name;

				p_script->member_node_ids[name] = m_enum_val.identifier->script_node_id;

				DocData::ConstantDoc const_doc;
				const_doc.name = name;
				const_doc.value = _docvalue_from_variant(m_enum_val.value);
				const_doc.is_value_valid = true;
				const_doc.type = "int";
				const_doc.enumeration = "@unnamed_enums";
				const_doc.description = m_enum_val.doc_data.description;
				const_doc.is_deprecated = m_enum_val.doc_data.is_deprecated;
				const_doc.deprecated_message = m_enum_val.doc_data.deprecated_message;
				const_doc.is_experimental = m_enum_val.doc_data.is_experimental;
				const_doc.experimental_message = m_enum_val.doc_data.experimental_message;
				doc.constants.push_back(const_doc);
			    break;
			}
			default:
				break;
		}
	}

	// Add doc to the outermost class.
	p_script->_add_doc(doc);
}

void OScriptDocGen::generate_docs(OScript* p_script, const Parser::ClassNode* p_class) {
    for (const KeyValue<StringName, GDE::ProjectSettings::AutoloadInfo>& E : GDE::ProjectSettings::get_autoload_list()) {
        if (E.value.is_singleton) {
            _singletons[E.value.path] = E.key;
        }
    }
    _generate_docs(p_script, p_class);
    _singletons.clear();
}

// This method is needed for the editor, since during autocompletion the script is not compiled, only analyzed.
void OScriptDocGen::doc_type_from_script_type(const Type& p_script_type, String& r_type, String& r_enum, bool p_is_return) {
    for (const KeyValue<StringName, GDE::ProjectSettings::AutoloadInfo>& E : GDE::ProjectSettings::get_autoload_list()) {
        if (E.value.is_singleton) {
            _singletons[E.value.path] = E.key;
        }
    }
    _doctype_from_script_type(p_script_type, r_type, r_enum, p_is_return);
    _singletons.clear();
}

String OScriptDocGen::doc_value_from_expression(const Parser::ExpressionNode* p_expression) {
    ERR_FAIL_NULL_V(p_expression, String());

    if (p_expression->is_constant) {
        return _docvalue_from_variant(p_expression->reduced_value);
    }

    switch (p_expression->type) {
        case Parser::Node::ARRAY: {
            const Parser::ArrayNode* array = static_cast<const Parser::ArrayNode*>(p_expression); // NOLINT
            return array->elements.is_empty() ? "[]" : "[...]";
        }
        case Parser::Node::CALL: {
            const Parser::CallNode* call = static_cast<const Parser::CallNode*>(p_expression); // NOLINT
            if (call->get_callee_type() == Parser::Node::IDENTIFIER) {
                return String(call->function_name) + (call->arguments.is_empty() ? "()" : "(...)");
            }
            break;
        }
        case Parser::Node::DICTIONARY: {
            const Parser::DictionaryNode* dict = static_cast<const Parser::DictionaryNode*>(p_expression); // NOLINT
            return dict->elements.is_empty() ? "{}" : "{...}";
        }
        case Parser::Node::IDENTIFIER: {
            const Parser::IdentifierNode* id = static_cast<const Parser::IdentifierNode*>(p_expression); // NOLINT
            return id->name;
        }
        default: {
            // Nothing to do.
        } break;
    }

    return "<unknown>";
}
#endif