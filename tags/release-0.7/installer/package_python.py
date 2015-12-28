# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http:/www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import os
import compiler
import shutil

import zipfile

folders = [
    # "bsddb", # Interface to Berkeley DB database library
    # "compiler", # Deprecated: Python code compiler written in Python.
    "ctypes", # A foreign function library for Python.
    # "curses", # An interface to the curses library, providing portable terminal handling.
    # "distutils", # An interface to the curses library, providing portable terminal handling.
    # "email", # Package supporting the parsing, manipulating, and generating email messages, including MIME documents.
    "encodings", # Internationalized Domain Names implementation
    # "hotshot", # High performance logging profiler, mostly written in C.
    # "idlelib",
    "importlib", #Convenience wrappers for __import__
    #"json", # Encode and decode the JSON format.
    # "lib-tk",
    # "lib2to3", # the 2to3 library (convert Python 2.# to 3)
    # "logging", # Logging facility for Python
    # "msilib", # Read and write Microsoft Installer files
    "multiprocessing",
    "pydoc_data", # Documentation generator and online help system
    # "site-packages", Extensions
    "sqlite3",
    # "test", # Regression tests package containing the testing suite for Python.
    # "unittest", Unit testing frameworK
    "wsgiref", #WSGI Utilities and Reference Implementation
    "xml", # Oh no...
    "xml/dom", # ...
    "xml/etree", # ...
    "xml/parsers", # ...
    "xml/sax", # ...
    ]

dlls = [
    "bz2.pyd",
    # "py.ico",
    # "pyc.ico",
    "pyexpat.pyd",
    "select.pyd",
    "sqlite3.dll",
    # "tcl85.dll",
    # "tclpip85.dll",
    # "tk85.dll",
    "unicodedata.pyd",
    # "winsound.pyd",
    "_bsddb.pyd",
    "_ctypes.pyd",
    # "_ctypes_test.pyd",
    "_elementtree.pyd",
    "_hashlib.pyd",
    # "_msi.pyd",
    "_multiprocessing.pyd",
    "_socket.pyd",
    "_sqlite3.pyd",
    "_ssl.pyd",
    # "_testcapi.pyd",
    # "_tkinter.pyd"
]

def zipit():
    if not os.path.exists("installdep"):
        os.mkdir("installdep")
        os.mkdir("installdep/python")
        os.mkdir("installdep/SysWOW64")
    zf = zipfile.ZipFile('installdep/python/python27.zip', mode='w')
    for file in [file for file in os.listdir("pythonbundle") if file.endswith(".pyc")]:
        zf.write(os.path.join("pythonbundle", file), file)

    for folder in folders:
        srcFolder = os.path.join("pythonbundle", folder)
        zf.write(srcFolder, folder )
        for file in [ file for file in os.listdir(srcFolder) if file.endswith(".pyc")]:
            zf.write( os.path.join(srcFolder, file), os.path.join(folder, file))

def create_python_bundle(pythonRoot):
    pythonLib = os.path.join(pythonRoot, "lib")
    for file in [ os.path.join(pythonLib, file) for file in os.listdir(pythonLib) if file.endswith(".py")]:
        print "Compiling: ", file
        compiler.compileFile( file )
    for file in [ os.path.join(pythonLib, file) for file in os.listdir(pythonLib) if file.endswith(".pyc")]:
        shutil.copy(file, "pythonbundle")

    for folder in folders:
        srcFolder = os.path.join(pythonLib, folder)
        dstFolder = os.path.join("pythonbundle", folder)
        if not os.path.exists(dstFolder):
            os.mkdir(dstFolder)

        for file in [os.path.join(srcFolder, file) for file in os.listdir(srcFolder) if file.endswith(".py")]:
            print "Compiling: ", file
            compiler.compileFile( file )
        for file in [ os.path.join(srcFolder, file) for file in os.listdir(srcFolder) if file.endswith(".pyc")]:
            shutil.copy(file, dstFolder)

def copy_pyd( pythonRoot, dest ):
    for file in dlls:
        shutil.copy(os.path.join(pythonRoot, "DLLs", file), dest)

if not os.path.exists("pythonbundle"):
    os.mkdir("pythonbundle")

if __name__ == '__main__':
    import sys
    pythonRoot = sys.argv[1]
    create_python_bundle(pythonRoot)
    zipit()
    copy_pyd(pythonRoot, "installdep\\python")
