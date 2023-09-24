@tool
extends VBoxContainer
## Provides the list of available nodes in the main view.

# The file to be applied
var filter : String = "" : set = set_filter, get = get_filter

# All nodes from last scan
var _available_nodes : Array

@onready var filters = $NodeFilters
@onready var tree = $Nodes


func _ready():
	_apply_theme()


func update_available_nodes(nodes: Array) -> void:
	_available_nodes = nodes
	apply_filter()


## Set the filter to limit the nodes shown.
func set_filter(new_filter: String) -> void:
	filter = new_filter
	apply_filter()


## Get the current filter.
func get_filter() -> String:
	return filter


func apply_filter() -> void:
	if tree.get_root():
		tree.get_root().free()

	# Setup the root node
	tree.create_item()
	tree.hide_root = true

	var resources = []
	var categories = []
	for resource in _available_nodes:
		if filter == "" or filter.to_lower() in resource.name.to_lower():
			resources.push_back(resource)
			if not categories.has(resource.category):
				categories.push_back(resource.category)

	# Create groups and items
	categories.sort()
	for node_type in categories:
		var group = _create_node_group(node_type)
		for resource in resources:
			if resource.category == node_type:
				_create_node_type(resource.name, resource.type, resource.icon,
					resource.usage_hint, resource.description, group)


func _on_filters_text_changed(new_text: String) -> void:
	filter = new_text


func _create_node_group(name: String, parent: TreeItem = null) -> TreeItem:
	parent = parent if not null else tree.get_root()
	var group = tree.create_item(parent)
	group.set_text(0, "%s" % name)
	group.set_selectable(0, false)
	group.set_selectable(0, false)
	group.set_tooltip_text(0, " ")
	return group


func _create_node_type(name: String, type: int, icon: String, usage: int,
		description: String, parent: TreeItem) -> TreeItem:
	var node = tree.create_item(parent)
	node.set_meta("_node_type", type)
	node.set_text(0, name)
	node.set_icon(0, _get_node_type_icon(icon, usage))
	node.set_tooltip_text(0, name + ":\n" + description)
	return node


func _get_node_type_icon(icon: String, usage: int) -> Texture2D:
	if icon == null or icon.length() == 0:
		match usage:
			OrchestrationNode.ORCHESTRATION_NODE_USAGE_2D:
				return get_theme_icon("Node2D", "EditorIcons")
			OrchestrationNode.ORCHESTRATION_NODE_USAGE_3D:
				return get_theme_icon("Node3D", "EditorIcons")
		return get_theme_icon("Node", "EditorIcons")
	elif icon.begins_with("res://addons"):
		return load(icon)
	else:
		return get_theme_icon(icon, "EditorIcons")


func _apply_theme() -> void:
	if is_instance_valid(filters):
		filters.right_icon = get_theme_icon("Search", "EditorIcons")
