"""
Run the Orchestrator integration tests against Godot.

Downloads (or reuses) the appropriate Godot build, copies the addon into the test
project, imports the project, then runs every scene under scenes/ and compares its
output against the matching .out file.

Usage:
    python3 run_integration_tests.py [options]

Options:
    -k, --filter SUBSTR   Only run scenes whose path (relative to scenes/) contains
                          SUBSTR. Handy for iterating on a single failing test.
    --no-color            Disable colored output (also auto-disabled when stdout is
                          not a TTY, e.g. in CI logs).
    --version X.Y         Godot version to test against. Overrides the
                          compatibility_minimum read from the .gdextension file.
    -j, --jobs N          Number of scenes to run in parallel. Defaults to the CPU
                          count, capped at 4; use -j 1 to force sequential runs.
    -h, --help            Show the argparse-generated help and exit.

Exit code is 0 when all tests pass (or skip), and 1 if any test fails, crashes, or
hits an unknown directive.
"""

import argparse
import json
import os
import time
import re
import shutil
import signal
import subprocess
import sys
import time
import urllib.error
import urllib.request

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

MAX_JOBS = 4

scenes_dir = (Path(__file__).parent / "scenes").resolve()

use_color = sys.stdout.isatty()
GREEN  = "\033[32m"
RED    = "\033[31m"
YELLOW = "\033[33m"
RESET  = "\033[0m"

STATUS_COLORS = {
    "PASS":  GREEN,
    "FAIL":  RED,
    "CRASH": RED,
    "ERROR": RED,
    "SKIP":  YELLOW,
}

counts = {"PASS": 0, "FAIL": 0, "CRASH": 0, "ERROR": 0, "SKIP": 0}

def color(text, c):
    return f"{c}{text}{RESET}" if use_color else text

def format_result(status, elapsed, scene_file):
    label = color(f"{status:<6}", STATUS_COLORS[status])
    rel = scene_file.resolve().relative_to(scenes_dir)
    return f"{label}  ({elapsed:.2f}s)  {rel}"

def summary_part(n, label, c):
    text = f"{n} {label}"
    return color(text, c) if n else text

def print_summary(total_elapsed):
    total = sum(counts.values())
    print("-" * 80)
    parts = [
        summary_part(counts["PASS"],  "passed",  GREEN),
        summary_part(counts["FAIL"],  "failed",  RED),
        summary_part(counts["CRASH"], "crashed", RED),
        summary_part(counts["ERROR"], "errored", RED),
        summary_part(counts["SKIP"],  "skipped", YELLOW),
    ]
    print(", ".join(parts) + f"  ({total} total in {total_elapsed:.2f}s)")

def truncate(text, max_lines=30):
    lines = text.splitlines()
    if len(lines) <= max_lines:
        return text
    omitted = len(lines) - max_lines
    return "\n".join(lines[-max_lines:] + [f"... {omitted} more line(s) omitted"])

def parse_version(v):
    return tuple(int(x) for x in v.split(".")[:2])

def read_meta(scene_file):
    """Parse the optional sibling .meta file into a key/value dict.
    Returns an empty dict when no .meta file exists.
    """
    meta_file = scene_file.with_suffix(".meta")
    if not meta_file.exists():
        return {}

    meta = {}
    for line in meta_file.read_text().splitlines():
        # Allow full-line comments (starting with #) and blank lines.
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if ":" in line:
            key, value = line.split(":", 1)
            # Allow an inline trailing comment after the value.
            value = value.split("#", 1)[0]
            meta[key.strip()] = value.strip()
    return meta

def is_version_supported(version, scene_file):
    meta = read_meta(scene_file)
    if not meta:
        # If no meta exists, the test is acceptable to run on all versions
        return True

    current = parse_version(version)
    if "min_version" in meta and current < parse_version(meta["min_version"]):
        return False
    if "max_version" in meta and current > parse_version(meta["max_version"]):
        return False

    return True

def strip_backtrace(text):
    return "\n".join(
        line for line in text.splitlines()
        if not line.startswith("   OScript backtrace") and not line.startswith("       [")
    ).strip()

def normalize_cpp_lines(text):
    return re.sub(r'(\w+\.cpp):\d+', r'\1', text)

def validate_output(source, result, elapsed):
    source = source.resolve()
    out_file = source.with_suffix(".out")

    if not out_file.exists():
        return "SKIP", format_result("SKIP", elapsed, source) + \
            "\n  No .out file was defined, test will be skipped."

    lines = out_file.read_text().strip().splitlines()
    if len(lines) == 0:
        return "FAIL", format_result("FAIL", elapsed, source) + \
            "\n  The .out file is empty, test failed."

    # First line is the directive
    directive = lines[0].strip()
    expected = "\n".join(lines[1:]).strip()

    if directive == "OSCRIPT_TEST_PASS":
        stderr = result.stderr.strip()
        if stderr:
            return "FAIL", format_result("FAIL", elapsed, source) + \
                f"\nExpected empty stderr, but got:\n----------\n{stderr}\n"
        actual = result.stdout.strip()
    elif directive == "OSCRIPT_TEST_FAILURE":
        actual = result.stderr.strip()
        # Godot did not add backtrace support until Godot 4.5+
        if version == "4.4":
            actual = strip_backtrace(actual)
            expected = strip_backtrace(expected)

        actual = normalize_cpp_lines(actual)
        expected = normalize_cpp_lines(expected)

    else:
        return "ERROR", format_result("ERROR", elapsed, source) + \
            f"\n  Unknown directive '{directive}' in {out_file}"

    if actual == expected:
        return "PASS", format_result("PASS", elapsed, source)
    else:
        return "FAIL", format_result("FAIL", elapsed, source) + \
            f"\nExpected:\n----------\n{expected}\nOutput:\n----------\n{actual}\n"

