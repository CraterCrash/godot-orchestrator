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
#include "core/godot/doc_data.h"

bool DocData::ArgumentDoc::operator<(const ArgumentDoc& p_doc) const {
    if (name == p_doc.name) {
        return type < p_doc.type;
    }
    return name < p_doc.name;
}

DocData::ArgumentDoc DocData::ArgumentDoc::from_dict(const Dictionary& p_dict) {
    ArgumentDoc doc;
    doc.name = p_dict.get("name", "");
    doc.type = p_dict.get("type", "");
    doc.enumeration = p_dict.get("enumeration", "");
    doc.is_bitfield = p_dict.get("is_bitfield", false);
    doc.default_value = p_dict.get("default_value", "");
    return doc;
}

Dictionary DocData::ArgumentDoc::to_dict(const ArgumentDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.name.is_empty()) {
        dict["name"] = p_doc.name;
    }
    if (!p_doc.type.is_empty()) {
        dict["type"] = p_doc.type;
    }
    if (!p_doc.enumeration.is_empty()) {
        dict["enumeration"] = p_doc.enumeration;
        dict["is_bitfield"] = p_doc.is_bitfield;
    }
    if (!p_doc.default_value.is_empty()) {
        dict["default_value"] = p_doc.default_value;
    }
    return dict;
}

bool DocData::MethodDoc::operator<(const MethodDoc& p_doc) const {
    if (name == p_doc.name) {
        // Must be an operator or a constructor since there is no other overloading
        if (name.substr(8) == "operator") {
            if (arguments.size() == p_doc.arguments.size()) {
                if (arguments.is_empty()) {
                    return false;
                }
                return arguments[0].type < p_doc.arguments[0].type;
            }
            return arguments.size() < p_doc.arguments.size();
        }

        // Must be a constructor
        // We want this arbitrary order for a class "Foo":
        // - 1. Default constructor: Foo()
        // - 2. Copy constructor: Foo(Foo)
        // - 3. Other constructor: Foo(Bar, ...) based on first argument's name
        if (arguments.is_empty() || p_doc.arguments.is_empty()) {
            return arguments.size() < p_doc.arguments.size();
        }
        if (arguments[0].type == return_type && p_doc.arguments[0].type == p_doc.return_type) {
            return arguments[0].type == return_type || p_doc.arguments[0].type == p_doc.return_type;
        }
        return arguments[0] < p_doc.arguments[0];
    }
    return name.naturalcasecmp_to(p_doc.name) < 0;
}

DocData::MethodDoc DocData::MethodDoc::from_dict(const Dictionary& p_dict) {
    MethodDoc doc;
    doc.name = p_dict.get("name", "");
    doc.return_type = p_dict.get("return_type", "");
    if (p_dict.has("return_enum")) {
        doc.return_enum = p_dict.get("return_enum", "");
        doc.return_is_bitfield = p_dict.get("return_is_bitfield", false);
    }
    doc.qualifiers = p_dict.get("qualifiers", "");
    doc.description = p_dict.get("description", "");

    #ifndef DISABLE_DEPRECATED
    doc.is_deprecated = p_dict.get("is_deprecated", false);
    doc.is_experimental = p_dict.get("is_experimental", false);
    #endif

    if (p_dict.has("deprecated")) {
        doc.is_deprecated = true;
        doc.deprecated_message = p_dict["deprecated"];
    }
    if (p_dict.has("experimental")) {
        doc.is_experimental = true;
        doc.experimental_message = p_dict["experimental"];
    }

    const Array arguments = p_dict.get("arguments", Array());
    for (int i = 0; i < arguments.size(); i++) {
        doc.arguments.push_back(ArgumentDoc::from_dict(arguments[i]));
    }

    const Array errors_returned = p_dict.get("errors_returned", Array());
    for (int i = 0; i < errors_returned.size(); i++) {
        doc.errors_returned.push_back(errors_returned[i]);
    }

    doc.keywords = p_dict.get("keywords", "");
    return doc;
}

