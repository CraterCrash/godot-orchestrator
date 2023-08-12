@tool
extends Tree

func _get_drag_data(at_position: Vector2) -> Variant:
	var tree_item : TreeItem = get_selected()

	var data = {}
	data["origin"] = self
	data["origin_tree_item"] = tree_item

	var label = Label.new()
	label.text = tree_item.get_text(0)
	set_drag_preview(label)

	return data
