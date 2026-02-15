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
#ifndef ORCHESTRATOR_SCRIPT_DOCGEN_H
#define ORCHESTRATOR_SCRIPT_DOCGEN_H

#include "script/parser/parser.h"

using namespace godot;

#ifdef TOOLS_ENABLED
class OScriptDocGen {
    using Parser = OScriptParser;
    using Type = OScriptParser::DataType;

    static HashMap<String, String> _singletons; // Script path to singleton name.

    static String _get_script_name(const String& p_path);
    static String _get_class_name(const Parser::ClassNode& p_class);
    static void _doctype_from_script_type(const Type& p_script_type, String& r_type, String& r_enum, bool p_is_return = false);
    static String _docvalue_from_variant(const Variant& p_value, int p_recursion_level = 1);
    static void _generate_docs(OScript* p_script, const Parser::ClassNode* p_class);

public:
    static void generate_docs(OScript* p_script, const Parser::ClassNode* p_class);
    static void doc_type_from_script_type(const Type& p_script_type, String& r_type, String& r_enum, bool p_is_return = false);
    static String doc_value_from_expression(const Parser::ExpressionNode* p_expression);
};
#endif

#endif // ORCHESTRATOR_SCRIPT_DOCGEN_H