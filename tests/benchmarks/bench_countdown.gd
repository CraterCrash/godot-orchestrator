extends RefCounted
class_name BenchCountdown

# Custom iterator used by the iterate_object case: yields COUNT values.
const COUNT := 100000

var _current := 0

func _iter_init(_iter: Array) -> bool:
	_current = COUNT
	return _current > 0

func _iter_next(_iter: Array) -> bool:
	_current -= 1
	return _current > 0

func _iter_get(_iter: Variant) -> Variant:
	return _current
