## Orchestrator Editor Main View
@tool
extends Control

const OrchestratorSettings = preload("res://addons/orchestrator/orchestrator_settings.gd")
const OrchestrationGraphScene = preload("res://addons/orchestrator/components/editor/orchestration_graph.tscn")

enum FileMenuIds {
	NEW = 1,
	OPEN = 2,
	SAVE = 3,
	SAVE_AS = 4,
	SAVE_ALL = 5,
	SHOW_IN_FILESYSTEM = 6,
	CLOSE = 7,
	CLOSE_ALL = 8,
	RUN = 9,
	TOGGLE_PANEL = 10
}

enum HelpMenuIds {
	ABOUT = 1
}

@onready var file_menu : MenuButton = $Margin/VBoxContainer/HBoxContainer/MenuBar/HBoxContainer/FileMenu
@onready var help_menu : MenuButton = $Margin/VBoxContainer/HBoxContainer/MenuBar/HBoxContainer/HelpMenu

@onready var panel = $Margin/VBoxContainer/HSplitContainer/VSplitContainer

@onready var about = $About

@onready var new_file_dialog = $NewFileDialog
@onready var open_file_dialog = $OpenFileDialog
@onready var save_file_dialog = $SaveFileDialog
@onready var close_discard_confirm_dialog = $CloseDiscardConfirmDialog
@onready var menu_version = $Margin/VBoxContainer/HBoxContainer/MenuBar2/HBoxContainer/Version
@onready var view_version = $Margin/VBoxContainer/HSplitContainer/VBoxContainer/HBoxContainer/Label

# The orchestator plugin
var editor_plugin : EditorPlugin

# A reference to the currently opened resources
var _open_files : Dictionary = {}

var _current_file : String = "": set = set_current_file, get = get_current_file
var _orchestration : Orchestration
var _initially_hidden := false

func _ready():
	_apply_theme()
	_build_menu()
	_setup_dialogs()

	%FilesList.file_selected.connect(_on_files_list_file_selected)
	about.close_requested.connect(_on_about_closed)

	set_current_file("")

	%TogglePanelButton.pressed.connect(_toggle_panel)

	editor_plugin.get_editor_interface().get_file_system_dock().files_moved.connect(_on_files_moved)
	menu_version.text = "v" + editor_plugin.get_version()
	view_version.text = "Godot Orchestrator v" + editor_plugin.get_version()


################################################################################
# Public API

func set_plugin(plugin: EditorPlugin) -> void:
	# Set the plugin and delegate it to children that require it.
	editor_plugin = plugin
	%NodeTree.set_plugin(plugin)


func set_version(version: String) -> void:
	menu_version = "v" + version
	view_version = "Godot Orchestrator v" + version


func open_orchestration(orchestration: Orchestration) -> void:
	_open_file(orchestration.resource_path)


func close_orchestration() -> void:
	_close_file(_current_file)


func close_all_orchestrations() -> void:
	while not _open_files.keys().is_empty():
		var key = _open_files.keys()[0]
		_close_file(key)


func apply_changes() -> void:
	_save_all_orchestrations()


func get_current_file() -> String:
	return _current_file


func set_current_file(value: String) -> void:
	_current_file = value

	%FilesList._current_file_path = _current_file

	# Set menu visibility
	var no_files = %FilesList._files.size() == 0
	var fmpm = file_menu.get_popup()
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.SAVE), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.SAVE_AS), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.SAVE_ALL), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.SHOW_IN_FILESYSTEM), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.CLOSE), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.CLOSE_ALL), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(FileMenuIds.RUN), no_files)

	for child in %OrchestrationGraphs.get_children():
		child.visible = false

	if _current_file != "":
		var file = _open_files.get(_current_file)
		file["scene"].visible = true


################################################################################
# Private