def clean_godot_cache():
    godot_cache = Path(__file__).parent / ".godot"
    if godot_cache.exists():
        shutil.rmtree(godot_cache)

def import_project():
    result = subprocess.run(
        [
            godot_path,
            "--no-header",
            "--headless",
            "--path",
            Path(__file__).parent,
            "--import",
            "--quiet"],
        capture_output=True,
        text=True)

    if result.returncode != 0:
        # A segfault surfaces as a negative return code (e.g. -11 for SIGSEGV).
        # Surface whatever the binary emitted so the crash can be diagnosed
        # instead of failing silently on a discarded DEVNULL.
        signal_name = ""
        if result.returncode < 0:
            signal_name = f" ({signal.Signals(-result.returncode).name})"
        print(f"Project import failed with exit code {result.returncode}"
              f"{signal_name}", file=sys.stderr)
        if result.stdout.strip():
            print("---------- stdout ----------", file=sys.stderr)
            print(result.stdout.rstrip(), file=sys.stderr)
        if result.stderr.strip():
            print("---------- stderr ----------", file=sys.stderr)
            print(result.stderr.rstrip(), file=sys.stderr)
        sys.exit(1)

def run_scene(scene_file):
    meta = read_meta(scene_file)

    # A scene may opt into a fixed timestep so that physics frames are deterministic.
    # Without --fixed-fps the main loop free-runs against the wall clock, so an unpredictable number of idle
    # (aka _process) frames elapse before the first physics (_physics_process) tick.
    # This makes physics lifecycle output impossible to pin down if an .out record needs an explicit order.
    frame_args = []
    if "fixed_fps" in meta:
        frame_args += ["--fixed-fps", meta["fixed_fps"]]

    # Number of main-loop iterations before Godot force-quits.
    # Defaults to 2.
    # Physics scenes typically pair this with fixed_fps and quit_after: 1 to capture a single deterministic frame.
    frame_args += ["--quit-after", meta.get("quit_after", "2")]

    start = time.monotonic()
    try:
        result = subprocess.run(
            [
                godot_path,
                "--no-header",
                "--headless",
                "--path",
                Path(__file__).parent,
                *frame_args,
                "--scene",
                str(scene_file)],
            capture_output=True,
            check=True,
            text=True)
    except subprocess.CalledProcessError as e:
        elapsed = time.monotonic() - start
        text = format_result("CRASH", elapsed, scene_file)
        text += f"\n  exit code {e.returncode}"
        if e.stdout.strip():
            text += "\n" + truncate(e.stdout.strip())
        if e.stderr.strip():
            text += "\n" + truncate(e.stderr.strip())
        return "CRASH", text + "\n"

    elapsed = time.monotonic() - start
    return validate_output(scene_file, result, elapsed)

def test_scenes(version, name_filter, jobs):
    scenes = []
    for scene_file in sorted(scenes_dir.rglob("*.tscn")):
        rel = scene_file.relative_to(scenes_dir)
        if name_filter and name_filter not in str(rel):
            continue
        if not is_version_supported(version, scene_file.resolve()):
            continue
        scenes.append(scene_file)

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        # map() preserves submission order, so output stays deterministic
        # regardless of which scene finishes first.
        for status, text in executor.map(run_scene, scenes):
            counts[status] += 1
            print(text)

def atomic_copy(src, dst):
    src = Path(src)
    dst = Path(dst)
    for source_file in src.rglob("*"):
        if source_file.is_dir():
            continue
        dest_file = dst / source_file.relative_to(src)
        dest_file.parent.mkdir(parents=True, exist_ok=True)
        if dest_file.exists():
            dest_file.unlink()
        shutil.copy2(source_file, dest_file)
def update_libraries():
    atomic_copy(
    # shutil.copytree(
        Path(__file__).parent / "../project/addons/orchestrator",
        Path(__file__).parent / "addons/orchestrator")
      #  dirs_exist_ok=True)

def get_minimum_godot_version():
    gdext_path = Path(__file__).parent / "../project/addons/orchestrator/orchestrator.gdextension"
    content = gdext_path.read_text()
    match = re.search(r'compatibility_minimum="([^"]+)"', content)
    if not match:
        raise RuntimeError("Could not find compatibility_minimum in .gdextension file")
    return match.group(1)

def _github_token():
    return os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")

