## A factory that can create OrchestrationNode instances.
@tool
extends Node

## Emitted when the node factory detects filesystem changes.
signal node_factory_updated

# Collection of [OrchestrationNode] resources, keyed by type
var _nodes : Dictionary

## Returns an array of [OrchestrationNode] script instances
func get_resources() -> Array:
	return _nodes.values()


## Gets a node's resource object by its type.
func get_node_resource(node_type: int) -> OrchestrationNode:
	if _nodes.has(node_type):
		return _nodes[node_type].duplicate()
	return null


## Rescans for [OrchestrationNode] scripts
func rescan_for_resources() -> void:
	var nodes : Dictionary = {}
	for script_file in _get_all_scripts_in_path():
		var script : GDScript = _load_script(script_file)
		if not script:
			continue

		# All Orchestrator scripts are resources
		if script.get_instance_base_type() != "Resource":
			continue

		if _is_script_valid_with_noarg_init(script):
			var script_inst = script.new()
			if script_inst is OrchestrationNode:
				if not script_inst.type in nodes:
					nodes[script_inst.type] = script_inst
					continue
				else:
					printerr("node %s has duplicate id %s" % [script_inst.name, script_file])
			_free(script_inst)
			continue
	_nodes = nodes
	node_factory_updated.emit()


func _load_script(script_file : String) -> GDScript:
	if ResourceLoader.has_cached(script_file):
		# Load it from the cache
		return load(script_file)

	# Explicitly use sub threads to avoid failure on main thread
	var error = ResourceLoader.load_threaded_request(script_file, "", true)
	if error != OK:
		return null

	# Wait for loading
	var status = ResourceLoader.THREAD_LOAD_IN_PROGRESS
	while status == ResourceLoader.THREAD_LOAD_IN_PROGRESS:
		status = ResourceLoader.load_threaded_get_status(script_file)

	if status == ResourceLoader.THREAD_LOAD_FAILED:
		return null

	return ResourceLoader.load_threaded_get(script_file)

func _is_script_valid_with_noarg_init(script: Script) -> bool:
	if script.can_instantiate():
		for method in script.get_script_method_list():
			if method.has("name") and method["name"] == "_init":
				if method.has("args") and method["args"].size() == 0:
					return true
	return false


static func _get_all_scripts_in_path(path: String = "res://") -> PackedStringArray:
	var files: PackedStringArray = []
	if DirAccess.dir_exists_absolute(path):
		var dir = DirAccess.open(path)
		dir.list_dir_begin()
		var file_name = dir.get_next()
		while file_name != "":
			var file_path : String = (path + "/" + file_name).simplify_path()
			if dir.current_is_dir():
				if not file_name in [".godot", ".tmp"]:
					files.append_array(_get_all_scripts_in_path(file_path))
			elif file_name.get_extension() == "gd":
				files.append(file_path)
			file_name = dir.get_next()
	return files


static func _free(any) -> void:
	if any is Node and is_instance_valid(any):
		any.queue_free()
	elif any is RefCounted:
		return
	elif any is Object:
		any.free()
	elif any is Array:
		for element in any:
			_free(element)
