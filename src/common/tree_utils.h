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
#ifndef ORCHESTRATOR_TREE_UTILS_H
#define ORCHESTRATOR_TREE_UTILS_H

#include <godot_cpp/classes/tree.hpp>

using namespace godot;

/// An iterator implementation for <code>Tree</code> that uses depth-first
struct TreeIterator
{
private:
    TreeItem* _current = nullptr;

public:

    TreeIterator& operator++();
    TreeItem* operator*() const;
    bool operator!=(const TreeIterator& p_other) const;

    explicit TreeIterator(TreeItem* p_root) : _current(p_root) { }
};

/// Provides a reusable iterable for-loop semantics for traversing a <code>Tree</code> using depth-first
struct TreeIterable
{
private:
    TreeItem* _root;

public:
    TreeIterator begin() const;
    TreeIterator end() const;

    TreeIterable(TreeItem* p_root) : _root(p_root) { }
};

#endif