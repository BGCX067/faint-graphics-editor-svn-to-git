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

import sys
sys.path.append("../build-sys/")
sys.path.append("../test-sys/")
import os
from optparse import OptionParser
import build_sys
#from build_sys.opts import BuildOptions
import build_sys.genresource as genresource
#import build_sys.build as builder # Fixme
import test_sys.gen_testrunner as gen_testrunner
import configparser
import gencpp
import faint_info # Fixme: Remove? (or ensmallen)

join_path = os.path.join

def recreate_config(platform):
    f = open( "build.cfg" ,'w')
    f.write("[folders]\n")
    f.write("wx_root=\n")
    f.write("cairo_include=\n")
    f.write("cairo_lib=\n")
    f.write("python_include=\n")
    f.write("python_lib=\n")
    f.write("pango_include=\n")
    f.write("pango_lib=\n")
    f.write("glib_include=\n")
    f.write("glib_lib=\n")
    f.write("glib_config_include=\n")
    if platform == 'msw':
        f.write("[nsis]\n")
        f.write("makensis=\n")
    f.write("[other]\n")
    if platform != 'msw':
        f.write('compiler=gcc\n')
    f.write("parallell_compiles=0\n")
    f.write("etags_folder=\n")
    print('Config file "build.cfg" created.\nYou must update the file with correct paths.')

def read_config(platform):
    bo = build_sys.BuildOptions()
    bo.platform = platform
    config = configparser.RawConfigParser()
    config.read( 'build.cfg')
    wx_root = config.get('folders', 'wx_root')
    wx_vc_lib = os.path.join(wx_root, "lib", "vc_lib")

    cairo_include = config.get('folders', 'cairo_include')
    cairo_lib = config.get('folders', 'cairo_lib')

    pango_include = config.get('folders', 'pango_include')
    pango_lib = config.get('folders', 'pango_lib')

    python_include = config.get('folders', 'python_include')
    python_lib = config.get( 'folders', 'python_lib' )

    glib_include = config.get('folders', 'glib_include' )
    glib_lib = config.get('folders', 'glib_lib' )
    glib_config_include = config.get('folders', 'glib_config_include' )
    bo.parallell_compiles = int(config.get('other', 'parallell_compiles'))

    bo.extra_resource_root = wx_root
    if bo.platform == 'msw':
        bo.makensis_exe = config.get('nsis', 'makensis')

    if bo.platform == 'linux':
        compiler = config.get('other', 'compiler')
        if compiler is None:
            print("Error: Compiler not specified in build.cfg.")
            print("Expected compiler=clang or compiler=gcc under [other].")
            exit(1)
        elif compiler not in ('gcc', 'clang'):
            print('Error: Unsupported compiler specified in build.cfg: "%s"'
                  % opts.compiler)
            print('Expected "clang" or "gcc"')
            exit(1)
        bo.compiler = compiler
    elif bo.platform == 'msw':
        bo.compiler = 'msvc'

    if (wx_root == "" or
        python_lib == "" or
        python_include == "" or
        cairo_include == "" or
        pango_include == ""):

        print("Error: Incorrect paths in build.cfg")
        exit(1)

    if cairo_lib == "" and not platform.startswith("linux"):
        print("Error: Incorrect paths in build.cfg")
        exit(1)

    bo.lib_paths = [
        cairo_lib,
        pango_lib,
        python_lib,
        glib_lib]

    bo.lib_paths = [l for l in bo.lib_paths if len(l) != 0]

    if bo.platform == "msw":
        bo.lib_paths.append(join_path(wx_root, 'lib', 'vc_lib'))


    bo.project_root = faint_info.faintRoot
    bo.system_include_folders = [
        os.path.join( wx_vc_lib, "mswu" ),
        os.path.join( wx_root, "include" ),
        python_include,
        cairo_include,
        pango_include,
        glib_include,
        glib_config_include,
    ]

    bo.include_folders = [bo.project_root]

    bo.wx_root = wx_root
    return bo

