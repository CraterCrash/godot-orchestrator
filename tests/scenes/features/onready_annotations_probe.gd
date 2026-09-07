# Reads the node-path variables after the parent's onready assignments have run.
# The parent's implicit ready runs after its children are ready, so wait for its ready signal.
extends Node

func _ready() -> void:
	get_parent().ready.connect(_parent_ready)

func _parent_ready() -> void:
	var owner_node: Node = get_parent()
	print("label=", owner_node.label.name, " is_label=", owner_node.label is Label, " text=", owner_node.label.text)
	print("any=", owner_node.any.name, " is_label=", owner_node.any is Label)