Dictionary DocData::MethodDoc::to_dict(const MethodDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.name.is_empty()) {
        dict["name"] = p_doc.name;
    }
    if (!p_doc.return_type.is_empty()) {
        dict["return_type"] = p_doc.return_type;
    }
    if (!p_doc.return_enum.is_empty()) {
        dict["return_enum"] = p_doc.return_enum;
        dict["return_is_bitfield"] = p_doc.return_is_bitfield;
    }
    if (!p_doc.qualifiers.is_empty()) {
        dict["qualifiers"] = p_doc.qualifiers;
    }
    if (!p_doc.description.is_empty()) {
        dict["description"] = p_doc.description;
    }
    if (p_doc.is_deprecated) {
        dict["deprecated"] = p_doc.deprecated_message;
    }
    if (p_doc.is_experimental) {
        dict["experimental"] = p_doc.experimental_message;
    }
    if (!p_doc.keywords.is_empty()) {
        dict["keywords"] = p_doc.keywords;
    }
    if (!p_doc.arguments.is_empty()) {
        Array arguments;
        for (int i = 0; i < p_doc.arguments.size(); i++) {
            arguments.push_back(ArgumentDoc::to_dict(p_doc.arguments[i]));
        }
        dict["arguments"] = arguments;
    }
    if (!p_doc.errors_returned.is_empty()) {
        Array errors_returned;
        for (int i = 0; i < p_doc.errors_returned.size(); i++) {
            errors_returned.push_back(p_doc.errors_returned[i]);
        }
        dict["errors_returned"] = errors_returned;
    }
    return dict;
}

bool DocData::ConstantDoc::operator<(const ConstantDoc& p_doc) const {
    return name < p_doc.name;
}

DocData::ConstantDoc DocData::ConstantDoc::from_dict(const Dictionary& p_dict) {
    ConstantDoc doc;
    doc.name = p_dict.get("name", "");
    doc.value = p_dict.get("value", "");
    doc.is_value_valid = p_dict.get("is_value_valid", false);
    doc.type = p_dict.get("type", "");
    if (p_dict.has("enumeration")) {
        doc.enumeration = p_dict["enumeration"];
        doc.is_bitfield = p_dict.get("is_bitfield", false);
    }
    doc.description = p_dict.get("description", "");

    #ifndef DISABLE_DEPRECATED
    doc.is_deprecated = p_dict.get("is_deprecated", false);
    doc.is_experimental = p_dict.get("is_experimental", false);
    #endif

    if (p_dict.has("deprecated")) {
        doc.is_deprecated = true;
        doc.deprecated_message = p_dict["deprecated"];
    }
    if (p_dict.has("experimental")) {
        doc.is_experimental = true;
        doc.experimental_message = p_dict["experimental"];
    }
    doc.keywords = p_dict.get("keywords", "");
    return doc;
}

Dictionary DocData::ConstantDoc::to_dict(const ConstantDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.name.is_empty()) {
        dict["name"] = p_doc.name;
    }
    if (!p_doc.value.is_empty()) {
        dict["value"] = p_doc.value;
    }

    dict["is_value_valid"] = p_doc.is_value_valid;
    dict["type"] = p_doc.type;

    if (!p_doc.enumeration.is_empty()) {
        dict["enumeration"] = p_doc.enumeration;
        dict["is_bitfield"] = p_doc.is_bitfield;
    }
    if (!p_doc.description.is_empty()) {
        dict["description"] = p_doc.description;
    }
    if (p_doc.is_deprecated) {
        dict["deprecated"] = p_doc.deprecated_message;
    }
    if (p_doc.is_experimental) {
        dict["experimental"] = p_doc.experimental_message;
    }
    if (!p_doc.keywords.is_empty()) {
        dict["keywords"] = p_doc.keywords;
    }
    return dict;
}

bool DocData::PropertyDoc::operator<(const PropertyDoc& p_doc) const {
    return name.naturalcasecmp_to(p_doc.name) < 0;
}

