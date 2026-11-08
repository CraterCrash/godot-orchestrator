"""
Run the Orchestrator VM micro-benchmarks against Godot.

Each case under benchmarks/cases is an orchestration whose run() function is a tight loop around one
code path. The driver scene (benchmarks/bench.tscn) times run() over several rounds; this script runs
the driver in a few separate processes and keeps the best numbers, which filters out most scheduler noise.

Typical A/B workflow:
    # build main, then
    python3 run_benchmarks.py --save /tmp/main.json
    # build the branch, then
    python3 run_benchmarks.py --compare /tmp/main.json

Usage:
    python3 run_benchmarks.py [options]

Options:
    -k, --filter SUBSTR   Only run cases whose name contains SUBSTR.
    --runs N              Number of Godot processes to run (default 3).
    --rounds N            Timed rounds per case in each process (default 9).
    --save FILE           Write the results as JSON.
    --compare FILE        Compare against results saved earlier with --save.
    --version X.Y         Godot version (overrides the .gdextension minimum).
    --godot-binary PATH   Use a specific Godot binary (or set GODOT_BINARY).

Benchmarks need a build with DEBUG_ENABLED (the editor build used by the integration tests) since several
measured paths only exist in debug builds. Timings are only comparable on the same machine.
"""

import argparse
import datetime
import json
import os
import statistics
import subprocess
import sys
from pathlib import Path

# Importing the test runner would otherwise leave a __pycache__ directory in tests/.
sys.dont_write_bytecode = True
import run_integration_tests as rit

root = Path(__file__).parent
bench_scene = "res://benchmarks/bench.tscn"


def git(*args):
    try:
        return subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def run_once(name_filter, rounds):
    user_args = [f"--rounds={rounds}"]
    if name_filter:
        user_args.append(f"--filter={name_filter}")

    result = subprocess.run(
        [rit.godot_path, "--no-header", "--headless", "--path", root, bench_scene, "--", *user_args],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True)

    cases = {}
    for line in result.stdout.splitlines():
        if not line.startswith("BENCH "):
            continue
        _, name, best, median, mem = line.split()
        cases[name] = {"min_usec": int(best), "median_usec": int(median), "mem_bytes": int(mem)}

    if result.returncode != 0 or not cases:
        print(f"Benchmark run failed (exit code {result.returncode})", file=sys.stderr)
        print(rit.truncate(result.stdout.strip()), file=sys.stderr)
        sys.exit(1)

    return cases


def run_benchmarks(name_filter, runs, rounds):
    samples = {}
    for i in range(runs):
        print(f"Run {i + 1}/{runs}...", flush=True)
        for name, data in run_once(name_filter, rounds).items():
            samples.setdefault(name, []).append(data)

    results = {}
    for name, runs_data in samples.items():
        results[name] = {
            "min_usec": min(d["min_usec"] for d in runs_data),
            "median_usec": int(statistics.median(d["median_usec"] for d in runs_data)),
            "mem_bytes": int(statistics.median(d["mem_bytes"] for d in runs_data)),
        }
    return results


def delta(new, old):
    if not old:
        return ""
    pct = (new - old) * 100.0 / old
    text = f"{pct:+.1f}%"
    if abs(pct) < 3.0:
        return text
    return rit.color(text, rit.GREEN if pct < 0 else rit.RED)


def format_mem(value):
    return f"{value / 1024:.0f} KiB" if abs(value) >= 1024 else f"{value} B"


def print_results(results, baseline):
    header = f"{'case':<24} {'min':>10} {'median':>10} {'memory':>12}"
    if baseline:
        header += f"   {'Δ min':>8} {'Δ median':>9} {'Δ memory':>9}"
    print()
    print(header)
    print("-" * len(header))

    for name in sorted(results):
        r = results[name]
        line = f"{name:<24} {r['min_usec'] / 1000:>8.2f}ms {r['median_usec'] / 1000:>8.2f}ms {format_mem(r['mem_bytes']):>12}"
        if baseline:
            b = baseline.get(name)
            if b:
                line += (f"   {delta(r['min_usec'], b['min_usec']):>8} {delta(r['median_usec'], b['median_usec']):>9}"
                         f" {delta(r['mem_bytes'], b['mem_bytes']) if b['mem_bytes'] > 4096 else '':>9}")
            else:
                line += "   (new)"
        print(line)
    print()


def parse_args():
    parser = argparse.ArgumentParser(description="Run the Orchestrator VM micro-benchmarks.")
    parser.add_argument("-k", "--filter", dest="filter", default=None, help="Only run cases containing this substring.")
    parser.add_argument("--runs", type=int, default=3, help="Godot processes to run (default 3).")
    parser.add_argument("--rounds", type=int, default=9, help="Timed rounds per case per process (default 9).")
    parser.add_argument("--save", default=None, help="Write results as JSON to this file.")
    parser.add_argument("--compare", default=None, help="Compare against a JSON file written with --save.")
    parser.add_argument("--no-color", action="store_true", help="Disable colored output.")
    parser.add_argument("--version", default=None, help="Godot version (overrides the .gdextension minimum).")
    parser.add_argument("--godot-binary", default=None, help="Path to a Godot binary (or set GODOT_BINARY).")
    return parser.parse_args()


def main():
    args = parse_args()
    rit.use_color = sys.stdout.isatty() and not args.no_color

    rit.version = args.version or rit.get_minimum_godot_version()
    binary = args.godot_binary or os.environ.get("GODOT_BINARY")
    if binary:
        rit.godot_path = Path(binary).expanduser()
        if not rit.godot_path.exists():
            print(f"Godot binary not found: {rit.godot_path}", file=sys.stderr)
            sys.exit(1)
    else:
        rit.godot_path = rit.download_godot(rit.version)

    baseline = None
    if args.compare:
        with open(args.compare) as f:
            saved = json.load(f)
        baseline = saved["results"]
        print(f"Comparing against {args.compare} ({saved.get('branch')} @ {saved.get('commit')}, {saved.get('date')})")

    rit.update_libraries()
    rit.clean_godot_cache()
    rit.import_project()

    results = run_benchmarks(args.filter, max(1, args.runs), max(1, args.rounds))
    print_results(results, baseline)

    if args.save:
        payload = {
            "branch": git("rev-parse", "--abbrev-ref", "HEAD"),
            "commit": git("rev-parse", "--short", "HEAD"),
            "dirty": bool(git("status", "--porcelain", "--untracked-files=no")),
            "date": datetime.datetime.now().isoformat(timespec="seconds"),
            "godot": str(rit.godot_path.name),
            "results": results,
        }
        with open(args.save, "w") as f:
            json.dump(payload, f, indent=2)
        print(f"Saved results to {args.save}")


if __name__ == "__main__":
    main()
