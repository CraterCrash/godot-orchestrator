## A GraphEdit implementation for designing Orchestrations
@tool
extends GraphEdit

## The editor plugin
var editor_plugin : EditorPlugin

# The associated orchestration
var _orchestration : Orchestration

func _ready():
	minimap_enabled = false
	right_disconnects = true


func _can_drop_data(at_position: Vector2, data: Variant) -> bool:
	if not data or typeof(data) != TYPE_DICTIONARY:
		return false

	if not (data.has("origin") and data.has("origin_tree_item")):
		return false

	return true


func _drop_data(at_position: Vector2, data: Variant) -> void:
	var tree_item : TreeItem = data["origin_tree_item"]
	var node_type = tree_item.get_meta("_node_type")
	var resource = _get_node_factory().get_node_resource(node_type)
	if not resource:
		return

	if not _is_multiple_of_node_allowed(resource):
		OS.alert("There is already a \"" + resource.name + "\" node in the orchestration.")
		return

	var scene_node = OrchestrationGraphNode.new()
	add_child(scene_node)
	scene_node.name = "GraphNode-%s" % _get_next_node_id()
	scene_node.position_offset = (at_position + scroll_offset) / zoom
	var attributes = OrchestratorDictionary.new({})
	scene_node.initialize(attributes, resource)


###############################################################################
# Public API

## Intitialize the [GraphEdit] based on the provided orchestration.
func set_orchestration(orchestration: Orchestration) -> void:
	_orchestration = orchestration

	# Apply graph nodes
	for node in orchestration.nodes:
		_add_orchestration_graph_node(node)

	# Apply connections
	for connection in orchestration.connections:
		_add_orchestration_connection(connection)

	# This is needed to make sure that the GraphEdit is fully initialized
	# with all the node data and connections before we apply the zoom and
	# scroll offset, or else the UI won't update properly.
	await get_tree().process_frame

	# These must be set afterward once the graph has been loaded.
	zoom = orchestration.zoom
	scroll_offset = orchestration.scroll_offset


## Stores the orchestration data in the orchestration resource
func apply_graph_to_orchestration(orchestration: Orchestration) -> void:
	# Clear the nodes and populate the basic information
	orchestration.nodes.clear()
	orchestration.connections = get_connection_list()
	orchestration.zoom = zoom
	orchestration.scroll_offset = scroll_offset

	# Iterate all graph nodes and create orchestration node entries
	for child in get_children():
		if child is OrchestrationGraphNode:
			var orchestration_node = child.get_orchestration_node()
			var attributes = {}
			attributes["id"] = orchestration_node.id
			attributes["type"] = orchestration_node.type
			attributes["type_name"] = orchestration_node.name
			attributes["name"] = child.name
			attributes["position"] = child.position_offset
			attributes["size"] = child.size
			attributes["attributes"] = orchestration_node.get_attributes()
			attributes["connections"] = get_node_connections(child)
			orchestration.nodes.push_back(attributes)


## Get the output connection from a node with a given [code]id[/code] and [code]port[/code].
func get_node_output(id: Variant, port: int) -> Dictionary:
	for entry in get_connection_list():
		if entry["from"] == id and entry["from_port"] == port:
			return entry
	return {}


## Get all connections for the specified [GraphNode].
func get_node_connections(node: GraphNode) -> Array[Dictionary]:
	var node_connections : Array[Dictionary]
	for connection in get_connection_list():
		if connection["from"] == node.name:
			var node_connection = {}
			node_connection["source_port"] = connection["from_port"]
			node_connection["target_node"] = connection["to"]
			node_connection["target_port"] = connection["to_port"]
			var other_node = find_child(connection["to"], false, false)
			if other_node:
				node_connection["target_id"] = other_node.name
			node_connections.push_back(node_connection)
	return node_connections


## Clear all node connections for the [GraphEdit].
func clear() -> void:
	clear_connections()
	for child in get_children():
		if child is OrchestrationGraphNode:
			remove_child(child)
			child.queue_free()


## Clear all node connections for the specified [GraphNode].
func clear_node_connections(node: GraphNode) -> void:
	# todo: maybe notify child of disconnect?
	for conn in get_connection_list():
		if conn["from"] == node.name:
			disconnect_node(conn["from"], conn["from_port"], conn["to"], conn["to_port"])
		elif conn["to"] == node.name:
			disconnect_node(conn["from"], conn["from_port"], conn["to"], conn["to_port"])


## Disconnect an output connection based on node's [code]id[/code] and [code]port[/code].
func disconnect_output_connection(id: Variant, port: int) -> void:
	var entry = get_node_output(id, port)
	disconnect_node(entry["from"], entry["from_port"], entry["to"], entry["to_port"])


## Move an output port connection for a node with [code]id[/code] to the [code]dest[/code]
## port number from the [code]from[/code] port number.
func move_output_connections(id: Variant, dest: int, from: int) -> void:
	var entry = get_node_output(id, from)
	disconnect_node(entry["from"], entry["from_port"], entry["to"], entry["to_port"])
	connect_node(entry["from"], dest, entry["to"], entry["to_port"])


## Swap two output connections for a node with [code]id[/code] exchanging the connections
## at ports [code]index[/code] and [code]other[/code].
func swap_output_connections(id: Variant, index: int, other: int) -> void:
	var entry_index = get_node_output(id, index)
	var entry_other = get_node_output(id, other)

	disconnect_node(entry_index["from"], entry_index["from_port"], entry_index["to"], entry_index["to_port"])
	disconnect_node(entry_other["from"], entry_other["from_port"], entry_other["to"], entry_other["to_port"])

	connect_node(entry_index["from"], entry_index["from_port"], entry_other["to"], entry_other["to_port"])
	connect_node(entry_other["from"], entry_other["from_port"], entry_index["to"], entry_index["to_port"])


################################################################################
# Private API

func _add_orchestration_graph_node(data: Dictionary) -> void:
	var orchestration_node = _get_node_factory().get_node_resource(data["type"])
	if not orchestration_node:
		await get_tree().process_frame
		var message = ("Unable to find node '%s' registered. " + \
			"The orchestration will be loaded, excluding this node.") % data["type_name"]
		OS.alert(message, "Failed to load orchestration")
		return

	orchestration_node.id = data["id"]

	var scene_node = OrchestrationGraphNode.new()
	if not scene_node:
		return

	add_child(scene_node)
	scene_node.name = data["name"]
	scene_node.position_offset = data["position"]
	scene_node.size = data["size"]

	var attributes = OrchestratorDictionary.new(data["attributes"])
	scene_node.initialize(attributes, orchestration_node)


func _add_orchestration_connection(conn: Dictionary) -> void:
	connect_node(conn["from"], conn["from_port"], conn["to"], conn["to_port"])


func _get_next_node_id() -> int:
	var node_id = _orchestration.next_node_id
	_orchestration.next_node_id += 1
	return node_id


func _is_multiple_of_node_allowed(resource: OrchestrationNode) -> bool:
	if not resource.allow_multiple:
		for child in get_children():
			if child is OrchestrationGraphNode:
				if child.get_orchestration_node().type == resource.type:
					return false
	return true


func _get_node_factory():
	return get_tree().root.find_child("OrchestratorNodeFactory", true, false)


################################################################################
# Event handlers

func _on_connection_request(from_node, from_port, to_node, to_port):
	connect_node(from_node, from_port, to_node, to_port)


func _on_disconnection_request(from_node, from_port, to_node, to_port):
	disconnect_node(from_node, from_port, to_node, to_port)