func _build_menu() -> void:
	file_menu.get_popup().clear()
	file_menu.get_popup().add_item("New Orchestration...", FileMenuIds.NEW, KEY_N | KEY_MASK_CTRL)
	file_menu.get_popup().add_item("Open...", FileMenuIds.OPEN)
	# todo: add open recent
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Save", FileMenuIds.SAVE, KEY_S | KEY_MASK_CTRL | KEY_MASK_ALT)
	file_menu.get_popup().add_item("Save As...", FileMenuIds.SAVE_AS)
	file_menu.get_popup().add_item("Save All", FileMenuIds.SAVE_ALL, KEY_S | KEY_MASK_SHIFT | KEY_MASK_ALT);
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Show in FileSystem", FileMenuIds.SHOW_IN_FILESYSTEM)
	file_menu.get_popup().add_separator()

	file_menu.get_popup().add_item("Close", FileMenuIds.CLOSE, KEY_W | KEY_MASK_CTRL)
	file_menu.get_popup().add_item("Close All", FileMenuIds.CLOSE_ALL)
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Run", FileMenuIds.RUN, KEY_X | KEY_MASK_SHIFT | KEY_MASK_CTRL)
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Toggle Orchestrations Panel", FileMenuIds.TOGGLE_PANEL, KEY_BACKSLASH | KEY_MASK_CTRL)
	if not file_menu.get_popup().id_pressed.is_connected(_on_file_menu_id_pressed):
		file_menu.get_popup().id_pressed.connect(_on_file_menu_id_pressed)

	help_menu.get_popup().clear()
	help_menu.get_popup().add_item("About Orchestrator", HelpMenuIds.ABOUT)
	if not help_menu.get_popup().id_pressed.is_connected(_on_help_menu_id_pressed):
		help_menu.get_popup().id_pressed.connect(_on_help_menu_id_pressed)


func _setup_dialogs() -> void:
	new_file_dialog.file_selected.connect(_on_new_file_selected)
	new_file_dialog.clear_filters()
	new_file_dialog.add_filter("*.tres", "Resources")

	open_file_dialog.file_selected.connect(_on_open_file_selected)
	open_file_dialog.clear_filters()
	open_file_dialog.add_filter("*.tres", "Resources")

	save_file_dialog.file_selected.connect(_on_save_file_selected)
	save_file_dialog.clear_filters()
	save_file_dialog.add_filter("*.tres", "Resources")


func _save_all_orchestrations() -> void:
	for key in _open_files.keys():
		_save_file(key)


func _show_in_filesystem() -> void:
	editor_plugin.get_editor_interface().get_file_system_dock().navigate_to_path(_current_file)


func _run_orchestration() -> void:
	OrchestratorSettings.set_user_value("is_running_test_scene", true)
	OrchestratorSettings.set_user_value("run_resource_path", _current_file)
	var test_scene_path = OrchestratorSettings.get_setting("custom_test_scene_path", "res://addons/orchestrator/test/test.tscn")
	if test_scene_path:
		editor_plugin.get_editor_interface().play_custom_scene(test_scene_path)


func _apply_toggle_state() -> void:
	var state = "GuiTreeArrowRight" if not panel.visible else "GuiTreeArrowLeft"
	%TogglePanelButton.icon = get_theme_icon(state, "EditorIcons")


func _toggle_panel() -> void:
	panel.visible = !panel.visible
	_apply_toggle_state()


func _new_file(file_name: String) -> bool:
	if _open_files.has(file_name):
		_remove_from_open_files(file_name)

	var orchestration = Orchestration.new()
	var err = ResourceSaver.save(orchestration, file_name)
	if err == OK:
		editor_plugin.get_editor_interface().get_resource_filesystem().scan()
		return true
	return false


