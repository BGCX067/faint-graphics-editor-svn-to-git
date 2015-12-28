import os
import subprocess

obj_ext = '.o'

def get_wxflags(wxRoot):
    wxcfg = subprocess.Popen( "%s/wx-config --cxxflags" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip()

def compile( fileList, faintRoot, includes, out, err, objRoot, wxRoot, rvalue_ref, version, debug, num_parallell ):
    assert(not debug) # Not implemented
    includes = [ "-isystem " + include for include in includes ]
    includes.append("-I%s" % faintRoot)
    args = " ".join( includes ) + " "
    args += "-Wall -Wextra -pedantic -ansi -pedantic -Wconversion -O2 -Wno-strict-aliasing "
    if rvalue_ref:
        args += "-std=c++0x -DFAINT_RVALUE_REFERENCES "
    wxflags = get_wxflags( wxRoot )
    old = os.getcwd()
    os.chdir( objRoot )
    for file in fileList:
        print file
        cmd = "g++ " + args + wxflags + ' -c ' + file
        cl = subprocess.Popen( cmd, 0, None, None, out, err, shell=True )
        if cl.wait() != 0:
            print "Compilation failed"
            exit(1)
    os.chdir( old )


def get_wxlibs( wxRoot ):
    wxcfg = subprocess.Popen( "%s/wx-config --libs" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip()

def link( files, faintRoot, wxRoot, cairoLib, pythonlib, pangoLib, glibLib, debug ):
    assert(not debug) # Not implemented
    old = os.getcwd()
    os.chdir( faintRoot )
    wxlibs = get_wxlibs( wxRoot )

    cmd = "g++ -o faint " + " ".join(files) + " " + wxlibs + " -L" + pythonlib + " -l python2.7 -O2"
    print cmd
    linker = subprocess.Popen(cmd, shell=True)
    linker.wait()
    os.chdir( old )

def create_installer(*arg, **kwarg):
    assert(False)
