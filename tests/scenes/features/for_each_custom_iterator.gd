extends RefCounted
class_name ForEachCustomIterator

# A custom iterator that counts down from 3 to 1.
var _current := 0

func _iter_init(_iter: Array) -> bool:
	_current = 3
	return _current > 0

func _iter_next(_iter: Array) -> bool:
	_current -= 1
	return _current > 0

func _iter_get(_iter: Variant) -> Variant:
	return _current