func _open_file(file_name: String) -> bool:
	if not _open_files.has(file_name):
		for child in %OrchestrationGraphs.get_children():
			child.visible = false

		var scene = OrchestrationGraphScene.instantiate()
		%OrchestrationGraphs.add_child(scene)
		scene.editor_plugin = editor_plugin

		var resource = ResourceLoader.load(file_name, "", ResourceLoader.CACHE_MODE_IGNORE)
		_open_files[file_name] = {
			"resource": resource,
			"scene": scene
		}

		scene.set_orchestration(resource)

	%FilesList._files = _open_files.keys()
	%FilesList.select_file(file_name)
	_current_file = file_name

	return true


func _close_file(file_name: String) -> bool:
	if file_name in _open_files.keys():
		var file = _open_files[file_name]
		# todo: check dirty flag
		_remove_from_open_files(file_name)
		return true
	return false


func _save_file(file_name: String) -> bool:
	if _open_files.has(file_name):
		var entry = _open_files[file_name]
		entry["scene"].apply_graph_to_orchestration(entry["resource"])
		return ResourceSaver.save(entry["resource"], file_name) == OK
	return false


func _remove_from_open_files(file_name: String) -> void:
	if not file_name in _open_files.keys():
		return

	var entry = _open_files[file_name]
	%OrchestrationGraphs.remove_child(entry["scene"])
	entry["scene"].queue_free()

	var index = _open_files.keys().find(file_name)
	_open_files.erase(file_name)

	if _open_files.size() == 0:
		%FilesList._files = _open_files.keys()
		_current_file = ""
	else:
		index = clamp(index, 0, _open_files.size() - 1)
		_current_file = _open_files.keys()[index]
		%FilesList._files = _open_files.keys()


func _apply_theme() -> void:
	_apply_toggle_state()

################################################################################
# Events/Signal Handlers

func _on_files_moved(old_file: String, new_file: String) -> void:
	for open_file in _open_files:
		if open_file == old_file:
			var resource = ResourceLoader.load(new_file, "", ResourceLoader.CACHE_MODE_IGNORE)
			_open_files[new_file] = {
				"resource": resource,
				"scene": _open_files[open_file]["scene"]
			}
			_open_files.erase(old_file)

			%FilesList._files = _open_files.keys()
			%FilesList.select_file(new_file)
			_current_file = new_file
			break


func _on_file_menu_id_pressed(id: int) -> void:
	match id:
		FileMenuIds.NEW:
			new_file_dialog.show()
		FileMenuIds.OPEN:
			open_file_dialog.show()
		FileMenuIds.SAVE:
			_save_file(_current_file)
		FileMenuIds.SAVE_AS:
			save_file_dialog.show()
		FileMenuIds.SAVE_ALL:
			_save_all_orchestrations()
		FileMenuIds.SHOW_IN_FILESYSTEM:
			_show_in_filesystem()
		FileMenuIds.CLOSE:
			close_orchestration()
		FileMenuIds.CLOSE_ALL:
			close_all_orchestrations()
		FileMenuIds.RUN:
			_run_orchestration()
		FileMenuIds.TOGGLE_PANEL:
			_toggle_panel()


func _on_help_menu_id_pressed(id: int) -> void:
	if HelpMenuIds.ABOUT == id:
		about.show()


func _on_files_list_file_selected(file_name: String) -> void:
	_current_file = file_name


func _on_new_file_selected(file_name: String) -> void:
	if _open_files.has(file_name):
		_remove_from_open_files(file_name)
	if not _new_file(file_name):
		OS.alert("Failed to create new file %s" % file_name)
		return
	_open_file(file_name)


func _on_open_file_selected(file_name: String) -> void:
	_open_file(file_name)


func _on_save_file_selected(file_name: String) -> void:
	if not _open_files.has(file_name):
		if _current_file != "":
			var orchestration = Orchestration.new()
			var entry = _open_files[_current_file]
			entry["scene"].apply_graph_to_orchestration(orchestration)
			if ResourceSaver.save(orchestration, file_name) == OK:
				_open_file(file_name)
	else:
		_save_file(file_name)


func _on_about_closed() -> void:
	about.hide()
