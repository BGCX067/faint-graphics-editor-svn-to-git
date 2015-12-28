# Copyright 2012 Lukas Kemmer
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

"""Generate resource loading code and resource identifiers for all
files in <faint-root>/graphics>"""

import os
import sys

DEFAULT_CURSORS = {"ARROW": "wxCURSOR_ARROW",
                   "CARET": "wxCURSOR_IBEAM",
                   "RESIZE_NW": "wxCURSOR_SIZENWSE",
                   "RESIZE_NE": "wxCURSOR_SIZENESW",
                   "RESIZE_WE": "wxCURSOR_SIZEWE",
                   "RESIZE_NS": "wxCURSOR_SIZENS",
                   "BLANK": "wxCURSOR_BLANK" }

class Paths:
    def __init__(self, faint_root):
        self.faint_root = faint_root
        self.graphics_dir = os.path.join(faint_root, "graphics")
        self.generated_dir = os.path.join(faint_root, "generated")
        self.target_hh = os.path.join(self.generated_dir, "load-resources.hh")
        self.target_cpp = os.path.join(self.generated_dir, "load-resources.cpp")
        self.resource_id_hh = os.path.join(self.generated_dir, "gen-resource-id.hh")
        self.index_file = os.path.join(self.generated_dir, "index.txt")
        self.resource_files = self._list_resources()

    def _list_resources(self):
        return sorted([file for file in os.listdir(self.graphics_dir)
                       if file.find(".") != -1])

def _need_generate(paths):
    """Check if the resource loading needs to be generated"""
    if not os.path.exists(paths.generated_dir):
        return True

    if not os.path.exists(paths.index_file):
        return True

    indexed = [item for item in
               open(paths.index_file).read().split("\n") if len(item) != 0]
    return indexed != paths.resource_files

def _write_index(paths):
    index_file = open(paths.index_file, 'w')
    for resource in paths.resource_files:
        index_file.write(resource + "\n")

def _strip_ext(file_name):
    dot = file_name.find('.')
    assert(dot != -1)
    return file_name[:dot]

def _write_load_function(paths):
    cpp_file = open(paths.target_cpp, 'w')
    cpp_file.write('#include "util/artcontainer.hh"\n')
    cpp_file.write('#include "util/pathutil.hh"\n')
    cpp_file.write('\n')
    cpp_file.write('void load_faint_resources( ArtContainer& art ) {\n')
    cpp_file.write('  art.SetRoot( get_data_dir() + "/graphics" );\n')

    cpp_file.write('\n')
    cpp_file.write('  // Cursors\n')
    for cursor_file in _cursor_files(paths):
        res_id = _resource_identifier(cursor_file)
        cpp_file.write('  art.Load( "%s", Cursor::%s );\n' % (cursor_file, res_id) )

    cpp_file.write('\n')
    cpp_file.write('  // Cursors (wx-defaults)\n')
    for cursorId in sorted(DEFAULT_CURSORS.keys()):
        cpp_file.write('  art.Add( wxCursor(%s), Cursor::%s );\n' % (
                DEFAULT_CURSORS[cursorId], cursorId))

    cpp_file.write('\n')
    cpp_file.write('  // Icons\n')
    for icon_file in _icon_files(paths):
        cpp_file.write('  art.Load( "%s", Icon::%s );\n' % (icon_file, _resource_identifier(icon_file)))
    cpp_file.write('}\n')

    hh_file = open(paths.target_hh, 'w')
    hh_file.write('#ifndef %s\n' % _define_label(paths.target_hh))
    hh_file.write('#define %s\n' % _define_label(paths.target_hh))
    hh_file.write('void load_faint_resources( ArtContainer& );\n')
    hh_file.write('#endif\n')

def _define_label(file_path):
    base_name = os.path.basename(file_path)
    return _strip_ext(base_name).upper().replace('-','_')

PREFIXES = ["ICON_", "CURSOR_"]

def _resource_identifier(file_path):
    identifier = _strip_ext(file_path).upper().replace("-", "_")
    for prefix in PREFIXES:
        if identifier.startswith(prefix):
            identifier = identifier[len(prefix):]
            break
    return identifier

def _cursor_files(paths):
    return [p for p in paths.resource_files if p.startswith("cursor")]

def _icon_files(paths):
    return [p for p in paths.resource_files if not p.startswith("cursor")]

def _cursor_labels(paths):
    return [_resource_identifier(p) for p in _cursor_files(paths)]

def _icon_labels(paths):
    return [_resource_identifier(p) for p in _icon_files(paths)]

def _write_header(paths):
    hh_file_name = paths.resource_id_hh
    hh_file = open(hh_file_name, 'w')
    hh_file.write('#ifndef %s\n' % _define_label(hh_file_name))
    hh_file.write('#define %s\n' % _define_label(hh_file_name))
    hh_file.write("\n")
    hh_file.write("enum class Cursor{\n")
    cursor_labels = _cursor_labels(paths)
    cursor_labels.append('DONT_CARE')
    cursor_labels.extend(DEFAULT_CURSORS.keys())
    cursor_labels = sorted(cursor_labels)
    hh_file.write('  ' +  ',\n  '.join(cursor_labels) + '\n')
    hh_file.write('};\n')
    hh_file.write('\n')

    hh_file.write('enum class Icon{\n')
    icon_labels = _icon_labels(paths)
    hh_file.write('  ' +  ',\n  '.join(icon_labels) + '\n')
    hh_file.write('};\n')
    hh_file.write('\n')
    hh_file.write('#endif')

def run(faint_root, force=False):
    paths = Paths(faint_root)
    if not force and not _need_generate(paths):
        print " Resources up to date."
        return

    if not os.path.exists(paths.generated_dir):
        os.mkdir(paths.generated_dir)

    _write_load_function(paths)
    _write_header(paths)
    _write_index(paths)

if __name__ == '__main__':
    force = "--force" in sys.argv
    faint_root = os.path.abspath("../")
    run(faint_root, force)
