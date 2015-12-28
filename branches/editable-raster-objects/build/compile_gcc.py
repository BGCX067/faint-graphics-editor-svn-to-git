import os
import subprocess
from compile_msw import print_timing

obj_ext = '.o'

def gen_makefile( fileList, faintRoot, includes, objRoot, wxRoot, version, debug ):
    makefile = open("generated_makefile", 'w')
    includes = [ "-isystem " + include for include in includes ]
    includes.append("-I%s" % faintRoot)
    makefile.write("INCLUDES=%s\n" % " \\\n  ".join(includes))
    if debug:
        makefile.write("CCFLAGS=-Wall -Wextra -pedantic -ansi -Wconversion -Wno-strict-aliasing -std=c++11 -g -c\n")
    else:
        makefile.write("CCFLAGS=-Wall -Wextra -pedantic -ansi -Wconversion -O2 -Wno-strict-aliasing -std=c++11 -c\n")
    makefile.write("WXFLAGS:=$(shell %s/wx-config --cxxflags)\n" % wxRoot)
    makefile.write("\n")
    makefile.write("all: " + " \\\n  ".join([os.path.join(objRoot, os.path.basename(f).replace(".cpp", ".o")) for f in fileList]))
    makefile.write('\n\n')

    for f in fileList:
        objName = os.path.join(objRoot, os.path.basename(f).replace(".cpp", ".o"))
        makefile.write('%s: %s\n' % (objName, f))
        makefile.write('\tg++ $(CCFLAGS) $(INCLUDES) $(WXFLAGS) $< -o $@\n')
        makefile.write('\n')

@print_timing
def compile( fileList, faintRoot, includes, out, err, objRoot, wxRoot, version, debug, num_parallell ):
    gen_makefile( fileList, faintRoot, includes, objRoot, wxRoot, version, debug )
    make_cmd = 'make -f generated_makefile'
    if num_parallell != 0:
        make_cmd += " -j %d" % num_parallell
    print make_cmd
    make = subprocess.Popen( make_cmd, 0, None, None, out, err, shell=True )
    if make.wait() != 0:
        print "Compilation failed"
        exit(1)

def get_wxlibs( wxRoot ):
    wxcfg = subprocess.Popen( "%s/wx-config --libs" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip()

@print_timing
def link( files, faintRoot, wxRoot, cairoLib, pythonlib, pangoLib, glibLib, debug ):
    old = os.getcwd()
    os.chdir( faintRoot )
    wxlibs = get_wxlibs( wxRoot )
    outName = "faintd" if debug else "faint"
    print files
    cmd = "g++ -std=c++11 -g -o %s " % outName + " ".join(files) + " " + wxlibs + " -L" + pythonlib + " -l python2.7 -O2"
    linker = subprocess.Popen(cmd, shell=True)
    linker.wait()
    os.chdir( old )

def create_installer(*arg, **kwarg):
    assert(False)
