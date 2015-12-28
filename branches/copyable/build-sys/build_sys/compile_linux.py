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

import os
import subprocess
from . linux.gen_makefile import gen_makefile
from build_sys.util.util import print_timing
obj_ext = '.o'

@print_timing
def _compile(fileList, opts, out, err, debug, cc):
    makefile_name = gen_makefile(fileList, opts, debug, cc)

    make_cmd = 'make -f %s' % makefile_name
    if opts.parallell_compiles != 0:
        make_cmd += " -j %d" % opts.parallell_compiles
    print(make_cmd)
    make = subprocess.Popen(make_cmd, 0, None, None, out, err, shell=True)
    if make.wait() != 0:
        print("Compilation failed")
        exit(1)

def _get_wxlibs( wxRoot ):
    # Fixme: duplicates compile_clang
    wxcfg = subprocess.Popen( "%s/wx-config --libs" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip().decode("ascii") # Fixme

@print_timing
def _link(files, opts, debug, cc):
    old = os.getcwd()
    os.chdir(opts.project_root)
    wxlibs = _get_wxlibs(opts.wx_root)
    outName = opts.out_name
    lib_paths = " ".join(["-L%s" % p for p in opts.lib_paths])
    cmd = cc + " -std=c++11 -g -o %s " % outName + " ".join(files) + " " + wxlibs + " " + lib_paths + " -l python3.4 -O2"
    linker = subprocess.Popen(cmd, shell=True)
    linker.wait()
    os.chdir( old )

def create_installer(*arg, **kwarg):
    assert(False)


def compile_gcc(fileList, opts, out, err, debug):
    _compile(fileList, opts, out, err, debug, cc='g++')

def link_gcc(files, opts, debug):
    _link(files, opts, debug, cc='g++')


def compile_clang(fileList, opts, out, err, debug):
    _compile(fileList, opts, out, err, debug, cc='clang++')

def link_clang(files, opts, debug):
    _link(files, opts, debug, cc='clang++')
