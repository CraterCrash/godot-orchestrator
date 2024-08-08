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
#ifndef ORCHESTRATOR_SCRIPT_CONNECTION_H
#define ORCHESTRATOR_SCRIPT_CONNECTION_H

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Defines a connection between two nodes and their respective ports.
struct OScriptConnection
{
    union
    {
        struct
        {
            // Allow for 24 million nodes, each with 255 ports per script.
            uint64_t from_node : 24;
            uint64_t from_port : 8;
            uint64_t to_node   : 24;
            uint64_t to_port   : 8;
        };
        uint64_t id{ 0 };
    };

    /// Check whether this connection is connected with the specified node ID.
    /// @param p_id the node id
    /// @return true if the connection is linked with the specified node, false otherwise
    bool is_linked_to(uint64_t p_id) const { return from_node == p_id || to_node == p_id; }

    /// Get the connection in a formatted string
    /// @return the formatted connection as a string
    String to_string() const { return vformat("%d:%d-%d:%d", from_node, from_port, to_node, to_port); }

    /// Convert the connection to a Godot dictionary for storage.
    Dictionary to_dict() const;

    /// Creates a script connection from a dictionary of values.
    /// @param p_dict the dictionary of values
    /// @return the new script connection
    static OScriptConnection from_dict(const Dictionary& p_dict);

    /// Compare two connections with the less-than operator.
    /// @note Needed for associative containers, i.e. RBSet
    bool operator<(const OScriptConnection& p_connection) const { return id < p_connection.id; }

    /// Create a connection from a given connection ID
    /// @param p_id the connection id
    explicit OScriptConnection(uint64_t p_id) : id(p_id) { }

    /// Create a connection with default values
    OScriptConnection() : from_node(0), from_port(0), to_node(0), to_port(0) { }
};

#endif // ORCHESTRATOR_SCRIPT_CONNECTION_H