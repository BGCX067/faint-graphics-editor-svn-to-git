import os
import subprocess
import time
from util import changed
obj_ext = '.obj'

def compile( file, includes, out, err, objRoot, wxRoot, strip="", rvalue_ref=False, version="" ):
    file = file.replace("/", "\\")
    includes = [ "/I" + include.replace("/", "\\") for include in includes ]
    defines = ["_LIB" , "__WXMSW__" , "WXBUILDING" , "wxUSE_GUI=1" , "wxUSE_BASE=1" , "_UNICODE" , "_WINDOWS" , "NOPCH" , "_CRT_SECURE_NO_WARNINGS" , "UNICODE", "WIN32", 'FAINT_VERSION=\\"%s\\"' % version ]
    defines = [ '/D%s ' % define for define in defines]

    switches = [
        "EHsc",   # Exception mode sc
        "GF",     # String pooling
        #"GL",     # Whole program optimization
        "GS-",    # Disable Buffer overrun check
        "GT",     # Fiber-safe thread local storage
        "Gm-",    # Disable minimal rebuild
        "Gy-",    # Disable function level linking (why?)
        "MD",     # Link windows Multi-threaded static windows runtime
        "Ob2",    # Auto-inline
        "Oi",     # Intrinsic functions (faster but larger)
        "Ot",     # Favor fast code
        "Ox",     # Optimize, Favor speed
        "Oy-",    # Do not suppress frame pointer (old 386 optimization?)
        "W4",     # Warning level
        "WX-",    # Do not treat warnings as errors
        "Y-",     # Ignore precompiled header options
        "Zc:forScope", # ISO-Conformance
        "Zc:wchar_t", # ISO-Conformance
        "fp:precise", # Floating point model
        "nologo", # No startup banner
        "Gd",     # cdecl calling convention
        "analyze-",
        "errorReport:queue",
        ]

    if rvalue_ref:
        switches.append( "DFAINT_RVALUE_REFERENCES" )

    default = " ".join(["/%s" % switch for switch in switches ])
    args = " ".join( includes ) + " ".join( defines ) + " " + default
    cmd = "cl.exe " + args + ' /c ' + file
    print file.replace( strip, "" )
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

def link( files, faintRoot, wxRoot, cairo_lib, python_lib, pango_lib, glib_lib ):
    resFile = compile_resources( faintRoot, wxRoot )
    wx_vc_lib = os.path.join( wxRoot, "lib", "vc_lib" )
    old = os.getcwd()
    os.chdir( faintRoot )
    cmd = "Link.exe /NOLOGO /OUT:Faint.exe /SUBSYSTEM:WINDOWS /OPT:REF /LIBPATH:" + wx_vc_lib + " /LIBPATH:" + cairo_lib + " /LIBPATH:" + python_lib + " /LIBPATH:" + pango_lib + " /LIBPATH:" + glib_lib + " " + " ".join(files) + " wxbase29u.lib wxbase29u_net.lib wxbase29u_xml.lib wxexpat.lib wxjpeg.lib wxmsw29u_adv.lib wxmsw29u_aui.lib wxmsw29u_core.lib wxmsw29u_gl.lib wxmsw29u_html.lib wxmsw29u_media.lib wxmsw29u_propgrid.lib wxmsw29u_qa.lib wxmsw29u_richtext.lib wxmsw29u_stc.lib wxmsw29u_xrc.lib wxpng.lib wxregexu.lib wxscintilla.lib wxtiff.lib wxzlib.lib comctl32.lib rpcrt4.lib shell32.lib gdi32.lib kernel32.lib gdiplus.lib cairo.lib comdlg32.lib user32.lib Advapi32.lib Ole32.lib Oleaut32.lib Winspool.lib pango-1.0.lib pangocairo-1.0.lib pangoft2-1.0.lib pangowin32-1.0.lib gio-2.0.lib glib-2.0.lib gmodule-2.0.lib gobject-2.0.lib gthread-2.0.lib " + resFile
    linker = subprocess.Popen(cmd)
    if linker.wait() != 0:
        print "Linking failed"
        exit(1)

    # The manifest compiler can sometimes not write to the exe after the linking,
    # maybe waiting a tiny bit helps
    time.sleep( 0.5 )

    # Embed the manifest to get the correct common controls version etc.
    # See: http://msdn.microsoft.com/en-us/library/ms235591(v=vs.80).aspx
    manifestCmd = 'mt.exe -manifest Faint.exe.manifest -outputresource:Faint.exe;1'
    embedManifest = subprocess.Popen( manifestCmd )
    if embedManifest.wait() != 0:
        print "Embedding Manifest failed."
        exit(1)
    os.chdir( old )
