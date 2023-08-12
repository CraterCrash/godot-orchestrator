@tool
extends VBoxContainer

const FileIcon = preload("res://addons/orchestrator/assets/icons/Orchestrator_16x16.png")

## Emitted when a file is selected
signal file_selected(file_path: String)

## Emitted when clicking right mouse button on a file
signal file_popup_menu_requested(at_position: Vector2)

## Emitted when double clicking a file
signal file_double_clicked(file_path: String)

@onready var filters : LineEdit = $FileFilters
@onready var files : ItemList = $Files

# The file to be applied
var filter : String : set = set_filter, get = get_filter

# Lookup of files
var _files_lookup : Dictionary = {}
var _files : PackedStringArray = [] : set = set_files, get = get_files

# List of files that have been modified but not yet saved
var _unsaved_files_lookup : Array[String] = []

# Current selected file path
var _current_file_path : String = ""


func _ready() -> void:
	_apply_theme()
	filters.text_changed.connect(_on_filters_text_changed)
	files.item_activated.connect(_on_files_activated)
	files.item_clicked.connect(_on_files_clicked)
	theme_changed.connect(_on_theme_changed)


## Set the filter to limit the files shown.
func set_filter(new_filter: String) -> void:
	filter = new_filter
	apply_filter()


## Get the current filter.
func get_filter() -> String:
	return filter


func set_files(next_files) -> void:
	_files = next_files
	_files.sort()
	_update_files_lookup()
	apply_filter()


func get_files() -> PackedStringArray:
	return _files


func get_pretty_file(file: String, size: int = 1) -> String:
	var parts = file.replace("res://", "").replace(".tres", "").split("/")
	parts = parts.slice(-size)
	return "/".join(parts)


func select_file(file_name: String) -> void:
	files.deselect_all()
	for index in range(0, files.get_item_count()):
		var item = files.get_item_text(index).replace("(*)", "")
		if item == get_pretty_file(file_name, item.count("/") + 1):
			files.select(index)


func apply_filter() -> void:
	files.clear()
	for file in _files_lookup.keys():
		if filter == "" or filter.to_lower() in file.to_lower():
			var pretty_file_name = _files_lookup[file]
			if file in _unsaved_files_lookup:
				pretty_file_name += "(*)"
			files.add_item(pretty_file_name, FileIcon)
	select_file(_current_file_path)


func _on_theme_changed() -> void:
	_apply_theme()


func _on_filters_text_changed(text: String) -> void:
	filter = text


func _on_files_activated(index: int) -> void:
	var file = _files_lookup.find_key(_get_file_from_list(index))
	select_file(file)
	file_double_clicked.emit(file)


func _on_files_clicked(index: int, at_position: Vector2, mouse_button_index: int) -> void:
	if mouse_button_index == MOUSE_BUTTON_LEFT:
		var file = _files_lookup.find_key(_get_file_from_list(index))
		select_file(file)
		file_selected.emit(file)
	elif mouse_button_index == MOUSE_BUTTON_RIGHT:
		file_popup_menu_requested.emit(at_position)


func _get_file_from_list(index: int, no_suffix: bool = true) -> String:
	var item = files.get_item_text(index)
	if no_suffix:
		item = item.replace("(*)", "")
	return item


func _apply_theme() -> void:
	if is_instance_valid(filters):
		filters.right_icon = get_theme_icon("Search", "EditorIcons")


func _update_files_lookup() -> void:
	_files_lookup = {}
	for file in _files:
		var pretty_file_name : String = get_pretty_file(file)
		for key in _files_lookup.keys():
			if _files_lookup[key] == pretty_file_name:
				var parts_count = pretty_file_name.count("/") + 2
				var existing_pretty_file_name = get_pretty_file(key, parts_count)
				pretty_file_name = get_pretty_file(file, parts_count)
				while pretty_file_name == existing_pretty_file_name:
					parts_count += 1
					existing_pretty_file_name = get_pretty_file(key, parts_count)
					pretty_file_name = get_pretty_file(file, parts_count)
				_files_lookup[key] = existing_pretty_file_name
		_files_lookup[file] = pretty_file_name


