// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "string_utils.h"

#include <godot_cpp/variant/packed_string_array.hpp>

namespace StringUtils
{
    String default_if_empty(const String& p_value, const String& p_default_value)
    {
        return p_value.is_empty() ? p_default_value : p_value;
    }

    String replace_first(const String& p_value, const String& p_key, const String& p_with)
    {
        int pos = p_value.find(p_key);
        if (pos >= 0)
            return p_value.substr(0, pos) + p_with + p_value.substr(pos + p_key.length(), p_value.length());

        return p_value;
    }

    String path_to_file(const String &p_local, const String &p_path)
    {
        // Don't get base dir for src, this is expected to be a dir already
        String src = p_local.replace("\\", "/");
        String dst = p_path.replace("\\", "/").get_base_dir();
        String rel = path_to(src, dst);

        // If they're equal, this failed
        return rel == dst ? p_path : rel + p_path.get_file();
    }

    String path_to(const String &p_local, const String &p_path)
    {
        String src = p_local.replace("\\", "/");
        String dst = p_path.replace("\\", "/");

        if (!src.ends_with("/"))
            src += "/";
        if (!dst.ends_with("/"))
            dst += "/";

        if (src.begins_with("res://") && dst.begins_with("res://"))
        {
            src = src.replace("res://", "/");
            dst = dst.replace("res://", "/");
        }
        else if (src.begins_with("user://") && dst.begins_with("user://"))
        {
            src = src.replace("user://", "/");
            dst = dst.replace("user://", "/");
        }
        else if (src.begins_with("/") && dst.begins_with("/"))
        {
            // nothing
        }
        else
        {
            // dos style
            String src_begin = src.get_slicec('/', 0);
            String dst_begin = dst.get_slicec('/', 0);
            if (src_begin != dst_begin)
            {
                // Impossible to do this
                return p_path;
            }

            src = src.substr(src_begin.length(), src.length());
            dst = dst.substr(dst_begin.length(), dst.length());
        }

        // Remove leading and trailing slash and split
        PackedStringArray src_dirs = src.substr(1, src.length() - 2).split("/");
        PackedStringArray dst_dirs = dst.substr(1, dst.length() - 2).split("/");

        // Find common part
        int common_parent = 0;
        while (true)
        {
            if (src_dirs.size() == common_parent)
                break;
            if (dst_dirs.size() == common_parent)
                break;
            if (src_dirs[common_parent] != dst_dirs[common_parent])
                break;

            common_parent++;
        }
        common_parent--;

        int dirs_to_backtrack = (src_dirs.size() - 1) - common_parent;
        String dir = String("../").repeat(dirs_to_backtrack);

        for (int i = common_parent + 1; i < dst_dirs.size(); i++)
            dir += dst_dirs[i] + "/";

        if (dir.length() == 0)
            dir = "./";

        return dir;
    }
}