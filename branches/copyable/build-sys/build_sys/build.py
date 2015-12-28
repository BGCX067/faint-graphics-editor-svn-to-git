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

from optparse import OptionParser
from build_sys.util.util import print_timing, timed
import os
import sys
import build_sys.genbuildinfo as genbuildinfo
import build_sys.genhelp as genhelp
import build_sys.gennsis as gennsis
import build_sys.genresource as genresource
import build_sys.dependencies as depend # FIXME

unknown_version_str = "unknown"

def parse_bool( config, group, name ):
    value = config.get(group, name)
    if value.lower() in ['1', 'true', 'yes', 'y']:
        return True
    if value.lower() in ['0', 'false', 'no', 'n']:
        return False
    print("Error: %s should be 1 or 0 in build.cfg" % name)
    exit(1)

def parse_command_line():
    optParser = OptionParser()
    optParser.add_option("", "--rebuild",
                         action="store_true",
                         dest="do_rebuild",
                         default=False,
                         help="Rebuild and relink")

    optParser.add_option("", "--version",
                         dest="version",
                         default=unknown_version_str,
                         help="Application version number")

    optParser.add_option("", "--debug",
                         action="store_true",
                         dest="debug",
                         default=False,
                         help="Build with debug symbols")

    optParser.add_option("", "--filthy",
                         action="store_true",
                         dest="filthy",
                         default=False,
                         help="Allow building unclean installer")

    return optParser.parse_args()


def check_valid_release(cmd_opts, opts, clean_build):
    if not clean_build and cmd_opts.version != unknown_version_str and not cmd_opts.filthy:
        print("Error: Build with --version requires empty output folder (build/objs)")
        exit(1)

    if cmd_opts.version != unknown_version_str and not cmd_opts.filthy and genbuildinfo.working_copy_modified(opts.project_root):
        print("Error: Build with --version requires unmodified working copy.")
        exit(1)

    if cmd_opts.version != unknown_version_str and opts.makensis_exe is None:
        print("Error: Build with --version requires path to nsis in build.cfg")
        exit(1)

def prepare_out_dir(obj_root):
    """Recreates the output-dir. Returns true if the folder was recreated
    or empty. (this signifies a clean build)"""
    if not os.path.exists(obj_root):
        os.mkdir(obj_root)
        return True
    elif len(os.listdir(obj_root)) == 0:
        return True
    else:
        return False

def build(opts):
    opts.verify()

    if opts.compiler is None:
        print ("Error: No compiler specified.")
        exit(1)
    if opts.compiler == 'msvc':
        import build_sys.compile_msw as compile_impl
        compile = compile_impl.compile
        link = compile_impl.link

    else:
        import build_sys.compile_linux as compile_impl
        if opts.compiler == 'clang':
            compile = compile_impl.compile_clang
            link = compile_impl.link_clang
        elif opts.compiler == 'gcc':
            compile = compile_impl.compile_gcc
            link = compile_impl.link_gcc
        else:
            print("Error: Unsupported compiler (%s) specified." % opts.compiler)
            exit(1)

    obj_ext = compile_impl.obj_ext
    create_installer = compile_impl.create_installer

    cmd_opts, args = parse_command_line()

    def enumerate_obj(folder):
        return [file for file in os.listdir( folder ) if file.endswith(obj_ext)]

    opts.set_debug_compile(cmd_opts.debug)
    objRoot = opts.get_obj_root()

    clean_build = prepare_out_dir(objRoot)
    check_valid_release(cmd_opts, opts, clean_build)

    do_compile = True
    do_link = True
    do_rebuild = cmd_opts.do_rebuild

    err = open('err.txt', 'w')
    out = open('out.txt', 'w')

    print("* Generating Html-help") # Fixme: Generalize
    genhelp.run()

    modified = ()
    to_compile = set()
    depsChanged = set()
    if opts.check_deps:
        print("* Generating dependencies")
        deps = timed(depend.get_flat_header_dependencies, opts.project_root)
    else:
        deps = []

    modified = set()
    depModified = set()
    if do_compile:
        for cpp in opts.source_files:
            objFile = os.path.join(objRoot,
                                   os.path.split(cpp)[1].replace('.cpp',
                                                                 obj_ext))
            if not os.path.isfile(objFile) or do_rebuild or (os.path.isfile(objFile) and os.path.getmtime(objFile) < os.path.getmtime(cpp)):
                to_compile.add(cpp)
                modified.add(cpp)

        for incl in deps:
            for cpp in deps.get(incl, []):
                objFile = os.path.join(objRoot, os.path.basename(cpp).replace('.cpp', obj_ext))
                if cpp not in to_compile and not os.path.isfile( objFile ) or do_rebuild or ( os.path.isfile( objFile ) and os.path.getmtime( objFile ) < os.path.getmtime( incl ) ):
                    to_compile.add(cpp)
                    depModified.add(cpp)

    if len(to_compile) > 0:
        if opts.create_build_info:
            print("* Generating Build-info")
            to_compile.add(genbuildinfo.run(opts.project_root, cmd_opts.version))

        print("* Compiling")
        print(" ",len(modified), "cpp files modified,", len(depModified), "indirectly.")
        compile(sorted(list(to_compile)), opts, out, err, debug=cmd_opts.debug)
    else:
        print("* Compiling")
        print(" Up to date.")

    if do_link:
        print("* Linking")
        files = enumerate_obj(opts.get_obj_root())

        extra_objs = [o + obj_ext for o in opts.extra_objs]

        files.extend(extra_objs)
        print(" Object files: " + str(len(files)))
        files = [ os.path.join( objRoot, file ) for file in files ]
        link(files, opts, debug=cmd_opts.debug)

    if opts.platform == 'msw' and cmd_opts.version != unknown_version_str:
        print("* Generating %s installer" % cmd_opts.version)
        oldDir = os.getcwd()
        os.chdir("../installer")
        nsiFile = gennsis.run(cmd_opts.version)
        create_installer(opts.makensis_exe, nsiFile)
        os.chdir(oldDir)

if __name__ == '__main__':
    build()
