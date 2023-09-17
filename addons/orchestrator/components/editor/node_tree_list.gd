@tool
extends VBoxContainer

var editor_plugin : EditorPlugin

@onready var filters = $NodeFilters
@onready var tree = $Nodes

# The file to be applied
var filter : String = "" : set = set_filter, get = get_filter

# All nodes from last scan
var _available_nodes : Dictionary = {}

# Node factory
# Lazily loaded when editor plugin raises node_resources_updated
# Set set_plugin method
var _node_factory


func _ready():
	filters.text_changed.connect(_on_filters_text_changed)
	_apply_theme()


func set_plugin(editor_plugin: EditorPlugin) -> void:
	editor_plugin.node_resources_updated.connect(_on_node_factory_updated)


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
	for resource in _node_factory.get_resources():
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
				_create_node_type(resource.name, resource.type, group)


func _on_filters_text_changed(new_text: String) -> void:
	filter = new_text


func _on_node_factory_updated(node_factory) -> void:
	_node_factory = node_factory
	apply_filter()


func _create_node_group(name: String, parent: TreeItem = null) -> TreeItem:
	parent = parent if not null else tree.get_root()
	var group = tree.create_item(parent)
	group.set_text(0, "%s" % name)
	group.set_selectable(0, false)
	group.set_selectable(0, false)
	group.set_tooltip_text(0, " ")
	return group


func _create_node_type(name: String, type: int, parent: TreeItem) -> TreeItem:
	var node = tree.create_item(parent)
	node.set_meta("_node_type", type)
	node.set_text(0, name)
	return node


func _apply_theme() -> void:
	if is_instance_valid(filters):
		filters.right_icon = get_theme_icon("Search", "EditorIcons")
