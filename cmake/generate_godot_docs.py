## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
import os, sys, platform

def make_doc_source(target, source):
    import zlib

    dst = str(target[0])
    g = open(dst, "w", encoding="utf-8")
    buf = ""
    docbegin = ""
    docend = ""
    for src in source:
        src_path = str(src)
        if not src_path.endswith(".xml"):
            continue
        with open(src_path, "r", encoding="utf-8") as f:
            content = f.read()
        buf += content

    buf = (docbegin + buf + docend).encode("utf-8")
    decomp_size = len(buf)

    # Use maximum zlib compression level to further reduce file size
    # (at the cost of initial build times).
    buf = zlib.compress(buf, zlib.Z_BEST_COMPRESSION)

    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("\n")
    g.write("#include <godot_cpp/godot.hpp>\n")
    g.write("\n")

    g.write('static const char *_doc_data_hash = "' + str(hash(buf)) + '";\n')
    g.write("static const int _doc_data_uncompressed_size = " + str(decomp_size) + ";\n")
    g.write("static const int _doc_data_compressed_size = " + str(len(buf)) + ";\n")
    g.write("static const unsigned char _doc_data_compressed[] = {\n")
    for i in range(len(buf)):
        g.write("\t" + str(buf[i]) + ",\n")
    g.write("};\n")
    g.write("\n")

    g.write(
        "static godot::internal::DocDataRegistration _doc_data_registration(_doc_data_hash, _doc_data_uncompressed_size, _doc_data_compressed_size, _doc_data_compressed);\n"
    )
    g.write("\n")

    g.close()

def main():
    if len(sys.argv) > 2:
        target_str = sys.argv[1]
        target_list = target_str.split(",")
        source_str = sys.argv[2]
        source_list = source_str.split(",")
        make_doc_source(target_list, source_list)

if __name__ == "__main__":
    main()
