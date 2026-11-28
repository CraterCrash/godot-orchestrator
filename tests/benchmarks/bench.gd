extends Node

# Runs every orchestration in res://benchmarks/cases. Each case extends Node and exposes a run() function
# whose body is a tight loop around the code path being measured.
#
# User args (after "--"):
#   --filter=<substr>  only run cases whose file name contains <substr>
#   --rounds=<n>       timed rounds per case (default 9)
#
# Output, one line per case:
#   BENCH <case> <min_usec> <median_usec> <static_mem_delta_bytes>

const CASES_DIR := "res://benchmarks/cases/"

func _ready() -> void:
	var filter := ""
	var rounds := 9
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--filter="):
			filter = arg.trim_prefix("--filter=")
		elif arg.begins_with("--rounds="):
			rounds = int(arg.trim_prefix("--rounds="))

	var files := Array(DirAccess.get_files_at(CASES_DIR))
	files.sort()
	for file: String in files:
		if not file.ends_with(".torch"):
			continue
		var case_name := file.get_basename()
		if not filter.is_empty() and not case_name.contains(filter):
			continue
		_run_case(case_name, load(CASES_DIR + file), rounds)

	get_tree().quit()

func _run_case(case_name: String, script: Script, rounds: int) -> void:
	var subject: Node = script.new()
	add_child(subject)

	# Warm-up, so first-call costs (lazy caches, allocations) don't land in a timed round.
	_reset(subject)
	subject.run()

	var samples: Array[int] = []
	var mem_delta := 0
	for i in rounds:
		_reset(subject)
		var mem_before := OS.get_static_memory_usage()
		var start := Time.get_ticks_usec()
		subject.run()
		samples.append(Time.get_ticks_usec() - start)
		mem_delta = OS.get_static_memory_usage() - mem_before

	_reset(subject)
	subject.free()

	samples.sort()
	print("BENCH %s %d %d %d" % [case_name, samples[0], samples[samples.size() / 2], mem_delta])

# Cases that retain objects (to measure their memory) keep them in a "keep" array; drop them between rounds.
func _reset(subject: Node) -> void:
	if "keep" in subject:
		subject.keep = []