def github_api_get(url, max_retries=5):
    """GET a GitHub API URL as JSON, authenticating when a token is available
    and backing off on rate-limit (403/429) responses using the reset headers."""
    headers = {
        "User-Agent": "godot-test-runner",
        "Accept": "application/vnd.github+json",
    }
    token = _github_token()
    if token:
        headers["Authorization"] = f"Bearer {token}"

    for attempt in range(max_retries):
        req = urllib.request.Request(url, headers=headers)
        try:
            with urllib.request.urlopen(req) as r:
                return json.load(r)
        except urllib.error.HTTPError as e:
            if e.code not in (403, 429) or attempt == max_retries - 1:
                if e.code in (403, 429) and not token:
                    raise RuntimeError(
                        "GitHub API rate limit exceeded. Set GITHUB_TOKEN (or "
                        "GH_TOKEN) to raise the limit, or pass --godot-binary to "
                        "skip the download entirely."
                    ) from e
                raise

            # Prefer Retry-After, then the rate-limit reset epoch, else backoff.
            retry_after = e.headers.get("Retry-After")
            reset = e.headers.get("X-RateLimit-Reset")
            if retry_after and retry_after.isdigit():
                wait = int(retry_after)
            elif reset and reset.isdigit():
                wait = max(0, int(reset) - int(time.time())) + 1
            else:
                wait = 2 ** attempt
            wait = min(wait, 120)
            print(f"GitHub API rate-limited (HTTP {e.code}); retrying in {wait}s "
                  f"(attempt {attempt + 1}/{max_retries})...")
            time.sleep(wait)

    raise RuntimeError("GitHub API request failed after retries")

def find_latest_godot_release(major_minor):
    page = 1
    while True:
        url = f"https://api.github.com/repos/godotengine/godot-builds/releases?per_page=100&page={page}"
        releases = github_api_get(url)

        if not releases:
            break

        for release in releases:
            tag = release["tag_name"]  # e.g. "4.7-dev3", "4.6.2-rc2"
            if tag.startswith(f"{major_minor}-") or tag.startswith(f"{major_minor}."):
                for asset in release["assets"]:
                    asset_name = asset["name"]
                    if asset_name.endswith("linux.x86_64.zip"):
                        return release, tag, asset

        page += 1

    raise RuntimeError(f"No release found for Godot {major_minor}")

def download_godot(version):
    # Adjust the filename pattern to match platform

    Path("bin").mkdir(parents=True, exist_ok=True)

    release, tag, asset = find_latest_godot_release(version)
    filename = f"bin/Godot_v{tag}_linux.x86_64"

    url = asset["browser_download_url"]

    zip_path = Path(__file__).parent / f"{filename}.zip"
    godot_path = Path(__file__).parent / filename

    if godot_path.exists():
        print(f"Godot {version} already exists, skipping download.")
        return godot_path

    print(f"Downloading Godot {version} from {url}...")
    urllib.request.urlretrieve(url, zip_path)

    import zipfile
    with zipfile.ZipFile(zip_path, "r") as z:
        z.extractall(Path(__file__).parent / "bin")
    zip_path.unlink()

    godot_path.chmod(0o755)  # make executable
    return godot_path

def parse_args():
    parser = argparse.ArgumentParser(
        description="Run Orchestrator integration tests against Godot.")
    parser.add_argument(
        "-k", "--filter", dest="filter", default=None,
        help="Only run scenes whose path (relative to scenes/) contains this substring.")
    parser.add_argument(
        "--no-color", action="store_true",
        help="Disable colored output.")
    parser.add_argument(
        "--version", dest="version", default=None,
        help="Godot version to test against (overrides the .gdextension minimum).")
    parser.add_argument(
        "--godot-binary", dest="godot_binary", default=None,
        help="Path to a specific Godot binary to use, bypassing the download "
             "(also settable via the GODOT_BINARY environment variable).")
    default_jobs = min(os.cpu_count() or 1, MAX_JOBS)
    parser.add_argument(
        "-j", "--jobs", type=int, default=default_jobs,
        help=f"Number of scenes to run in parallel "
             f"(default: {default_jobs}, capped at {MAX_JOBS}).")
    return parser.parse_args()

def main():
    global version, godot_path, use_color

    args = parse_args()
    use_color = sys.stdout.isatty() and not args.no_color

    binary = args.godot_binary or os.environ.get("GODOT_BINARY")
    if binary:
        godot_path = Path(binary).expanduser()
        if not godot_path.exists():
            print(f"Godot binary not found: {godot_path}", file=sys.stderr)
            sys.exit(1)
        version = args.version or get_minimum_godot_version()
        print(f"Using Godot binary {godot_path}")
    else:
        version = args.version or get_minimum_godot_version()
        godot_path = download_godot(version)

    update_libraries()
    clean_godot_cache()
    import_project()

    jobs = max(1, min(args.jobs, MAX_JOBS))

    run_start = time.monotonic()
    test_scenes(version, args.filter, jobs)
    total_elapsed = time.monotonic() - run_start

    print_summary(total_elapsed)

    sys.exit(1 if counts["FAIL"] or counts["CRASH"] or counts["ERROR"] else 0)

if __name__ == "__main__":
    main()