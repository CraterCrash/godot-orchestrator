# General Utilities
class_name Utils

# `assert()` is not evaluated in non-debug builds. Do not use `assert()`
# for anything other than testing the `assert()` itself.
static func check(condition: Variant) -> void:
	if condition:
		return
		
	printerr("Check failed. Backtrace (most recent call first):")
	var stack: Array = get_stack()
	var dir: String
	for i: int in stack.size():
		var frame: Dictionary = stack[i]
		if i == 0:
			dir = str(frame.source).trim_suffix("utils.notest.gd")
		else:
			printerr("  %s:%d @ %s()" % [
				str(frame.source).trim_prefix(dir),
				frame.line,
				frame.function,
			])
