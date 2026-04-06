import argparse
import json
import time
import re
import shutil
import subprocess
import sys
import urllib.request

from pathlib import Path

scenes_dir  = Path(__file__).parent / "scenes"
exit_code = 0

def parse_version(v):
    return tuple(int(x) for x in v.split(".")[:2])

def is_version_supported(version, scene_file):
    meta_file = scene_file.with_suffix(".meta")
    if not meta_file.exists():
        # If no meta exists, the test is acceptable to run on all versions
        return True

    meta = {}
    for line in meta_file.read_text().splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            meta[key.strip()] = value.strip()

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
    global exit_code
    source = source.resolve()
    out_file = source.with_suffix(".out")

    if not Path(out_file).exists():
        print(f"SKIP: ({elapsed:.2f}s)")
        print(f"")
        print(f"No .out file was defined, test will be skipped.")
        return

    lines = out_file.read_text().strip().splitlines()
    if len(lines) == 0:
        print(f"FAIL: ({elapsed:.2f}s)")
        print(f"")
        print(f"The .out file is empty, test failed.")
        exit_code = 1
        return

    # First line is the directive
    directive = lines[0].strip()
    expected = "\n".join(lines[1:]).strip()

    if directive == "OSCRIPT_TEST_PASS":
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
        print(f"ERROR: Unknown directive '{directive}' in {out_file}")
        exit_code = 1
        return

    if actual == expected:
        print(f"PASS: ({elapsed:.2f}s)")
        print("")
    else:
        print(f"FAIL: ({elapsed:.2f}s)")
        print("")
        print(f"Expected:\n----------\n{expected}")
        print(f"Output:\n----------\n{actual}")
        print("")
        exit_code = 1

def test_scenes(version):
    global exit_code
    for scene_file in sorted(scenes_dir.rglob("*.tscn")):

        if not is_version_supported(version, scene_file.resolve()):
            continue

        print(f"-------------------------------------------------------------------------------------------------------------------")
        print(f"Scene: {scene_file.resolve()}")
        start = time.monotonic()
        try:
            result = subprocess.run(
                [
                    godot_path,
                    "--no-header",
                    "--headless",
                    "--path",
                    Path(__file__).parent,
                    "--quit-after",
                    "2",
                    "--scene",
                    str(scene_file)],
                capture_output=True,
                check=True,
                text=True)
        except subprocess.CalledProcessError as e:
            elapsed = time.monotonic() - start
            print(f"CRASH: (exit code {e.returncode}) ({elapsed:.2f}s)")
            print(f"{e.stdout.strip()}")
            print(f"{e.stderr.strip()}")
            print("")
            exit_code = 1
            continue

        elapsed = time.monotonic() - start

        validate_output(scene_file, result, elapsed)

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

def find_latest_godot_release(major_minor):
    page = 1
    while True:
        url = f"https://api.github.com/repos/godotengine/godot-builds/releases?per_page=100&page={page}"
        req = urllib.request.Request(url, headers={"User-Agent": "godot-test-runner"})
        with urllib.request.urlopen(req) as r:
            releases = json.load(r)

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

version = get_minimum_godot_version()
godot_path = download_godot(version)

update_libraries()
test_scenes(version)


sys.exit(exit_code)