from optparse import OptionParser
import ConfigParser
import os
import sys
import genhelp
import gencpp
import gennsis
import faint_info

unknown_version_str = "unknown-version"

def parse_bool( config, group, name ):
    value = config.get(group, name)
    if value.lower() in ['1', 'true', 'yes', 'y']:
        return True
    if value.lower() in ['0', 'false', 'no', 'n']:
        return False
    print "Error: %s should be 1 or 0 in build.cfg" % name
    exit(1)

def build():
    # Init platform
    if sys.platform.startswith('linux'):
        platform = 'linux'
        import compile_gcc as compile
    else:
        platform = 'msw'
        import compile_msw as compile

    link = compile.link
    obj_ext = compile.obj_ext
    create_installer = compile.create_installer
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
        if platform == 'msw':
            f.write("[nsis]\n")
            f.write("makensis=\n")
        f.write("[settings]\n")
        f.write("rvalue_references=1\n")
        f.write("[other]\n")
        f.write("etags_folder=\n")
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

    if platform == 'msw':
        makensis = config.get('nsis', 'makensis') # Oops, not a folder.


    rvaluerefs = parse_bool(config, 'settings', 'rvalue_references')
    if wx_root == "" or python_lib == "" or python_include == "" or cairo_include == "" or pango_include == "":
        print "Error: Incorrect paths in build.cfg"
        exit(1)

    if cairo_lib == "" and not sys.platform.startswith("linux"):
        print "Error: Incorrect paths in build.cfg"
        exit(1)

    # Read command line options
    optParser = OptionParser()
    optParser.add_option("", "--rebuild",
                      action="store_true", dest="do_rebuild", default=False,
                      help="Rebuild and relink")
    optParser.add_option("", "--version", dest="version", default=unknown_version_str, help="Faint version number")
    optParser.add_option("", "--debug",
                         action="store_true",
                         dest="debug",
                         default=False,
                         help="Build with debug symbols")
    opts, args = optParser.parse_args()

    def enumerate_obj( folder ):
        return [file for file in os.listdir( folder ) if file.endswith(obj_ext)]

    # Folder for writing/finding object files (.obj, .o)
    if opts.debug:
        objRoot = os.path.join( os.getcwd(), "objs_debug")
    else:
        objRoot = os.path.join( os.getcwd(), "objs")

    cleanBuild = False
    if not os.path.exists( objRoot ):
        os.mkdir( objRoot )
        cleanBuild = True
    elif len(os.listdir(objRoot)) == 0:
        cleanBuild = True

    if not cleanBuild and opts.version != unknown_version_str:
        print "Error: Build with --version requires empty output folder (build/objs)"
        exit(1)

    faintRoot = faint_info.faintRoot
    subFolders = faint_info.faintFolders

    folders = [ os.path.join( faintRoot, folder ) for folder in subFolders ]
    folders.append(faintRoot)
    include = [
        os.path.join( wx_vc_lib, "mswu" ),
        os.path.join( wx_root, "include" ),
        python_include,
        cairo_include,
        pango_include,
        glib_include,
        glib_config_include,
    ]

    do_compile = True
    do_link = True
    do_rebuild = opts.do_rebuild

    err = open('err.txt', 'w')
    out = open('out.txt', 'w')

    print "* Generating Python-C++-interface"
    oldDir = os.getcwd()
    os.chdir("../python/generate")
    gencpp.run()
    print oldDir
    os.chdir(oldDir)

    print "* Generating html-help"
    genhelp.run()
    if do_compile:
        print "* Compiling"
        for folder in folders:
            for file in faint_info.enumerate_cpp( folder ):
                if file != 'few.cpp' and file != 'sizertest.cpp':
                    objFile = os.path.join( objRoot, file.replace( '.cpp', obj_ext ) )
                    cppFile = os.path.join( folder, file )

                    if not os.path.isfile( objFile ) or do_rebuild or ( os.path.isfile( objFile ) and os.path.getmtime( objFile ) < os.path.getmtime( cppFile ) ):
                        compile( os.path.join( folder, file ), faintRoot, include, out, err, objRoot, wx_root, strip="", rvalue_ref=rvaluerefs, version=opts.version, debug=opts.debug )

    if do_link:
        print "* Linking"
        files = enumerate_obj( objRoot )
        print " Object files: " + str(len(files))
        files = [ os.path.join( objRoot, file ) for file in files ]
        link( files, faintRoot, wx_root, cairo_lib, python_lib, pango_lib, glib_lib, debug=opts.debug )

    if platform == 'msw' and opts.version != unknown_version_str:
        print "* Generating %s installer" % opts.version
        oldDir = os.getcwd()
        os.chdir("../installer")
        nsiFile = gennsis.run(opts.version)
        create_installer(makensis, nsiFile)
        os.chdir(oldDir)

if __name__ == '__main__':
    build()
