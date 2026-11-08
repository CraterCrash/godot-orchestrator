extends RefCounted
class_name CoroutineCompletedBacktraceProbe

# Captures backtraces with variables while a finished coroutine is resuming its caller,
# and prints the argument of that coroutine's frame, which must still be intact.
func probe() -> void:
	for bt in Engine.capture_script_backtraces(true):
		if bt.get_language_name() != "OScript":
			continue
		for i in bt.get_frame_count():
			if bt.get_frame_function(i) != "inner":
				continue
			for j in bt.get_local_variable_count(i):
				if bt.get_local_variable_name(i, j) == "value":
					print("inner value=%s" % bt.get_local_variable_value(i, j))
