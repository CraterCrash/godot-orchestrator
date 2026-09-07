# Prints the export hints the annotated orchestration variables produce on the parent node.
# Reference values come from the equivalent GDScript annotations on Godot 4.7.1.
extends Node

const WATCHED: Array[String] = ["speed", "mode", "layers", "legacy"]

func _ready() -> void:
	var owner_node: Node = get_parent()
	for property: Dictionary in owner_node.get_property_list():
		if property.name in WATCHED:
			print("%s type=%d hint=%d hint_string=%s" % [property.name, property.type, property.hint, property.hint_string])