DocData::PropertyDoc DocData::PropertyDoc::from_dict(const Dictionary& p_dict) {
    PropertyDoc doc;
    doc.name = p_dict.get("name", "");
    doc.type = p_dict.get("type", "");
    if (p_dict.has("enumeration")) {
        doc.enumeration = p_dict["enumeration"];
        doc.is_bitfield = p_dict.get("is_bitfield", false);
    }
    doc.description = p_dict.get("description", "");
    doc.setter = p_dict.get("setter", "");
    doc.getter = p_dict.get("getter", "");
    doc.default_value = p_dict.get("default_value", "");
    doc.overridden = p_dict.get("overridden", false);
    doc.overrides = p_dict.get("overrides", "");

    #ifndef DISABLE_DEPRECATED
    doc.is_deprecated = p_dict.get("is_deprecated", false);
    doc.is_experimental = p_dict.get("is_experimental", false);
    #endif

    if (p_dict.has("deprecated")) {
        doc.is_deprecated = true;
        doc.deprecated_message = p_dict["deprecated"];
    }
    if (p_dict.has("experimental")) {
        doc.is_experimental = true;
        doc.experimental_message = p_dict["experimental"];
    }
    doc.keywords = p_dict.get("keywords", "");
    return doc;
}

Dictionary DocData::PropertyDoc::to_dict(const PropertyDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.name.is_empty()) {
        dict["name"] = p_doc.name;
    }
    if (!p_doc.type.is_empty()) {
        dict["type"] = p_doc.type;
    }
    if (!p_doc.enumeration.is_empty()) {
        dict["enumeration"] = p_doc.enumeration;
        dict["is_bitfield"] = p_doc.is_bitfield;
    }
    if (!p_doc.description.is_empty()) {
        dict["description"] = p_doc.description;
    }
    if (!p_doc.setter.is_empty()) {
        dict["setter"] = p_doc.setter;
    }
    if (!p_doc.getter.is_empty()) {
        dict["getter"] = p_doc.getter;
    }
    if (!p_doc.default_value.is_empty()) {
        dict["default_value"] = p_doc.default_value;
    }
    dict["overridden"] = p_doc.overridden;
    if (!p_doc.overrides.is_empty()) {
        dict["overrides"] = p_doc.overrides;
    }
    if (p_doc.is_deprecated) {
        dict["deprecated"] = p_doc.deprecated_message;
    }
    if (p_doc.is_experimental) {
        dict["experimental"] = p_doc.experimental_message;
    }
    if (!p_doc.keywords.is_empty()) {
        dict["keywords"] = p_doc.keywords;
    }
    return dict;
}

bool DocData::ThemeItemDoc::operator<(const ThemeItemDoc& p_doc) const {
    // First sort by the data type, then by name.
    if (data_type == p_doc.data_type) {
        return name.naturalcasecmp_to(p_doc.name) < 0;
    }
    return data_type < p_doc.data_type;
}

DocData::ThemeItemDoc DocData::ThemeItemDoc::from_dict(const Dictionary& p_dict) {
    ThemeItemDoc doc;
    doc.name = p_dict.get("name", "");
    doc.type = p_dict.get("type", "");
    doc.data_type = p_dict.get("data_type", "");
    doc.description = p_dict.get("description", "");
    if (p_dict.has("deprecated")) {
        doc.is_deprecated = true;
        doc.deprecated_message = p_dict["deprecated"];
    }
    if (p_dict.has("experimental")) {
        doc.is_experimental = true;
        doc.experimental_message = p_dict["experimental"];
    }
    doc.default_value = p_dict.get("default_value", "");
    doc.keywords = p_dict.get("keywords", "");
    return doc;
}

Dictionary DocData::ThemeItemDoc::to_dict(const ThemeItemDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.name.is_empty()) {
        dict["name"] = p_doc.name;
    }
    if (!p_doc.type.is_empty()) {
        dict["type"] = p_doc.type;
    }
    if (!p_doc.data_type.is_empty()) {
        dict["data_type"] = p_doc.data_type;
    }
    if (!p_doc.description.is_empty()) {
        dict["description"] = p_doc.description;
    }
    if (p_doc.is_deprecated) {
        dict["deprecated"] = p_doc.deprecated_message;
    }
    if (p_doc.is_experimental) {
        dict["experimental"] = p_doc.experimental_message;
    }
    if (!p_doc.default_value.is_empty()) {
        dict["default_value"] = p_doc.default_value;
    }
    if (!p_doc.keywords.is_empty()) {
        dict["keywords"] = p_doc.keywords;
    }
    return dict;
}

DocData::TutorialDoc DocData::TutorialDoc::from_dict(const Dictionary& p_dict) {
    TutorialDoc doc;
    doc.link = p_dict.get("link", "");
    doc.title = p_dict.get("title", "");
    return doc;
}

