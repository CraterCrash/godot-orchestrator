## Orchestrator editor plugin
@tool
extends EditorPlugin

signal node_resources_updated(node_factory)

const ADDON_NAME = "Orchestrator"
const ADDON_NODE_FACTORY_NAME = "OrchestratorNodeFactory"

const MAIN_VIEW_SCENE = "res://addons/orchestrator/views/main_view.tscn"
const ORCHESTRATOR_SETTINGS = "res://addons/orchestrator/orchestrator_settings.gd"
const ORCHESTRATOR_NODE_FACTORY = "res://addons/orchestrator/orchestrator_node_factory.gd"
const ORCHESTRATOR = "res://addons/orchestrator/orchestrator.gd"

const OrchestratorEditorIcon = preload("res://addons/orchestrator/assets/icons/Orchestrator_16x16.png")

## The plugin's main view.
var main_view

# Reference to the node factory
var _node_factory

func _ready():
	# OrchestratorNodeFactory may enter scene later due to order of operations.
	# For example, when loading and plug-in is disabled, tool script loads early.
	# When loading Godot with plug-in enabled, tool script loads later.
	# This causes the functionality to be uniform across both scenarios.
	await get_tree().process_frame
	_node_factory = get_tree().root.find_child(ADDON_NODE_FACTORY_NAME, true, false)
	_rescan_for_resources()


func _enter_tree():
	add_autoload_singleton(ADDON_NODE_FACTORY_NAME, ORCHESTRATOR_NODE_FACTORY)
	add_autoload_singleton(ADDON_NAME, ORCHESTRATOR)

	if not Engine.is_editor_hint():
		return

	var settings = load(ORCHESTRATOR_SETTINGS)
	settings.prepare()

	main_view = load(MAIN_VIEW_SCENE).instantiate()
	main_view.set_plugin(self)

	get_editor_interface().get_editor_main_screen().add_child(main_view)
	_make_visible(false)

	var editor = get_editor_interface()
	editor.get_resource_filesystem().filesystem_changed.connect(_on_filesystem_changed)
	editor.get_file_system_dock().files_moved.connect(_on_files_moved)
	editor.get_file_system_dock().file_removed.connect(_on_file_removed)


func _exit_tree():
	remove_autoload_singleton(ADDON_NAME)
	remove_autoload_singleton(ADDON_NODE_FACTORY_NAME)

	if is_instance_valid(main_view):
		main_view.queue_free()

	var editor = get_editor_interface()
	editor.get_resource_filesystem().filesystem_changed.disconnect(_on_filesystem_changed)
	editor.get_file_system_dock().files_moved.disconnect(_on_files_moved)
	editor.get_file_system_dock().file_removed.disconnect(_on_file_removed)


func _has_main_screen() -> bool:
	return true


func _make_visible(next_visible: bool) -> void:
	if is_instance_valid(main_view):
		main_view.visible = next_visible


func _get_plugin_name() -> String:
	return ADDON_NAME


func _get_plugin_icon() -> Texture2D:
	return OrchestratorEditorIcon


func _handles(object: Object) -> bool:
	return object is Orchestration


func _edit(object: Object) -> void:
	if is_instance_valid(main_view) and is_instance_valid(object):
		main_view.open_orchestration(object)


func _apply_changes() -> void:
	if is_instance_valid(main_view):
		main_view.apply_changes()


func _on_filesystem_changed() -> void:
	_rescan_for_resources()


func _on_files_moved(old_file: String, new_file: String) -> void:
	_rescan_for_resources()


func _on_file_removed(file_name: String) -> void:
	_rescan_for_resources()


func _rescan_for_resources() -> void:
	if _node_factory:
		_node_factory.rescan_for_resources()
		node_resources_updated.emit(_node_factory)


func get_version() -> String:
	var config = ConfigFile.new()
	config.load("res://addons/orchestrator/plugin.cfg")
	return config.get_value("plugin", "version")
