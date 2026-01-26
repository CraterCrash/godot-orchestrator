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
#include "script/parser/parser_nodes.h"

#include "core/godot/object/script_language.h"
#include "script/parser/parser.h"

#include <godot_cpp/classes/resource_loader.hpp>

static Variant::Type _variant_type_to_typed_array_element_type(Variant::Type p_type) {
    switch (p_type) {
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
            return Variant::INT;
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
            return Variant::FLOAT;
        case Variant::PACKED_STRING_ARRAY:
            return Variant::STRING;
        case Variant::PACKED_VECTOR2_ARRAY:
            return Variant::VECTOR2;
        case Variant::PACKED_VECTOR3_ARRAY:
            return Variant::VECTOR3;
        case Variant::PACKED_COLOR_ARRAY:
            return Variant::COLOR;
        case Variant::PACKED_VECTOR4_ARRAY:
            return Variant::VECTOR4;
        default:
            return Variant::NIL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DataType

String OScriptParserNodes::DataType::to_string() const {
    switch (kind) {
        case VARIANT:
            return "Variant";

        case BUILTIN:
            if (builtin_type == Variant::NIL) {
                return "null";
            }
            if (builtin_type == Variant::ARRAY && has_container_element_type(0)) {
                return vformat("Array[%s]", get_container_element_type(0).to_string());
            }
            if (builtin_type == Variant::DICTIONARY && has_container_element_types()) {
                return vformat("Dictionary[%s, %s]", get_container_element_type_or_variant(0).to_string(), get_container_element_type_or_variant(1).to_string());
            }
            return Variant::get_type_name(builtin_type);

        case NATIVE:
            if (is_meta_type) {
                return String(OScriptNativeClass::get_class_static());
            }
            return String(native_type);

        case CLASS:
            if (class_type->identifier != nullptr) {
                return String(class_type->identifier->name);
            }
            return String(class_type->fqcn);

        case SCRIPT: {
            if (is_meta_type) {
                return String(script_type.is_valid() ? script_type->get_class() : "");
            }
            String name = script_type.is_valid() ? script_type->get_name() : "";
            if (!name.is_empty()) {
                return name;
            }
            name = script_path;
            if (!name.is_empty()) {
                return name;
            }
            return String(native_type);
        }

        case ENUM: {
            // native_type contains either the native class defining the enum
            // or the fully qualified class name of the script defining the enum
            return String(native_type).get_file(); // Remove path, keep filename
        }

        case RESOLVING:
        case UNRESOLVED:
            return "<unresolved type>";

        default:
            ERR_FAIL_V_MSG("<unresolved type>", "Kind set outside the enum range.");
    }
}

PropertyInfo OScriptParserNodes::DataType::to_property_info(const String& p_name) const {
    PropertyInfo result;
	result.name = p_name;
	result.usage = PROPERTY_USAGE_NONE;

	if (!is_hard_type()) {
		result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
		return result;
	}

	switch (kind) {
		case BUILTIN:
			result.type = builtin_type;
			if (builtin_type == Variant::ARRAY && has_container_element_type(0)) {
				const DataType elem_type = get_container_element_type(0);
				switch (elem_type.kind) {
					case BUILTIN:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = Variant::get_type_name(elem_type.builtin_type);
						break;

					case NATIVE:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = elem_type.native_type;
						break;

					case SCRIPT:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						if (elem_type.script_type.is_valid() && elem_type.script_type->get_global_name() != StringName()) {
							result.hint_string = elem_type.script_type->get_global_name();
						} else {
							result.hint_string = elem_type.native_type;
						}
						break;

					case CLASS:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						if (elem_type.class_type != nullptr && elem_type.class_type->get_global_name() != StringName()) {
							result.hint_string = elem_type.class_type->get_global_name();
						} else {
							result.hint_string = elem_type.native_type;
						}
						break;

					case ENUM:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = String(elem_type.native_type).replace("::", ".");
						break;

					case VARIANT:
					case RESOLVING:
					case UNRESOLVED:
						break;
				}
			} else if (builtin_type == Variant::DICTIONARY && has_container_element_types()) {
				const DataType key_type = get_container_element_type_or_variant(0);
				const DataType value_type = get_container_element_type_or_variant(1);
				if ((key_type.kind == VARIANT && value_type.kind == VARIANT) || key_type.kind == RESOLVING ||
						key_type.kind == UNRESOLVED || value_type.kind == RESOLVING || value_type.kind == UNRESOLVED) {
					break;
				}
				String key_hint, value_hint;
				switch (key_type.kind) {
					case BUILTIN:
						key_hint = Variant::get_type_name(key_type.builtin_type);
						break;

					case NATIVE:
						key_hint = key_type.native_type;
						break;

					case SCRIPT:
						if (key_type.script_type.is_valid() && key_type.script_type->get_global_name() != StringName()) {
							key_hint = key_type.script_type->get_global_name();
						} else {
							key_hint = key_type.native_type;
						}
						break;

					case CLASS:
						if (key_type.class_type != nullptr && key_type.class_type->get_global_name() != StringName()) {
							key_hint = key_type.class_type->get_global_name();
						} else {
							key_hint = key_type.native_type;
						}
						break;

					case ENUM:
						key_hint = String(key_type.native_type).replace("::", ".");
						break;

					default:
						key_hint = "Variant";
						break;
				}
				switch (value_type.kind) {
					case BUILTIN:
						value_hint = Variant::get_type_name(value_type.builtin_type);
						break;

					case NATIVE:
						value_hint = value_type.native_type;
						break;

					case SCRIPT:
						if (value_type.script_type.is_valid() && value_type.script_type->get_global_name() != StringName()) {
							value_hint = value_type.script_type->get_global_name();
						} else {
							value_hint = value_type.native_type;
						}
						break;

					case CLASS:
						if (value_type.class_type != nullptr && value_type.class_type->get_global_name() != StringName()) {
							value_hint = value_type.class_type->get_global_name();
						} else {
							value_hint = value_type.native_type;
						}
						break;

					case ENUM:
						value_hint = String(value_type.native_type).replace("::", ".");
						break;

					default:
						value_hint = "Variant";
						break;
				}
				result.hint = PROPERTY_HINT_DICTIONARY_TYPE;
				result.hint_string = key_hint + ";" + value_hint;
			}
			break;
		case NATIVE:
			result.type = Variant::OBJECT;
			if (is_meta_type) {
				result.class_name = OScriptNativeClass::get_class_static();
			} else {
				result.class_name = native_type;
			}
			break;

		case SCRIPT:
			result.type = Variant::OBJECT;
			if (is_meta_type) {
				result.class_name = script_type.is_valid() ? script_type->get_class() : String(Script::get_class_static());
			} else if (script_type.is_valid() && script_type->get_global_name() != StringName()) {
				result.class_name = script_type->get_global_name();
			} else {
				result.class_name = native_type;
			}
			break;

		case CLASS:
			result.type = Variant::OBJECT;
			if (is_meta_type) {
				result.class_name = OScript::get_class_static();
			} else if (class_type != nullptr && class_type->get_global_name() != StringName()) {
				result.class_name = class_type->get_global_name();
			} else {
				result.class_name = native_type;
			}
			break;

		case ENUM:
			if (is_meta_type) {
				result.type = Variant::DICTIONARY;
			} else {
				result.type = Variant::INT;
				result.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
				result.class_name = String(native_type).replace("::", ".");
			}
			break;

		case VARIANT:
		case RESOLVING:
		case UNRESOLVED:
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
			break;
	}

	return result;
}

bool OScriptParserNodes::DataType::is_typed_container_type() const {
    return kind == BUILTIN && _variant_type_to_typed_array_element_type(builtin_type) != Variant::NIL;
}

OScriptParserNodes::DataType OScriptParserNodes::DataType::get_typed_container_type() const {
    DataType type;
    type.kind = BUILTIN;
    type.builtin_type = _variant_type_to_typed_array_element_type(builtin_type);
    return type;
}

bool OScriptParserNodes::DataType::can_reference(const DataType& p_other) const {
    if (p_other.is_meta_type) {
        return false;
    } else if (builtin_type != p_other.builtin_type) {
        return false;
    } else if (builtin_type != Variant::OBJECT) {
        return true;
    }

    if (native_type == StringName()) {
        return true;
    } else if (p_other.native_type == StringName()) {
        return false;
    } else if (native_type != p_other.native_type && !ClassDB::is_parent_class(p_other.native_type, native_type)) {
        return false;
    }

    Ref<Script> script = script_type;
    if (kind == CLASS && script.is_null()) {
        Error err = OK;
        Ref<OScript> scr = OScriptCache::get_shallow_script(script_path, err);
        ERR_FAIL_COND_V_MSG(err, false, vformat("(Error while getting cache for script \"%s\".)", script_path));
        script.reference_ptr(scr->find_class(class_type->fqcn));
    }

    Ref<Script> other_script = p_other.script_type;
    if (p_other.kind == CLASS && other_script.is_null()) {
        Error err = OK;
        Ref<OScript> scr = OScriptCache::get_shallow_script(p_other.script_path, err);
        ERR_FAIL_COND_V_MSG(err, false, vformat("(Error while getting cache for script \"%s\".)", p_other.script_path));
        other_script.reference_ptr(scr->find_class(p_other.class_type->fqcn));
    }

    if (script.is_null()) {
        return true;
    } else if (other_script.is_null()) {
        return false;
    } else if (script != other_script && !GDE::Script::inherits_script(other_script, script)) {
        return false;
    }

    return true;
}

bool OScriptParserNodes::DataType::operator==(const DataType& p_other) const {
    if (type_source == UNDETECTED || p_other.type_source == UNDETECTED) {
        return true; // Can be considered equal for parsing purposes.
    }
    if (type_source == INFERRED || p_other.type_source == INFERRED) {
        return true; // Can be considered equal for parsing purposes.
    }
    if (kind != p_other.kind) {
        return false;
    }
    switch (kind) {
        case VARIANT:
            return true; // All variants are the same.
        case BUILTIN:
            return builtin_type == p_other.builtin_type;
        case NATIVE:
        case ENUM:
            return native_type == p_other.native_type;
        case SCRIPT:
            return script_type == p_other.script_type;
        case CLASS:
            return class_type == p_other.class_type || class_type->fqcn == p_other.class_type->fqcn;
        case RESOLVING:
        case UNRESOLVED:
            break;
    }
    return false;
}

bool OScriptParserNodes::DataType::operator!=(const DataType& p_other) const {
    return !(*this == p_other);
}

void OScriptParserNodes::DataType::operator=(const DataType& p_other) {
    kind = p_other.kind;
    type_source = p_other.type_source;
    is_read_only = p_other.is_read_only;
    is_constant = p_other.is_constant;
    is_meta_type = p_other.is_meta_type;
    is_pseudo_type = p_other.is_pseudo_type;
    is_coroutine = p_other.is_coroutine;
    builtin_type = p_other.builtin_type;
    native_type = p_other.native_type;
    enum_type = p_other.enum_type;
    script_type = p_other.script_type;
    script_path = p_other.script_path;
    class_type = p_other.class_type;
    method_info = p_other.method_info;
    enum_values = p_other.enum_values;
    container_element_types = p_other.container_element_types;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AnnotationNode

bool OScriptParserNodes::AnnotationNode::apply(OScriptParser* p_this, Node* p_target, ClassNode* p_class) {
    if (is_applied) {
        return true;
    }

    is_applied = true;
    return (p_this->*(p_this->valid_annotations[name].apply))(this, p_target, p_class);
}

bool OScriptParserNodes::AnnotationNode::applies_to(uint32_t p_target_kinds) const {
    return (info->target_kind & p_target_kinds) > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ClassNode

String OScriptParserNodes::ClassNode::Member::get_name() const {
    switch (type) {
        case UNDEFINED:
            return "<undefined member>";
        case CLASS:
            return m_class->identifier->name;
        case CONSTANT:
            return constant->identifier->name;
        case FUNCTION:
            return function->identifier->name;
        case VARIABLE:
            return variable->identifier->name;
        case ENUM:
            return m_enum->identifier->name;
        case ENUM_VALUE:
            return enum_value.identifier->name;
        case GROUP:
            return annotation->export_info.name;
        default:
            return "";
    }
}

String OScriptParserNodes::ClassNode::Member::get_type_name() const {
    switch (type) {
        case UNDEFINED:
            return "???";
        case CLASS:
            return "class";
        case CONSTANT:
            return "constant";
        case FUNCTION:
            return "function";
        case SIGNAL:
            return "signal";
        case VARIABLE:
            return "variable";
        case ENUM:
            return "enum";
        case ENUM_VALUE:
            return "enum value";
        case GROUP:
            return "group";
        default:
            return "";
    }
}

int OScriptParserNodes::ClassNode::Member::get_script_node_id() const {
    switch (type) {
        case CLASS:
            return m_class->script_node_id;
        case CONSTANT:
            return constant->script_node_id;
        case FUNCTION:
            return function->script_node_id;
        case SIGNAL:
            return signal->script_node_id;
        case VARIABLE:
            return variable->script_node_id;
        case ENUM:
            return m_enum->script_node_id;
        case ENUM_VALUE:
            return enum_value.script_node_id;
        case GROUP:
        case UNDEFINED:
        default:
            ERR_FAIL_V_MSG(-1, "Reaching undefined member type.");
    }
}

OScriptParserNodes::DataType OScriptParserNodes::ClassNode::Member::get_data_type() const {
    switch (type) {
        case CLASS:
            return m_class->data_type;
        case CONSTANT:
            return constant->data_type;
        case FUNCTION:
            return function->data_type;
        case SIGNAL:
            return signal->data_type;
        case VARIABLE:
            return variable->data_type;
        case ENUM:
            return m_enum->data_type;
        case ENUM_VALUE:
            return enum_value.identifier->get_datatype();
        case GROUP:
            return DataType();
        case UNDEFINED:
            return DataType();
        default:
            ERR_FAIL_V_MSG(DataType(), "Reaching unhandled type.");
    }
}

OScriptParserNodes::Node* OScriptParserNodes::ClassNode::Member::get_source_node() const {
    switch (type) {
        case CLASS:
            return m_class;
        case CONSTANT:
            return constant;
        case FUNCTION:
            return function;
        case SIGNAL:
            return signal;
        case VARIABLE:
            return variable;
        case ENUM:
            return m_enum;
        case ENUM_VALUE:
            return enum_value.identifier;
        case GROUP:
        case UNDEFINED:
            return nullptr;
        default:
            ERR_FAIL_V_MSG(nullptr, "Reaching unhandled type.");
    }
}

StringName OScriptParserNodes::ClassNode::get_global_name() const {
    return identifier != nullptr ? identifier->name : StringName();
}

OScriptParserNodes::ClassNode::Member OScriptParserNodes::ClassNode::get_member(const StringName& p_name) const {
    return members[members_indices[p_name]];
}

bool OScriptParserNodes::ClassNode::has_member(const StringName& p_name) const {
    return members_indices.has(p_name);
}

bool OScriptParserNodes::ClassNode::has_function(const StringName& p_name) const {
    return has_member(p_name) && members[members_indices[p_name]].type == Member::FUNCTION;
}

void OScriptParserNodes::ClassNode::add_member(const EnumNode::Value& p_enum_value) {
    members_indices[p_enum_value.identifier->name] = members_indices.size();
    members.push_back(Member(p_enum_value));
}

void OScriptParserNodes::ClassNode::add_member_group(AnnotationNode* p_annotation) {
    StringName name = vformat("@group_%d_%s", members.size(), p_annotation->export_info.name);
    members_indices[name] = members.size();
    members.push_back(Member(p_annotation));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SuiteNode

OScriptParserNodes::DataType OScriptParserNodes::SuiteNode::Local::get_data_type() const {
    switch (type) {
        case CONSTANT:
            return constant->get_datatype();
        case VARIABLE:
            return variable->get_datatype();
        case PARAMETER:
            return parameter->get_datatype();
        case FOR_VARIABLE:
        case PATTERN_BIND:
            return bind->get_datatype();
        case UNDEFINED:
        default:
            return DataType();
    }
}

String OScriptParserNodes::SuiteNode::Local::get_name() const {
    switch (type) {
        case PARAMETER:
            return "parameter";
        case CONSTANT:
            return "constant";
        case VARIABLE:
            return "variable";
        case FOR_VARIABLE:
            return "for loop iterator";
        case PATTERN_BIND:
            return "pattern bind";
        case UNDEFINED:
            return "<undefined>";
        default:
            return String();
    }
}

bool OScriptParserNodes::SuiteNode::has_local(const StringName& p_name) const {
    if (locals_indices.has(p_name)) {
        return true;
    }
    if (parent_block != nullptr) {
        return parent_block->has_local(p_name);
    }
    return false;
}

void OScriptParserNodes::SuiteNode::add_local(const Local& p_local) {
    locals_indices[p_local.name] = locals_indices.size();
    locals.push_back(p_local);
}

const OScriptParserNodes::SuiteNode::Local& OScriptParserNodes::SuiteNode::get_local(const StringName& p_name) const {
    if (locals_indices.has(p_name)) {
        return locals[locals_indices[p_name]];
    }
    if (parent_block != nullptr) {
        return parent_block->get_local(p_name);
    }
    return empty;
}

bool OScriptParserNodes::SuiteNode::has_alias(const Ref<OScriptNodePin>& p_pin) {
    const uint64_t key = create_alias_key(p_pin);
    if (aliases.has(key)) {
        return true;
    }
    if (parent_block != nullptr) {
        return parent_block->has_alias(p_pin);
    }
    return false;
}

void OScriptParserNodes::SuiteNode::add_alias(const Ref<OScriptNodePin>& p_output, const StringName& p_alias) {
    const uint64_t key = create_alias_key(p_output);
    aliases[key] = p_alias;
}

StringName OScriptParserNodes::SuiteNode::get_alias(const Ref<OScriptNodePin>& p_pin) const {
    const uint64_t key = create_alias_key(p_pin);
    if (aliases.has(key)) {
        return aliases[key];
    }
    if (parent_block != nullptr) {
        return parent_block->get_alias(p_pin);
    }
    return "";
}

uint64_t OScriptParserNodes::SuiteNode::create_alias_key(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_null()) {
        return 0;
    }
    return (static_cast<uint64_t>(p_pin->get_owning_node()->get_id()) << 32) | p_pin->get_pin_index();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TypeNode

OScriptParserNodes::TypeNode* OScriptParserNodes::TypeNode::get_container_type_or_null(int p_index) const {
    return p_index >= 0 && p_index < container_types.size() ? container_types[p_index] : nullptr;
}