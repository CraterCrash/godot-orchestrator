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
#ifndef ORCHESTRATOR_GODOT_EXTENSION_DB_GENERATED_H
#define ORCHESTRATOR_GODOT_EXTENSION_DB_GENERATED_H

#include "common/variant_operators.h"

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string_name.hpp>

// THIS FILE IS GENERATED. EDITS WILL BE LOST.

namespace godot
{
	/// Describes a mapping between an enum name and value
	struct EnumValue
	{
		StringName name;
		StringName friendly_name;
		int value{ 0 };
	};
	
	/// Describes a definition of an Enumeration type
	struct EnumInfo
	{
		StringName name;
		bool is_bitfield{ false };
		Vector<EnumValue> values;
	};
	
	/// Describes a function
	struct FunctionInfo
	{
		StringName name;
		PropertyInfo return_val;
		StringName category;
		bool is_vararg{ false };
		Vector<PropertyInfo> arguments;
	};
	
	/// Describes an operator for a Godot type
	struct OperatorInfo
	{
		VariantOperators::Code op{ VariantOperators::OP_EQUAL };
		StringName code;
		StringName name;
		Variant::Type left_type{ Variant::NIL };
		StringName left_type_name;
		Variant::Type right_type{ Variant::NIL };
		StringName right_type_name;
		Variant::Type return_type{ Variant::NIL };
	};
	
	/// Describes a constructor definition
	struct ConstructorInfo
	{
		Vector<PropertyInfo> arguments;
	};
	
	/// Describes a Constant definition
	struct ConstantInfo
	{
		StringName name;
		Variant::Type type{ Variant::NIL };
		Variant value;
	};
	
	/// Builtin Godot Type details
	struct BuiltInType
	{
		StringName name;
		Variant::Type type{ Variant::NIL };
		bool keyed{ false };
		bool has_destructor{ false };
		Vector<OperatorInfo> operators;
		Vector<ConstructorInfo> constructors;
		Vector<MethodInfo> methods;
		Vector<PropertyInfo> properties;
		Vector<ConstantInfo> constants;
		Vector<EnumInfo> enums;
		Variant::Type index_returning_type{ Variant::NIL };
	};
	
	/// Describes a Godot Class
	struct ClassInfo
	{
		StringName name;
		Vector<StringName> bitfield_enums;
		HashMap<StringName, int64_t> static_function_hashes;
	};
	
	namespace internal
	{
		/// Populates the contents of the ExtensionDB singleton database
		class ExtensionDBLoader
		{
			/// Populates Math Constants
			void prime_math_constants();
			
			/// Populates Global Enumerations
			void prime_global_enumerations();
			
			/// Populates Builtin Data Types
			void prime_builtin_classes();
			
			/// Populates Utility Functions
			void prime_utility_functions();
			
			/// Populate class details
			void prime_class_details();
		
		public:
			/// Populates the ExtensionDB
			void prime();
		};
	}
	
	/// A simple database that exposes GDExtension and Godot details
	/// This is intended to supplement ClassDB, which does not expose all details to GDExtension.
	class ExtensionDB
	{
		friend class internal::ExtensionDBLoader;
		static ExtensionDB* _singleton;
		
		HashMap<Variant::Type, StringName> _builtin_types_to_name;
		HashMap<StringName, BuiltInType> _builtin_types;
		PackedStringArray _builtin_type_names;
		
		PackedStringArray _global_enum_names;
		PackedStringArray _global_enum_value_names;
		HashMap<StringName, EnumInfo> _global_enums;
		
		PackedStringArray _math_constant_names;
		HashMap<StringName, ConstantInfo> _math_constants;
		
		PackedStringArray _function_names;
		HashMap<StringName, FunctionInfo> _functions;
		
		HashMap<StringName, ClassInfo> _classes;
		
	public:
		ExtensionDB();
		~ExtensionDB();
		
		static PackedStringArray get_builtin_type_names();
		static BuiltInType get_builtin_type(const StringName& p_type_name);
		static BuiltInType get_builtin_type(Variant::Type p_type);
		
		static PackedStringArray get_global_enum_names();
		static PackedStringArray get_global_enum_value_names();
		static EnumInfo get_global_enum(const StringName& p_enum_name);
		static EnumInfo get_global_enum_by_value(const StringName& p_name);
		static EnumValue get_global_enum_value(const StringName& p_enum_value_name);
		
		static PackedStringArray get_math_constant_names();
		static ConstantInfo get_math_constant(const StringName& p_constant_name);
		
		static PackedStringArray get_function_names();
		static FunctionInfo get_function(const StringName& p_name);
		
		static bool is_class_enum_bitfield(const StringName& p_class_name, const String& p_enum_name);
		
		static PackedStringArray get_static_function_names(const StringName& p_class_name);
		static int64_t get_static_function_hash(const StringName& p_class_name, const StringName& p_function_name);
	};
	
}


#endif // ORCHESTRATOR_GODOT_EXTENSION_DB_GENERATED_H
