import os
import subprocess
import time
import sys
from util import changed

obj_ext = '.obj'

cl_common_switches = [
    "EHsc",   # Exception mode sc
    "GF",     # String pooling
    "GT",     # Fiber-safe thread local storage
    "Gm-",    # Disable minimal rebuild
    "Gy-",    # Disable function level linking (why?)
    "Oi",     # Intrinsic functions (faster but larger)
    "W4",     # Warning level
    "WX",     # Treat warnings as errors
    "Y-",     # Ignore precompiled header options
    "Zc:forScope", # ISO-Conformance
    "Zc:wchar_t", # ISO-Conformance
    "fp:precise", # Floating point model
    "nologo", # No startup banner
    "Gd",     # cdecl calling convention
    "analyze-",
    "errorReport:queue",
    ]

cl_release_switches = [
    "MD",
    #"GL",     # Whole program optimization, requires LTCG for linker
    "GS-",    # Disable Buffer overrun check
    "Ot",     # Favor fast code
    "Ox",     # Optimize, Favor speed
    #"Oy-",    # Do not suppress frame pointer (old 386 optimization?)
    ]

cl_debug_switches = [
    "MDd", # Link windows Multi-threaded static windows runtime - debug
    "RTC1"    # Runtime-checks
    "GS",     # Buffer security check
    "Od",     # Disable optimization
    "Yd",     # Full debugging information in all object files
    "Zi",     # Full debugging information (pdb?)
    ]

def get_cl_switches(debug):
    switches = cl_common_switches[:]
    if debug:
        switches.extend(cl_debug_switches)
    else:
        switches.extend(cl_release_switches)
    return switches

def print_timing(func):
    def wrapper(*arg, **kwArgs):
        t1 = time.time()
        res = func(*arg, **kwArgs)
        t2 = time.time()
        print '%s took %0.3f ms' % (func.func_name, (t2-t1)*1000.0)
        return res
    return wrapper

@print_timing
def compile( fileList, faintRoot, includes, out, err, objRoot, wxRoot, version, debug, num_parallell ):
    fileList = [file.replace("/", "\\") for file in fileList]
    includes.append(faintRoot)
    includes = [ "/I" + include.replace("/", "\\") for include in includes ]
    defines = ["_LIB" , "__WXMSW__" , "WXBUILDING" , "wxUSE_GUI=1" , "wxUSE_BASE=1" , "_UNICODE" , "_WINDOWS" , "NOPCH" , "_CRT_SECURE_NO_WARNINGS" , "UNICODE", "WIN32", 'FAINT_VERSION=\\"%s\\"' % version ]
    defines = [ '/D%s ' % define for define in defines]
    defines.append('/DFAINT_MSW')

    switches = get_cl_switches(debug)
    default = " ".join(["/%s" % switch for switch in switches ])
    args = " ".join( defines ) + " " + default + " " +  " ".join( includes )
    args += " /FI " + os.path.join(faintRoot, "util", "msw_warn.hh")

    cmd = "cl.exe " + args
    if num_parallell > 0:
        cmd = cmd + " /MP%d" % num_parallell
    cmd += " /c " + " ".join(fileList)
    sys.stdout.flush()
    old = os.getcwd()
    os.chdir( objRoot )
    cl = subprocess.Popen( cmd, 0, None, None, out, err )
    os.chdir( old )
    if cl.wait() != 0:
        print "Compilation failed"
        exit(1)

def compile_resources(faintRoot, wxRoot):
    targetFile = os.path.join(faintRoot.replace("/", "\\"), "build", "objs", "faint.res")
    sourceFile = os.path.join(faintRoot.replace("/", "\\"), "faint.rc")
    if not changed(sourceFile, targetFile):
        print "Resources unchanged."
        return targetFile

    cmd = 'rc /fo "%s" /I"%s\\include" "%s\\faint.rc"' % ( targetFile, wxRoot, faintRoot )
    resourceCompile = subprocess.Popen(cmd)
    if resourceCompile.wait() != 0:
        print "Compiling resources failed"
        exit(1)
    return targetFile

