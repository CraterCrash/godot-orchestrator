@tool
extends Control
## Orchestrator Editor Main View

# Accelerator Menu Ids
enum AccelMenuIds {
	NEW = 1,
	OPEN = 2,
	SAVE = 3,
	SAVE_AS = 4,
	SAVE_ALL = 5,
	CLOSE = 6,
	CLOSE_ALL = 7,
	CLOSE_OTHERS = 8,
	RUN = 9,
	ABOUT = 10,
	SHOW_IN_FILESYSTEM = 11,
	COPY_PATH = 12,
	TOGGLE_PANEL = 13
}

const OrchestratorSettings = preload("res://addons/orchestrator/orchestrator_settings.gd")
const OrchestrationGraphScene = preload("res://addons/orchestrator/components/editor/orchestration_graph.tscn")
const OrchestratorVersion = preload("res://addons/orchestrator/orchestrator_version.gd")

# The orchestator plugin
var editor_plugin : EditorPlugin
var editor_interface : EditorInterface

# A reference to the currently opened resources
var _open_files : Dictionary = {}

var _current_file : String = "": set = set_current_file, get = get_current_file
var _orchestration : Orchestration
var _initially_hidden := false

@onready var file_menu : MenuButton = $Margin/VBoxContainer/HBoxContainer/MenuBar/HBoxContainer/FileMenu
@onready var help_menu : MenuButton = $Margin/VBoxContainer/HBoxContainer/MenuBar/HBoxContainer/HelpMenu

@onready var panel = $Margin/VBoxContainer/HSplitContainer/VSplitContainer

@onready var about = $About

@onready var new_file_dialog = $NewFileDialog
@onready var open_file_dialog = $OpenFileDialog
@onready var save_file_dialog = $SaveFileDialog
@onready var close_discard_confirm_dialog = $CloseDiscardConfirmDialog
@onready var context_menu : PopupMenu = $ContextMenu
@onready var menu_version = $Margin/VBoxContainer/HBoxContainer/MenuBar2/HBoxContainer/Version
@onready var view_version = $Margin/VBoxContainer/HSplitContainer/VBoxContainer/HBoxContainer/Label


func _ready():
	_apply_theme()
	_build_menu()
	_setup_dialogs()
	_setup_external_docs()
	set_current_file("")
	menu_version.text = OrchestratorVersion.get_full_version()
	view_version.text = OrchestratorVersion.get_full_version()


################################################################################
# Public API

func set_plugin(plugin: EditorPlugin) -> void:
	editor_plugin = plugin
	editor_interface = editor_plugin.get_editor_interface()

	# This must be called here rather than in _ready because if the scene is
	# opened in the editor, the editor will invoke the _enter_tree and _ready
	# functions because this is a tool-annotated script. Since the plugin.gd
	# isn't called in this case to initialize the editor plugin, the plugin
	# details aren't available and therefore must be guarded.
	editor_interface.get_file_system_dock().files_moved.connect(_on_files_moved)


func update_project_settings() -> void:
	for entry in _open_files.values():
		entry["scene"].update_project_settings()


func update_available_nodes(nodes: Array) -> void:
	%NodeTree.update_available_nodes(nodes)


func open_orchestration(orchestration: Orchestration) -> void:
	_open_file(orchestration.resource_path)


func close_orchestration() -> void:
	_close_file(_current_file)


func close_all_orchestrations() -> void:
	while not _open_files.keys().is_empty():
		var key = _open_files.keys()[0]
		_close_file(key)


func close_other_orchestrations() -> void:
	# Create a copy of the keys and the current file name
	# This is necessary as _close_file will potentially adjust both these
	var _keys = _open_files.keys().duplicate()
	var _current_open_file = _current_file
	for key in _keys:
		if key != _current_open_file:
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
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.SAVE), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.SAVE_AS), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.SAVE_ALL), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.SHOW_IN_FILESYSTEM), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.CLOSE), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.CLOSE_ALL), no_files)
	fmpm.set_item_disabled(fmpm.get_item_index(AccelMenuIds.RUN), no_files)

	%OrchestrationGraphs.get_children().map(func(child): child.visible = false)

	if _current_file != "":
		var file = _open_files.get(_current_file)
		file["scene"].visible = true


################################################################################
# Private

func _build_menu() -> void:
	file_menu.get_popup().clear()
	file_menu.get_popup().add_item("New Orchestration...", AccelMenuIds.NEW, KEY_N | KEY_MASK_CTRL)
	file_menu.get_popup().add_item("Open...", AccelMenuIds.OPEN)
	# todo: add open recent
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Save", AccelMenuIds.SAVE, KEY_S | KEY_MASK_CTRL | KEY_MASK_ALT)
	file_menu.get_popup().add_item("Save As...", AccelMenuIds.SAVE_AS)
	file_menu.get_popup().add_item("Save All", AccelMenuIds.SAVE_ALL, KEY_S | KEY_MASK_SHIFT | KEY_MASK_ALT);
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Show in FileSystem", AccelMenuIds.SHOW_IN_FILESYSTEM)
	file_menu.get_popup().add_separator()

	file_menu.get_popup().add_item("Close", AccelMenuIds.CLOSE, KEY_W | KEY_MASK_CTRL)
	file_menu.get_popup().add_item("Close All", AccelMenuIds.CLOSE_ALL)
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Run", AccelMenuIds.RUN, KEY_X | KEY_MASK_SHIFT | KEY_MASK_CTRL)
	file_menu.get_popup().add_separator()
	file_menu.get_popup().add_item("Toggle Orchestrations Panel", AccelMenuIds.TOGGLE_PANEL, KEY_BACKSLASH | KEY_MASK_CTRL)
	if not file_menu.get_popup().id_pressed.is_connected(_on_accel_menu_id_pressed):
		file_menu.get_popup().id_pressed.connect(_on_accel_menu_id_pressed)

	help_menu.get_popup().clear()
	help_menu.get_popup().add_item("About Orchestrator", AccelMenuIds.ABOUT)
	if not help_menu.get_popup().id_pressed.is_connected(_on_accel_menu_id_pressed):
		help_menu.get_popup().id_pressed.connect(_on_accel_menu_id_pressed)


