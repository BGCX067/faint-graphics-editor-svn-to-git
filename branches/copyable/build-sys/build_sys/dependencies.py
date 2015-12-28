#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright 2013 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import os
import time

# Fixme: Duplicates faint_info.py

def get_src_folders(platform='msw', test=False):
    assert(platform in ('linux', 'msw'))
    folders = [
        "",
        "app/",
        "bitmap/",
        "commands/",
        "formats/",
        "generated/",
        "geo/",
        "gui/",
        "gui/paint-dialog",
        "objects/",
        "python/",
        "python/generate/output",
        "rendering/",
        "tablet/",
        "tasks/",
        "tools/",
        "util/"
    ]
    if platform == 'msw':
        folders.extend((
            "tablet/msw",
            "tablet/msw/wintab"))
    if test:
        folders.append("test")
        folders.append("test/gen")
        folders.append("test-sys")
    return sorted(folders)

faintRoot = (os.path.abspath("../") + "/").replace("\\","/")

def enumerate_cpp(folder):
    return [file for file in os.listdir( folder ) if file.endswith('.cpp')]

def enumerate_hh(folder):
    return [file for file in os.listdir( folder ) if file.endswith('.hh')]

def find_includes(filename):
    import re
    with open(filename) as f:
        return re.findall(r'#include "(.*)"', f.read())

def find_header_dependencies(root, files):
    deps = {}
    for filename in files:
        includes = find_includes(filename)
        for include in [inc for inc in [root + incl for incl in includes] if inc in files]:
            if include not in deps:
                deps[include] = set()
            deps[include].add(filename)
    return deps

def follow(include, deps, items, flat, done):
    for includer in deps.get(include,[]):
        if includer.endswith(".cpp"):
            items.add(includer)
        elif includer.endswith(".hh"):
            if includer in done:
                items.update(flat[includer])
            else:
                follow(includer, deps, items, flat, done)

def flatten(deps):
    flat={}
    done = set()
    for include in deps:
        if include not in flat:
            flat[include] = set()
        for includer in deps[include]:
            if includer.endswith("cpp"):
                flat[include].add(includer)
            elif includer.endswith(".hh"):
                follow(include, deps, flat[include], flat, done)
        done.add(include)
    return flat


def find_header_dependencies_all(root):
    return find_header_dependencies(root, enumerate_all_sources(root))

def enumerate_all_sources(root):
    src = []
    for folder in get_src_folders():
        for file in [file for file in os.listdir(os.path.join(root,folder)) if file.endswith('cpp') or file.endswith('.hh')]:
            src.append(os.path.join(root, folder, file).replace("\\","/"))
    return src

def get_flat_header_dependencies(root):
    return flatten(find_header_dependencies_all(root))

def _test_flat_header_dependencies(root):
    """Prints a list of headers mapped to every file that sees that
    header."""

    deps = get_flat_header_dependencies(root)
    print("Header->dependents (recursive)")
    print()
    for dep in sorted(deps.keys(), key=lambda x:-len(deps[x])):
        num_deps = len(deps[dep])
        print("%s (%d):" % (dep.replace(root, ""), num_deps))
        for dependent in sorted(deps[dep]):
            print(" ", dependent.replace(root, ""))


def _mapped_includes(root):
    """Return a dictionary mapping each file to all headers it includes."""
    files = enumerate_all_sources(root)
    includes = {}
    for filename in files:
        adj_filename = filename.replace(root, "")
        if adj_filename.startswith("/"):
            adj_filename = adj_filename[1:]
        includes[adj_filename] = find_includes(filename)
    return includes


def _test_unflat_header_dependencies(root, count_only=False):
    """Prints a list of every header mapped to every file that directly
    includes the header.

    """

    deps = find_header_dependencies_all(root)
    print("Header->direct dependents!")
    print()
    if not count_only:
        for dep in sorted(deps.keys(), key=lambda x:-len(deps[x])):
            num_deps = len(deps[dep])
            print("%s (%d):" % (dep.replace(root, ""), num_deps))
            for dependent in sorted(deps[dep]):
                print(" ", dependent.replace(root, ""))
    else:
        for dep in sorted(deps.keys(), key=lambda x:-len(deps[x])):
            print(dep, len(deps[dep]))


if __name__ == '__main__':
    import sys
    if len(sys.argv) < 2:
        exit(1)
    root = sys.argv[1].replace("\\","/")
    if not root.endswith("/"):
        root += "/"

    if len(sys.argv) == 2:
        _test_unflat_header_dependencies(root)
    elif len(sys.argv) == 3:
        if sys.argv[2] == '--flat':
            _test_flat_header_dependencies(root)
        else:
            exit(1)
    else:
        exit(1)
