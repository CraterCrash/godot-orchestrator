@tool
extends Tree
## Provides drag-and-drop functionality for the node tree in the main view.

func _get_drag_data(at_position: Vector2) -> Variant:
	var tree_item : TreeItem = get_selected()

	var data = {}
	data["origin"] = self
	data["origin_tree_item"] = tree_item

	var item = PanelContainer.new()
	item.anchors_preset = PRESET_TOP_LEFT
	item.size_flags_vertical = SIZE_SHRINK_BEGIN

	var hbox = HBoxContainer.new()
	hbox.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	item.add_child(hbox)

	var texture_rect = TextureRect.new()
	texture_rect.texture = tree_item.get_icon(0)
	texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	texture_rect.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	texture_rect.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	hbox.add_child(texture_rect)

	var label = Label.new()
	label.text = tree_item.get_text(0)
	hbox.add_child(label)

	set_drag_preview(item)

	return data