func _setup_dialogs() -> void:
	new_file_dialog.clear_filters()
	new_file_dialog.add_filter("*.tres", "Resources")

	open_file_dialog.clear_filters()
	open_file_dialog.add_filter("*.tres", "Resources")

	save_file_dialog.clear_filters()
	save_file_dialog.add_filter("*.tres", "Resources")


func _setup_external_docs() -> void:
	%OpenDocs.flat = true
	%OpenDocs.icon = get_theme_icon("ExternalLink", "EditorIcons")

	var config = ConfigFile.new()
	config.load("res://addons/orchestrator/plugin.cfg")
	var doc_url = config.get_value("plugin", "documentation_url")
	%OpenDocs.pressed.connect(func(): OS.shell_open(doc_url))


func _save_all_orchestrations() -> void:
	for key in _open_files.keys():
		_save_file(key)


func _show_in_filesystem() -> void:
	editor_interface.get_file_system_dock().navigate_to_path(_current_file)


func _run_orchestration() -> void:
	OrchestratorSettings.set_user_value("is_running_test_scene", true)
	OrchestratorSettings.set_user_value("run_resource_path", _current_file)
	var test_scene_path = OrchestratorSettings.get_setting("run/test_scene")
	if test_scene_path:
		editor_interface.play_custom_scene(test_scene_path)


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
		editor_interface.get_resource_filesystem().scan()
		return true
	return false


func _open_file(file_name: String) -> bool:
	if not _open_files.has(file_name):
		%OrchestrationGraphs.get_children().map(func(child): child.visible = false)

		var scene = OrchestrationGraphScene.instantiate()
		%OrchestrationGraphs.add_child(scene)

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


func _on_accel_menu_id_pressed(menu_id: int) -> void:
	match menu_id:
		AccelMenuIds.NEW:
			new_file_dialog.show()
		AccelMenuIds.OPEN:
			open_file_dialog.show()
		AccelMenuIds.SAVE:
			_save_file(_current_file)
		AccelMenuIds.SAVE_AS:
			save_file_dialog.show()
		AccelMenuIds.SAVE_ALL:
			_save_all_orchestrations()
		AccelMenuIds.SHOW_IN_FILESYSTEM:
			_show_in_filesystem()
		AccelMenuIds.CLOSE:
			close_orchestration()
		AccelMenuIds.CLOSE_ALL:
			close_all_orchestrations()
		AccelMenuIds.CLOSE_OTHERS:
			close_other_orchestrations()
		AccelMenuIds.RUN:
			_run_orchestration()
		AccelMenuIds.COPY_PATH:
			DisplayServer.clipboard_set(_current_file)
		AccelMenuIds.ABOUT:
			about.show()
		AccelMenuIds.TOGGLE_PANEL:
			_toggle_panel()


func _on_files_list_file_selected(file_name: String) -> void:
	_current_file = file_name


func _on_files_list_context_menu(at_position: Vector2) -> void:
	context_menu.clear()

	if %FilesList.files.get_selected_items().is_empty():
		return

	context_menu.add_item("Save", AccelMenuIds.SAVE, KEY_S | KEY_MASK_CTRL | KEY_MASK_ALT)
	context_menu.add_item("Save As...", AccelMenuIds.SAVE_AS)
	context_menu.add_item("Close", AccelMenuIds.CLOSE, KEY_W | KEY_MASK_CTRL)
	context_menu.add_item("Close All", AccelMenuIds.CLOSE_ALL)
	context_menu.add_item("Close Others", AccelMenuIds.CLOSE_OTHERS)
	if _open_files.size() == 1:
		context_menu.set_item_disabled(context_menu.get_item_index(AccelMenuIds.CLOSE_OTHERS), true)
	context_menu.add_separator()
	context_menu.add_item("Run", AccelMenuIds.RUN, KEY_X | KEY_MASK_CTRL | KEY_MASK_SHIFT)
	context_menu.add_separator()
	context_menu.add_item("Copy Resource Path", AccelMenuIds.COPY_PATH)
	context_menu.add_item("Show in Filesystem", AccelMenuIds.SHOW_IN_FILESYSTEM)
	context_menu.add_separator()
	context_menu.add_item("Toggle Orchestrations Panel", AccelMenuIds.TOGGLE_PANEL)

	context_menu.set_position(get_screen_position() + get_local_mouse_position())
	context_menu.reset_size()
	context_menu.show()


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


func _on_restart_editor() -> void:
	editor_interface.restart_editor(true)
