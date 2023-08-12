## Helper class for managing a [Dictionary].
@tool
class_name OrchestratorDictionary
extends RefCounted

var _data : Dictionary

## Create the [OrchestratorDictionary] with a reference to the underlying dictionary.
func _init(data: Dictionary) -> void:
	_data = data


## Get dictionary element by key, with an optional default value if the key isn't found.
func get_default(property_name: StringName, default_value : Variant = null) -> Variant:
	if _data.has(property_name):
		return _data.get(property_name)
	return default_value


## Set the dictionary element by key with a value.
func set(property_name: StringName, value: Variant) -> void:
	_data[property_name] = value


## Removes the specified dictionary element by key.
func erase(property_name: StringName) -> void:
	_data.erase(property_name)

