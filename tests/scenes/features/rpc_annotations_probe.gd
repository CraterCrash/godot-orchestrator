# Prints the RPC configuration the annotated orchestration functions compile to on the parent's script.
# Reference value comes from the equivalent GDScript annotations on Godot 4.7.1.
extends Node

func _ready() -> void:
	var script: Script = get_parent().get_script()
	print(script.get_rpc_config())