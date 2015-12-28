#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright 2014 Lukas Kemmer
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

import re

def parse_py_common(path):
    """Parses the definitions in py-common.hh. Returns a dict mapping
    struct-names to name and doc.

    Example return:
     {"Common_hello_world : ("hello_world", "says hello to the
     world.")}

    """
    re_struct_entry = re.compile(r"struct\s*(\w*)\s*\{(\s*.*?)\n};", re.DOTALL)
    structs = {}
    with open(path) as f:
        for item in re_struct_entry.finditer(f.read()):
            structs[item.group(1)] = (_find_name_str(item.group(2)),
                                      _find_doc_str(item.group(2)))
    return structs


def parse_common_methoddef_entries(text):
    """Parses a methoddef-section. Finds all entries that appear to be a
    forward to a Common_-structure, and returns the struct-names as a
    list.

    """

    re_methoddef_common = re.compile(r'\s*\w*\((Common_\w*)\),')
    return [match.group(1) for match in re_methoddef_common.finditer(text)]


def methoddef_common_to_name_doc(commonMethodList, commonDict):
    """Returns a dictionary of name-strings to doc-strings for the list of
    Common-struct names by looking them up in the commonDict.

    Example:
    With a commonMethodList = ["Common_hello_world"]
    and commonDict = {"Common_hello_world" : ("hello_world", Says
    hello to the world"}

    the return would be {"hello_world" :  "Says hello to the world"}

    """
    methods = {}
    for item in commonMethodList:
        methods[commonDict[item][0]] = commonDict[item][1]
    return methods


def _find_name_str(text):
    """Finds the name-entry in the struct. Returns None on failure."""

    re_doc = re.compile(r'static const char\* Name\(\)\{\s*return\s*\"(.*?)\"\;')
    match = re_doc.search(text)
    return match.group(1) if match else None


def _find_doc_str(text):
    """Finds the doc-entry in the struct.

    Fails for fancier things (e.g. building by combining strings, or
    using C++-template defined ranges) - returns an empty string in
    these cases.

    """
    re_doc = re.compile(r'static const char\* Doc\(\)\{\s*return\s*\"(.*?)\"\;')
    match = re_doc.search(text)
    return match.group(1).strip() if match else ""

def _parse_PyGetSetDef_entries(text):
    re_getsetdef_entry = re.compile(r"\{\s*\(char\*\)\s*\"(.*?)\"\s*,.*?,.*?,\s*\(char\*\)\s*\"(.*?)\"\s*,\s*nullptr\}\s*,", re.DOTALL)

    name_to_doc = {}
    for item in re_getsetdef_entry.finditer(text):
        name_to_doc[item.group(1)] = item.group(2)
    return name_to_doc


def _find_PyGetSetDef(text):
    re_getsetdef = re.compile(r"static PyGetSetDef ([a-zA-Z_]+)\[\]\s*=\s*\{(.*?)\};", re.DOTALL)

    match = re_getsetdef.search(text)
    return match.group(2) if match else ""

def parse_PyGetSetDef(file_path):
    with open(file_path) as f:
        getsetdef = _find_PyGetSetDef(f.read())
    return _parse_PyGetSetDef_entries(getsetdef)

def _test_MethodDef(args):
    """Test code"""
    if len(args) != 2:
        print("For methoddef test, pass:")
        print(" path to py-common.hh as arg1")
        print(" path to file with a methoddef as arg2.")
        exit(1)
    py_common_path = args[0]
    method_def_path = args[1]

    commonDict = parse_py_common(py_common_path)

    with open(method_def_path) as f:
        commonMethodList = parse_common_methoddef_entries(f.read())

    print(methoddef_common_to_name_doc(commonMethodList, commonDict))

def _test_GetSetDef(args):
    """Test code"""
    if len(args) != 1:
        print("For getsetdef test, pass:")
        print(" path to file with a PyGetSetDef as arg1")
        exit(1)

    with open(args[0]) as f:
        getsetdef = _find_PyGetSetDef(f.read())

    print(getsetdef)
    print(_parse_PyGetSetDef_entries(getsetdef))


if __name__ == '__main__':
    import sys
    if len(sys.argv) == 1:
        print("For self test, pass methoddef or getsetdef as arg1")
        exit(1)

    if sys.argv[1] == "methoddef":
        _test_MethodDef(sys.argv[2:])
    elif sys.argv[1] == "getsetdef":
        _test_GetSetDef(sys.argv[2:])
    else:
        print("Expected methoddef or getsetdef, got %s" % sys.argv[1])
        exit(1)
