@tool
extends OrchestrationNode

var _comment_text : String = ""

func _init():
	type = 3
	name = "Comment"
	category = "Utility"
	description = "Comments."


func get_attributes() -> Dictionary:
	return { "comment": _comment_text }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	_comment_text = attributes.get_default("comment", "")

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)
	margin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	margin.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var comment = TextEdit.new()
	comment.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	comment.size_flags_vertical = Control.SIZE_EXPAND_FILL
	comment.autowrap_mode = TextServer.AUTOWRAP_WORD
	comment.wrap_mode = TextEdit.LINE_WRAPPING_BOUNDARY
	comment.custom_minimum_size = Vector2(256, 64)
	comment.text_changed.connect(func(): _comment_text = comment.text)
	comment.text = _comment_text
	margin.add_child(comment)

	scene_node.add_child(margin)
