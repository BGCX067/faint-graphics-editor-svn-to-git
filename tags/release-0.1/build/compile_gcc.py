import os
import subprocess

obj_ext = '.o'

def get_wxflags(wxRoot):
    wxcfg = subprocess.Popen( "%s/wx-config --cxxflags" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip()

def compile( file, includes, out, err, objRoot, wxRoot, strip="", rvalue_ref=False, version="" ):
    includes = [ "-I" + include for include in includes ]
    args = " ".join( includes ) + " "
    args += "-Wall -Wextra -pedantic -ansi -pedantic -Wconversion -O2 -Wno-strict-aliasing "
    if rvalue_ref:
        args += "-std=c++0x -DFAINT_RVALUE_REFERENCES "
    wxflags = get_wxflags( wxRoot )
    cmd = "g++ " + args + wxflags + ' -c ' + file    
    print file.replace( strip, "" )
    old = os.getcwd()
    os.chdir( objRoot )    
    
    cl = subprocess.Popen( cmd, 0, None, None, out, err, shell=True )
    os.chdir( old )
    if cl.wait() != 0:
        print "Compilation failed"
        exit(1)

def get_wxlibs( wxRoot ):
    wxcfg = subprocess.Popen( "%s/wx-config --libs" % wxRoot, 0, shell=True, stdout=subprocess.PIPE )
    return wxcfg.communicate()[0].strip()

def link( files, faintRoot, wxRoot, cairoLib, pythonlib, pangoLib, glibLib ):
    old = os.getcwd()
    os.chdir( faintRoot )
    wxlibs = get_wxlibs( wxRoot )

    cmd = "g++ -o faint " + " ".join(files) + " " + wxlibs + " -L" + pythonlib + " -l python2.7 -O2"
    print cmd
    linker = subprocess.Popen(cmd, shell=True)
    linker.wait()
    os.chdir( old )