Dictionary DocData::TutorialDoc::to_dict(const TutorialDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.link.is_empty()) {
        dict["link"] = p_doc.link;
    }
    if (!p_doc.title.is_empty()) {
        dict["title"] = p_doc.title;
    }
    return dict;
}

DocData::EnumDoc DocData::EnumDoc::from_dict(const Dictionary& p_dict) {
    EnumDoc doc;
    doc.description = p_dict.get("description", "");
    #ifndef DISABLE_DEPRECATED
    doc.is_deprecated = p_dict.get("is_deprecated", false);
    doc.is_experimental = p_dict.get("is_experimental", false);
    #endif
    if (p_dict.has("deprecated")) {
        doc.is_deprecated = true;
        doc.deprecated_message = p_dict["deprecated"];
    }
    if (p_dict.has("experimental")) {
        doc.is_experimental = true;
        doc.experimental_message = p_dict["experimental"];
    }
    return doc;
}

Dictionary DocData::EnumDoc::to_dict(const EnumDoc& p_doc) {
    Dictionary dict;
    if (!p_doc.description.is_empty()) {
        dict["description"] = p_doc.description;
    }
    if (p_doc.is_deprecated) {
        dict["deprecated"] = p_doc.deprecated_message;
    }
    if (p_doc.is_experimental) {
        dict["experimental"] = p_doc.experimental_message;
    }
    return dict;
}

bool DocData::ClassDoc::operator<(const ClassDoc& p_doc) const {
    return name < p_doc.name;
}

DocData::ClassDoc DocData::ClassDoc::from_dict(const Dictionary& p_dict) {
    ClassDoc doc;
    doc.name = p_dict.get("name", "");
	doc.inherits = p_dict.get("inherits", "");
    doc.brief_description = p_dict.get("brief_description", "");
    doc.description = p_dict.get("description", "");
    doc.keywords = p_dict.get("keywords", "");

	const Array tutorials = p_dict.get("tutorials", Array());
	for (int i = 0; i < tutorials.size(); i++) {
		doc.tutorials.push_back(TutorialDoc::from_dict(tutorials[i]));
	}

	const Array constructors = p_dict.get("constructors", Array());
	for (int i = 0; i < constructors.size(); i++) {
		doc.constructors.push_back(MethodDoc::from_dict(constructors[i]));
	}

	const Array methods = p_dict.get("methods", Array());
	for (int i = 0; i < methods.size(); i++) {
		doc.methods.push_back(MethodDoc::from_dict(methods[i]));
	}

	const Array operators = p_dict.get("operators", Array());
	for (int i = 0; i < operators.size(); i++) {
		doc.operators.push_back(MethodDoc::from_dict(operators[i]));
	}

	const Array signals = p_dict.get("signals", Array());
	for (int i = 0; i < signals.size(); i++) {
		doc.signals.push_back(MethodDoc::from_dict(signals[i]));
	}

	const Array constants = p_dict.get("constants", Array());
	for (int i = 0; i < constants.size(); i++) {
		doc.constants.push_back(ConstantDoc::from_dict(constants[i]));
	}

	const Dictionary enums = p_dict.get("enums", Dictionary());
    for (const Variant& key : enums.keys()) {
        doc.enums[key] = EnumDoc::from_dict(enums[key]);
	}

	const Array properties = p_dict.get("properties", Array());
	for (int i = 0; i < properties.size(); i++) {
		doc.properties.push_back(PropertyDoc::from_dict(properties[i]));
	}

	const Array annotations = p_dict.get("annotations", Array());
	for (int i = 0; i < annotations.size(); i++) {
		doc.annotations.push_back(MethodDoc::from_dict(annotations[i]));
	}

	const Array theme_properties = p_dict.get("theme_properties", Array());
	for (int i = 0; i < theme_properties.size(); i++) {
		doc.theme_properties.push_back(ThemeItemDoc::from_dict(theme_properties[i]));
	}

    #ifndef DISABLE_DEPRECATED
    doc.is_deprecated = p_dict.get("is_deprecated", false);
    doc.is_experimental = p_dict.get("is_experimental", false);
    #endif

	if (p_dict.has("deprecated")) {
		doc.is_deprecated = true;
		doc.deprecated_message = p_dict["deprecated"];
	}
	if (p_dict.has("experimental")) {
		doc.is_experimental = true;
		doc.experimental_message = p_dict["experimental"];
	}

    doc.is_script_doc = p_dict.get("is_script_doc", false);
    doc.script_path = p_dict.get("script_path", "");
	return doc;
}

