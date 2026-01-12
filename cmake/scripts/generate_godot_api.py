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
import os
import sys
import zlib

def make_api_source(api_data, target):
    buf = api_data.encode("utf-8")
    decomp_size = len(buf)

    buf = zlib.compress(buf, zlib.Z_BEST_COMPRESSION)

    with open(target, 'w', encoding='utf-8') as f:
        f.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
        f.write("\n")
        f.write("#include \"api/extension_db.h\"\n")
        f.write("\n")
        f.write("#include <godot_cpp/variant/packed_byte_array.hpp>\n")
        f.write("\n")
        f.write(f"static const char* _api_data_hash = \"{hash(buf)}\";\n")
        f.write(f"static const int _api_data_uncompressed_size = {decomp_size};\n")
        f.write(f"static const int _api_data_compressed_size = {len(buf)};\n")
        f.write("static const unsigned char _api_data_compressed[] = {\n")
        for i in range(len(buf)):
            f.write(f"\t{buf[i]},\n")
        f.write("};\n")
        f.write("\n")
        f.write("void ExtensionDB::_decompress_and_load() {\n")
        f.write("\tPackedByteArray compressed;\n")
        f.write("\tcompressed.resize(_api_data_compressed_size);\n")
        f.write("\tmemcpy(compressed.ptrw(), _api_data_compressed, _api_data_compressed_size);\n")
        f.write("\n")
        f.write("\tPackedByteArray decompressed = compressed.decompress(_api_data_uncompressed_size, 1);\n")
        f.write("\t_load(decompressed);\n")
        f.write("}\n")
        f.write("\n")

def main():
    if len(sys.argv) < 3:
        print("Usage: generate_godot_api.py <extension_api_json_path> <output_dir>")
        sys.exit(1)

    extension_api_json = sys.argv[1]
    output_dir = sys.argv[2]

    if not os.path.exists(extension_api_json):
        print(f"Error: extension_api.json was not found at {extension_api_json}")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    with open(extension_api_json, 'r', encoding='utf-8') as f:
        api_data = f.read()

    make_api_source(api_data, os.path.join(output_dir, "api_data.cpp"))

if __name__ == "__main__":
    main()