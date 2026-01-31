## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##		http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
import base64, configparser, dataclasses, json, os, pathlib, sys, zlib;

header = """\
// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
"""

@dataclasses.dataclass
class Template:
    name: str
    description: str
    data_size: int # size of the uncompressed data
    data: str # base64 encoded of the zlib compressed data

def write_templates(templates_dir, out):
    items = []
    for ini_file in pathlib.Path(templates_dir).glob("*/*.ini"):
        config = configparser.ConfigParser()
        config.read(ini_file)

        if not config.has_section("meta"):
            print(f"Error: {ini_file} does not contain 'meta' section, template skipped.")
            continue

        description = config.get("meta", "description")

        script_filename = ini_file.with_suffix(".torch")
        if not os.path.exists(script_filename):
            print(f"Error: Cannot find torch {script_filename}, template skipped.")
            continue

        with open(script_filename, 'r', encoding='utf-8') as script:
            script_data = script.read()

        buf = script_data.encode("utf-8")
        decomp_size = len(buf)
        buf = zlib.compress(buf, zlib.Z_BEST_COMPRESSION)
        buf = base64.b64encode(buf).decode('utf-8')

        items.append({
            "name": str(script_filename.name),
            "inherits": str(script_filename.parent.name),
            "description": description,
            "data_size": decomp_size,
            "data": buf
        })

    json_obj = { "templates": items }

    json_data = json.dumps(json_obj).encode("utf-8")
    json_decomp_size = len(json_data)
    json_data = zlib.compress(json_data, zlib.Z_BEST_COMPRESSION)

    out.write("#pragma once\n")
    out.write("\n")
    out.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    out.write("\n")
    out.write(f"static const char* _template_data_hash = \"{hash(json_data)}\";\n")
    out.write(f"static constexpr int _template_data_uncompressed_size = {json_decomp_size};\n")
    out.write(f"static constexpr int _template_data_compressed_size = {len(json_data)};\n")
    out.write(f"static const unsigned char _template_data_compressed[] = {{\n")
    for i in range(len(json_data)):
        out.write(f"\t{json_data[i]},\n")
    out.write(f"}};\n")
    out.write("\n")

def main():
    if len(sys.argv) < 3:
        print("Usage: generate_script_templates.py <templates-directory> <generated-filename>")
        sys.exit(1)

    # Make sure we can find the templates directory
    templates_directory = sys.argv[1]
    if not os.path.exists(templates_directory):
        print(f"Error: Cannot find templates directory {templates_directory}")
        sys.exit(1)

    # Make sure we can write to the generated location
    generated_file = sys.argv[2]
    generated_filepath = pathlib.Path(generated_file)
    generated_filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(generated_file, 'w', encoding='utf-8') as out:
        out.write(header)
        write_templates(templates_directory, out)

if __name__ == "__main__":
    main()