Dictionary DocData::ClassDoc::to_dict(const ClassDoc& p_doc) {
    Dictionary dict;
	if (!p_doc.name.is_empty()) {
		dict["name"] = p_doc.name;
	}
	if (!p_doc.inherits.is_empty()) {
		dict["inherits"] = p_doc.inherits;
	}
	if (!p_doc.brief_description.is_empty()) {
		dict["brief_description"] = p_doc.brief_description;
	}
	if (!p_doc.description.is_empty()) {
		dict["description"] = p_doc.description;
	}
	if (!p_doc.tutorials.is_empty()) {
		Array tutorials;
		for (int i = 0; i < p_doc.tutorials.size(); i++) {
			tutorials.push_back(TutorialDoc::to_dict(p_doc.tutorials[i]));
		}
		dict["tutorials"] = tutorials;
	}
	if (!p_doc.constructors.is_empty()) {
		Array constructors;
		for (int i = 0; i < p_doc.constructors.size(); i++) {
			constructors.push_back(MethodDoc::to_dict(p_doc.constructors[i]));
		}
		dict["constructors"] = constructors;
	}
	if (!p_doc.methods.is_empty()) {
		Array methods;
		for (int i = 0; i < p_doc.methods.size(); i++) {
			methods.push_back(MethodDoc::to_dict(p_doc.methods[i]));
		}
		dict["methods"] = methods;
	}
	if (!p_doc.operators.is_empty()) {
		Array operators;
		for (int i = 0; i < p_doc.operators.size(); i++) {
			operators.push_back(MethodDoc::to_dict(p_doc.operators[i]));
		}
		dict["operators"] = operators;
	}
	if (!p_doc.signals.is_empty()) {
		Array signals;
		for (int i = 0; i < p_doc.signals.size(); i++) {
			signals.push_back(MethodDoc::to_dict(p_doc.signals[i]));
		}
		dict["signals"] = signals;
	}
	if (!p_doc.constants.is_empty()) {
		Array constants;
		for (int i = 0; i < p_doc.constants.size(); i++) {
			constants.push_back(ConstantDoc::to_dict(p_doc.constants[i]));
		}
		dict["constants"] = constants;
	}
	if (!p_doc.enums.is_empty()) {
		Dictionary enums;
		for (const KeyValue<String, EnumDoc> &E : p_doc.enums) {
			enums[E.key] = EnumDoc::to_dict(E.value);
		}
		dict["enums"] = enums;
	}
	if (!p_doc.properties.is_empty()) {
		Array properties;
		for (int i = 0; i < p_doc.properties.size(); i++) {
			properties.push_back(PropertyDoc::to_dict(p_doc.properties[i]));
		}
		dict["properties"] = properties;
	}
	if (!p_doc.annotations.is_empty()) {
		Array annotations;
		for (int i = 0; i < p_doc.annotations.size(); i++) {
			annotations.push_back(MethodDoc::to_dict(p_doc.annotations[i]));
		}
		dict["annotations"] = annotations;
	}
	if (!p_doc.theme_properties.is_empty()) {
		Array theme_properties;
		for (int i = 0; i < p_doc.theme_properties.size(); i++) {
			theme_properties.push_back(ThemeItemDoc::to_dict(p_doc.theme_properties[i]));
		}
		dict["theme_properties"] = theme_properties;
	}
	if (p_doc.is_deprecated) {
		dict["deprecated"] = p_doc.deprecated_message;
	}
	if (p_doc.is_experimental) {
		dict["experimental"] = p_doc.experimental_message;
	}
	dict["is_script_doc"] = p_doc.is_script_doc;
	if (!p_doc.script_path.is_empty()) {
		dict["script_path"] = p_doc.script_path;
	}
	if (!p_doc.keywords.is_empty()) {
		dict["keywords"] = p_doc.keywords;
	}
	return dict;
}