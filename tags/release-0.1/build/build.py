from optparse import OptionParser
import ConfigParser
import os
import subprocess
import sys

def build():
    # Init platform
    if sys.platform.startswith('linux'):
        import compile_gcc as compile
    else:
        import compile_msw as compile

    link = compile.link
    obj_ext = compile.obj_ext
    compile = compile.compile

    # Read config
    config = ConfigParser.RawConfigParser()
    if not os.path.exists( "build.cfg" ):
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
        print 'Config file "build.cfg" created.\nYou must update the file with correct paths.'
        exit(1)

    config.read( 'build.cfg')
    wx_root = config.get( 'folders', 'wx_root' )
    wx_vc_lib = os.path.join( wx_root, "lib", "vc_lib" )

    cairo_include = config.get('folders', 'cairo_include')
    cairo_lib = config.get('folders', 'cairo_lib')

    pango_include = config.get('folders', 'pango_include')
    pango_lib = config.get('folders', 'pango_lib')

    python_include = config.get('folders', 'python_include')
    python_lib = config.get( 'folders', 'python_lib' )

    glib_include = config.get('folders', 'glib_include' )
    glib_lib = config.get('folders', 'glib_lib' )

    glib_config_include = config.get('folders', 'glib_config_include' )    
    
    if wx_root == "" or python_lib == "" or python_include == "" or cairo_include == "" or pango_include == "":
        print "Incorret paths in build.cfg"
        exit(1)

    if cairo_lib == "" and not sys.platform.startswith("linux"):
        print "Incorret paths in build.cfg"
        exit(1)

    # Read command line options
    optParser = OptionParser()
    optParser.add_option("", "--rebuild",
                      action="store_true", dest="do_rebuild", default=False,
                      help="Rebuild and relink")
    optParser.add_option("", "--rvalue-ref", action="store_true", dest="rvalue_ref", default=False,
                         help="Enable rvalue references in Faint classes (c++0x)")
    optParser.add_option("", "--version", dest="version", default="unknown-version",
                      help="Faint version number")

    opts, args = optParser.parse_args()

    def enumerate_cpp( folder ):
        return [file for file in os.listdir( folder ) if file.endswith('.cpp')]

    def enumerate_obj( folder ):
        return [file for file in os.listdir( folder ) if file.endswith(obj_ext)]

    # Folder for writing/finding object files (.obj, .o)
    objRoot = os.path.join( os.getcwd(), "objs")
    if not os.path.exists( objRoot ):
        os.mkdir( objRoot )

    faintRoot = os.path.abspath("../") + "/"

    subFolders = [
        "commands/",
        "tools/",
        "objects/",
        "python/",
        "python/generate/output",
        "bitmap/",
        "",
        "formats/",
        "geo/",
        "gui/",
        "gui/resizedlg",
        "util/"
        ]

    folders = [ os.path.join( faintRoot, folder ) for folder in subFolders ]
    folders.append(faintRoot)

    include = [
        os.path.join( wx_vc_lib, "mswu" ),
        os.path.join( wx_root, "include" ),
        python_include,
        faintRoot,
        cairo_include,
        pango_include,
        glib_include,
        glib_config_include
    ]

    include.extend( folders )

    do_compile = True
    do_link = True
    do_rebuild = opts.do_rebuild

    err = open('err.txt', 'w')
    out = open('out.txt', 'w')

    if do_compile:
        print "* Compiling"
        for folder in folders:
            for file in enumerate_cpp( folder ):
                if file != 'few.cpp' and file != 'sizertest.cpp':
                    objFile = os.path.join( objRoot, file.replace( '.cpp', obj_ext ) )
                    cppFile = os.path.join( folder, file )

                    if not os.path.isfile( objFile ) or do_rebuild or ( os.path.isfile( objFile ) and os.path.getmtime( objFile ) < os.path.getmtime( cppFile ) ):
                        compile( os.path.join( folder, file ), include, out, err, objRoot, wx_root, rvalue_ref=opts.rvalue_ref, version=opts.version )

    if do_link:
        print "* Linking"
        files = enumerate_obj( objRoot )
        print files
        files = [ os.path.join( objRoot, file ) for file in files ]
        link( files, faintRoot, wx_root, cairo_lib, python_lib, pango_lib, glib_lib )

if __name__ == '__main__':
    build()
