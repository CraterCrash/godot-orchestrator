@tool
extends GraphEdit
## A GraphEdit implementation for designing Orchestrations

const FROM_PORT : String = "from_port"
const TO_PORT	: String = "to_port"

# Keys were changed between 4.1 and 4.2
var FROM 		= "from" if Engine.get_version_info().hex < 0x040200 else "from_node"
var TO   		= "to"   if Engine.get_version_info().hex < 0x040200 else "to_node"

# The associated orchestration
var _orchestration : Orchestration


func _ready():
	minimap_enabled = false
	right_disconnects = true


func update_project_settings() -> void:
	for child in get_children():
		if child is OrchestrationGraphNode:
			child.update_project_settings()


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
	resource.id = scene_node.name

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
	for conn in _deserialize_connection_list(orchestration.connections):
		_add_orchestration_connection(conn)

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
	orchestration.connections = _serialize_connection_list()
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


func get_output_connection(node_name: String, slot: int) -> Dictionary:
	for entry in get_connection_list():
		if entry[FROM] == node_name and entry[FROM_PORT] == slot:
			return entry
	return {}


## Get the output connection from a node with a given [code]id[/code] and [code]port[/code].
func get_node_output(id: Variant, port: int) -> Dictionary:
	if not id:
		push_error("Cannot get node output for node id '%s' with port '%s'" % [id,port])
	for entry in get_connection_list():
		if entry[FROM] == id and entry[FROM_PORT] == port:
			return entry
	return {}


## Get all connections for the specified [GraphNode].
func get_node_connections(node: GraphNode) -> Array[Dictionary]:
	var node_connections : Array[Dictionary]
	for connection in get_connection_list():
		if connection[FROM] == node.name:
			var node_connection = {}
			node_connection["source_port"] = connection[FROM_PORT]
			node_connection["target_node"] = connection[TO]
			node_connection["target_port"] = connection[TO_PORT]
			var other_node = find_child(connection[TO], false, false)
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
		if conn[FROM] == node.name:
			disconnect_node(conn[FROM], conn[FROM_PORT], conn[TO], conn[TO_PORT])
		elif conn[TO] == node.name:
			disconnect_node(conn[FROM], conn[FROM_PORT], conn[TO], conn[TO_PORT])


func disconnect_connection(connection: Dictionary) -> void:
	if connection != null and not connection.is_empty():
		disconnect_node(connection[FROM], \
			connection[FROM_PORT], \
			connection[TO], \
			connection[TO_PORT])


func disconnect_output_port(node_name: String, port: int) -> void:
	for entry in get_connection_list():
		if entry[FROM] == node_name and entry[FROM_PORT] == port:
			disconnect_node(entry[FROM], entry[FROM_PORT], entry[TO], entry[TO_PORT])


## Disconnect an output connection based on node's [code]id[/code] and [code]port[/code].
func disconnect_output_connection(id: Variant, port: int) -> void:
	var entry = get_node_output(id, port)
	if not entry.is_empty():
		disconnect_node(entry[FROM], entry[FROM_PORT], entry[TO], entry[TO_PORT])


## Move an output port connection for a node with [code]id[/code] to the [code]dest[/code]
## port number from the [code]from[/code] port number.
func move_output_connections(id: Variant, dest: int, from: int) -> void:
	var entry = get_node_output(id, from)
	if not entry.is_empty():
		disconnect_node(entry[FROM], entry[FROM_PORT], entry[TO], entry[TO_PORT])
		connect_node(entry[FROM], dest, entry[TO], entry[TO_PORT])


## Swap two output connections for a node with [code]id[/code] exchanging the connections
## at ports [code]index[/code] and [code]other[/code].
func swap_output_connections(id: Variant, index: int, other: int) -> void:
	var entry_index = get_node_output(id, index)
	var entry_other = get_node_output(id, other)
	if not entry_index.is_empty() and not entry_other.is_empty():
		disconnect_node(entry_index[FROM], entry_index[FROM_PORT], entry_index[TO], entry_index[TO_PORT])
		disconnect_node(entry_other[FROM], entry_other[FROM_PORT], entry_other[TO], entry_other[TO_PORT])

		connect_node(entry_index[FROM], entry_index[FROM_PORT], entry_other[TO], entry_other[TO_PORT])
		connect_node(entry_other[FROM], entry_other[FROM_PORT], entry_index[TO], entry_index[TO_PORT])


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
	connect_node(conn[FROM], conn[FROM_PORT], conn[TO], conn[TO_PORT])


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


func _serialize_connection_list() -> Array[Dictionary]:
	if Engine.get_version_info().hex < 0x040200:
		return get_connection_list()

	# Godot 4.2 reworked the connections and changed some of the dictionary
	# keys.  So we need to adjust the new format back to pre-4.2 for save
	# purposes so that the same format is compatible with older versions.
	var connections : Array[Dictionary]
	for entry in get_connection_list():
		var connection = {}
		connection["from"] = entry[FROM]
		connection["from_port"] = entry[FROM_PORT]
		connection["to"] = entry[TO]
		connection["to_port"] = entry[TO_PORT]
		connections.append(connection)
	return connections


func _deserialize_connection_list(list: Array[Dictionary]) -> Array[Dictionary]:
	if Engine.get_version_info().hex < 0x040200:
		return list

	# Converts the serialized list using Godot 4.1 and prior format to the
	# Godot 4.2 and later formats
	var connections: Array[Dictionary]
	for entry in list:
		var connection = {}
		connection[FROM] = entry["from"]
		connection[FROM_PORT] = entry[FROM_PORT]
		connection[TO] = entry["to"]
		connection[TO_PORT] = entry[TO_PORT]
		connections.append(connection)
	return connections


################################################################################
# Event handlers

func _on_connection_request(from_node, from_port, to_node, to_port):
	connect_node(from_node, from_port, to_node, to_port)


func _on_disconnection_request(from_node, from_port, to_node, to_port):
	disconnect_node(from_node, from_port, to_node, to_port)

