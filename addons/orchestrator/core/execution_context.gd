## An execution context for running orchestrations at runtime.
class_name OrchestrationExecutionContext
extends RefCounted

var _data : Dictionary
var _state : Dictionary
var _orchestration : Orchestration
var _locals : Dictionary


func get_node_data() -> Dictionary:
	return _data


func get_orchestration() -> Orchestration:
	return _orchestration


func get_locals() -> Dictionary:
	return _locals


func get_state(property_name: String, default_value : Variant = null) -> Variant:
	if _state.has(property_name):
		return _state[property_name]
	return default_value


func set_state(property_name: String, value : Variant) -> void:
	_state[property_name] = value


func remove_state() -> void:
	_state = {}


func require_attribute(attribute_name: String) -> void:
	if not has_attribute(attribute_name):
		assert(false, "Node expected attribute '%s'." % attribute_name)


func require_attributes(attribute_names: Array) -> void:
	for attribute_name in attribute_names:
		require_attribute(attribute_name)


func has_attribute(attribute_name: String) -> bool:
	if _data.has("attributes"):
		return _data["attributes"].has(attribute_name)
	return false


func get_attribute(attribute_name: String, default_value : Variant = null) -> Variant:
	if has_attribute(attribute_name):
		return _data["attributes"][attribute_name]
	return default_value


func get_output_target_node_id(port: int) -> Variant:
	var target_id : Variant = -1
	if _data.has("connections"):
		var connections = _data["connections"]
		for connection in connections:
			if connection["source_port"] == port:
				target_id = connection["target_id"]
	return target_id


func editor_print(message: String) -> void:
	if OS.has_feature("editor"):
		print(message)