def faint_source_files(platform, project_root):
    src_folders = faint_info.get_src_folders(platform)
    src_folders = [os.path.join(project_root, folder)
            for folder in src_folders ]
    src_folders.append(project_root)

    files = []
    for folder in src_folders:
        files.extend([join_path(folder, f)
                     for f in faint_info.enumerate_cpp(folder)])
    return files

def read_build_options(platform):
    if not os.path.exists("build.cfg"):
        recreate_config(platform)
        exit(1)

    return read_config(platform)

def build_tests(platform):
    bo = read_build_options(platform)
    test_root = os.path.join(bo.project_root, "test")
    print("* Generating Test-runner")
    gen_testrunner.run(root_dir=join_path(test_root),
                       out_file=join_path(test_root, 'gen', 'test-runner.cpp'))

    obj_root = join_path(os.getcwd(), "objs")
    bo.extra_objs = [
        join_path(obj_root, "alpha-map"),
        join_path(obj_root, "angle"),
        join_path(obj_root, "bitmap"),
        join_path(obj_root, "brush"),
        join_path(obj_root, "cairo-context"),
        join_path(obj_root, "char-constants"),
        join_path(obj_root, "color"),
        join_path(obj_root, "color-bitmap-util"),
        join_path(obj_root, "cpp_settingid"),
        join_path(obj_root, "draw"),
        join_path(obj_root, "faint-cairo-stride"),
        join_path(obj_root, "geo-func"),
        join_path(obj_root, "geo-list-points"),
        join_path(obj_root, "gradient"),
        join_path(obj_root, "index"),
        join_path(obj_root, "intpoint"),
        join_path(obj_root, "intrect"),
        join_path(obj_root, "intsize"),
        join_path(obj_root, "line"),
        join_path(obj_root, "math-constants"),
        join_path(obj_root, "measure"),
        join_path(obj_root, "mouse"),
        join_path(obj_root, "paint"),
        join_path(obj_root, "pathpt"),
        join_path(obj_root, "pattern"),
        join_path(obj_root, "point"),
        join_path(obj_root, "primitive"),
        join_path(obj_root, "radii"),
        join_path(obj_root, "range"),
        join_path(obj_root, "rect"),
        join_path(obj_root, "rotate"),
        join_path(obj_root, "rotation"),
        join_path(obj_root, "scale"),
        join_path(obj_root, "settings"),
        join_path(obj_root, "size"),
        join_path(obj_root, "tri"),
        join_path(obj_root, "utf8-string"),
        join_path(obj_root, "zoom-level"),
    ]

    bo.obj_root_release = join_path(os.getcwd(), "objs_test_release") # Fixme
    bo.obj_root_debug = join_path(os.getcwd(), "objs_test_debug") # Fixme
    bo.out_name = "Test"
    bo.out_name_debug = "Testd"
    test_files = []
    for folder in (test_root, join_path(test_root, 'gen')):
        test_files.extend([join_path(folder, f)
                           for f in faint_info.enumerate_cpp(folder)])
    bo.source_files = test_files
    bo.check_deps = False
    bo.compile_resources = False
    bo.create_build_info = False
    bo.msw_subsystem = "console"
    build_sys.build(bo)

# Todo: Read build sys-args here maybe.
def build(platform):
    bo = read_build_options(platform)
    bo.obj_root_release = join_path(os.getcwd(), "objs")
    bo.obj_root_debug = join_path(os.getcwd(), "objs_debug")
    bo.out_name = "faint"
    bo.out_name_debug = "faintd"

    print("* Generating Python-C++-interface") # Fixme: Generalize
    gencpp.run("../python/generate")

    print("* Generating Resource-loading")
    genresource.run(bo.project_root)

    bo.source_files = faint_source_files(platform, bo.project_root)
    bo.forced_include = join_path(bo.project_root, "util", "msw_warn.hh")
    bo.msw_subsystem = "windows"
    build_sys.build(bo)

if __name__ == '__main__':
    platform = ("linux" if sys.platform.startswith('linux') else "msw")
    build(platform)
    build_tests(platform)