wxlibs_debug = [
    "wxbase29ud.lib",
    "wxbase29ud_net.lib",
    "wxbase29ud_xml.lib",
    "wxexpatd.lib",
    "wxjpegd.lib",
    "wxmsw29ud_adv.lib",
    "wxmsw29ud_aui.lib",
    "wxmsw29ud_core.lib",
    "wxmsw29ud_gl.lib",
    "wxmsw29ud_html.lib",
    "wxmsw29ud_media.lib",
    "wxmsw29ud_propgrid.lib",
    "wxmsw29ud_qa.lib",
    "wxmsw29ud_richtext.lib",
    "wxmsw29ud_stc.lib",
    "wxmsw29ud_xrc.lib",
    "wxpngd.lib",
    "wxregexud.lib",
    "wxscintillad.lib",
    "wxtiffd.lib",
    "wxzlibd.lib" ]

wxlibs_release = [
    "wxbase29u.lib",
    "wxbase29u_net.lib",
    "wxbase29u_xml.lib",
    "wxexpat.lib",
    "wxjpeg.lib",
    "wxmsw29u_adv.lib",
    "wxmsw29u_aui.lib",
    "wxmsw29u_core.lib",
    "wxmsw29u_gl.lib",
    "wxmsw29u_html.lib",
    "wxmsw29u_media.lib",
    "wxmsw29u_propgrid.lib",
    "wxmsw29u_qa.lib",
    "wxmsw29u_richtext.lib",
    "wxmsw29u_stc.lib",
    "wxmsw29u_xrc.lib",
    "wxpng.lib",
    "wxregexu.lib",
    "wxscintilla.lib",
    "wxtiff.lib",
    "wxzlib.lib" ]

def get_wxlibs(debug):
    if debug:
        return wxlibs_debug
    else:
        return wxlibs_release

@print_timing
def link( files, faintRoot, wxRoot, cairo_lib, python_lib, pango_lib, glib_lib, debug ):
    resFile = compile_resources( faintRoot, wxRoot )
    wx_vc_lib = os.path.join( wxRoot, "lib", "vc_lib" )
    old = os.getcwd()
    os.chdir( faintRoot )
    flags = "/NOLOGO"
    wxlibs = get_wxlibs(debug)
    if debug:
        outName = "Faintd"
        flags += " /OUT:%s.exe /DEBUG /PDB:%s.pdb" % (outName, outName)
    else:
        outName = "Faint"
        flags += " /OUT:%s.exe" % outName
    cmd = "Link.exe " + flags + " /SUBSYSTEM:WINDOWS /OPT:REF /LIBPATH:" + wx_vc_lib + " /LIBPATH:" + cairo_lib + " /LIBPATH:" + python_lib + " /LIBPATH:" + pango_lib + " /LIBPATH:" + glib_lib + " " + " ".join(files) + " " + " ".join(wxlibs) + " comctl32.lib rpcrt4.lib shell32.lib gdi32.lib kernel32.lib gdiplus.lib cairo.lib comdlg32.lib user32.lib Advapi32.lib Ole32.lib Oleaut32.lib Winspool.lib pango-1.0.lib pangocairo-1.0.lib pangoft2-1.0.lib pangowin32-1.0.lib gio-2.0.lib glib-2.0.lib gmodule-2.0.lib gobject-2.0.lib gthread-2.0.lib " + resFile
    linker = subprocess.Popen(cmd)
    if linker.wait() != 0:
        print "Linking failed."
        exit(1)

    # The manifest compiler can sometimes not write to the exe after the linking,
    # maybe waiting a tiny bit helps
    time.sleep( 0.5 )

    # Embed the manifest to get the correct common controls version etc.
    # See: http://msdn.microsoft.com/en-us/library/ms235591(v=vs.80).aspx
    manifestCmd = 'mt.exe -manifest %s.exe.manifest -outputresource:%s.exe;1' % (outName, outName)
    embedManifest = subprocess.Popen( manifestCmd )
    if embedManifest.wait() != 0:
        print "Embedding Manifest failed."
        exit(1)
    os.chdir( old )

def create_installer(makensis, nsifile):
    nsis = subprocess.Popen(makensis + " " + nsifile)
    if nsis.wait() != 0:
        print "Installer creation failed."
        exit(1)
