extends RefCounted
class_name ReleaseAtEndOfStatementHelper

class Tracker:
	extends RefCounted
	func _notification(what: int) -> void:
		if what == NOTIFICATION_PREDELETE:
			print("released")

func get_strings() -> PackedStringArray:
	return PackedStringArray(["a"])

func get_tracker() -> RefCounted:
	return Tracker.new()

func consume(_strings: PackedStringArray, _value: RefCounted) -> void:
	pass
